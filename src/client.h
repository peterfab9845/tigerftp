#ifndef CLIENT_H
#define CLIENT_H

enum ftp_state { IDLE, CONNECTED };
enum ftp_command { TCONNECT, TGET, TPUT, EXIT };

int open_conn(char *host);
int do_auth(int sockfd, char *user, char *pass);
int do_get(int sockfd, char *filename);
int do_put(int sockfd, char *filename);
int close_conn(int sockfd);
int parse_cmd(char *line, enum ftp_command *cmd, char **hostname,
    char **username, char **password, char **filename);
void usage(void);

#endif

