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

//    struct list_head list;
};

typedef struct _event_queue event_queue_t;
struct _event_queue {
    event_t ev;
    struct list_head list;
};

typedef void * (*event_callback) (event_t * ev);

void EVENT_INIT(event_t * ev);
int  event_queue_run      (struct list_head * queue);
int  event_queue_stop     (struct list_head * queue);
int  event_queue_push_back(struct list_head * queue, event_t ev, long int delay);
void event_queue_delete   (struct list_head * queue, event_queue_t * item);
void event_queue_release  (struct list_head * queue);

void * DEFAULT_EVENT_CALLBACK(struct list_head * queue, event_t * ev);
void * DEFAULT_TIMEOUT       (struct list_head * queue, event_t * ev);

#endif  /* _EVENT_QUEUE_H_ */
