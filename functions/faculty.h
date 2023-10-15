#ifndef FACULTY_FUNCTIONS
#define FACULTY_FUNCTIONS

// Semaphores are necessary joint account due the design choice I've made
#include <sys/ipc.h>
#include <sys/sem.h>

#include "./common.h"
struct Faculty loggedInFaculty;
int semIdentifier;

// Function Prototypes =================================

bool faculty_operation_handler(int connFD);
bool addNewCourse(int connFD, struct Faculty loggedInFaculty);
bool removeOfferedCourse(int connFD, struct Faculty loggedInFaculty);
bool viewEnrollmentsFaculty(int connFD, struct Faculty loggedInFaculty);
bool changePassword(int connFD);
// bool deposit(int connFD);
// bool withdraw(int connFD);
// bool get_balance(int connFD);
// bool change_password(int connFD);
bool lock_critical_section(struct sembuf *semOp);
bool unlock_critical_section(struct sembuf *sem_op);
// void write_transaction_to_array(int *transactionArray, int ID);
// int write_transaction_to_file(int accountNumber, long int oldBalance, long int newBalance, bool operation);

// =====================================================

// Function Definition =================================

bool faculty_operation_handler(int connFD)
{
    if (login_handler(1, connFD, &loggedInFaculty,NULL))
    {
        ssize_t writeBytes, readBytes;            // Number of bytes read from / written to the client
        char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading & writing to the client

        // Get a semaphore for the user
        key_t semKey = ftok("./database.faculty_db.txt", loggedInFaculty.id); // Generate a key based on the course number hence, different facultys will have different semaphores

        union semun
        {
            int val; // Value of the semaphore
        } semSet;

        int semctlStatus;
        semIdentifier = semget(semKey, 1, 0); // Get the semaphore if it exists
        if (semIdentifier == -1)
        {
            semIdentifier = semget(semKey, 1, IPC_CREAT | 0700); // Create a new semaphore
            if (semIdentifier == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            semSet.val = 1; // Set a binary semaphore
            semctlStatus = semctl(semIdentifier, 0, SETVAL, semSet);
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, FACULTY_LOGIN_SUCCESS);
        while (1)
        {
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, FACULTY_MENU);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing FACULTY_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for FACULTY_MENU");
                return false;
            }
            
            // printf("READ BUFFER : %s\n", readBuffer);
            int choice = atoi(readBuffer);
            // printf("CHOICE : %d\n", choice);
            switch (choice)
            {
            case 1:
                addNewCourse(connFD, loggedInFaculty);
                break;
            case 2:
                removeOfferedCourse(connFD, loggedInFaculty);
                break;
            case 3:
                viewEnrollmentsFaculty(connFD, loggedInFaculty);
                break;
            case 4:
                changePassword(connFD);
                break;
            default:
                writeBytes = write(connFD, FACULTY_LOGOUT, strlen(FACULTY_LOGOUT));
                return false;
            }
        }
    }
    else
    {
        // FACULTY LOGIN FAILED
        return false;
    }
    return true;
}

bool addNewCourse(int connFD, struct Faculty loggedInFaculty)
{
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Courses newcourse, prevcourse;

    int courseFD = open("./database/course_db.txt", O_RDONLY);

    if (courseFD == -1 && errno == ENOENT)
    {
        // Course file was never created
        newcourse.id = 0;
        newcourse.studentsEnrolled = 0;
    }
    else if (courseFD == -1)
    {
        perror("Error while opening course file");
        return false;
    }
    else
    {
        int offset = lseek(courseFD, -sizeof(struct Courses), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Course record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Courses), getpid()};
        int lockingStatus = fcntl(courseFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Course record!");
            return false;
        }

        readBytes = read(courseFD, &prevcourse, sizeof(struct Courses));
        if (readBytes == -1)
        {
            perror("Error while reading faculty record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(courseFD, F_SETLK, &lock);

        close(courseFD);

        newcourse.id = prevcourse.id + 1;
        newcourse.studentsEnrolled = prevcourse.studentsEnrolled + 1;
    }
    
    // Lock the critical section
    struct sembuf semOp;

    lock_critical_section(&semOp);

    writeBytes = write(connFD, FACULTY_ADD_NEW_COURSE, strlen(FACULTY_ADD_NEW_COURSE));
    if (writeBytes == -1)
    {
        perror("Error writing FACULTY_ADD_NEW_COURSE to client!");
        unlock_critical_section(&semOp);
        return false;
    }
    loggedInFaculty.courseID[loggedInFaculty.numCoursesf] = newcourse.id;
    loggedInFaculty.numCoursesf = loggedInFaculty.numCoursesf + 1; 

    courseFD = open("./database/course_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    off_t offset = lseek(courseFD, newcourse.id * sizeof(struct Courses), SEEK_SET);
    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Courses), getpid()};
    int lockingStatus = fcntl(courseFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining write lock on account file!");
        unlock_critical_section(&semOp);
        return false;
    }
    writeBytes = write(courseFD, &newcourse, sizeof(struct Courses));
    if (writeBytes == -1)
    {
        perror("Error storing added course in courses record!");
        unlock_critical_section(&semOp);
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer,"\nCourse added with id: %d\n",newcourse.id);
    write(connFD, writeBuffer,sizeof(writeBuffer));
    // write(connFD, writeBuffer,sizeof(writeBuffer));
    lock.l_type = F_UNLCK;
    fcntl(courseFD, F_SETLK, &lock);

    write(connFD, COURSE_ADD_SUCCESS, strlen(COURSE_ADD_SUCCESS));
    // read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    unlock_critical_section(&semOp);
    return true;
}

bool removeOfferedCourse(int connFD, struct Faculty loggedInFaculty)
{
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Courses course;
    int removeID;
    int courseFD = open("./database/course_db.txt", O_RDONLY);

    if (courseFD == -1 && errno == ENOENT)
    {
        // Course file was never created
        write(connFD, COURSE_FILE_DOESNT_EXIST, strlen(COURSE_FILE_DOESNT_EXIST));
        return false;
    }
    else if (courseFD == -1)
    {
        perror("Error while opening course file");
        return false;
    }
    else
    {
        write(connFD, COURSE_REMOVE_ID, strlen(COURSE_REMOVE_ID));
        bzero(readBuffer, sizeof(readBuffer));
        read(connFD, readBuffer, sizeof(readBuffer));
        removeID = atoi(readBuffer);
        int offset = lseek(courseFD, removeID * sizeof(struct Courses), SEEK_SET);
        if (offset == -1)
        {
            perror("Error seeking to Course record!");
            return false;
        }
        
        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Courses), getpid()};
        int lockingStatus = fcntl(courseFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Course record!");
            return false;
        }

        readBytes = read(courseFD, &course, sizeof(struct Courses));
        if (readBytes == -1)
        {
            perror("Error while reading faculty record from file!");
            return false;
        }
        course.isActive = false;

        lock.l_type = F_UNLCK;
        fcntl(courseFD, F_SETLK, &lock);

        close(courseFD);
    }
    
    // Lock the critical section
    struct sembuf semOp;

    lock_critical_section(&semOp);

    writeBytes = write(connFD, COURSE_DEACTIVATED, strlen(COURSE_DEACTIVATED));
    if (writeBytes == -1)
    {
        perror("Error writing COURSE_DEACTIVATED to client!");
        unlock_critical_section(&semOp);
        return false;
    }
    
    courseFD = open("./database/course_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    off_t offset = lseek(courseFD, removeID * sizeof(struct Courses), SEEK_SET);
    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Courses), getpid()};
    int lockingStatus = fcntl(courseFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining write lock on course file!");
        unlock_critical_section(&semOp);
        return false;
    }
    writeBytes = write(courseFD, &course, sizeof(struct Courses));
    if (writeBytes == -1)
    {
        perror("Error removing course in courses record!");
        unlock_critical_section(&semOp);
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer,"Course deactivated with id: %d",removeID);
    write(connFD, writeBuffer,sizeof(writeBuffer));
    lock.l_type = F_UNLCK;
    fcntl(courseFD, F_SETLK, &lock);

    write(connFD, COURSE_DEACTIVATE_SUCCESS, strlen(COURSE_DEACTIVATE_SUCCESS));
    read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    unlock_critical_section(&semOp);
    return true;
}   

bool viewEnrollmentsFaculty(int connFD, struct Faculty loggedInFaculty){
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;
    struct Courses course;

    writeBytes = write(connFD, FACULTY_VIEW_ENROLL_ID, strlen(FACULTY_VIEW_ENROLL_ID));
    if(writeBytes == -1){
        perror("Error writing data to view enrollments\n");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if(readBytes == -1){
        perror("Error reading enroll id to see enrollments\n");
        return false;
    }
    int courseId = atoi(readBuffer);
    int courseFD = open("./database/course_db.txt", O_RDONLY);
    if(courseFD == -1){
        perror("Error opening course record");
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    write(connFD, FACULTY_ENROLLED_COURSES, strlen(FACULTY_ENROLLED_COURSES));
    
    int lseekPointer = lseek(courseFD, courseId * sizeof(struct Courses), SEEK_SET);
    if(lseekPointer == -1){
        perror("Error seeking to the course record");
        return false;
    }

    bzero(writeBuffer,sizeof(writeBuffer));
    for(int i = 0; i < course.studentsEnrolled; i++){
        sprintf(writeBuffer,"%d, ",course.studentID[i]);
    }
    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    // write(connFD, STUDENT_COURSES_VIEWED, strlen(STUDENT_COURSES_VIEWED));
    close(courseFD);
    return true;
}

bool changePassword(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000], hashedPassword[1000];

    char newPassword[1000];

    // Lock the critical section
    struct sembuf semOp = {0, -1, SEM_UNDO};
    int semopStatus = semop(semIdentifier, &semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }

    writeBytes = write(connFD, PASSWORD_CHANGE_OLD_PASS, strlen(PASSWORD_CHANGE_OLD_PASS));
    if (writeBytes == -1)
    {
        perror("Error writing PASSWORD_CHANGE_OLD_PASS message to client!");
        unlock_critical_section(&semOp);
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading old password response from client");
        unlock_critical_section(&semOp);
        return false;
    }

    if (strcmp(readBuffer, loggedInFaculty.password) == 0)
    {
        // Password matches with old password
        writeBytes = write(connFD, PASSWORD_CHANGE_NEW_PASS, strlen(PASSWORD_CHANGE_NEW_PASS));
        if (writeBytes == -1)
        {
            perror("Error writing PASSWORD_CHANGE_NEW_PASS message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading new password response from client");
            unlock_critical_section(&semOp);
            return false;
        }

        strcpy(newPassword, readBuffer);

        writeBytes = write(connFD, PASSWORD_CHANGE_NEW_PASS_RE, strlen(PASSWORD_CHANGE_NEW_PASS_RE));
        if (writeBytes == -1)
        {
            perror("Error writing PASSWORD_CHANGE_NEW_PASS_RE message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading new password reenter response from client");
            unlock_critical_section(&semOp);
            return false;
        }

        if (strcmp(readBuffer, newPassword) == 0)
        {
            // New & reentered passwords match

            strcpy(loggedInFaculty.password, newPassword);

            int facultyFileDescriptor = open("./database/faculty_db.txt", O_WRONLY);
            if (facultyFileDescriptor == -1)
            {
                perror("Error opening faculty file!");
                unlock_critical_section(&semOp);
                return false;
            }

            off_t offset = lseek(facultyFileDescriptor, loggedInFaculty.id * sizeof(struct Faculty), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the faculty record!");
                unlock_critical_section(&semOp);
                return false;
            }

            struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};
            int lockingStatus = fcntl(facultyFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on faculty record!");
                unlock_critical_section(&semOp);
                return false;
            }

            writeBytes = write(facultyFileDescriptor, &loggedInFaculty, sizeof(struct Faculty));
            if (writeBytes == -1)
            {
                perror("Error storing updated faculty password into faculty record!");
                unlock_critical_section(&semOp);
                return false;
            }

            lock.l_type = F_UNLCK;
            lockingStatus = fcntl(facultyFileDescriptor, F_SETLK, &lock);

            close(facultyFileDescriptor);

            writeBytes = write(connFD, PASSWORD_CHANGE_SUCCESS, strlen(PASSWORD_CHANGE_SUCCESS));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

            unlock_critical_section(&semOp);

            return true;
        }
        else
        {
            // New & reentered passwords don't match
            writeBytes = write(connFD, PASSWORD_CHANGE_NEW_PASS_INVALID, strlen(PASSWORD_CHANGE_NEW_PASS_INVALID));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        }
    }
    else
    {
        // Password doesn't match with old password
        writeBytes = write(connFD, PASSWORD_CHANGE_OLD_PASS_INVALID, strlen(PASSWORD_CHANGE_OLD_PASS_INVALID));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
    }

    unlock_critical_section(&semOp);

    return false;
}


bool lock_critical_section(struct sembuf *semOp)
{
    semOp->sem_flg = SEM_UNDO;
    semOp->sem_op = -1;
    semOp->sem_num = 0;
    int semopStatus = semop(semIdentifier, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }
    return true;
}

bool unlock_critical_section(struct sembuf *semOp)
{
    semOp->sem_op = 1;
    int semopStatus = semop(semIdentifier, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while operating on semaphore!");
        _exit(1);
    }
    return true;
}

// =====================================================

#endif