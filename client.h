#ifndef CLIENT_H
#define CLIENT_H

enum ftp_state { IDLE, CONNECTED };
enum ftp_command { TCONNECT, TGET, TPUT, EXIT };

int open_connection(char *host);
int send_auth(char *user, char *pass);
int send_get(char *filename);
int send_put(char *filename);
void close_connection(void);
int parse_cmd(char *line, enum ftp_command *cmd, char **hostname,
    char **username, char **password, char **filename);
void usage(void);

#endif

