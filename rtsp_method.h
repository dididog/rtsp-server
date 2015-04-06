#ifndef _RTSP_METHODS_H_
#define _RTSP_METHODS_H_

typedef struct rtsp_buffer rtsp_buffer_t;

int rtsp_method_options(rtsp_buffer_t *rtsp);
int rtsp_method_setup(rtsp_buffer_t * rtsp);
int rtsp_method_play(rtsp_buffer_t * rtsp);
int rtsp_method_teardown(rtsp_buffer_t * rtsp);


#endif  /* _RTSP_METHODS_H_ */
