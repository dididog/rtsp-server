#include "rtp.h"
#include "rtsp.h"
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


static void rtp_header_init(rtp_header_t * h)
{
    struct timeval tv;

    h->v = 2;
    h->p = 0;
    h->x = 0;
    h->cc = 0;
    h->m = 0;
    h->sequence = 0;
    gettimeofday(&tv, NULL);
    h->timestamp = htonl(tv.tv_sec);
}

void rtp_session_seq_increase(rtp_session_t * session)
{
    unsigned short t = ntohs(session->head.sequence);
    t++;
    session->head.sequence = htons(t);
}

void rtp_session_update_timestamp(rtp_session_t * session)
{
    assert(session != NULL);
    unsigned int ts = ntohl(session->head.timestamp);
    float diff = session->clock_rate / session->packet_rate;

    ts += (unsigned int)diff;
    session->head.timestamp = htonl(ts);
}

rtp_session_t * rtp_session_push_back(rtp_session_t * rtp_list,
                                      rtp_session_t * item)
{
    list_add_tail(&(rtp_list->list), &(item->list));
    return rtp_list;
}

rtp_session_t * rtp_session_create(GET_PACKET_CALLBACK callback)
{
    rtp_session_t * p = (rtp_session_t *)malloc(sizeof(rtp_session_t));
    INIT_LIST_HEAD(&(p->list));
    rtp_header_init(&(p->head));
    p->get_packet = callback;
    p->data = p->out_buff + sizeof(rtp_header_t);

    return p;
}

void rtp_session_delete(rtp_session_t * item)
{
    rtp_session_t * tmp = NULL;

    tmp = list_entry(&(item->list), rtp_session_t, list);
    list_del(&(item->list));
    free(tmp);
}

void rtp_session_release(rtp_session_t * rtp_list)
{
    rtp_session_t * tmp = NULL;
    struct list_head * pos, *n;

    list_for_each_safe(pos, n, &(rtp_list->list)) {
        tmp = list_entry(pos, rtp_session_t, list);
        list_del(pos);
        free(tmp);
    }

}

void rtp_session_set_payload_type(rtp_session_t * rtp, int pt)
{
    rtp->head.pt = pt;
}

int rtp_session_send_packet(rtp_session_t * rtp)
{
    int ret = 0;
    if (rtp->out_len > 0 && rtp->out_len < MTU) {
        memcpy(rtp->out_buff, &(rtp->head), sizeof(rtp_header_t));
        rtp->out_len += sizeof(rtp_header_t);
        ret = sendto(rtp->sock_fd, rtp->out_buff, rtp->out_len, 0,
                     (const struct sockaddr *)&(rtp->destaddr),
                     sizeof(rtp->destaddr));
        if (ret < 0) {
            LOG("Send RTP Packet failed:%s\n", strerror(errno));
        }
    }

    return ret;
}

int get_packet_ts(void * data, size_t len, void * src, size_t max_src_len)
{
#define TS_PACKET_SIZE 188
    unsigned char * source = (unsigned char *)src;
    unsigned char * dest = (unsigned char *)data;
    size_t size = 0;

    while((size + TS_PACKET_SIZE <= len) && (max_src_len > 0)) {
        memcpy(dest + size, source + size, TS_PACKET_SIZE);
        if (*(dest + size) != 0x47) {
            printf("Bad Sync Header\n");
        }
        size += TS_PACKET_SIZE;
        max_src_len -= TS_PACKET_SIZE;
    }

    return size;
}

int get_packet_ts_file(void * data, size_t len, void * src, size_t max_src_len)
{
#define TS_PACKET_SIZE 188
    unsigned char * dest = (unsigned char *)data;
    FILE * fp = (FILE *)src;
    int size = 0;

    while(!feof(fp)) {
        int once = fread(data + size, 1, TS_PACKET_SIZE, fp);
        if (*(dest + size) != 0x47) {
            printf("TS Bad Sync Header\n");
            continue;
        }

        size += once;
        if (size + TS_PACKET_SIZE >= len) {
            break;
        }
    }

    return size;
}
