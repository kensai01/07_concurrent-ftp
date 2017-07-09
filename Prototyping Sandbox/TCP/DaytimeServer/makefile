# Tweaked from http://mrbook.org/tutorials/make/

# Use the gcc compiler
CC=gcc

# Set some CFLAGS for compiling.
CFLAGS=-c -Wall -O2

# Set some flags to compile socket programs
LDFLAGS= -lnsl -lsocket -lresolv -lrt

# Inlcude the source files that you wish 
# You change this line to include your own files
#SOURCES=daytimeClient.c NP.c
#SOURCES=iterDaytimeServer.c NP.c
SOURCES=conDaytimeServer.c NP.c

# Helps to create the object files
OBJECTS=$(SOURCES:.c=.o)

# The name of your executable. 
#EXECUTABLE=daytimeClient
#EXECUTABLE=iterDaytimeServer
EXECUTABLE=conDaytimeServer

# Compiles the source files into the executable
all: $(SOURCES) $(EXECUTABLE)

# Shows that the executable depends upon the objects	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# A rule to translate .c files into .o files
.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Used in case there exists a file named clean
.PHONY = clean
# clean up some temporary files
clean:
	rm *.o $(EXECUTABLE) $(EXECUTABLE1) $(EXECUTABLE2)
