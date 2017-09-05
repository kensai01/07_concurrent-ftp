# Tweaked from http://mrbook.org/tutorials/make/
# This particular makefile can compile two programs,
# in this case, the server and client.
# As such, one can simply type make.

# Use the gcc compiler
CC=gcc

# Set some CFLAGS for compiling.
CFLAGS=-c -Wall -O2

# Set some flags to compile socket programs
LDFLAGS= -pthread -lcurses -lm -g

# Inlcude the source files that you wish 
# You change this line to include your own files
SOURCES_SERVER=EchoServer.c NP.c
SOURCES_CLIENT=EchoClient.c NP.c

# Helps to create the object files
OBJECTS_SERVER=$(SOURCES_SERVER:.c=.o)
OBJECTS_CLIENT=$(SOURCES_CLIENT:.c=.o)

# The name of your executable. 
EXECUTABLE_SERVER=tcpServer
EXECUTABLE_CLIENT=tcpClient

# Compiles the source files into the executable
all: $(SOURCES_SERVER) $(EXECUTABLE_SERVER) \
	$(SOURCES_CLIENT) $(EXECUTABLE_CLIENT)

# Shows that the executable depends upon the objects	
$(EXECUTABLE_SERVER): $(OBJECTS_SERVER) 
	$(CC) $(OBJECTS_SERVER) $(LDFLAGS) -o $@

$(EXECUTABLE_CLIENT): $(OBJECTS_CLIENT) 
	$(CC) $(OBJECTS_CLIENT) $(LDFLAGS) -o $@

# A rule to translate .c files into .o files
.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Used in case there exists a file named clean
.PHONY = clean
# clean up some temporary files
clean:
	rm *.o $(EXECUTABLE_SERVER) $(EXECUTABLE_CLIENT)
