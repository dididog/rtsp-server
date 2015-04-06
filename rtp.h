#ifndef RTP_H
#define RTP_H

#include "list.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MTU 1500

//#ifdef DEBUG
//#define DEBUG(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
//#else
//#define DEBUG(fmt, ...)
//#endif

typedef struct _rtp_header {
    unsigned char cc:4;
    unsigned char x:1;
    unsigned char p:1;
    unsigned char v:2;

    unsigned char pt:7;  //must set manually
    unsigned char m:1;

    unsigned short sequence;
    unsigned int timestamp;
    unsigned int ssrc;
} rtp_header_t;

typedef struct _rtp_session {
    unsigned char out_buff[MTU];
    unsigned char * data;
    size_t out_len;
    float clock_rate;  //must set manually
    float packet_rate; //must set manually
    rtp_header_t head;
    int sock_fd;

    void * src;
    size_t src_len;

    struct sockaddr_in destaddr; //must set manually

    int (*get_packet)(void * data, size_t len, void * src, size_t max_src_len);
    int state;

    struct list_head list;
} rtp_session_t;

typedef int (*GET_PACKET_CALLBACK)(void *data, size_t len, void * src, size_t src_len);

void rtp_session_set_payload_type(rtp_session_t * rtp, int pt);
void rtp_session_seq_increase(rtp_session_t * item);
void rtp_session_update_timestamp(rtp_session_t * item);
rtp_session_t * rtp_session_create(GET_PACKET_CALLBACK callback);
void rtp_session_delete(rtp_session_t * item);
void rtp_session_release(rtp_session_t * rtp_list);
int rtp_session_send_packet(rtp_session_t * item);
rtp_session_t * rtp_session_push_back(rtp_session_t * rtp_list,
                                      rtp_session_t * item);

int get_packet_ts(void * data, size_t len, void * src, size_t max_src_len);
int get_packet_ts_file(void * data, size_t len, void * src, size_t max_src_len);

#endif /* RTP_H */
