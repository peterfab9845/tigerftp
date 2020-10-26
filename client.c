// Data & Communication Networks
// Project 1 - Socket Programming
// Peter Fabinski (pnf9945)
// TigerC - client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "common.h"
#include "client.h"

#define CMDLEN 255

int main(void) {
  int line_max;

  // find max line length
  if (LINE_MAX >= CMDLEN) {
    line_max = CMDLEN;
  } else {
    long limit = sysconf(_SC_LINE_MAX);
    if (limit < 0 || limit > CMDLEN) {
      line_max = CMDLEN;
    } else {
      line_max = (int)limit;
    }
  }

  // alloc memory for the line
  char *line = malloc(line_max + 1);
  if (line == NULL) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  enum ftp_state state = IDLE;
  enum ftp_command cmd;

  char *hostname;
  char *username;
  char *password;
  char *filename;

  while (1) {
    printf("TigerC> ");
    // get a line
    if (fgets(line, line_max+1, stdin) == NULL) {
      if (errno) {
        fprintf(stderr, "Error getting command.\n");
        exit(1);
      } else {
        printf("exit\n"); // fake command for EOF
        break;
      }
    }

    // check for too long line
    if (strchr(line, '\n') == NULL) {
      fprintf(stderr, "Input line too long.\n");
      // clear the rest of the input
      while (strchr(line, '\n') == NULL) {
        fgets(line, line_max+1, stdin);
      }
      continue;
    }

    // interpret the command
    int r = parse_cmd(line, &cmd, &hostname, &username, &password, &filename);
    if (r) {
      // error parsing command, start the loop again
      continue;
    }
    // do what the command asks

    if (cmd == TCONNECT) {
      r = open_connection(hostname);
      if (r) {
        fprintf(stderr, "Could not connect to server.\n");
        break;
      }

      r = send_auth(username, password);
      if (r) {
        fprintf(stderr, "Incorrect username or password.\n");
      }
    } else if (cmd == TGET) {
      if (state != CONNECTED) {
        fprintf(stderr, "You need to connect first.\n");
        continue;
      }

      r = send_get(filename);

    } else if (cmd == TPUT) {
      if (state != CONNECTED) {
        fprintf(stderr, "You need to connect first.\n");
      }

      r = send_put(filename);

    } else if (cmd == EXIT) {
      // close down the client
      if (state == CONNECTED) {
        close_connection();
      }
      break;
    }
    else {
      printf("shouldn't get here!");
    }
  }

  // clean up input stuff and show message
  free(line);
  printf("Quitting\n");
  return 0;
}

// connect to the given server
int open_connection(char *host) {
  printf("tconnect host: %s\n", host);

  return 0;
}

// send an authentication request to the server
int send_auth(char *user, char *pass) {
  printf("tconnect user: %s\n", user);
  printf("tconnect pass: %s\n", pass);

  return 0;
}

// send a get request to the server
int send_get(char *filename) {
  printf("tget filename: %s\n", filename);
  return 0;
}

// send a put request to the server
int send_put(char *filename) {
  printf("tput filename: %s\n", filename);

  return 0;
}

void close_connection(void) {
  printf("close conn\n");
}

// parse the input and provide the command
int parse_cmd(char *line, enum ftp_command *cmd, char **hostname,
    char **username, char **password, char **filename) {

  // Parse the line. First, separate the command.
  static char *strtok_state;
  char *token;
  token = strtok_r(line, " \r\n", &strtok_state);

  // check for no command
  if (token == NULL) {
    // go to next line for new prompt
    printf("\n");
    // start over
    return -1;
  }
  if (strcmp(token, "help") == 0) {
    // help command, list commands
    usage();
    return -1;
  } else if (strcmp(token, "tconnect") == 0) {
    // tconnect command, manage the connection to the server
    *cmd = TCONNECT;
    token = strtok_r(NULL, " \n", &strtok_state);
    if (token) {
      *hostname = token;
    } else {
      fprintf(stderr, "tconnect requires a host to connect to.\n");
      return -1;
    }
    token = strtok_r(NULL, " \n", &strtok_state);
    if (token) {
      *username = token;
    } else {
      fprintf(stderr, "tconnect requires a username.\n");
      return -1;
    }
    token = strtok_r(NULL, " \n", &strtok_state);
    if (token) {
      *password = token;
    } else {
      fprintf(stderr, "tconnect requires a password.\n");
      return -1;
    }
  } else if (strcmp(token, "tget") == 0) {
    // tget command, print the filename
    *cmd = TGET;
    token = strtok_r(NULL, "\n", &strtok_state);
    if (token) {
      *filename = token;
    } else {
      fprintf(stderr, "tget requires a filename.\n");
      return -1;
    }
  } else if (strcmp(token, "tput") == 0) {
    // tput command, print the filename
    *cmd = TPUT;
    token = strtok_r(NULL, "\n", &strtok_state);
    if (token) {
      *filename = token;
    } else {
      fprintf(stderr, "tput requires a filename.\n");
      return -1;
    }
  } else if (strcmp(token, "exit") == 0) {
    // exit command
    *cmd = EXIT;
  } else {
    fprintf(stderr, "Command not recognized: %s\n", token);
    return -1;
  }
  return 0;
}
// print usage message
void usage(void) {
  printf("Commands:\n");
  printf("  tconnect <ip> <user> <pass>\n");
  printf("  tget <filename>\n");
  printf("  tput <filename>\n");
  printf("  help\n");
}

