#ifndef _RTSP_SESSION_H_
#define _RTSP_SESSION_H_

#include "list.h"
#include "rtsp.h"
#include "rtp.h"

struct rtsp_session {
    int state;
    int session_id;
    int client_port[2];
    struct list_head rtp_list;

    struct list_head list;
};
typedef struct rtsp_session rtsp_session_t;


rtsp_session_t * rtsp_session_create();

rtsp_session_t * rtsp_session_push_back(struct list_head * session_list,
                                        rtsp_session_t * item);
rtsp_session_t * rtsp_session_find_by_id(struct list_head * session_list,
                                         int session_id);
void rtsp_session_remove(struct list_head * session_list,
                                     int session_id);
void rtsp_session_delete(rtsp_session_t * item);

void rtsp_session_release(struct list_head * session_list);

#endif /* _RTSP_SESSION_H_ */
