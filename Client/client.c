/*****************************************************************************
​* Copyright​ ​(C)​ ​2023 ​by​ ​Nileshkartik Ashokkumar
​*
​* ​​Redistribution,​ ​modification​ ​or​ ​use​ ​of​ ​this​ ​software​ ​in​ ​source​ ​or​ ​binary
​* ​​forms​ ​is​ ​permitted​ ​as​ ​long​ ​as​ ​the​ ​files​ ​maintain​ ​this​ ​copyright.​ ​Users​ ​are
​​* ​permitted​ ​to​ ​modify​ ​this​ ​and​ ​use​ ​it​ ​to​ ​learn​ ​about​ ​the​ ​field​ ​of​ ​embedded
​* software.​ Nileshkartik Ashokkumar ​and​ ​the​ ​University​ ​of​ ​Colorado​ ​are​ ​not​ ​liable​ ​for
​​* ​any​ ​misuse​ ​of​ ​this​ ​material.
​*
*****************************************************************************
​​*​ ​@file​ client.c
​​*​ ​@brief ​ functionality of the socket client communication : Client side C/C++ program to demonstrate Socket
* @reference https://www.geeksforgeeks.org/socket-programming-cc/
* @reference https://stackoverflow.com/questions/13097375/sending-images-over-sockets
​​*
​​*​ ​@author​ ​Nileshkartik Ashokkumar
​​*​ ​@date​ ​Apr ​23​ ​2023
​*​ ​@version​ ​1.0
​*
*/


/************************************include files***************************/
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>

/***********************************Define**********************************/
#define PORT 8080	
#define SERVER_IP_ADDR "10.0.0.91"	
#define IMAGE_CAP "capture2.jpeg"

/***********************************Global Variable***************************/
bool st_kill_process = false;
int client_fd = -1;
FILE *image;

/*****************************Function declaration****************************/
int receive_image();
void cleanup_on_exit();

/*****************************Function definition******************************/
int receive_image()
{ 
	int recv_size = 0, size = 0, read_size, write_size, packet_index = 1, retval;
	char imagearray[10241];

	// Find the size of the image
	do
	{
	   retval = read(client_fd, &size, sizeof(int));
	} while (retval < 0);
	syslog(LOG_INFO,"size of the image identified\n");

	char buffer[] = "Got it";

	// Send our verification signal
	do
	{
		retval = write(client_fd, &buffer, sizeof(int));
	} while (retval < 0);


	image = fopen(IMAGE_CAP, "w");

	if (image == NULL)
	{
		printf("Error has occurred. Image file could not be opened\n");
		return -1;
	}

	// Loop while we have not received the entire file yet

	struct timeval timeout = {10, 0};

	fd_set fds;
	int buffer_fd;

	while (recv_size < size)
	{
		FD_ZERO(&fds);
		FD_SET(client_fd, &fds);

		buffer_fd = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);

		if (buffer_fd < 0)
		{
			syslog(LOG_ERR,"error: bad file descriptor set.\n");
			cleanup_on_exit();
		}

		if (buffer_fd == 0)
		{
			syslog(LOG_ERR,"error: buffer read timeout expired.\n");
			cleanup_on_exit();
		}

		if (buffer_fd > 0)
		{
			do
			{
				read_size = read(client_fd, imagearray, 10241);
			} while (read_size < 0);

			// Write the currently read data into our image file
			write_size = fwrite(imagearray, 1, read_size, image);

			if (read_size != write_size)
			{
				printf("error in read write\n");
			}

			// Increment the total number of bytes read
			recv_size += read_size;
			packet_index++;
			//syslog(LOG_INFO,"Total received image size: %i\n", recv_size);
		}
	}
	syslog(LOG_INFO,"Total received image size: %i\n", recv_size);

	fclose(image);
	return 1;
}


void cleanup_on_exit()
{
    close(client_fd);
	fclose(image);
    remove(IMAGE_CAP);

    closelog();
}



/*signal handler for closing the connection*/
void sig_handler()
{
    st_kill_process = 1;
    syslog(LOG_INFO,"SIGINT/ SIGTERM encountered: exiting the process...");
    cleanup_on_exit();
    exit(EXIT_SUCCESS);
}


int main()
{
	int retvalus;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from client";
	char buffer[1024] = {0};
    remove(IMAGE_CAP);      //remove image on startup


    openlog(NULL, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER); /*open connection for sys logging, ident is NULL to use this Program for the user level messages*/


	if ((signal(SIGINT, sig_handler) == SIG_ERR) || (signal(SIGTERM, sig_handler) == SIG_ERR))
    {
        syslog(LOG_ERR, " Signal handler error");
        cleanup_on_exit();
        exit(EXIT_FAILURE);
    }

	//create a socket for connection with the server rpi
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		syslog(LOG_ERR,"Socket creation error\n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary
	if (inet_pton(AF_INET, SERVER_IP_ADDR, &serv_addr.sin_addr) <= 0)
	{
		syslog(LOG_ERR,"Invalid address/ Address not supported\n");
		return -1;
	}

	// Initiate a connection to socket
	if ((retvalus = connect(client_fd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))) < 0)
	{
		syslog(LOG_ERR,"Connection Failed\n");
		return -1;
	}

	// Send a string to socket to verify send works
	if(send(client_fd, hello, strlen(hello), 0) == -1)
	{
		syslog(LOG_ERR,"Failure to send the string\n");
		return -1; 
	}
	
	// read a string from the server to verify read works
	if(read(client_fd, buffer, 1024) == -1)
	{
		syslog(LOG_ERR,"Failure to read a sample string from server\n");
		return -1;
	}


		//recieve image from the server
	if(receive_image() == -1)
	{
		syslog(LOG_ERR,"Failure to recieve image from server \n");
		return -1;
	}
	

	close(client_fd);
	return 0;
}
