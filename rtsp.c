#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "list.h"
#include "rtsp.h"
#include "rtp.h"
#include "rtsp_method.h"

extern struct list_head g_event_queue;

int rtsp_get_method(const char * name)
{
    static struct {
        int id;
        const char * name;
    } methods[] = {
//    { RTSP_ID_DESCRIBE,       "DESCRIBE"       },
//    { RTSP_ID_ANNOUNCE,       "ANNOUNCE"       },
//    { RTSP_ID_GET_PARAMETERS, "GET_PARAMETERS" },
//    { RTSP_ID_RECORD,         "RECORD"         },
//    { RTSP_ID_REDIRECT,       "REDIRECT"       },
//    { RTSP_ID_SET_PARAMETER,  "SET_PARAMETER"  },
            { RTSP_ID_OPTIONS,        "OPTIONS"        },
            { RTSP_ID_SETUP,          "SETUP"          },
            { RTSP_ID_PLAY,           "PLAY"           },
            { RTSP_ID_PAUSE,          "PAUSE"          },
            { RTSP_ID_TEARDOWN,       "TEARDOWN"       },
            { -1,                      NULL            },
    };

    int i;
    for (i = 0; methods[i].id != -1; ++i) {
        if(strcmp(name, methods[i].name) == 0) {
            break;
        }
    }

    return methods[i].id;
}

const char * rtsp_get_err_state(int err)
{
    static struct {
        char * token;
        int code;
    } status[] = {
        { "Continue"          , 100 },
        { "OK"                , 200 },
        { "Created"           , 201 },
        { "Accepted"          , 202 },
        { "Non-Authoritative Information", 203 },
        { "No Content"        , 204 },
        { "Reset Content"     , 205 },
        { "Partial Content"   , 206 },
        { "Multiple Choices"  , 300 },
        { "Moved Permanently" , 301 },
        { "Moved Temporarily" , 302 },
        { "Bad Request"       , 400 },
        { "Unauthorized"      , 401 },
        { "Payment Required"  , 402 },
        { "Forbidden"         , 403 },
        { "Not Found"         , 404 },
        { "Method Not Allowed", 405 },
        { "Not Acceptable"    , 406 },
        { "Proxy Authentication Required", 407 },
        { "Request Time-out"  , 408 },
        { "Conflict"          , 409 },
        { "Gone"              , 410 },
        { "Length Required"   , 411 },
        { "Precondition Failed"        , 412 },
        { "Request Entity Too Large"   , 413 },
        { "Request-URI Too Large"      , 414 },
        { "Unsupported Media Type"     , 415 },
        { "Bad Extension"              , 420 },
        { "Invalid Parameter"          , 450 },
        { "Parameter Not Understood"   , 451 },
        { "Conference Not Found"       , 452 },
        { "Not Enough Bandwidth"       , 453 },
        { "Session Not Found"          , 454 },
        { "Method Not Valid In This State"     , 455 },
        { "Header Field Not Valid for Resource", 456 },
        { "Invalid Range"              , 457 },
        { "Parameter Is Read-Only"     , 458 },
        { "Unsupported transport"      , 461 },
        { "Internal Server Error"      , 500 },
        { "Not Implemented"            , 501 },
        { "Bad Gateway"                , 502 },
        { "Service Unavailable"        , 503 },
        { "Gateway Time-out"           , 504 },
        { "RTSP Version Not Supported" , 505 },
        { "Option not supported"       , 551 },
        { "Extended Error:"            , 911 },
        { NULL, -1 }
    };

    int i;
    for (i = 0; status[i].code != err && status[i].code != -1; ++i);

    return status[i].token;
}

rtsp_buffer_t * rtsp_buffer_create()
{
    rtsp_buffer_t * t = NULL;

    t = (rtsp_buffer_t *)malloc(sizeof(rtsp_buffer_t));
    memset(t, 0, sizeof(rtsp_buffer_t));
    INIT_LIST_HEAD(&(t->session_list));

    return t;
}

/*
 *
 *
 */
int rtsp_check_request(rtsp_buffer_t * rtsp)
{
    char method[32] = {0};
    char hdr[16]    = {0};
    char url[256]   = {0};
    char ver[32]    = {0};
    char trash[256] = {0};
    char * p = NULL;
    int cseq = 0;
    int ret;
    //int index;

    char filename[256];
    unsigned short port;
    char server[128];

    /* 检查是否有 METHOD URL VERSION HDR  */
    if ((ret = sscanf(rtsp->in_buffer, " %31s %255s %31s\n%15s",
                      method, url, ver, hdr)) != 4) {
        DEBUG("%s ", method);
        DEBUG("%s ", url);
        DEBUG("%s ", ver);
        DEBUG("%s ", hdr);
        return ERR_GENERIC;
    }

    strcpy(rtsp->url, url);
    if ((p = strstr(rtsp->in_buffer, "CSeq")) == NULL) {
        return ERR_GENERIC;
    } else {
        if (sscanf(p, "%255s %d", trash, &cseq) != 2) {
            return ERR_GENERIC;
        }
    }
    rtsp->cseq = cseq;

    rtsp->method = rtsp_get_method(method);
    if (rtsp->method < 0) {
        rtsp_send_reply(400, NULL, rtsp);
        return ERR_GENERIC;
    }

    return rtsp_parse_url(url, server, sizeof(server), &port, filename,
                   sizeof(filename));
}

int rtsp_random()
{
    int out;

    srand(time(NULL));
    out = 1 + (int) (10.0 * rand() / (100000 + 1.0));

    return out;
}

int rtsp_fill_out_buffer(rtsp_buffer_t * rtsp, const char * data, int size)
{
    if (rtsp->out_size + size >= RTSP_BUFFER_SIZE + RTSP_DESCR_SIZE) {
        DEBUG("RTSP out buffer is too small");
        return ERR_GENERIC;
    }

    memcpy(&rtsp->out_buffer[rtsp->out_size], data, size);
    rtsp->out_size += size;

    return ERR_NOERROR;
}

int rtsp_send_reply(int errcode, char * addon,  rtsp_buffer_t * rtsp)
{
    char buffer[4096] = {0};
    int len = 0;

    sprintf(buffer, "%s %d %s"RTSP_ENDLINE"CSeq: %d"RTSP_ENDLINE,
            RTSP_VER, errcode, rtsp_get_err_state(errcode), rtsp->cseq);
    if (addon) {
        strcat(buffer, addon);
    }
    strcat(buffer, RTSP_ENDLINE);

    len = strlen(buffer);
    if (rtsp->out_size + len >= RTSP_BUFFER_SIZE + RTSP_DESCR_SIZE) {
        fprintf(stderr, "out buffer is too small");
        return ERR_GENERIC;
    }

    memcpy(rtsp->out_buffer + rtsp->out_size, buffer, len);
    rtsp->out_size += len;
    rtsp->out_buffer[rtsp->out_size] = '\0';

    return ERR_NOERROR;
}

/*
 *
 *
 */
int rtsp_parse_url(const char * url, char * server, size_t server_len, 
        unsigned short *  port, char * filename, size_t filename_len)
{
    char buffer[256] = {0};

    char _server[256];
    int  _server_len = 256;
    char _filename[256];
    int  _filename_len = 256;
    unsigned short _port = 0;

    strcpy(buffer, url);
    if(strncmp(buffer, "rtsp://", 7) == 0) {
        char * token = 0;
        int has_port = 0;
        char sub_str[256] = {0};
        char * p = NULL;

        strcpy(sub_str, &buffer[7]);
        if((p = strchr(sub_str, '/')) == NULL) {
            fprintf(stderr, "server:port not found\n");
            return ERR_GENERIC;
        } else {
            char server_str[256] = {0};
            char * t = NULL;

            strncpy(server_str, sub_str, p - sub_str);
            if ((t = strchr(server_str, ':')) != NULL) {
                has_port = 1;
            }
        }

        token = strtok(sub_str, " :/\t\n");
        if(token) {
            strncpy(_server, token, _server_len);
            if (_server[_server_len - 1]) {
                fprintf(stderr, "server buffer is too small\n");
                return ERR_GENERIC;
            }

            if (has_port) {
                char * port_str = NULL;
                int temp_port = 0;

                port_str = strtok(NULL, " /\t\n");
                temp_port = atoi(port_str);
                if(temp_port == 0) {
                    return ERR_GENERIC;
                } else {
                    _port = (unsigned short)temp_port;
                }
            } else {
                _port = (unsigned short)RTSP_DEFAULT_PORT;
            }

            token = strtok(NULL, " ");
            if (token) {
                strncpy(_filename, token, _filename_len);
                if(_filename[_filename_len - 1]) {
                    fprintf(stderr, "filename buffer is too small\n");
                    return ERR_GENERIC;
                }
            } else {
                _filename[0] = '\0';
                return ERR_GENERIC;
            }
        }
    } else {
        return ERR_GENERIC;
    }

    if (server) {
        strncpy(server, _server, server_len);
    }

    if (filename) {
        strncpy(filename, _filename, filename_len);
    }

    if (port) {
        *port = _port;
    }

    return ERR_NOERROR;
}


int rtsp_state_machine(rtsp_buffer_t * rtsp, int method_id)
{
    char trash[256] = {0};
    long int session_id;
    char * p;
    rtsp_session_t * session = NULL;
    struct list_head * pos = NULL;
    rtsp_session_t * item = NULL;

    /* 对于中带有Session的请求，状态机根据对应的值找到对应的Session */
    if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(p, "%255s %ld", trash, &session_id) != 2) {
            rtsp_send_reply(454, NULL, rtsp);
            return ERR_NOERROR;
        }

        if (!list_empty(&(rtsp->session_list))) {
            list_for_each(pos, &(rtsp->session_list)) {
                item = list_entry(pos, rtsp_session_t, list);
                if (item->session_id == session_id) {
                    session = item;
                    break;
                }
            }
        }
        if (session == NULL) {
            rtsp_send_reply(454, NULL, rtsp);
            return ERR_NOT_FOUND;
        }
        switch(session->state) {
            case STATE_INIT:
                {
                    switch(method_id) {
                        case RTSP_ID_OPTIONS:
                            rtsp_method_options(rtsp);
                            break;
                        case RTSP_ID_PLAY:
                            rtsp_send_reply(455, NULL, rtsp);
                            break;
                        case RTSP_ID_SETUP:
                            rtsp_method_setup(rtsp);
                            session->state = STATE_READY;
                            break;
                        case RTSP_ID_TEARDOWN:
                            rtsp_method_teardown(rtsp);
                            break;
                        case RTSP_ID_RECORD:
                        case RTSP_ID_REDIRECT:
                        case RTSP_ID_ANNOUNCE:
                        case RTSP_ID_GET_PARAMETERS:
                        case RTSP_ID_SET_PARAMETER:
                        default:
                            rtsp_send_reply(501, NULL, rtsp);
                            break;
                    }
                };

            case STATE_READY:
                switch(method_id) {
                    case RTSP_ID_OPTIONS:
                        rtsp_method_options(rtsp);
                        break;
                    case RTSP_ID_PLAY:
                        rtsp_method_play(rtsp);
                        session->state = STATE_PLAY;
                        break;
                    case RTSP_ID_SETUP:
                        rtsp_method_setup(rtsp);
                        session->state = STATE_READY;
                        break;
                    case RTSP_ID_TEARDOWN:
                        rtsp_method_teardown(rtsp);
                        session->state = STATE_INIT;
                        break;
                    case RTSP_ID_RECORD:
                    case RTSP_ID_REDIRECT:
                    case RTSP_ID_ANNOUNCE:
                    case RTSP_ID_GET_PARAMETERS:
                    case RTSP_ID_SET_PARAMETER:
                    default:
                        rtsp_send_reply(501, NULL, rtsp);
                        break;
                }

            case STATE_PLAY:
                switch(method_id) {
                    case RTSP_ID_OPTIONS:
                        rtsp_method_options(rtsp);
                        break;
                    case RTSP_ID_PLAY:
                        rtsp_method_play(rtsp);
                        session->state = STATE_PLAY;
                        break;
                    case RTSP_ID_SETUP:
                        rtsp_send_reply(455, NULL, rtsp);
                        break;
                    case RTSP_ID_TEARDOWN:
                        rtsp_method_teardown(rtsp);
                        session->state = STATE_INIT;
                        break;
                    case RTSP_ID_RECORD:
                    case RTSP_ID_REDIRECT:
                    case RTSP_ID_ANNOUNCE:
                    case RTSP_ID_GET_PARAMETERS:
                    case RTSP_ID_SET_PARAMETER:
                    default:
                        rtsp_send_reply(501, NULL, rtsp);
                        break;
                }

            default:
                rtsp_send_reply(501, NULL, rtsp);
                break;
        }
    } else {
        /* 请求中没有Session字段 */
        switch(method_id) {
            case RTSP_ID_OPTIONS:
                rtsp_method_options(rtsp);
                break;
            case RTSP_ID_SETUP:
                rtsp_method_setup(rtsp);
            default:
                rtsp_send_reply(501, NULL, rtsp);
                break;
        }
    }

    return ERR_GENERIC;
}

int rtp_ts_file_callback(event_t * ev)
{
    rtp_session_t * rtp = (rtp_session_t *)ev->opaque;
    int n = 0;

    n = rtp->get_packet(rtp->data, MTU-sizeof(rtp_header_t),
                            rtp->src, 0);
#ifdef DEBUG
    static int count = 0;

    {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        DEBUG("tv.sec = %ld    \tv.usec = %ld\n", tv.tv_sec, tv.tv_usec);
        DEBUG("expire.sec = %ld\texpire.usec=%ld\n", ev->expire_time.tv_sec,
              ev->expire_time.tv_usec);
    }
    count++;
#endif

    rtp->out_len = n;
    /* 当n<=0时，表明已经读到文件结尾 */
    if (n <= 0) {
        return 0;
    }
    rtp_session_seq_increase(rtp);
    rtp_session_update_timestamp(rtp);
    rtp_session_send_packet(rtp);

    /* 重复插入事件 */
    event_queue_push_back(&g_event_queue, *ev, 10000);

    return 0;
}
