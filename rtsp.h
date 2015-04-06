#ifndef _RTSP_H
#define _RTSP_H

/* ERROR CODE */
#define ERR_NOERROR           0
#define ERR_GENERIC          -1
#define ERR_NOT_FOUND        -2
#define ERR_PARSE            -3
#define ERR_ALLOC            -4
#define ERR_INPUT_PARAM      -5
#define ERR_NOT_SD           -6
#define ERR_UNSUPPORTED_PT   -7
#define ERR_EOF              -8
#define ERR_FATAL            -9
#define ERR_CONNECTION_CLOSE -10

/* RTSP STATE */
#define STATE_INIT  0
#define STATE_READY 1
#define STATE_PLAY  2

/* RTSP MESSAGE HEADER */
#define HDR_CONTENTLENGTH    "Content-Length"
#define HDR_ACCEPT           "Accept"
#define HDR_ALLOW            "Allow"
#define HDR_BLOCKSIZE        "Blocksize"
#define HDR_CONTENTTYPE      "Content-Type"
#define HDR_DATE             "Date"
#define HDR_REQUIRE          "Require"
#define HDR_TRANSPORTREQUIRE "Transport-Require"
#define HDR_SEQUENCENO       "SequenceNo"
#define HDR_CSEQ             "CSeq"
#define HDR_STREAM           "Stream"
#define HDR_SESSION          "Session"
#define HDR_TRANSPORT        "Transport"
#define HDR_RANGE            "Range"
#define HDR_USER_AGENT       "User-Agent"

/* RTSP METHOD ID */
#define RTSP_ID_DESCRIBE       0
#define RTSP_ID_ANNOUNCE       1
#define RTSP_ID_GET_PARAMETERS 2
#define RTSP_ID_OPTIONS        3
#define RTSP_ID_PAUSE          4
#define RTSP_ID_PLAY           5
#define RTSP_ID_RECORD         6
#define RTSP_ID_REDIRECT       7
#define RTSP_ID_SETUP          8
#define RTSP_ID_SET_PARAMETER  9
#define RTSP_ID_TEARDOWN       10

/* RTSP VERSIOIN INFO */
#define RTSP_ENDLINE "\r\n"
#define RTSP_VER "RTSP/1.0"
#define RTSP_RTP_AVP "RTP/AVP"

/*  RTSP DEFAULT PORT */
#define RTSP_DEFAULT_PORT 554

#define RTSP_BUFFER_SIZE  2048
#define RTSP_DESCR_SIZE   2048
#define RTSP_URL_SIZE     256

#ifdef DEBUG
#define DEBUG(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...)
#endif

#include "list.h"
#include "event_queue.h"
//#include "rtp.h"

struct rtsp_session {
    int state;
    int session_id;
    int client_port[2];

    struct list_head rtp_list;

    struct list_head list;
};
typedef struct rtsp_session rtsp_session_t;

struct rtsp_buffer {
    int fd;
    int cseq;
    //int client_port[2];
    //int server_port[2];
    char url[RTSP_URL_SIZE];
    int method;

    char in_buffer[RTSP_BUFFER_SIZE];
    int in_size;

    char out_buffer[RTSP_BUFFER_SIZE + RTSP_DESCR_SIZE];
    int out_size;

    struct list_head session_list; //rtsp_session_t
    struct list_head list;
};
typedef struct rtsp_buffer rtsp_buffer_t;

rtsp_buffer_t * rtsp_buffer_create();
int rtsp_check_request(rtsp_buffer_t* rtsp);
int rtsp_state_machine(rtsp_buffer_t* rtsp, int method_id);

int rtsp_send_reply(int errcode, char * addon, rtsp_buffer_t * rtsp);
const char * rtsp_get_err_state(int err);
int rtsp_random();
int rtsp_fill_out_buffer(rtsp_buffer_t * rtsp, const char * data, int size);
int rtsp_parse_url(const char * url, char * server, size_t server_len,
                   unsigned short *  port, char * filename, size_t filename_len);


/* RTSP server method handler */
int rtsp_method_options (rtsp_buffer_t * rtsp);
int rtsp_method_setup   (rtsp_buffer_t * rtsp);
int rtsp_method_play    (rtsp_buffer_t * rtsp);
int rtsp_method_teardown(rtsp_buffer_t * rtsp);


/* RTSP session */
rtsp_session_t * rtsp_session_create();

void rtsp_session_push_back(struct list_head * session_list,
                                        rtsp_session_t * item);
rtsp_session_t * rtsp_session_find_by_id(struct list_head * session_list,
                                         int session_id);
void rtsp_session_remove(struct list_head * session_list,
                                     int session_id);
void rtsp_session_delete(rtsp_session_t * item);

void rtsp_session_release(struct list_head * session_list);


/* RTP Callback */
int rtp_ts_file_callback(event_t * ev);
#endif /* _RTSP_H */

