#ifndef COMMON_H
#define COMMON_H

#define STR_X(x) #x
#define STR(x) STR_X(x)

#define FTP_PORT 2100

enum ftp_req_type { AUTH_REQ = 0x01, AUTH_RESP = 0x02, GET = 0x03, PUT = 0x04, END = 0x05 };

struct ftp_auth_request {
  enum ftp_req_type type;
  size_t username_len;
  size_t password_len;
};

struct ftp_file_request {
  enum ftp_req_type type;
  size_t filename_len;
};

enum ftp_result { SUCCESS = 0x01, FAILURE = 0x02, UNKNOWN = 0x03 };

struct ftp_auth_response {
  enum ftp_req_type type;
  enum ftp_result result;
};

struct ftp_file_response {
  enum ftp_req_type type;
  enum ftp_result result;
  size_t filesize;
};

int send_all(int sockfd, void *buf, int len);
int send_close(int sockfd);
int close_conn(int sockfd);

#endif
