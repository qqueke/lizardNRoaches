# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra -Wno-int-conversion -Wno-switch-outside-range -Wno-uninitialized -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch

# Libraries
LIBS = -lncurses -lzmq -lm 

# Source files
SERVER_SRC = lizardsNroaches-server.c lizardNroach.c

# Dependency files
SERVER_DPD = lizardsNroaches-server.c lizardNroach.c lizardNroach.h

# Output executables
SERVER_OUT = ../bin/lizardsNroaches-server

# Executables
all: $(SERVER_OUT)

$(SERVER_OUT): $(SERVER_DPD)
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(SERVER_SRC) $(LIBS)

clean:
	rm -f $(SERVER_OUT)

.PHONY: all clean
