#include "event_queue.h"
#include "list.h"
#include "rtp.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

//int ports[] = {6660, 6662, 6664, 6666, 6668, 6670};
int ports[]={6666};
LIST_HEAD(rtp_list);
LIST_HEAD(queue);

int init_rtp_list()
{
    int i;
    rtp_session_t * temp = NULL;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    for (i = 0; i < sizeof(ports) / sizeof(ports[0]); i++) {
        struct sockaddr_in addr;
        FILE * fp = fopen("out.ts", "r");

        temp = rtp_session_create(get_packet_ts_file);
        temp->sock_fd = fd;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ports[i]);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        bzero(&(addr.sin_zero), 8);
        temp->destaddr = addr;
        temp->clock_rate = 90000.0;
        temp->packet_rate = 24.0;
        temp->src_fp = fp;
        rtp_session_set_payload_type(temp, 33);

        list_add_tail(&(temp->list), &rtp_list);
    }

    return i - 1;
}

int rtp_ts_file_callback(event_t * ev)
{
    rtp_session_t * rtp = (rtp_session_t *)ev->opaque;
    int n = 0;

    n = rtp->get_packet(rtp->data, MTU-sizeof(rtp_header_t),
                            rtp->src_fp, 0);
#ifdef DEBUG
    static int count = 0;

    {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        LOG("tv.sec = %ld    \tv.usec = %ld\n", tv.tv_sec, tv.tv_usec);
        LOG("expire.sec = %ld\texpire.usec=%ld\n", ev->expire_time.tv_sec,
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
    event_queue_push_back(&queue, *ev, 10000);

    return 0;
}

int main(int argc, char ** argv)
{
    struct list_head * pos;
    rtp_session_t * rtp = NULL;

    event_queue_run(&queue);
    init_rtp_list();
    list_for_each(pos, &rtp_list) {
        rtp = list_entry(pos, rtp_session_t, list);
        event_t ev;

        EVENT_INIT(&ev);
        ev.opaque = rtp;
        ev.fire = rtp_callback;
        event_queue_push_back(&queue, ev, 0);
    }

    while(1) {
        sleep(1);
    }

    return 0;
}
