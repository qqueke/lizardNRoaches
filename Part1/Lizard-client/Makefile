# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra -Wno-int-conversion -Wno-switch-outside-range -Wno-uninitialized -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch

# Libraries
LIBS = -lncurses -lzmq -lm 

# Source files
CLIENT_SRC = Lizard-client.c lizardNroach.c

# Dependency files
CLIENT_DPD = Lizard-client.c lizardNroach.c lizardNroach.h

# Output executables
CLIENT_OUT = ../bin/Lizard-client

# Executables
all: $(CLIENT_OUT) 

$(CLIENT_OUT): $(CLIENT_DPD)
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRC) $(LIBS)

clean:
	rm -f $(CLIENT_OUT) 
