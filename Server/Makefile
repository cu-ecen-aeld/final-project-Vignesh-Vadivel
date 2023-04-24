CC?=$(CROSS_COMPILE)gcc					#by default compiler is gcc is specified aarch64-none-linux-gnu gcc will be appended at the end
CFLAGS?=-g -Wall -Werror -Wextra 				#variable to hold the various flag for cross compiling
LDFLAGS?=-pthread -lrt
.PHONY: all
all: server

server : server_o
	$(CC) $(CFLAGS) -o server server.o  $(LDFLAGS) 	#linking the object file to the executable , with warning error and Wextra flag

server_o:
	$(CC) $(CFLAGS)  -c server.c -o server.o	        #creating the object file for writer.c and warning erorr and Wextra flag being set
	
.PHONY: clean
clean:
	rm -f *.o server           				#cleaning all the associated object file pertaining to writer executable
