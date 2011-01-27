// TFTP server implementation

#ifndef __TFTP_H__
#define __TFTP_H__

#define WIZ_SOCK_TFTP         0
#define WIZ_FWUPD_WAIT        100

// Protocol constants
#define TFTP_PORT             69
#define TFTP_TIMEOUT          500
#define TFTP_BLOCK_SIZE       512
#define TFTP_DATA_HEADER_SIZE 4
#define TFTP_MAX_PACKET_SIZE  ( TFTP_BLOCK_SIZE + TFTP_DATA_HEADER_SIZE )    
#define TFTP_PACKET_RRQ       1
#define TFTP_PACKET_WRQ       2
#define TFTP_PACKET_DATA      3
#define TFTP_PACKET_ACK       4
#define TFTP_PACKET_ERROR     5
#define TFTP_ERR_ILLEGAL_OP   4

void tftp_server_run();

#endif
