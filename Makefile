# Programs and sources
SERVER_SRC = server.c
SERVER_BIN = TigerS

CLIENT_SRC = client.c
CLIENT_BIN = TigerC

COMMON_H = common.h

# result files
TEST_SCRIPT = test.sh

# compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g

# disable echoing commands for nicer output
.SILENT:

# default - build the programs
.PHONY: all
all: $(SERVER_BIN) $(CLIENT_BIN)

# compile modules and programs
$(SERVER_BIN): $(SERVER_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $@

$(CLIENT_BIN): $(CLIENT_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $@

# run the test script
.PHONY: test
test:
	chmod a+x ./test.sh
	./test.sh

# clean up binaries and output files
.PHONY: clean
clean:
	-rm -f $(SERVER_BIN) $(CLIENT_BIN)

# help target - lists all targets
.PHONY: help
help:
	echo "Targets:"
	echo "all:   build TigerS and TigerC"
	echo "test:  run test.sh"
	echo "clean: remove output and binary files"
	echo "help:  show this help"
