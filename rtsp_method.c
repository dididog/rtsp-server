#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rtsp.h"
#include "list.h"
#include "rtp.h"

extern struct list_head g_event_queue;
extern int g_server_port;

//static int method_setup_reply(rtsp_buffer_t * rtsp, int session_id)
static int method_setup_reply(rtsp_buffer_t * rtsp, rtsp_session_t * session)
{
    char out[RTSP_BUFFER_SIZE] = {0};

    sprintf(out, "%s %d %s"RTSP_ENDLINE"CSeq: %d"RTSP_ENDLINE, RTSP_VER,
            200, rtsp_get_err_state(200), rtsp->cseq);
    sprintf(out + strlen(out), "Session: %d"RTSP_ENDLINE, session->session_id);
    sprintf(out + strlen(out), "Transport: RTP/AVP/UDP;unicast;"
            "client_port=%d-%d;server_port=%d-"RTSP_ENDLINE,
            session->client_port[0], session->client_port[1],
            g_server_port);
    strcat(out, RTSP_ENDLINE);

    return rtsp_fill_out_buffer(rtsp, out, strlen(out));
}

static int method_play_reply(rtsp_buffer_t * rtsp)
{
    char out[RTSP_BUFFER_SIZE] = {0};

    sprintf(out, "%s %d %s"RTSP_ENDLINE"CSeq: %d"RTSP_ENDLINE, RTSP_VER,
            200, rtsp_get_err_state(200), rtsp->cseq);

    //Range
    //RTP-Info
    return rtsp_fill_out_buffer(rtsp, out, strlen(out));
}

int rtsp_method_options(rtsp_buffer_t *rtsp)
{
    char out[RTSP_BUFFER_SIZE] = {0};

//    LOG("OPTIONS METHOD\n");
    sprintf(out, "%s %d %s"RTSP_ENDLINE"CSeq: %d"RTSP_ENDLINE, RTSP_VER,
            200, rtsp_get_err_state(200), rtsp->cseq);
    strcat(out, "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN"RTSP_ENDLINE);
    strcat(out, RTSP_ENDLINE);

    return rtsp_fill_out_buffer(rtsp, out, strlen(out));
}

static void add_time_stamp(char * b)    
{
    struct tm * t;
    time_t now;
    
    now = time(NULL);
    t = gmtime(&now);
    strftime(b + strlen(b), 38, "Date: %a, %d %b %Y %H:%M:%S GMT"RTSP_ENDLINE,
            t);  
}

static unsigned long get_file_size(const char * filename)
{
    struct stat statbuff;

    if (stat(filename, &statbuff) < 0){
        return 0;
    }

    return statbuff.st_size;
}

static void read_file_content(const char * filename,
        char * content, unsigned long size)
{
    FILE * fp = NULL;

    fp = fopen(filename, "r");
    fread(content, 1, size, fp);
    fclose(fp);
}
            

int rtsp_method_describe(rtsp_buffer_t * rtsp)
{
    char out[RTSP_BUFFER_SIZE + RTSP_DESCR_SIZE] = {0};
    char filename[256];
    size_t filename_len = 256;
    char server[256];
    size_t server_len = 256;
    unsigned short port;
    char * p;
    char sdpfile[256] = {0};
//    FILE * fp = NULL;
    char * sdp_content = NULL;
    unsigned long sdp_content_len = 0;
    char * filename_b;

    //LOG("DESCRIBE METHOD\n");
    rtsp_parse_url(rtsp->url, server, server_len, &port, filename, filename_len);

    if (access(filename, R_OK) < 0) {
        rtsp_send_reply(404, NULL, rtsp);
        return ERR_NOT_FOUND;
    }

    filename_b = strdup(filename);
    p = strrchr(filename, '.');
    *p = '\0';
    strcpy(sdpfile, filename);
    strcat(sdpfile, ".sdp");
    if (access(sdpfile, R_OK) < 0) {
        rtsp_send_reply(404, NULL, rtsp);
        return ERR_NOT_FOUND;
    }

    sprintf(out, "%s %d %s"RTSP_ENDLINE"CSeq: %d"RTSP_ENDLINE, RTSP_VER,
            200, rtsp_get_err_state(200), rtsp->cseq);
    add_time_stamp(out);
    sdp_content_len = get_file_size(sdpfile);
    if (sdp_content_len == 0) {
        rtsp_send_reply(404, NULL, rtsp);
        return ERR_NOT_FOUND;
    }

    strcat(out, "Content-Type: application/sdp"RTSP_ENDLINE);
    sprintf(out + strlen(out), "Content-Base: rtsp://%s:%d/%s/"RTSP_ENDLINE,
            server, port, filename_b);
    sprintf(out + strlen(out), "Content-Length: %lu"RTSP_ENDLINE,
            sdp_content_len);
    //消息头结束
    strcat(out + strlen(out), RTSP_ENDLINE);

    //消息体
    sdp_content = (char *)malloc(sdp_content_len);
    read_file_content(sdpfile, sdp_content, sdp_content_len);
    strcat(out + strlen(out), sdp_content);
    free(sdp_content);
    free(filename_b);

    return rtsp_fill_out_buffer(rtsp, out, strlen(out));
}

int rtsp_method_setup(rtsp_buffer_t * rtsp)
{
    char hdr_trans[128] = {0};
    char * ptr = NULL;
    int client_port[2];
    int valid = 0;
    int session_id;
    rtsp_session_t * session = NULL;
    char filename[256];
    size_t filename_size = 256;

    //LOG("SETUP METHOD\n");
    if ((ptr = strstr(rtsp->in_buffer, HDR_TRANSPORT)) == NULL) {
        rtsp_send_reply(400, NULL, rtsp);
        fprintf(stderr, "HDR Transport is not found\n");
        return ERR_GENERIC;
    }

    /*
     * 将Transport后的内容读入内存，依次比较请求是否合法
     */
    if (sscanf(ptr, "%*10s %127s", hdr_trans) != 2) {
        char * trans = NULL;

        if ((trans = strstr(hdr_trans, RTSP_RTP_AVP))) {
            trans += strlen(RTSP_RTP_AVP);
            if ((*trans == ';' || *trans == ' ' || !*trans) ||
                ((trans == strstr(trans, "/UDP")))) {
                if (strstr(hdr_trans, "unicast")) {
                    char * port_str = NULL;

                    /*
                     * 读取client port
                     */
                    if ((port_str = strstr(hdr_trans, "client_port"))) {
                        if ((port_str = strstr(hdr_trans, "="))) {
                            port_str += 1;
                            if (sscanf(port_str, "%d", &client_port[0])) {
                                if ((port_str = strstr(hdr_trans, "-"))) {
                                    port_str += 1;
                                    sscanf(port_str, "%d", &client_port[1]);
                                    valid = 1;
                                }
                                valid = 1;
                            }
                        }
                    }
                } else {
                    /* 目前只支持单播 */
                    rtsp_send_reply(461, "Only unicast Supported", rtsp);
                    return ERR_GENERIC;
                }
            } else if (strstr(trans, "/TCP")) {
                /* 目前只支持UDP */
                rtsp_send_reply(461, "RTP over TCP not Supported", rtsp);
                return ERR_GENERIC;
            }
        }
    }

    /* 如果请求不合法 */
    if (!valid) {
        rtsp_send_reply(400, NULL, rtsp);
        return ERR_GENERIC;
    }

    session_id = rtsp_random();

    /* 加入rtsp session链表 */
    session = rtsp_session_create();
    session->client_port[0] = client_port[0];
    session->client_port[1] = client_port[1];
    session->state = STATE_READY;
    list_add_tail(&(session->list), &(rtsp->session_list));

    /* 加入rtp session 链表 */
    rtsp_parse_url(rtsp->url, NULL, 0, NULL, filename, filename_size);

    {
        //需要检测文件名的合法性

        FILE * fp;
//        struct sockaddr_in addr;
 //       struct sockaddr_in src_addr;
  //      socklen_t src_len;
        //rtp_session_t * rtp = NULL;
        //rtsp_session_t * session = NULL;

        fp = fopen(filename, "r");
        if (fp == NULL) {
            LOG("open '%s' error:%s\n", filename, strerror(errno));
            rtsp_send_reply(404, NULL, rtsp);
            return ERR_GENERIC;
        }

//        session = rtsp_session_find_by_id(&(rtsp->session_list), session_id);
//        ret = getpeername(rtsp->fd, (struct sockaddr *)&src_addr, &src_len);
//        if (ret < 0) {
//            LOG("error: getpeername");
//        }
//        addr.sin_family = AF_INET;
//        addr.sin_port = htons(session->client_port[0]);
//        addr.sin_addr.s_addr = src_addr.sin_addr.s_addr;
//        bzero(&(addr.sin_zero), 8);
//        rtp = rtp_session_create(get_packet_ts_file);
//        rtp->destaddr = addr;
//        rtp->clock_rate = 90000.0;
//        rtp->packet_rate = 24.0;
//        rtp->src = fp;
//        rtp_session_set_payload_type(rtp, 33);
//        list_add_tail(&(rtp->list), &(session->rtp_list));
    }

    return method_setup_reply(rtsp, session);
}


int rtsp_method_play(rtsp_buffer_t * rtsp)
{
    int session_id = 0;
//    int hour_s, min_s, sec_s, frame_s, subframe_s; //Range start
//    int hour_e, min_e, sec_e, frame_e, subframe_e; //Rage end
    char * ptr = NULL;
    int valid = 0;
    char filename[256];
    size_t filename_size = 256;

    //LOG("PLAY METHOD\n");
    if ((ptr = strstr(rtsp->in_buffer, HDR_SESSION))) {
        ptr += strlen(HDR_SESSION) + 1;
        if (sscanf(ptr, "%d", &session_id)) {
//            if ((ptr = strstr(ptr, HDR_RANGE))) {
//                if ((ptr = strstr(ptr, "smpte="))) {
//                    ptr += strlen("smpte=");
//
//                    hour_s = 0;  min_s   = 0;
//                    sec_s  = 0;  frame_s = 0;
//                    subframe_s = 0;
//                    sscanf(ptr, "%d:%d:%d:%d.%d", &hour_s, &min_s, &sec_s,
//                           &frame_s, &subframe_s);
//
//                    if (!(ptr = strstr(ptr, "-"))) {
//                        //error 400
//                        return ERR_GENERIC;
//                    }
//
//                    hour_e = 0;  min_e   = 0;
//                    sec_e  = 0;  frame_e = 0;
//                    subframe_e = 0;
//                    sscanf(ptr, "%d:%d:%d:%d.%d", &hour_e, &min_e, &sec_e,
//                           &frame_e, &subframe_e);
//
//                    valid = 1;
//                }
//            }
            valid = 1;
        }
    }

    if (!valid) {
        rtsp_send_reply(400, NULL, rtsp);
        return ERR_GENERIC;
    }

    /* RTSP Session处理 */
    if(list_empty(&(rtsp->session_list))) {
        rtsp_send_reply(503, NULL, rtsp);
        return ERR_FATAL; //session list must not be NULL
    }

    //{
    //    rtsp_session_t * session;
    //    event_t ev;

    //    session = rtsp_session_find_by_id(session_id);
    //    ev.opaque = rtp;
    /* 加入rtp session 链表 */
    rtsp_parse_url(rtsp->url, NULL, 0, NULL, filename, filename_size);

    {
        //需要检测文件名的合法性

        FILE * fp;
        struct sockaddr_in addr;
        struct sockaddr_in src_addr;
        int ret;
        int sock_fd;
        socklen_t src_len;
        rtp_session_t * rtp = NULL;
        rtsp_session_t * session = NULL;
        event_t ev;

        fp = fopen(filename, "r");
        if (fp == NULL) {
            LOG("open '%s' error:%s\n", filename, strerror(errno));
            rtsp_send_reply(404, NULL, rtsp);
            return ERR_GENERIC;
        }
        session = rtsp_session_find_by_id(&(rtsp->session_list), session_id);
        ret = getpeername(rtsp->fd, (struct sockaddr *)&src_addr, &src_len);
        if (ret < 0) {
            LOG("error: getpeername");
        }
        addr.sin_family = AF_INET;
        addr.sin_port = htons(session->client_port[0]);
        addr.sin_addr.s_addr = src_addr.sin_addr.s_addr;
        bzero(&(addr.sin_zero), 8);
        rtp = rtp_session_create(get_packet_ts_file);
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        rtp->sock_fd = sock_fd;
        rtp->destaddr = addr;
        rtp->clock_rate = 90000.0;
        rtp->packet_rate = 24.0;
        rtp->src = fp;
        rtp_session_set_payload_type(rtp, 33);
        list_add_tail(&(rtp->list), &(session->rtp_list));

        EVENT_INIT(&ev);
        ev.opaque = rtp;
        ev.fire = rtp_ts_file_callback;
        event_queue_push_back(&g_event_queue, ev, 0);
    }

    return method_play_reply(rtsp);
}

int rtsp_method_teardown(rtsp_buffer_t * rtsp)
{
    char * ptr = NULL;
    char out[RTSP_BUFFER_SIZE] = {0};

    if ((ptr = strstr(rtsp->in_buffer, HDR_SESSION)) == NULL) {
        rtsp_send_reply(400, NULL, rtsp);
        return ERR_GENERIC;
    }

    /* session操作 */

    sprintf(out, "%s %d %s"RTSP_ENDLINE"CSeq: %d"RTSP_ENDLINE, RTSP_VER,
            200, rtsp_get_err_state(200), rtsp->cseq);
    strcat(out, RTSP_ENDLINE);

    return rtsp_fill_out_buffer(rtsp, out, strlen(out));
}
