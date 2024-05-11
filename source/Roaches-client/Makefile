# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall -Wextra 

# Libraries
LIBS = -lncurses -lzmq -lm -lprotobuf-c -lpthread

# Source files
MACHINE_SRC = Roaches-client.c ../src/clients.c ../src/client_server.pb-c.c ../src/client_server.c

# Dependency files
MACHINE_DPD = Roaches-client.c ../src/clients.h ../src/client_server.pb-c.h  ../src/client_server.h

# Output executables
MACHINE_OUT = ../bin/Roaches-client

# Executables
all: $(MACHINE_OUT)

$(MACHINE_OUT): $(MACHINE_DPD)
	$(CC) $(CFLAGS) -o $(MACHINE_OUT) $(MACHINE_SRC) $(LIBS)

clean:
	@rm -v -f $(MACHINE_OUT)

.PHONY: all clean