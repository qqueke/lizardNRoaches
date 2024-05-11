# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra -Wpointer-sign

# Libraries
LIBS = -lncurses -lzmq -lm -lpthread -lprotobuf-c

# Source files
SERVER_SRC = lizardsNroaches-server.c ../src/lizardNroach.c  ../src/global.c ../src/client_server.pb-c.c ../src/client_server.c

# Dependency files
SERVER_DPD = lizardsNroaches-server.c  ../src/lizardNroach.h ../src/global.h ../src/client_server.pb-c.h ../src/client_server.h

# Output executables
SERVER_OUT = ../bin/lizardsNroaches-server

# Executables
all: $(SERVER_OUT)

$(SERVER_OUT): $(SERVER_DPD)
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(SERVER_SRC) $(LIBS)

clean:
	@rm -v -f $(SERVER_OUT)

.PHONY: all clean
