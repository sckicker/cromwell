#pragma once

namespace cromwell {

#define SE_OK 0
#define SE_ERR -1

#define SE_NONE 0
#define SE_READABLE 1
#define SE_WRITABLE 2

#define SE_FILE_EVENTS 1
#define SE_TIME_EVENTS 2
#define SE_ALL_EVENTS (SE_FILE_EVENTS|SE_TIME_EVENTS)
#define SE_DONT_WAIT 4

#define SE_NOMORE -1

/* Macros */
#define SE_NOTUSED(V) ((void) V)

struct SeEventLoop;

/* Types and data structures */
typedef void SeFileProc(struct SeEventLoop *event_loop, int fd, void *client, int mask);
typedef int SetTimeProc(struct SeEventLoop *event_loop, long long id, void *client);
typedef void SeEventFinalizerProc(struct SeEventLoop *event_loop, void *client);
typedef void SeBeforeSleepProc(struct SeEventLoop *event_loop);

/* File event structure */
typedef struct SeFileEvent {
    int mask; /* one of SE_(READABLE|WRITABLE) */
    SeFileProc* rfile_proc;
    SeFileProc* wfile_proc;
    void* client;
} SeFileEvent;

/* Time event structure */
typedef struct SeTimeEvent {
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    SeTimeProc* time_proc;
    SeEventFinalizerProc *finalizer_proc;
    void* client;
    struct SeTimeEvent *next;
} SeTimeEvent;

/* A fired event */
typedef struct SeFiredEvent {
    int fd;
    int mask;
} SeFiredEvent;

/* State of an event based program */
typedef struct SeEventLoop {
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */
    long long time_event_next_id;
    time_t last_time;     /* Used to detect system clock skew */
    SeFileEvent* events; /* Registered events */
    SeFiredEvent* fired; /* Fired events */
    SeTimeEvent* time_event_head;
    int stop;
    void* api_data; /* This is used for polling API specific data */
    SeBeforeSleepProc* before_sleep;
} SeEventLoop;

/* Prototypes */
SeEventLoop *SeCreateEventLoop(int setsize);
void SeDeleteEventLoop(SeEventLoop *event_loop);
void SeStop(SeEventLoop *event_loop);
int SeCreateFileEvent(SeEventLoop *event_loop, int fd, int mask, SeFileProc* proc, void* client);
void SeDeleteFileEvent(SeEventLoop *event_loop, int fd, int mask);
int SeGetFileEvents(SeEventLoop *event_loop, int fd);
long long SeCreateTimeEvent(SeEventLoop *event_loop, long long milliseconds,
        SeTimeProc *proc, void *client,
        SeEventFinalizerProc *finalizer_proc);
int SeDeleteTimeEvent(SeEventLoop* event_loop, long long id);
int SeProcessEvents(SeEventLoop* event_loop, int flags);
int SeWait(int fd, int mask, long long milliseconds);
void SeMain(SeEventLoop* event_loop);
char *SeGetApiName(void);
void SeSetBeforeSleepProc(SeEventLoop* event_loop, SeBeforeSleepProc* before_sleep);
int SeGetSetSize(SeEventLoop* event_loop);
int SeResizeSetSize(SeEventLoop* event_loop, int setsize);

}//end-cromwell.