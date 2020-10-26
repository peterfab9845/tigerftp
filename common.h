#ifndef COMMON_H
#define COMMON_H

#define STR_X(x) #x
#define STR(x) STR_X(x)

#define FTP_PORT (2100)

enum ftp_action { AUTH = 0x01, GET = 0x02, PUT = 0x03 };

struct ftp_request {
  enum ftp_action action;
  char *username;
  char *password;
  char *filename;
};

enum ftp_ack_result { SUCCESS = 0x00, FAILURE = 0x01 };

struct ftp_response {
  enum ftp_action action;
  enum ftp_ack_result result;
  size_t filesize;
};

#endif
