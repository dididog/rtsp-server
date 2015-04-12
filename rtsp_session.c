//#include "rtsp_sesion.h"
#include "rtsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

rtsp_session_t * rtsp_session_create()
{
    rtsp_session_t * it = (rtsp_session_t *)malloc(sizeof(rtsp_session_t));
    it->state = STATE_INIT;
    it->session_id = rtsp_random();
    INIT_LIST_HEAD(&(it->rtp_list));

    return it;
}

void rtsp_session_push_back(struct list_head * session_list,
                                        rtsp_session_t * item)
{
    assert(item != NULL);
    assert(session_list != NULL);
    list_add_tail(&(item->list), session_list);

//    return session_list;
}

void rtsp_session_remove(struct list_head * session_list,
                                     int session_id)
{
    assert(session_list != NULL);
    rtsp_session_t * item = rtsp_session_find_by_id(session_list, session_id);
    if (item == NULL) {
        return;
    }

    rtsp_session_delete(item);
}

rtsp_session_t * rtsp_session_find_by_id(struct list_head * session_list,
                                         int session_id)
{
    assert(session_list != NULL);
    rtsp_session_t * item = NULL;
    struct list_head * pos = NULL;

    list_for_each(pos, session_list)  {
        item = list_entry(pos, rtsp_session_t, list);
        if (item->session_id == session_id) {
            break;
        }
        item = NULL;
    }

    return item;
}

void rtsp_session_delete(rtsp_session_t * item)
{
    list_del(&(item->list));
    free(item);
}

void rtsp_session_release(struct list_head * session_list)
{
    struct list_head * pos = NULL;
    struct list_head * tmp = NULL;
    rtsp_session_t * item = NULL;

    //if (!&(session_list->list))
    if (list_empty(session_list))
        return;

    list_for_each_safe(pos, tmp, session_list) {
        item = list_entry(pos, rtsp_session_t, list);
        free(item);
    }
}

