#include "list.h"
#include "rtp.h"
#include "event_queue.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

LIST_HEAD(g_event_list);
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_thread_keep = 1;

static int _event_queue_work_thread(void * arg)
{
    /* 在处理事件到期的时候，微秒数的误差设定一个范围 */
#define ABS_TIME 2000
    struct list_head *pos, *n;
    event_t * ev = NULL;
    while(s_thread_keep) {
        list_for_each_safe(pos, n, &g_event_list) {
            struct timeval tv;

            ev = list_entry(pos, event_t, list);
            gettimeofday(&tv, NULL);
            if (tv.tv_sec == ev->expire_time.tv_sec &&
                labs(tv.tv_usec - ev->expire_time.tv_usec) < ABS_TIME) {
                /* 事件到期 */
                //DEBUG("handle event\n");
                ev->fire(ev);
                event_queue_delete(ev);
            } else if (tv.tv_sec > ev->expire_time.tv_sec) {
                /* 超时处理 */
                struct timeval tv;

                //DEBUG("event timeout\n");
                ev->timeout(ev);
                gettimeofday(&tv, NULL);
                tv.tv_usec += 2000;
                ev->expire_time = tv;
                //event_queue_delete(ev);
            }
        }
        usleep(100);
    }
    event_queue_release();

    return 0;
}

void EVENT_INIT(event_t * ev)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    ev->expire_time = tv;
    ev->fire = DEFAULT_EVENT_CALLBACK;
    ev->timeout = DEFAULT_TIMEOUT;
}

int event_queue_run()
{
    pthread_t pid;

    s_thread_keep = 1;
    if(pthread_create(&pid, NULL, _event_queue_work_thread, NULL) != 0) {
        return -1;
    }

    return 0;
}

int event_queue_stop()
{
    s_thread_keep = 0;

    return 0;
}

int event_queue_push_back(event_t * ev, long int delay)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    if (delay > 0) {
        tv.tv_sec  += delay / 1000000;
        tv.tv_usec += delay % 1000000;
    }

    ev->expire_time = tv;
    event_t * p = (event_t *)malloc(sizeof(event_t));
    memcpy(p, ev, sizeof(event_t));
    pthread_mutex_lock(&s_mutex);
    list_add_tail(&(p->list), &(g_event_list));
    pthread_mutex_unlock(&s_mutex);

    return 0;
}

void event_queue_delete(event_t * ev)
{
    pthread_mutex_lock(&s_mutex);
    list_del(&(ev->list));
    free(ev);
    pthread_mutex_unlock(&s_mutex);
}

void event_queue_release()
{
    struct list_head *pos, *n;
    event_t * ev;

    pthread_mutex_lock(&s_mutex);
    list_for_each_safe(pos, n, &(g_event_list)) {
        ev = list_entry(pos, event_t, list);
        list_del(pos);
        free(ev);
    }
    pthread_mutex_unlock(&s_mutex);
}

void * DEFAULT_EVENT_CALLBACK(event_t * ev)
{
    return NULL;
}

void * DEFAULT_TIMEOUT(event_t * ev)
{
    return NULL;
}

