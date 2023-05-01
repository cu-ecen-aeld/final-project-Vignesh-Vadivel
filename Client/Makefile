CC=g++					#by default compiler is gcc is specified aarch64-none-linux-gnu gcc will be appended at the end
CFLAGS?=-g -Wall -Wextra 				#variable to hold the various flag for cross compiling
LDFLAGS?= -lwiringPi
.PHONY: all
all: client

client : 
	g++ client.c -o client `pkg-config opencv4 --libs --cflags` -lwiringPi


.PHONY: clean
clean:
	rm -f *.o client           				#cleaning all the associated object file pertaining to writer executable
