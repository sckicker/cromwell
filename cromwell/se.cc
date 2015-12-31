#include "se.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <errno.h>

namespace cromwell {

/* Include the best multiplexing layer supported by this system.
 * The following should be ordered by performances, descending. */
#ifdef HAVE_EPOLL
	#include "se_epool.cc"
#else
	#include "se_select.cc"
#endif

SeEventLoop* SeCreateEventLoop(int setsize) {
    SeEventLoop* event_loop;
    int i;

    if ((event_loop = malloc(sizeof(*event_loop))) == NULL) goto err;
    event_loop->events = malloc(sizeof(SeFileEvent)*setsize);
    event_loop->fired = malloc(sizeof(SeFiredEvent)*setsize);
    if (event_loop->ievents == NULL || event_loop->fired == NULL) goto err;
    event_loop->setsize = setsize;
    event_loop->last_time = time(NULL);
    event_loop->time_event_head = NULL;
    event_loop->time_event_next_id = 0;
    event_loop->stop = 0;
    event_loop->maxfd = -1;
    event_loop->before_sleep = NULL;
    if (api_create(event_loop) == -1) goto err;
    /* Events with mask == SE_NONE are not set. So let's initialize the
     * vector with it. */
    for (i = 0; i < setsize; i++)
        event_loop->events[i].mask = SE_NONE;
    return event_loop;

err:
    if (event_loop) {
        free(eventLoop->events);
        free(eventLoop->fired);
        free(eventLoop);
    }
    return NULL;
}

/* Return the current set size. */
int SeGetSetSize(SeEventLoop *event_loop) {
    return event_loop->setsize;
}

/* Resize the maximum set size of the event loop.
 * If the requested set size is smaller than the current set size, but
 * there is already a file descriptor in use that is >= the requested
 * set size minus one, SE_ERR is returned and the operation is not
 * performed at all.
 *
 * Otherwise SE_OK is returned and the operation is successful. */
int SeResizeSetSize(SeEventLoop* event_loop, int setsize) {

    if (setsize == event_loop->setsize) return SE_OK;
    if (event_loop->maxfd >= setsize) return SE_ERR;
    if (se_api_resize(event_loop, setsize) == -1) return SE_ERR;

    event_loop->events = realloc(event_loop->events, sizeof(SeFileEvent) * setsize);
    event_loop->fired = realloc(event_loop->fired, sizeof(SeFiredEvent) * setsize);
    event_loop->setsize = setsize;

    /* Make sure that if we created new slots, they are initialized with
     * an SE_NONE mask. */
    for (int i = event_loop->maxfd+1; i < setsize; ++i)
        event_loop->events[i].mask = SE_NONE;
    return SE_OK;
}

void SeDeleteEventLoop(SeEventLoop* event_loop) {
    api_free(event_loop);
    free(event_loop->events);
    free(event_loop->fired);
    free(event_loop);
}

void SeStop(SeEventLoop* event_loop) {
    event_loop->stop = 1;
}

int SeCreateFileEvent(SeEventLoop *event_loop, int fd, int mask, SeFileProc *proc, void *client) {
    if (fd >= event_loop->setsize) {
        errno = ERANGE;
        return SE_ERR;
    }
    SeFileEvent *fe = &event_loop->events[fd];

    if (api_add_event(event_loop, fd, mask) == -1)
        return SE_ERR;
    fe->mask |= mask;
    if (mask & SE_READABLE) fe->rfile_proc = proc;
    if (mask & SE_WRITABLE) fe->wfile_proc = proc;
    fe->client = client;
    if (fd > event_loop->maxfd)
        event_loop->maxfd = fd;
    return SE_OK;
}//end-SeCreateFileEvent.

void SeDeleteFileEvent(SeEventLoop *event_loop, int fd, int mask) {
    if (fd >= event_loop->setsize) return;
    SeFileEvent *fe = &event_loop->events[fd];
    if (fe->mask == SE_NONE) return;

    api_del_event(event_loop, fd, mask);
    fe->mask = fe->mask & (~mask);
    if (fd == event_loop->maxfd && fe->mask == SE_NONE) {
        /* Update the max fd */
        int j;
        for (j = event_loop->maxfd-1; j >= 0; --j)
            if (event_loop->events[j].mask != SE_NONE) break;
        event_loop->maxfd = j;
    }
}//end-SeDeleteFileEvent.

int SeGetFileEvents(SeEventLoop* event_loop, int fd) {
    if (fd >= event_loop->setsize) return 0;
    SeFileEvent* fe = &event_loop->events[fd];

    return fe->mask;
}//end-SeGetFileEvents.

static void SeGetTime(long *seconds, long *milliseconds) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec/1000;
}

static void SeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms) {
    long cur_sec, cur_ms, when_sec, when_ms;

    SeGetTime(&cur_sec, &cur_ms);
    when_sec = cur_sec + milliseconds/1000;
    when_ms = cur_ms + milliseconds%1000;
    if (when_ms >= 1000) {
        when_sec ++;
        when_ms -= 1000;
    }
    *sec = when_sec;
    *ms = when_ms;
}

long long SeCreateTimeEvent(SeEventLoop* event_loop, long long milliseconds,
        SeTimeProc *proc, void *client, SeEventFinalizerProc *finalizer_proc) {
    long long id = event_loop->time_event_next_id++;
    SeTimeEvent *te;

    te = malloc(sizeof(*te));
    if (te == NULL) return SE_ERR;
    te->id = id;
    SeAddMillisecondsToNow(milliseconds, &te->when_sec, &te->when_ms);
    te->time_proc = proc;
    te->finalizer_proc = finalizer_proc;
    te->client = client;
    te->next = event_loop->time_event_head;
    event_loop->time_event_head = te;
    return id;
}

int SeDeleteTimeEvent(SeEventLoop* eventLoop, long long id) {
    SeTimeEvent *te, *prev = NULL;

    te = eventLoop->time_event_head;
    while(te) {
        if (te->id == id) {
            if (prev == NULL)
                eventLoop->time_event_head = te->next;
            else
                prev->next = te->next;
            if (te->finalizer_proc)
                te->finalizer_proc(eventLoop, te->client);
            free(te);
            return SE_OK;
        }
        prev = te;
        te = te->next;
    }
    return SE_ERR; /* NO event with the specified ID found */
}

/* Search the first timer to fire.
 * This operation is useful to know how many time the select can be
 * put in sleep without to delay any event.
 * If there are no timers NULL is returned.
 *
 * Note that's O(N) since time events are unsorted.
 * Possible optimizations (not needed by Redis so far, but...):
 * 1) Insert the event in order, so that the nearest is just the head.
 *    Much better but still insertion or deletion of timers is O(N).
 * 2) Use a skiplist to have this operation as O(1) and insertion as O(log(N)).
 */
static SeTimeEvent* SeSearchNearestTimer(SeEventLoop *event_loop) {
    SeTimeEvent* te = event_loop->time_event_head;
    SeTimeEvent* nearest = NULL;

    while(te) {
        if (!nearest || te->when_sec < nearest->when_sec ||
                (te->when_sec == nearest->when_sec &&
                 te->when_ms < nearest->when_ms))
            nearest = te;
        te = te->next;
    }//end-while.
    return nearest;
}//end-SeSearchNearestTimer.

/* Process time events */
static int ProcessTimeEvents(SeEventLoop* event_loop) {
    int processed = 0;
    SeTimeEvent *te;
    long long max_id;
    time_t now = time(NULL);

    /* If the system clock is moved to the future, and then set back to the
     * right value, time events may be delayed in a random way. Often this
     * means that scheduled operations will not be performed soon enough.
     *
     * Here we try to detect system clock skews, and force all the time
     * events to be processed ASAP when this happens: the idea is that
     * processing events earlier is less dangerous than delaying them
     * indefinitely, and practice suggests it is. */
    if (now < event_loop->last_time) {
        te = event_loop->time_event_head;
        while(te) {
            te->when_sec = 0;
            te = te->next;
        }//end-while.
    }//end-if.
    event_loop->last_time = now;

    te = event_loop->time_event_head;
    max_id = event_loop->time_event_next_id - 1;
    while(te) {
        long now_sec, now_ms;
        long long id;

        if (te->id > max_id) {
            te = te->next;
            continue;
        }
        SeGetTime(&now_sec, &now_ms);
        if (now_sec > te->when_sec ||
            (now_sec == te->when_sec && now_ms >= te->when_ms)) {
            int retval;

            id = te->id;
            retval = te->time_proc(eventLoop, id, te->clientData);
            processed++;
            /* After an event is processed our time event list may
             * no longer be the same, so we restart from head.
             * Still we make sure to don't process events registered
             * by event handlers itself in order to don't loop forever.
             * To do so we saved the max ID we want to handle.
             *
             * FUTURE OPTIMIZATIONS:
             * Note that this is NOT great algorithmically. Redis uses
             * a single time event so it's not a problem but the right
             * way to do this is to add the new elements on head, and
             * to flag deleted elements in a special way for later
             * deletion (putting references to the nodes to delete into
             * another linked list). */
            if (retval != SE_NOMORE) {
                SeAddMillisecondsToNow(retval, &te->when_sec, &te->when_ms);
            } else {
                SeDeleteTimeEvent(event_loop, id);
            }
            te = event_loop->time_event_head;
        } else {
            te = te->next;
        }
    }
    return processed;
}

/* Process every pending time event, then every pending file event
 * (that may be registered by time event callbacks just processed).
 * Without special flags the function sleeps until some file event
 * fires, or when the next time event occurs (if any).
 *
 * If flags is 0, the function does nothing and returns.
 * if flags has SE_ALL_EVENTS set, all the kind of events are processed.
 * if flags has SE_FILE_EVENTS set, file events are processed.
 * if flags has SE_TIME_EVENTS set, time events are processed.
 * if flags has SE_DONT_WAIT set the function returns ASAP until all
 * the events that's possible to process without to wait are processed.
 *
 * The function returns the number of events processed. */
int SeProcessEvents(SeEventLoop *event_loop, int flags) {
    int processed = 0, numevents;

    /* Nothing to do? return ASAP */
    if (!(flags & SE_TIME_EVENTS) && !(flags & SE_FILE_EVENTS)) return 0;

    /* Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire. */
    if (event_loop->maxfd != -1 ||
        ((flags & SE_TIME_EVENTS) && !(flags & SE_DONT_WAIT))) {
        int j;
        SeTimeEvent *shortest = NULL;
        struct timeval tv, *tvp;

        if (flags & SE_TIME_EVENTS && !(flags & SE_DONT_WAIT))
            shortest = SeSearchNearestTimer(event_loop);
        if (shortest) {
            long now_sec, now_ms;

            /* Calculate the time missing for the nearest
             * timer to fire. */
            SeGetTime(&now_sec, &now_ms);
            tvp = &tv;
            tvp->tv_sec = shortest->when_sec - now_sec;
            if (shortest->when_ms < now_ms) {
                tvp->tv_usec = ((shortest->when_ms+1000) - now_ms)*1000;
                tvp->tv_sec --;
            } else {
                tvp->tv_usec = (shortest->when_ms - now_ms)*1000;
            }
            if (tvp->tv_sec < 0) tvp->tv_sec = 0;
            if (tvp->tv_usec < 0) tvp->tv_usec = 0;
        } else {
            /* If we have to check for events but need to return
             * ASAP because of SE_DONT_WAIT we need to set the timeout
             * to zero */
            if (flags & SE_DONT_WAIT) {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } else {
                /* Otherwise we can block */
                tvp = NULL; /* wait forever */
            }
        }

        numevents = api_poll(event_loop, tvp);
        for (j = 0; j < numevents; ++j) {
            SeFileEvent *fe = &event_loop->events[event_loop->fired[j].fd];
            int mask = event_loop->fired[j].mask;
            int fd = event_loop->fired[j].fd;
            int rfired = 0;

	        /* note the fe->mask & mask & ... code: maybe an already processed
             * event removed an element that fired and we still didn't
             * processed, so we check if the event is still valid. */
            if (fe->mask & mask & SE_READABLE) {
                rfired = 1;
                fe->rfile_proc(event_loop, fd, fe->client, mask);
            }
            if (fe->mask & mask & SE_WRITABLE) {
                if (!rfired || fe->wfileProc != fe->rfileProc)
                    fe->wfile_proc(event_loop, fd, fe->client, mask);
            }
            ++processed;
        }
    }
    /* Check time events */
    if (flags & SE_TIME_EVENTS)
        processed += ProcessTimeEvents(event_loop);

    return processed; /* return the number of processed file/time events */
}

}//end-cromwell.
