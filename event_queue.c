#include "event_queue.h"
#include "list.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

static volatile int s_thread_keep = 1;

void EVENT_INIT(event_t * ev)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    ev->expire_time = tv;
    ev->fire = NULL;
    ev->timeout = NULL;
}

int event_queue_push_back(struct list_head * queue, event_t ev, long int delay)
{
    int usecs_per_sec = 1000000;
    
    assert(queue != NULL);
    event_queue_t * item = (event_queue_t *)malloc(sizeof(event_queue_t));
    if (delay > 0) {
        ev.expire_time.tv_sec  += delay / usecs_per_sec;
        ev.expire_time.tv_usec += delay % usecs_per_sec;
    }
    item->ev = ev;
    list_add_tail(&(item->list), queue);

    return 0;
}

void event_queue_delete(struct list_head * queue, event_queue_t * item)
{
    assert(item != NULL && queue != NULL);
    list_del(&(item->list));
    free(item);
}

void event_queue_release(struct list_head * queue)
{
    struct list_head * pos, * n;
    event_queue_t * item;

    list_for_each_safe(pos, n, queue) {
        item = list_entry(pos, event_queue_t, list);
        list_del(pos);
        free(item);
    }
}

static void * _event_queue_worker_thread(void * arg)
{
    struct list_head * queue = (struct list_head *)arg;
    event_queue_t * item = NULL;
    struct list_head * pos, *n;
    event_t * ev;

#define ABS_ERROR_TIME 200

    while(s_thread_keep) {
        list_for_each_safe(pos, n, queue) {
            struct timeval tv;

            item = list_entry(pos, event_queue_t, list);
            ev = &item->ev;
            gettimeofday(&tv, NULL);
            if ((tv.tv_sec == ev->expire_time.tv_sec) && 
                    (labs(tv.tv_usec - ev->expire_time.tv_usec) < ABS_ERROR_TIME)) {
                if (ev->fire != NULL) {
                    ev->fire(ev);
                }
                event_queue_delete(queue, item);
            } else if (tv.tv_sec > ev->expire_time.tv_sec) {
                struct timeval tv;

                if (ev->timeout != NULL) {
                    ev->timeout(ev);
                }
                gettimeofday(&tv, NULL);
                tv.tv_usec += ABS_ERROR_TIME;
                ev->expire_time = tv;
            }
        }
        usleep(100);
    }
    event_queue_release(queue);

    return (void *)0;
}

int event_queue_run(struct list_head * queue)
{
    pthread_t pid;
    s_thread_keep = 1;
    
    if (pthread_create(&pid, NULL, _event_queue_worker_thread, (void *)queue) != 0) {
        return -1;
    }

    return 0;
}

int event_queue_stop(struct list_head * queue)
{
    s_thread_keep = 0;

    return 0;
}

