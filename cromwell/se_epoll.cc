#include <sys/epoll.h>

namespace cromwell {

typedef struct ApiState {
    int epfd;
    struct epoll_event* events;
} ApiState;

static int api_create(SeEventLoop* event_loop) {
    ApiState* state = malloc(sizeof(ApiState));

    if (!state) return -1;
    state->events = malloc(sizeof(struct epoll_event) * event_loop->setsize);
    if (!state->events) {
        free(state);
        return -1;
    }
    state->epfd = epoll_create(1024); /* 1024 is just a hint for the kernel */
    if (state->epfd == -1) {
        free(state->events);
        free(state);
        return -1;
    }
    eventLoop->api_data = state;
    return 0;
}

static int api_resize(SeEventLoop* event_loop, int setsize) {
    ApiState *state = event_loop->api_data;

    state->events = realloc(state->events, sizeof(struct epoll_event) * setsize);
    return 0;
}

static void api_free(SeEventLoop* event_loop) {
    ApiState* state = event_loop->api_data;

    close(state->epfd);
    free(state->events);
    free(state);
}

static int api_add_event(SeEventLoop* event_loop, int fd, int mask) {
    ApiState* state = eventLoop->api_data;
    struct epoll_event ee;
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = event_loop->events[fd].mask == AE_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= event_loop->events[fd].mask; /* Merge old events */
    if (mask & SE_READABLE) ee.events |= EPOLLIN;
    if (mask & SE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (epoll_ctl(state->epfd, op, fd, &ee) == -1) return -1;
    return 0;
}//end-api_add_event.

static void api_del_event(SeEventLoop* event_loop, int fd, int delmask) {
    ApiState* state = eventLoop->api_data;
    struct epoll_event ee;
    int mask = event_loop->events[fd].mask & (~delmask);

    ee.events = 0;
    if (mask & SE_READABLE) ee.events |= EPOLLIN;
    if (mask & SE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (mask != SE_NONE) {
        epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
    }
}//end-api_del_event.

static int api_poll(SeEventLoop* event_loop, struct timeval* tvp) {
    ApiState* state = event_loop->api_data;

    int numevents = 0;
    int retval = epoll_wait(state->epfd, state->events, event_loop->setsize, 
        tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
    if (retval > 0) {
        numevents = retval;
        for (int j = 0; j < numevents; ++j) {
            int mask = 0;
            struct epoll_event* e = state->events+j;

            if (e->events & EPOLLIN) mask |= SE_READABLE;
            if (e->events & EPOLLOUT) mask |= SE_WRITABLE;
            if (e->events & EPOLLERR) mask |= SE_WRITABLE;
            if (e->events & EPOLLHUP) mask |= SE_WRITABLE;
            event_loop->fired[j].fd = e->data.fd;
            event_loop->fired[j].mask = mask;
        }//end-for.
    }//end-if.

    return numevents;
}//end-api_poll.

static const char* api_name(void) {
    static const char* name = "epoll";
    return name;
}

}//end-cromwell.