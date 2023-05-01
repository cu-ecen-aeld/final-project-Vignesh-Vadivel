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
#include <sys/time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>         //Needed for I2C port
#include <sys/ioctl.h>     //Needed for I2C port
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
pthread_mutex_t lock;
int lwd_counter = 0;
int displayLines[1000][1000] = {0};

unsigned char letterL[5] = {0x40, 0x7E, 0x40, 0x40, 0x40};
unsigned char letterD[5] = {0x40, 0x7E, 0x42, 0x42, 0x3C};
unsigned char letterW[5] = {0x40, 0x7E, 0x20, 0x20, 0x7E};
unsigned char letterE[5] = {0x40, 0x7E, 0x52, 0x52, 0x42}; // E
unsigned char letterC[5] = {0x40, 0x7e, 0x42, 0x42, 0x42}; // C
unsigned char letterT[5] = {0x40, 0x02, 0x7E, 0x02, 0x02}; // T
unsigned char hyphen[4] = {0x40, 0x08, 0x08, 0x08};
unsigned char space[3] = {0x40, 0x00, 0x00};

/*****************************Function declaration****************************/
int receive_image(int);
void cleanup_on_exit();
int buzzer_init();
void sig_handler(int signal_number);
void init_oled(void);
void updateDisplayFull();

/*****************************Function definition******************************/




void writeI2C(unsigned char *data, int bytes)
{
    int i2cAddress = 0x3C;
    int i2cHandle;

    char *deviceName = (char *)"/dev/i2c-1";
    if ((i2cHandle = open(deviceName, O_RDWR)) < 0)
    {
        printf("error opening I2C\n");
    }
    else
    {
        if (ioctl(i2cHandle, I2C_SLAVE, i2cAddress) < 0)
        {
            printf("Error at ioctl\n");
        }
        else
        {
            write(i2cHandle, data, bytes);
        }

        // Close the i2c device bus
        close(*deviceName);
    }
}

void clearDisplay()
{
    // blank out the line buffers
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 128; j++)
        {
            displayLines[i][j] = 0;
        }
    }

    // write it out
    updateDisplayFull();
}

void updateDisplayFull()
{
    for (int line = 0; line < 8; line++)
    {
        unsigned char buffer[129] = {0};
        buffer[0] = 0x40;
        for (int i = 0; i < 128; i++)
        {
            buffer[1 + i] = displayLines[line][i];
        }
        writeI2C(buffer, 129);
    }
}

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

void write_oledwarning()
{
    writeI2C(letterL, 5);
    writeI2C(space, 3);
    writeI2C(letterD, 5);
    writeI2C(space, 3);
    writeI2C(letterW, 5);
    writeI2C(space, 3);
    writeI2C(hyphen, 4);
    writeI2C(space, 3);
    writeI2C(letterD, 5);
    writeI2C(space, 3);
    writeI2C(letterE, 5);
    writeI2C(space, 3);
    writeI2C(letterT, 5);
    writeI2C(space, 3);
    writeI2C(letterE, 5);
    writeI2C(space, 3);
    writeI2C(letterC, 5);
    writeI2C(space, 3);
    writeI2C(letterT, 5);
    writeI2C(space, 3);
    writeI2C(letterE, 5);
    writeI2C(space, 3);
    writeI2C(letterD, 5);

    lwd_counter++;
    for (int i = 0; i < 29; i++)
    {
        writeI2C(space, 3);
    }
    writeI2C(space, 2);
}

int receive_image(int thresh)
{
    int recv_size = 0, size = 0, read_size, write_size, packet_index = 1, retval;
    char imagearray[10241];
    char ack = '#';
    std::vector<cv::Point> nonZeroPoints;
    if (lwd_counter > 10)
    {
        init_oled();
        lwd_counter = 0;
        clearDisplay();
    }

    digitalWrite(GPIO_PIN0, LOW);

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
            cleanup_on_exit();
        }

        if (buffer_fd == 0)
        {
            syslog(LOG_ERR, "error: buffer read timeout expired.\n");
            cleanup_on_exit();
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
        }
    }
    send(client_fd, &ack, 1, 0);
    printf("sent ack\n");
    syslog(LOG_INFO, "Total received image size: %i\n", recv_size);

    fclose(image);
    cv::Mat inputImage = cv::imread(IMAGE_CAP, cv::IMREAD_GRAYSCALE);
    cv::threshold(inputImage, binaryImage, 100, 255, cv::THRESH_BINARY_INV);
    cv::imshow("DISPLAY", binaryImage);
    cv::waitKey(10);
    cv::findNonZero(binaryImage, nonZeroPoints);
    for (int i = 0; i < nonZeroPoints.size(); i++)
    {
        if (abs((binaryImage.cols / 2) - (nonZeroPoints[i].x)) < thresh)
        {
            digitalWrite(GPIO_PIN0, LOW);
            delay(100); // delay of 100ms
            digitalWrite(GPIO_PIN0, HIGH);
            delay(100); // delay of 100ms       
            write_oledwarning();
            break;
        }
    }
    return 1;
}

void cleanup_on_exit()
{
    close(client_fd);
    fclose(image);
    remove(IMAGE_CAP);
    closelog();
}

// signal handler for closing the connection
void sig_handler(int signal_number)
{
    st_kill_process = 1;
    syslog(LOG_INFO, "SIGINT/ SIGTERM encountered: exiting the process...");
    cleanup_on_exit();
    exit(EXIT_SUCCESS);
}

void init_oled()
{
    // initialise the display
    unsigned char initSequence[26] = {0x00, 0xAE, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
                                      0xA4, 0xA6, 0xD5, 0x80, 0x8D, 0x14, 0xD9, 0x22, 0xD8, 0x30, 0x20, 0x00, 0xAF};
    writeI2C(initSequence, 26);

    // set the range we want to use (whole display)
    unsigned char setFullRange[7] = {0x00, 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
    writeI2C(setFullRange, 7);

    clearDisplay();
}

int main(int argc, char *argv[])
{
    int thresh = std::atoi(argv[1]);
    int retvalus;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    remove(IMAGE_CAP); // remove image on startup
    init_oled();

    openlog(NULL, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER); /*open connection for sys logging, ident is NULL to use this Program for the user level messages*/

    if ((signal(SIGINT, sig_handler) == SIG_ERR) || (signal(SIGTERM, sig_handler) == SIG_ERR))
    {
        syslog(LOG_ERR, " Signal handler error");
        cleanup_on_exit();
        exit(EXIT_FAILURE);
    }

    if (buzzer_init() == -1)
    {
        exit(EXIT_FAILURE);
    }

    // create a socket for connection with the server rpi
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        syslog(LOG_ERR, "Socket creation error\n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    if (inet_pton(AF_INET, SERVER_IP_ADDR, &serv_addr.sin_addr) <= 0)
    {
        syslog(LOG_ERR, "Invalid address/ Address not supported\n");
        exit(EXIT_FAILURE);
    }

    // Initiate a connection to socket
    if ((retvalus = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        syslog(LOG_ERR, "Connection Failed\n");
        exit(EXIT_FAILURE);
    }

    cv::namedWindow("DISPLAY", cv::WINDOW_NORMAL);
    cv::resizeWindow("DISPLAY", 500, 500);

    while (!st_kill_process)
    {
        // recieve image from the server
        if (receive_image(thresh) == -1)
        {
            syslog(LOG_ERR, "Failure to recieve image from server \n");
            exit(EXIT_FAILURE);
        }
    }
    close(client_fd);
    return 0;
}
