/*****************************************************************************
​* Copyright​ ​(C)​ ​2023 ​by​ ​Vignesh Vadivel
​*
​* ​​Redistribution,​ ​modification​ ​or​ ​use​ ​of​ ​this​ ​software​ ​in​ ​source​ ​or​ ​binary
​* ​​forms​ ​is​ ​permitted​ ​as​ ​long​ ​as​ ​the​ ​files​ ​maintain​ ​this​ ​copyright.​ ​Users​ ​are
​​* ​permitted​ ​to​ ​modify​ ​this​ ​and​ ​use​ ​it​ ​to​ ​learn​ ​about​ ​the​ ​field​ ​of​ ​embedded
​* software.​ Vignesh Vadivel ​and​ ​the​ ​University​ ​of​ ​Colorado​ ​are​ ​not​ ​liable​ ​for
​​* ​any​ ​misuse​ ​of​ ​this​ ​material.
​*
*****************************************************************************
​​*​ ​@file​ server.c
​​*​ ​@brief ​ functionality of the socket server communication : Server side C/C++ program to send data over Socket
* @reference https://www.geeksforgeeks.org/socket-programming-cc/
* @reference https://stackoverflow.com/questions/13097375/sending-images-over-sockets
​​*
​​*​ ​@author​ ​Vignesh Vadivel
​​*​ ​@date​ ​Apr ​23​ ​2023
​*​ ​@version​ ​1.0
​*
*/


/************************************header files***************************/
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>

/***********************************Macros**********************************/
#define PORT 8080

/*****************************Function definition******************************/
void send_image(int socket)
{
	// File descriptor to read the image data
	FILE *picture;
	int size, read_size, stat, packet_index;
	char send_buffer[10240], read_buffer[256];
	packet_index = 1;

	// Open the image file
	picture = fopen("capture.jpeg", "r");
	syslog(LOG_INFO,"Getting Picture Size\n");

	if (picture == NULL)
	{
		syslog(LOG_ERR,"Error Opening Image File");
	}

	int returnStatus = fseek(picture, 0, SEEK_END);
	if (returnStatus == -1)
	{
		syslog(LOG_ERR,"fseek end failed");
		exit(EXIT_FAILURE);
	}
	// Get the size of the image
	size = ftell(picture);
	if (size == -1)
	{
		syslog(LOG_ERR,"ftell failed");
		exit(EXIT_FAILURE);
	}
	returnStatus = fseek(picture, 0, SEEK_SET);
	if (returnStatus == -1)
	{
		syslog(LOG_ERR,"fseek set failed");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO,"Total Picture size: %i\n", size);

	// Send Picture Size
	syslog(LOG_INFO,"Sending Picture Size\n");
	returnStatus = write(socket, (void *)&size, sizeof(int));
	if (returnStatus == -1)
	{
		syslog(LOG_ERR,"Write failed");
		exit(EXIT_FAILURE);
	}
	// Send Picture as Byte Array
	syslog(LOG_INFO,"Sending Picture as Byte Array\n");

	do
	{ // Read while we get errors that are due to signals.
		stat = read(socket, &read_buffer, 255);
		 if (stat == -1)
		{
			syslog(LOG_ERR,"Read failed");
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO,"Bytes read: %i\n", stat);
	} while (stat < 0);

	syslog(LOG_INFO,"Received data in socket\n");
	syslog(LOG_INFO,"Socket data: %s\n", read_buffer);

	while (!feof(picture))
	{
		// while(packet_index = 1){
		// Read from the file into our send buffer
		read_size = fread(send_buffer, 1, sizeof(send_buffer) - 1, picture);
		if (read_size < 0)
			{
				syslog(LOG_ERR,"fread failed");
				exit(EXIT_FAILURE);
			}
		// Send data through our socket
		do
		{
			stat = write(socket, send_buffer, read_size);
			if (stat == -1)
			{
				syslog(LOG_ERR,"Write failed");
				exit(EXIT_FAILURE);
			}
		} while (stat < 0);

		syslog(LOG_INFO,"Packet Number: %i\n", packet_index);
		syslog(LOG_INFO,"Packet Size Sent: %i\n", read_size);
		syslog(LOG_INFO," \n");
		syslog(LOG_INFO," \n");

		packet_index++;

		// Zero out our send buffer
		bzero(send_buffer, sizeof(send_buffer));
	}
}

int main()
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = {0};
	char *hello = "Hello from server";

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET,
				   SO_REUSEADDR | SO_REUSEPORT, &opt,
				   sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address,
			 sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
							 (socklen_t *)&addrlen)) < 0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	// Read sample data from client
	valread = read(new_socket, buffer, 1024);
	if (valread == -1)
	{
		perror("read from client failed");
		exit(EXIT_FAILURE);
	
	}
	syslog(LOG_INFO,"%s\n", buffer);
	// Send a sample test data
	int returnStatus = send(new_socket, hello, strlen(hello), 0);
	if (returnStatus == -1)
	{
		perror("Send to client failed");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO,"Hello message sent\n");
	
	syslog(LOG_INFO,"Init Image send\n");
	send_image(new_socket);
	syslog(LOG_INFO,"Image sending Completed\n");

	// closing the connected socket
	returnStatus = close(new_socket);
	if (returnStatus == -1)
	{
		perror("Close File descriptor");
		exit(EXIT_FAILURE);
	}
	return 0;
}
