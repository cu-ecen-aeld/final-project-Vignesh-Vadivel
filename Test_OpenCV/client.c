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

/*
g++ client.c -o client `pkg-config opencv4 --libs --cflags` -lwiringPi
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
#include <fcntl.h>		   //Needed for I2C port
#include <sys/ioctl.h>	   //Needed for I2C port
#include <linux/i2c-dev.h> //Needed for I2C port
#include <pthread.h>
#include <errno.h>
#include <wiringPi.h>
#include <opencv2/opencv.hpp>

/***********************************Define**********************************/
#define PORT 8080
#define SERVER_IP_ADDR "10.0.0.91"
#define IMAGE_CAP "capture2.jpeg"
#define GPIO_PIN0 0
#define LWD_THRESHOLD 20

/***********************************Global Variable***************************/
bool st_kill_process = false;
int client_fd = -1;
FILE *image;
bool ldw_detected = true;
pthread_t buzzer_thread_t;
pthread_t oled_thread_t;
cv::Mat binaryImage;
std::vector<cv::Vec4i> lines;
	
/*****************************Function declaration****************************/
int receive_image(int);
void cleanup_on_exit();
void *buzzer_thread();
void *oled_thread();


/*****************************Function definition******************************/

int buzzer_init()
{
	int retval = 0;
	if ((retval = wiringPiSetup()) == -1)
	{
		syslog(LOG_ERR, "setup wiringPi failed!");
		return retval;
	}
	pinMode(GPIO_PIN0, OUTPUT);
	return retval;
}

void *buzzer_thread()
{
	while (!st_kill_process)
	{
		if (ldw_detected == true)
		{
			digitalWrite(GPIO_PIN0, LOW);
			delay(100); // delay of 100ms
			digitalWrite(GPIO_PIN0, HIGH);
			delay(100); // delay of 100ms
		}
		else
		{
			digitalWrite(GPIO_PIN0, HIGH);
			delay(100); // delay of 100ms
		}
	}
}

/*
void *oled_thread()
{
	SSD1306 oled_object;
	while(!st_kill_process)
	{
		if(ldw_detected == true)
		{
			oled_object.textDisplay("LDW detected\n");
		}
		else{
			oled_object.textDisplay("safe Mode\n");
		}
	}

}
*/

int receive_image(int thresh)
{
	int recv_size = 0, size = 0, read_size, write_size, packet_index = 1, retval;
	char imagearray[10241];
	char ack = '#';

	// Find the size of the image
	do
	{
		retval = read(client_fd, &size, sizeof(int));
	} while (retval < 0);

	syslog(LOG_INFO, "size of the image identified as %d\n", size);

	char buffer[] = "Got it";

	// Send our verification signal
	do
	{
		retval = write(client_fd, &buffer, sizeof(buffer));
	} while (retval < 0);

	image = fopen(IMAGE_CAP, "wb+");

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
		// syslog(LOG_INFO,"Select %d",errno);

		if (buffer_fd < 0)
		{
			syslog(LOG_ERR, "error: bad file descriptor set.\n");
			// cleanup_on_exit();
		}

		if (buffer_fd == 0)
		{
			syslog(LOG_ERR, "error: buffer read timeout expired.\n");
			// cleanup_on_exit();
		}

		if (buffer_fd > 0)
		{
			do
			{
				read_size = read(client_fd, imagearray, 10241);
			} while (read_size < 0);
			syslog(LOG_INFO, "read_size%d", read_size);

			// Write the currently read data into our image file
			write_size = fwrite(imagearray, 1, read_size, image);
			syslog(LOG_INFO, "write_size%d", write_size);

			if (read_size != write_size)
			{
				printf("error in read write\n");
			}

			// Increment the total number of bytes read
			recv_size += read_size;
			syslog(LOG_INFO, "recv_size%d", recv_size);
			syslog(LOG_INFO, "packet_index%d", packet_index);

			packet_index++;
			// syslog(LOG_INFO,"Total received image size: %i\n", recv_size);
		}
	}
	send(client_fd, &ack, 1, 0);
	printf("sent ack\n");
	syslog(LOG_INFO, "Total received image size: %i\n", recv_size);

	// buzzer_thread();
	// oled_thread();

	fclose(image);
	cv::Mat inputImage = cv::imread(IMAGE_CAP, cv::IMREAD_GRAYSCALE);
	cv::threshold(inputImage, binaryImage, 150, 255, cv::THRESH_BINARY);
	cv::line(binaryImage, cv::Point(binaryImage.cols/2, 0), cv::Point(binaryImage.cols/2, binaryImage.rows), cv::Scalar::all(0), 3);
	cv::imshow("DISPLAY", binaryImage);
	cv::waitKey(10);
	cv::Canny(binaryImage,binaryImage,10,350);
	cv::HoughLinesP(binaryImage, lines, 1, CV_PI/180, thresh ,0, 0 );
	std::cout << "LINES SIZE - " << lines.size() << std::endl;
	std::cout << "COL - Midpoint - " << binaryImage.cols/2 << std::endl;
   	for( size_t i = 0; i < lines.size(); i++ )
    	{
    		if ( abs((lines[i][0]) - (binaryImage.cols/2)) > LWD_THRESHOLD){
    			printf("OUT_OF_LANE \n\r");
    		}
    		else{
    			printf("IN_LANE \n\r");
    		}
    	}
	
	return 1;
}
/*
void cleanup_on_exit()
{
	close(client_fd);
	fclose(image);
	pthread_join(oled_thread_t, NULL);
	pthread_join(buzzer_thread_t, NULL);

	remove(IMAGE_CAP);

	closelog();
}

//signal handler for closing the connection
void sig_handler()
{
	st_kill_process = 1;
	syslog(LOG_INFO, "SIGINT/ SIGTERM encountered: exiting the process...");
	cleanup_on_exit();
	exit(EXIT_SUCCESS);
}*/

int main(int argc, char* argv[])
{
	int thresh = std::atoi(argv[1]);
	int retvalus;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from client";
	char buffer[1024] = {0};

	// remove(IMAGE_CAP); // remove image on startup

	openlog(NULL, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER); /*open connection for sys logging, ident is NULL to use this Program for the user level messages*/

	if (buzzer_init() == -1)
	{
		return -1;
	}

	/*if ((signal(SIGINT, sig_handler) == SIG_ERR) || (signal(SIGTERM, sig_handler) == SIG_ERR))
	{
		syslog(LOG_ERR, " Signal handler error");
		cleanup_on_exit();
		exit(EXIT_FAILURE);
	}*/

	// pthread_create(&buzzer_thread_t, NULL, buzzer_thread, NULL);
	//  pthread_create(&oled_thread_t, NULL, oled_thread, NULL);

	// create a socket for connection with the server rpi
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		syslog(LOG_ERR, "Socket creation error\n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary
	if (inet_pton(AF_INET, SERVER_IP_ADDR, &serv_addr.sin_addr) <= 0)
	{
		syslog(LOG_ERR, "Invalid address/ Address not supported\n");
		return -1;
	}

	// Initiate a connection to socket
	if ((retvalus = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
	{
		syslog(LOG_ERR, "Connection Failed\n");
		return -1;
	}

	// Send a string to socket to verify send works
	/*if (send(client_fd, hello, strlen(hello), 0) == -1)
	{
		syslog(LOG_ERR, "Failure to send the string\n");
		return -1;
	}*/

	// read a string from the server to verify read works
	/*if (read(client_fd, buffer, 1024) == -1)
	{
		syslog(LOG_ERR, "Failure to read a sample string from server\n");
		return -1;
	}*/
	cv::namedWindow("DISPLAY", cv::WINDOW_NORMAL);
	cv::resizeWindow("DISPLAY", 500,500);

	while (!st_kill_process)
	{
		// recieve image from the server
		if (receive_image(thresh) == -1)
		{
			syslog(LOG_ERR, "Failure to recieve image from server \n");
			return -1;
		}
	}

	close(client_fd);
	return 0;
}
