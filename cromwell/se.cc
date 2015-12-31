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
    if (se_api_create(event_loop) == -1) goto err;
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
int SeResizeSetSize(SeEventLoop *event_loop, int setsize) {

    if (setsize == event_loop->setsize) return SE_OK;
    if (event_loop->maxfd >= setsize) return SE_ERR;
    if (se_api_resize(event_loop, setsize) == -1) return SE_ERR;

    event_loop->events = realloc(event_loop->events, sizeof(SeFileEvent)*setsize);
    event_loop->fired = realloc(event_loop->fired, sizeof(SeFiredEvent)*setsize);
    event_loop->setsize = setsize;

    /* Make sure that if we created new slots, they are initialized with
     * an SE_NONE mask. */
    for (int i = event_loop->maxfd+1; i < setsize; i++)
        event_loop->events[i].mask = SE_NONE;
    return SE_OK;
}

void SeDeleteEventLoop(SeEventLoop* event_loop) {
    se_api_free(event_loop);
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

    if (se_api_add_event(event_loop, fd, mask) == -1)
        return SE_ERR;
    fe->mask |= mask;
    if (mask & SE_READABLE) fe->rfile_proc = proc;
    if (mask & SE_WRITABLE) fe->wfile_proc = proc;
    fe->client = client;
    if (fd > event_loop->maxfd)
        event_loop->maxfd = fd;
    return SE_OK;
}

}//end-cromwell.
