#include <stdio.h> // Import for `printf` & `perror` functions
#include <errno.h> // Import for `errno` variable

#include <fcntl.h>      // Import for `fcntl` functions
#include <unistd.h>     // Import for `fork`, `fcntl`, `read`, `write`, `lseek, `_exit` functions
#include <sys/types.h>  // Import for `socket`, `bind`, `listen`, `accept`, `fork`, `lseek` functions
#include <sys/socket.h> // Import for `socket`, `bind`, `listen`, `accept` functions
#include <netinet/ip.h> // Import for `sockaddr_in` stucture

#include <string.h>  // Import for string functions
#include <stdbool.h> // Import for `bool` data type
#include <stdlib.h>  // Import for `atoi` function

//#include "./functions/server-constants.h"
#include "./functions/faculty.h"
#include "./functions/student.h"
#include "./functions/admin.h"
#include "./functions/server-constants.h"

void connection_handler(int connFD); // Handles the communication with the client

void main()
{
    printf("Server Started Successfully!!\n");
    int sockFD, sockBS, sockLS, connFD;
    struct sockaddr_in serverAddress, clientAddress;

    sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFD == -1)
    {
        perror("Error while creating server socket!");
        _exit(0);
    }

    serverAddress.sin_family = AF_INET;                // IPv4
    serverAddress.sin_port = htons(8080);              // Server will listen to port 8080
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Binds the socket to all interfaces

    sockBS = bind(sockFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (sockBS == -1)
    {
        perror("Error while binding to server socket!");
        _exit(0);
    }

    sockLS = listen(sockFD, 10);
    if (sockLS == -1)
    {
        perror("Error while listening for connections on the server socket!");
        close(sockFD);
        _exit(0);
    }

    int clientSize;
    while (1)
    {
        clientSize = (int)sizeof(clientAddress);
        connFD = accept(sockFD, (struct sockaddr *)&clientAddress, &clientSize);
        if (connFD == -1)
        {
            perror("Error while connecting to client!");
            close(sockFD);
        }
        else
        {
            if (!fork())
            {
                // Child will enter this branch
                connection_handler(connFD);
                close(connFD);
                _exit(0);
            }
        }
    }

    close(sockFD);
}

void connection_handler(int connFD)
{
    printf("Client has connected to the server!\n");

    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;
    int userChoice;

    writeBytes = write(connFD, INITIAL_PROMPT ,strlen(INITIAL_PROMPT));
    if (writeBytes == -1)
        perror("Error while sending first prompt to the user!");
    else
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
            perror("Error while reading from client");
        else if (readBytes == 0)
            printf("No data was sent by the client");
        else
        {
            userChoice = atoi(readBuffer);
            switch (userChoice)
            {
            case 1:
                // Admin
                admin_operation_handler(connFD);
                break;
            case 2:
                // Student
                student_operation_handler(connFD);
                break;
            case 3:
                // Faculty
                faculty_operation_handler(connFD);
            default:
                // Exit
                break;
            }
        }
    }
    printf("Terminating connection to client!\n");
}