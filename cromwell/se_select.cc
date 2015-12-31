#include <string.h>

namespace cromwell {

typedef struct ApiState {
	fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds;
} ApiState;

static int api_create(SeEventLoop* event_loop) {
    ApiState *state = malloc(sizeof(ApiState));
    if (!state) return -1;

    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    event_loop->api_data = state;
    return 0;
}

static int api_resize(SeEventLoop* eventLoop, int setsize) {
    /* Just ensure we have enough room in the fd_set type. */
    if (setsize >= FD_SETSIZE) return -1;
    return 0;
}

static void api_free_data(SeEventLoop* event_loop) {
    free(event_loop->api_data);
}

static int api_add_event(SeEventLoop* event_loop, int fd, int mask) {
    SeApiState *state = event_loop->api_data;

    if (mask & SE_READABLE) FD_SET(fd, &state->rfds);
    if (mask & SE_WRITABLE) FD_SET(fd, &state->wfds);
    return 0;
}

static void api_del_event(SeEventLoop* event_loop, int fd, int mask) {
    SeApiState *state = event_loop->api_data;

    if (mask & SE_READABLE) FD_CLR(fd, &state->rfds);
    if (mask & SE_WRITABLE) FD_CLR(fd, &state->wfds);
}

static int api_poll(SeEventLoop* event_loop, struct timeval* tvp) {
    SeApiState* state = event_loop->api_data;

    memcpy(&state->_rfds, &state->rfds, sizeof(fd_set));
    memcpy(&state->_wfds, &state->wfds, sizeof(fd_set));

    int numevents = 0;
    int retval = select(event_loop->maxfd+1, &state->_rfds, &state->_wfds, NULL, tvp);
    if (retval > 0) {    	
        for (int j = 0; j <= event_loop->maxfd; ++j) {
            int mask = 0;
            SeFileEvent* fe = &event_loop->events[j];

            if (fe->mask == SE_NONE) continue;
            if (fe->mask & SE_READABLE && FD_ISSET(j, &state->_rfds))
                mask |= SE_READABLE;
            if (fe->mask & SE_WRITABLE && FD_ISSET(j, &state->_wfds))
                mask |= SE_WRITABLE;
            eventLoop->fired[numevents].fd = j;
            eventLoop->fired[numevents].mask = mask;
            ++numevents;
        }//end-for
    }//end-if

    return numevents;
}//end-api-poll.

static const char* api_name(void) {
    static const char* name = "select";
    return name;
}

}//end-cromwell.