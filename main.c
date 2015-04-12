#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include "rtp.h"
#include "rtsp.h"

/*
 * 考虑到在method处理中需要用到event queue对象，而目前的接口没有提供
 * 传递event queue参数的方法，暂时使用将event queue对象声明为全局变
 */
LIST_HEAD(g_event_queue);

/*
 * 服务器端口
 */
int g_server_port = 6668;

int tcp_listen(int port)
{
    int main_fd;
    int v = 1;
    int on = 1;
    struct sockaddr_in s;

    main_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (main_fd < 0) {
        return ERR_GENERIC;
    }

    setsockopt(main_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&v, sizeof(int));
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = inet_addr("127.0.0.1");
    s.sin_port = htons(port);

    if (bind(main_fd, (struct sockaddr *)&s, sizeof(struct sockaddr_in))) {
        return ERR_GENERIC;
    }

    if (ioctl(main_fd, FIONBIO, &on) < 0){
        return ERR_GENERIC;
    }

    if (listen(main_fd, SOMAXCONN) < 0) {
        return ERR_GENERIC;
    }

    return main_fd;
}

int rtsp_handler(struct list_head * rtsp_list)
{
    fd_set rset, wset;
    struct timeval tv;
    //struct list_head * pos;
    rtsp_buffer_t * item = NULL;
    rtsp_buffer_t * tmp = NULL;
    int ret;
    int max_fd = 1024;

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    
    if (list_empty(rtsp_list)) {
        return 0;
    }

    list_for_each_entry(item, rtsp_list, list) {
        if (item != NULL) {
            FD_SET(item->fd, &rset);
            FD_SET(item->fd, &wset);
        }
    }

    ret = select(max_fd, &rset, &wset, NULL, &tv);
    if (ret < 0) {
        return -1;
    }

    list_for_each_entry_safe(item, tmp, rtsp_list, list) {
        if (item != NULL) {
            if (FD_ISSET(item->fd, &rset)) {
                ret = recv(item->fd, item->in_buffer, RTSP_BUFFER_SIZE, 0);
                if (ret < 0) {
                    //recv error
                } else if (ret == 0) {
                    //closed by peer
                    close(item->fd);
                    list_del(&(item->list));
                    free(item);
                    continue;
                } else {
                    //recv success
                    item->in_size = ret;
                    item->in_buffer[item->in_size] = '\0';
                    if (rtsp_check_request(item) < 0) {
                        LOG("request invalid\n");
                        continue;
                    }
                    if (rtsp_state_machine(item, item->method) < 0) {
                    }
                }
            }

            if (FD_ISSET(item->fd, &wset)) {
                if (item->out_size > 0) {
                    ret = send(item->fd, item->out_buffer, item->out_size, 0);
                    item->out_size = 0;
                }
            }
        }
    }

    return 0;
}

int main(int argc, char ** argv)
{
    LIST_HEAD(rtsp_buffer_list);
    int main_fd;
    int fd;
    struct sockaddr_in client_addr;
    socklen_t addr_size;

    main_fd = tcp_listen(g_server_port);
    event_queue_run(&g_event_queue);
    while(1) {
        rtsp_buffer_t * item = NULL;
        int not_found = 1;

        fd = accept(main_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (fd < 0) {

        } else {
            list_for_each_entry(item, &rtsp_buffer_list, list) {
                if (item->fd == fd) {
                    not_found = 0;
                }
            }

            /* new connection */
            if (not_found) {
                item = rtsp_buffer_create();
                item->fd = fd;
                list_add_tail(&(item->list), &rtsp_buffer_list);
            }
        }

        /* handle established connections */
        rtsp_handler(&rtsp_buffer_list);
        usleep(1);
    }

    event_queue_stop(&g_event_queue);

    return 0;
}
