#ifndef _EVENT_QUEUE_H_
#define _EVENT_QUEUE_H_
#include "list.h"
#include <pthread.h>
#include <sys/time.h>

typedef struct _event event_t;
struct _event {
    void * opaque;
    struct timeval expire_time;
    void * (*fire) (event_t * ev);
    void * (*timeout) (event_t * ev);

    struct list_head list;
};

extern struct list_head g_event_list;

typedef void * (*event_callback) (event_t * ev);

void EVENT_INIT(event_t * ev);
int event_queue_run();
int event_queue_stop();
int event_queue_push_back(event_t * ev, long int delay);
void event_queue_delete(event_t * ev);
void event_queue_release();

void * DEFAULT_EVENT_CALLBACK(event_t * ev);
void * DEFAULT_TIMEOUT(event_t * ev);

#endif  /* _EVENT_QUEUE_H_ */
