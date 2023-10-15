#ifndef COMMON_FUNCTIONS
#define COMMON_FUNCTIONS

#include <stdio.h>     // Import for `printf` & `perror`
#include <unistd.h>    // Import for `read`, `write & `lseek`
#include <string.h>    // Import for string functions
#include <stdbool.h>   // Import for `bool` data type
#include <sys/types.h> // Import for `open`, `lseek`
#include <sys/stat.h>  // Import for `open`
#include <fcntl.h>     // Import for `open`
#include <stdlib.h>    // Import for `atoi`
#include <errno.h>     // Import for `errno`


#include "./admin-credentials.h"
#include "./server-constants.h"
#include "../record_struct/Faculty.h"
#include "../record_struct/Student.h"
#include "../record_struct/Courses.h"
// Function Prototypes =================================

bool login_handler(int isAdmin, int connFD, struct Faculty *ptrToFaculty, struct Student *ptrToStudent);
// bool get_account_details(int connFD, struct Account *customerAccount);
// bool get_customer_details(int connFD, int customerID);
// bool get_transaction_details(int connFD, int accountNumber);

// =====================================================

// Function Definition =================================

bool login_handler(int isAdmin, int connFD, struct Faculty *ptrToFaculty, struct Student *ptrToStudent)
{
    ssize_t readBytes, writeBytes;            // Number of bytes written to / read from the socket
    char readBuffer[1000], writeBuffer[1000]; // Buffer for reading from / writing to the client
    char tempBuffer[1000];
    struct Faculty faculty;
    struct Student student;

    int ID;

    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));

    // Get login message for respective user type
    if (isAdmin == 0)
        strcpy(writeBuffer, "Welcome Admin!");
    else if(isAdmin == 1)
        strcpy(writeBuffer, "Welcome Faculty!");
    else  
        strcpy(writeBuffer, "Welcome Student");
    // Append the request for LOGIN ID message
    strcat(writeBuffer, "\n");
    strcat(writeBuffer, "Enter Your Login ID");

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing WELCOME & LOGIN_ID message to the client!");
        return false;
    }
    // printf("Error before reading from buffer");
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading login ID from client!");
        return false;
    }

    bool userFound = false;

    if (isAdmin == 0)
    {
        if (strcmp(readBuffer, ADMIN_LOGIN_ID) == 0)
            userFound = true;
    }
    else if(isAdmin == 1)
    {
        // printf("Entered isAdmin == 1 condition");
        bzero(tempBuffer, sizeof(tempBuffer));
        strcpy(tempBuffer, readBuffer);
        strtok(tempBuffer, "-");
        ID = atoi(strtok(NULL, "-"));

        int facultyFD = open("./database/faculty_db.txt", O_RDONLY);
        // printf("Opened faculty file in common");
        if (facultyFD == -1)
        {
            perror("Error opening faculty file in read mode!");
            return false;
        }

        off_t offset = lseek(facultyFD, ID * sizeof(struct Faculty), SEEK_SET);
        if (offset >= 0)
        {
            struct flock lock = {F_RDLCK, SEEK_SET, ID * sizeof(struct Faculty), sizeof(struct Faculty), getpid()};

            int lockingStatus = fcntl(facultyFD, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on faculty record!");
                return false;
            }

            readBytes = read(facultyFD, &faculty, sizeof(struct Faculty));
            if (readBytes == -1)
            {
                perror("Error reading faculty record from file!");
            }

            lock.l_type = F_UNLCK;
            fcntl(facultyFD, F_SETLK, &lock);

            if (strcmp(faculty.login, readBuffer) == 0)
                userFound = true;

            close(facultyFD);
        }
    }
    else if(isAdmin == 2)
    {
        // printf("Entered isAdmin == 2 condition");
        bzero(tempBuffer, sizeof(tempBuffer));
        strcpy(tempBuffer, readBuffer);
        strtok(tempBuffer, "-");
        ID = atoi(strtok(NULL, "-"));

        int studentFD = open("./database/student_db.txt", O_RDONLY);
        // printf("Opened student file in common");
        if (studentFD == -1)
        {
            perror("Error opening student file in read mode!");
            return false;
        }

        off_t offset = lseek(studentFD, ID * sizeof(struct Student), SEEK_SET);
        if (offset >= 0)
        {
            struct flock lock = {F_RDLCK, SEEK_SET, ID * sizeof(struct Student), sizeof(struct Student), getpid()};

            int lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining read lock on student record!");
                return false;
            }

            readBytes = read(studentFD, &student, sizeof(struct Student));
            if (readBytes == -1)
            {
                perror("Error reading student record from file!");
            }

            lock.l_type = F_UNLCK;
            fcntl(studentFD, F_SETLK, &lock);

            if (strcmp(student.login, readBuffer) == 0)
                userFound = true;

            close(studentFD);
        }
        else
        {
            writeBytes = write(connFD, STUDENT_LOGIN_ID_DOESNT_EXIST, strlen(STUDENT_LOGIN_ID_DOESNT_EXIST));
        }
    }

    if (userFound)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, PASSWORD, strlen(PASSWORD));
        if (writeBytes == -1)
        {
            perror("Error writing PASSWORD message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == 1)
        {
            perror("Error reading password from the client!");
            return false;
        }

        if (isAdmin == 0)
        {
            if (strcmp(readBuffer, ADMIN_PASSWORD) == 0)
                return true;
        }
        else if(isAdmin == 1)
        {
            if (strcmp(readBuffer, faculty.password) == 0)
            {
                *ptrToFaculty = faculty;
                return true;
            }
        }
        else if(isAdmin == 2)
        {
            if(strcmp(readBuffer, student.password) == 0)
            {
                *ptrToStudent = student;
                return true;
            }
        }

        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, INVALID_PASSWORD, strlen(INVALID_PASSWORD));
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, INVALID_LOGIN, strlen(INVALID_LOGIN));
    }

    return false;
}

#endif