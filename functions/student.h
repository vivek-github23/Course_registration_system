#ifndef STUDENT_FUNCTIONS
#define STUDENT_FUNCTIONS

// Semaphores are necessary joint account due the design choice I've made
#include <sys/ipc.h>
#include <sys/sem.h>

#include "./common.h"
struct Student loggedInStudent;
int semIdentifier;

// Function Prototypes =================================

bool student_operation_handler(int connFD);
bool enrollNewCourse(int connFD, struct Student loggedInStudent);
bool unenrollOfferedCourse(int connFD, struct Student loggedInStudent);
bool viewEnrolledCourses(int connFD, struct Student loggedInStudent);
bool changePasswordStudent(int connFD);
// bool deposit(int connFD);
// bool withdraw(int connFD);
// bool get_balance(int connFD);
// bool change_password(int connFD);
bool lock_critical_section_s(struct sembuf *semOp);
bool unlock_critical_section_s(struct sembuf *sem_op);
// void write_transaction_to_array(int *transactionArray, int ID);
// int write_transaction_to_file(int accountNumber, long int oldBalance, long int newBalance, bool operation);

// =====================================================

// Function Definition =================================

bool student_operation_handler(int connFD)
{
    if (login_handler(2, connFD, NULL, &loggedInStudent))
    {
        ssize_t writeBytes, readBytes;            // Number of bytes read from / written to the client
        char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading & writing to the client

        // Get a semaphore for the user
        // key_t semKey = ftok("./database.student_db.txt", loggedInStudent.courseID[loggedInStudent.numCourses]); // Generate a key based on the account number hence, different students will have different semaphores

        // union semun
        // {
        //     int val; // Value of the semaphore
        // } semSet;

        // int semctlStatus;
        // semIdentifier = semget(semKey, 1, 0); // Get the semaphore if it exists
        // if (semIdentifier == -1)
        // {
        //     semIdentifier = semget(semKey, 1, IPC_CREAT | 0700); // Create a new semaphore
        //     if (semIdentifier == -1)
        //     {
        //         perror("Error while creating semaphore!");
        //         _exit(1);
        //     }

        //     semSet.val = 1; // Set a binary semaphore
        //     semctlStatus = semctl(semIdentifier, 0, SETVAL, semSet);
        //     if (semctlStatus == -1)
        //     {
        //         perror("Error while initializing a binary sempahore!");
        //         _exit(1);
        //     }
        // }

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, STUDENT_LOGIN_SUCCESS);
        while (1)
        {
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, STUDENT_MENU);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing STUDENT_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for STUDENT_MENU");
                return false;
            }
            
            // printf("READ BUFFER : %s\n", readBuffer);
            int choice = atoi(readBuffer);
            // printf("CHOICE : %d\n", choice);
            switch (choice)
            {
            case 1:
                enrollNewCourse(connFD, loggedInStudent);
                break;
            case 2:
                unenrollOfferedCourse(connFD, loggedInStudent);
                break;
            case 3:
                viewEnrolledCourses(connFD, loggedInStudent);
                break;
            case 4:
                changePasswordStudent(connFD);
                break;
            default:
                writeBytes = write(connFD, STUDENT_LOGOUT, strlen(STUDENT_LOGOUT));
                return false;
            }
        }
    }
    else
    {
        // STUDENT LOGIN FAILED
        return false;
    }
    return true;
}

bool enrollNewCourse(int connFD, struct Student loggedInStudent)
{
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Courses course;

    int courseFD = open("./database/course_db.txt", O_RDONLY);
    int enrollID;
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
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, STUDENT_ENROLL_COURSE);
        writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
        if(writeBytes == -1){
            perror("Cant write STUDENT_ENROLL_COURSE to write buffer\n");
            return false;
        }
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));

        enrollID = atoi(readBuffer);

        int offset = lseek(courseFD, enrollID * sizeof(struct Courses), SEEK_SET);
        if (offset == -1)
        {
            perror("Error seeking to enrollID Course record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Courses), getpid()};
        int lockingStatus = fcntl(courseFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Course record!");
            return false;
        }
        course.studentsEnrolled = course.studentsEnrolled + 1;
        readBytes = read(courseFD, &course, sizeof(struct Courses));
        if (readBytes == -1)
        {
            perror("Error while reading student record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(courseFD, F_SETLK, &lock);

        close(courseFD);
    }
    
    // Lock the critical section
    struct sembuf semOp;


    lock_critical_section(&semOp);

    // writeBytes = write(connFD, STUDENT_ADD_NEW_COURSE, strlen(STUDENT_ADD_NEW_COURSE));
    // if (writeBytes == -1)
    // {
    //     perror("Error writing STUDENT_ADD_NEW_COURSE to client!");
    //     unlock_critical_section(&semOp);
    //     return false;
    // }
    courseFD = open("./database/course_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    off_t offset = lseek(courseFD, enrollID * sizeof(struct Courses), SEEK_SET);
    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Courses), getpid()};
    int lockingStatus = fcntl(courseFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining write lock on course file!");
        unlock_critical_section(&semOp);
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    course.studentID[course.studentsEnrolled - 1] = loggedInStudent.id;
    course.studentsEnrolled = course.studentsEnrolled + 1;
    writeBytes = write(courseFD, &course, sizeof(struct Courses));
    if (writeBytes == -1)
    {
        perror("Error enrolling courseId to student record!");
        unlock_critical_section(&semOp);
        return false;
    }
    close(courseFD);

    int studentFD = open("./database/student_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    off_t offset1 = lseek(studentFD, loggedInStudent.id * sizeof(struct Student), SEEK_SET);
    struct flock lock1 = {F_WRLCK, SEEK_SET, offset1, sizeof(struct Student), getpid()};
    int lockingStatus1 = fcntl(studentFD, F_SETLKW, &lock1);
    if (lockingStatus1 == -1)
    {
        perror("Error obtaining write lock on student file!");
        unlock_critical_section(&semOp);
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    loggedInStudent.courseID[loggedInStudent.numCourses] = enrollID;
    loggedInStudent.numCourses = loggedInStudent.numCourses + 1;
    writeBytes = write(studentFD, &loggedInStudent, sizeof(struct Student));
    if (writeBytes == -1)
    {
        perror("Error enrolling courseId to student record!");
        unlock_critical_section(&semOp);
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer,"\nCourse enrolled with id: %d\n",enrollID);
    write(connFD, writeBuffer,sizeof(writeBuffer));
    // write(connFD, writeBuffer,sizeof(writeBuffer));
    lock1.l_type = F_UNLCK;
    fcntl(studentFD, F_SETLK, &lock1);

    write(connFD, STUDENT_COURSE_ENROLL_SUCCESS, strlen(STUDENT_COURSE_ENROLL_SUCCESS));
    // read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    unlock_critical_section(&semOp);
    return true;
}

bool unenrollOfferedCourse(int connFD, struct Student loggedInStudent)
{
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Courses course;
    int removeID;
    int studentFD = open("./database/student_db.txt", O_RDONLY);

    if (studentFD == -1 && errno == ENOENT)
    {
        // Student file was never created
        write(connFD, STUDENT_FILE_DOESNT_EXIST, strlen(STUDENT_FILE_DOESNT_EXIST));
        return false;
    }
    else if (studentFD == -1)
    {
        perror("Error while opening student file");
        return false;
    }
    else
    {
        write(connFD, STUDENT_COURSE_UNENROLL_ID, strlen(STUDENT_COURSE_UNENROLL_ID));
        bzero(readBuffer, sizeof(readBuffer));
        read(connFD, readBuffer, sizeof(readBuffer));
        removeID = atoi(readBuffer);
        int offset = lseek(studentFD, loggedInStudent.id * sizeof(struct Student), SEEK_SET);
        if (offset == -1)
        {
            perror("Error seeking to Student record!");
            return false;
        }
        
        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};
        int lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Student record!");
            return false;
        }

        readBytes = read(studentFD, &loggedInStudent, sizeof(struct Student));
        if (readBytes == -1)
        {
            perror("Error while reading student record from file!");
            return false;
        }
        

        lock.l_type = F_UNLCK;
        fcntl(studentFD, F_SETLK, &lock);

        close(studentFD);
    }
    
    // Lock the critical section
    struct sembuf semOp;

    lock_critical_section(&semOp);

    // writeBytes = write(studentFD, STUDENT_COURSE_UNENROLLED_ID, strlen(STUDENT_COURSE_UNENROLLED_ID));
    // if (writeBytes == -1)
    // {
    //     perror("Error writing COURSE_DEACTIVATED to client!");
    //     unlock_critical_section(&semOp);
    //     return false;
    // }
    
    int courseFD = open("./database/course_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if(courseFD == -1){
        perror("Error opening course file");
        return false;
    }
    off_t offset = lseek(courseFD, removeID * sizeof(struct Courses), SEEK_SET);
    struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Courses), getpid()};
    int lockingStatus = fcntl(courseFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error obtaining write lock on course file!");
        // unlock_critical_section(&semOp);
        return false;
    }
    for(int i = 0; i < course.studentsEnrolled; i++){
        if(course.studentID[i] == loggedInStudent.id){
            course.studentID[i] = 0;
        }
    }
    course.studentsEnrolled = course.studentsEnrolled - 1;
    write(courseFD, &course, sizeof(struct Courses));
    lock.l_type = F_UNLCK;
    fcntl(courseFD, F_SETLK, &lock);
    close(courseFD);

    studentFD = open("./database/student_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if(studentFD == -1){
        perror("Error opening student file");
        return false;
    }
    off_t offset1 = lseek(studentFD, loggedInStudent.id * sizeof(struct Student), SEEK_SET);
    struct flock lock1 = {F_WRLCK, SEEK_SET, offset1, sizeof(struct Courses), getpid()};
    lockingStatus = fcntl(studentFD, F_SETLKW, &lock1);
    if (lockingStatus == -1)
    {
        perror("Error obtaining write lock on student file!");
        unlock_critical_section(&semOp);
        return false;
    }
    for(int i = 0; i < loggedInStudent.numCourses; i++){
        if(loggedInStudent.courseID[i] == removeID){
            loggedInStudent.courseID[i] = 0;
            loggedInStudent.numCourses = loggedInStudent.numCourses - 1;
            break;
        }
    }
    writeBytes = write(studentFD, &loggedInStudent, sizeof(struct Student));
    if (writeBytes == -1)
    {
        perror("Error removing course in student record!");
        unlock_critical_section(&semOp);
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer,"Course unenrolled with id: %d for student id: %d\n",removeID,loggedInStudent.id);
    write(connFD, writeBuffer,sizeof(writeBuffer));
    lock1.l_type = F_UNLCK;
    fcntl(studentFD, F_SETLK, &lock1);

    write(connFD, COURSE_UNENROLL_SUCCESS, strlen(COURSE_UNENROLL_SUCCESS));
    // read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    unlock_critical_section(&semOp);
    return true;
}   

bool viewEnrolledCourses(int connFD, struct Student loggedInStudent){
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;

    int studentFD = open("./database/student_db.txt", O_RDONLY);
    if(studentFD == -1){
        perror("Error opening student record");
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    write(connFD, STUDENT_ENROLLED_COURSES, strlen(STUDENT_ENROLLED_COURSES));
    
    int lseekPointer = lseek(studentFD, loggedInStudent.id * sizeof(struct Student), SEEK_SET);
    if(lseekPointer == -1){
        perror("Error seeking to the student record");
        return false;
    }

    bzero(writeBuffer,sizeof(writeBuffer));
    for(int i = 0; i < loggedInStudent.numCourses; i++){
        sprintf(writeBuffer,"%d, ",loggedInStudent.courseID[i]);
    }
    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    // write(connFD, STUDENT_COURSES_VIEWED, strlen(STUDENT_COURSES_VIEWED));
    close(studentFD);
    return true;
}

bool changePasswordStudent(int connFD)
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

    if (strcmp(readBuffer, loggedInStudent.password) == 0)
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

            strcpy(loggedInStudent.password, newPassword);

            int studentFileDescriptor = open("./database/student_db.txt", O_WRONLY);
            if (studentFileDescriptor == -1)
            {
                perror("Error opening student file!");
                unlock_critical_section(&semOp);
                return false;
            }

            off_t offset = lseek(studentFileDescriptor, loggedInStudent.id * sizeof(struct Student), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the student record!");
                unlock_critical_section(&semOp);
                return false;
            }

            struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};
            int lockingStatus = fcntl(studentFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on student record!");
                unlock_critical_section(&semOp);
                return false;
            }

            writeBytes = write(studentFileDescriptor, &loggedInStudent, sizeof(struct Student));
            if (writeBytes == -1)
            {
                perror("Error storing updated student password into student record!");
                unlock_critical_section(&semOp);
                return false;
            }

            lock.l_type = F_UNLCK;
            lockingStatus = fcntl(studentFileDescriptor, F_SETLK, &lock);

            close(studentFileDescriptor);

            writeBytes = write(connFD, PASSWORD_CHANGE_SUCCESS, strlen(PASSWORD_CHANGE_SUCCESS));
            // readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

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

bool lock_critical_section_s(struct sembuf *semOp)
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

bool unlock_critical_section_s(struct sembuf *semOp)
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