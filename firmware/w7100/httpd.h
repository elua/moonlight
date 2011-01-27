// Wiz HTTP server

#ifndef __HTTPD_H__
#define __HTTPD_H__

#define HTTPD_SOCKET          2
#define HTTPD_PORT            80
#define HTTPD_SEND_TIMEOUT    0xFFFF
#define HTTPD_BUF_SIZE        4096
#define HTTPD_MAX_RESP_SIZE   8192
#define HTTPD_AUTH_SIZE       256
#define HTTPD_MAX_PARAM_SIZE  128

void httpd_init();
void httpd_run();

#endif
