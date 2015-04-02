#include "rtp.h"
#include "list.h"

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
#include <sys/mman.h>

//int ports[] = {554, 556, 558, 560, 562, 564};
int ports[] = {6660, 6662, 6664, 6666, 6668, 6670};
//int ports[] = {6666};

int main(int argc, char ** argv)
{
    rtp_session_t * rtp_list = NULL;
    struct list_head head;
    rtp_session_t * temp = NULL;
    char * src_addr = NULL;
    size_t src_len;
    const char * filename = "out.ts";
    int i = 0;
    size_t send_len = 0;
    FILE * fp = NULL;

    INIT_LIST_HEAD(&head);
    fp = fopen("out.ts", "r");

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    for (i = 0; i < sizeof(ports)/sizeof(ports[0]); i++) {
        struct sockaddr_in addr;

        temp = rtp_session_create(get_packet_ts_file);
        temp->sock_fd = fd;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ports[i]);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        bzero(&(addr.sin_zero), 8);
        temp->destaddr = addr;
        rtp_session_set_payload_type(temp, 33);
        temp->clock_rate = 90000.0;
        temp->packet_rate = 24.0;

        if (rtp_list == NULL) {
            rtp_list = temp;
        } else {
            rtp_session_push_back(temp, rtp_list);
        }
    }

   list_add(&head, &(rtp_list->list));

    while(1) {
        struct list_head * pos = NULL;
        rtp_session_t * rtp;
        size_t len = 0;
        unsigned char buffer[MTU];
        len = get_packet_ts_file(buffer, MTU-sizeof(rtp_header_t), fp, 0);
        if (len <= 0) {
            break;
        }

        list_for_each(pos, &(head)) {
            rtp = list_entry(pos, rtp_session_t, list);
            memcpy(rtp->data, buffer, len);
            rtp->out_len = len;
            rtp_session_seq_increase(rtp);
            rtp_session_update_timestamp(rtp);
            rtp_session_send_packet(rtp);
        }
        send_len += len;
        usleep(20000);
    }

    fclose(fp);

    rtp_session_release(rtp_list);

    return 0;
}


