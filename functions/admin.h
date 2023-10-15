#ifndef ADMIN_FUNCTIONS
#define ADMIN_FUNCTIONS


bool admin_operation_handler(int connFD);
bool addFaculty(int connFD);
bool addStudent(int connFD);
bool activate_deactivate_student(int connFD);
bool update_faculty_details(int connFD);
bool update_student_details(int connFD);
// bool add_account(int connFD);
// int add_customer(int connFD, bool isPrimary, int newAccountNumber);
// bool delete_account(int connFD);
// bool modify_customer_info(int connFD);

#include "./common.h"

bool admin_operation_handler(int connFD)
{

    if (login_handler(0, connFD, NULL, NULL))
    {
        ssize_t writeBytes, readBytes;            // Number of bytes read from / written to the client
        char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading & writing to the client
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ADMIN_LOGIN_SUCCESS);
        while (1)
        {
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, ADMIN_MENU);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }

            int choice = atoi(readBuffer);
            
            switch (choice)
            {
            case 1:
                addFaculty(connFD);
                break;
            case 2:
                addStudent(connFD);
                break;
            case 3: 
                activate_deactivate_student(connFD);
                break;
            case 4:
                update_faculty_details(connFD);
                break;
            case 5:
                update_student_details(connFD);
                break;
            
            default:
                writeBytes = write(connFD, ADMIN_LOGOUT, strlen(ADMIN_LOGOUT));
                return false;
            }
        }
    }
    else
    {
        // ADMIN LOGIN FAILED
        return false;
    }
    return true;
}

bool addFaculty(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Faculty newfaculty, prevfaculty;

    int facultyFD = open("./database/faculty_db.txt", O_RDONLY);
    if (facultyFD == -1 && errno == ENOENT)
    {
        // Faculty file was never created
        newfaculty.id = 0;
        newfaculty.numCoursesf = 0;
    }
    else if (facultyFD == -1)
    {
        perror("Error while opening faculty file");
        return false;
    }
    else
    {
        int offset = lseek(facultyFD, -sizeof(struct Faculty), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Faculty record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};
        int lockingStatus = fcntl(facultyFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Faculty record!");
            return false;
        }

        readBytes = read(facultyFD, &prevfaculty, sizeof(struct Faculty));
        if (readBytes == -1)
        {
            perror("Error while reading faculty record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(facultyFD, F_SETLK, &lock);

        close(facultyFD);

        newfaculty.id = prevfaculty.id + 1;
    }
    writeBytes = write(connFD, ADMIN_ADD_FACULTY_NAME, strlen(ADMIN_ADD_FACULTY_NAME));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_FACULTY_NAME message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading faculty name response from client!");
        return false;
    }

    strcpy(newfaculty.name, readBuffer);
    
    writeBytes = write(connFD, ADMIN_ADD_FACULTY_GENDER, strlen(ADMIN_ADD_FACULTY_GENDER));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_ACCOUNT_GENDER message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading faculty gender response from client!");
        return false;
    }
    if (readBuffer[0] == 'M' || readBuffer[0] == 'F' || readBuffer[0] == 'O')
        newfaculty.gender = readBuffer[0];
    else
    {
        writeBytes = write(connFD, ADMIN_ADD_FACULTY_WRONG_GENDER, strlen(ADMIN_ADD_FACULTY_WRONG_GENDER));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); 
        return false;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, ADMIN_ADD_FACULTY_AGE);
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_FACULTY_AGE message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading faculty age response from client!");
        return false;
    }

    int facultyAge = atoi(readBuffer);
    if (facultyAge == 0)
    {
        // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    newfaculty.age = facultyAge;

    strcpy(newfaculty.login, newfaculty.name);
    strcat(newfaculty.login, "-");
    sprintf(writeBuffer, "%d", newfaculty.id);
    strcat(newfaculty.login, writeBuffer);

    char hashedPassword[100] = AUTOGEN_PASSWORD;
    strcpy(newfaculty.password, hashedPassword);

    facultyFD = open("./database/faculty_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (facultyFD == -1)
    {
        perror("Error while creating / opening faculty file!");
        return false;
    }
    writeBytes = write(facultyFD, &newfaculty, sizeof(newfaculty));
    if (writeBytes == -1)
    {
        perror("Error while writing Faculty record to file!");
        return false;
    }

    close(facultyFD);

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "%s%s-%d\n%s%s", ADMIN_ADD_FACULTY_AUTOGEN_LOGIN, newfaculty.name, newfaculty.id, ADMIN_ADD_FACULTY_AUTOGEN_PASSWORD, AUTOGEN_PASSWORD);
    strcat(writeBuffer, "^");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error sending faculty loginID and password to the client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return newfaculty.id;
}

bool addStudent(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Student newstudent, prevstudent;

    int studentFD = open("./database/student_db.txt", O_RDONLY);
    if (studentFD == -1 && errno == ENOENT)
    {
        // Student file was never created
        newstudent.id = 0;
    }
    else if (studentFD == -1)
    {
        perror("Error while opening student file");
        return false;
    }
    else
    {
        int offset = lseek(studentFD, -sizeof(struct Student), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Student record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};
        int lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Student record!");
            return false;
        }

        readBytes = read(studentFD, &prevstudent, sizeof(struct Student));
        if (readBytes == -1)
        {
            perror("Error while reading student record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(studentFD, F_SETLK, &lock);

        close(studentFD);

        newstudent.id = prevstudent.id + 1;
    }
    writeBytes = write(connFD, ADMIN_ADD_STUDENT_NAME, strlen(ADMIN_ADD_STUDENT_NAME));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_STUDENT_NAME message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading student name response from client!");
        return false;
    }

    strcpy(newstudent.name, readBuffer);
    strcpy(newstudent.isActive, "true");
    writeBytes = write(connFD, ADMIN_ADD_STUDENT_GENDER, strlen(ADMIN_ADD_STUDENT_GENDER));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_ACCOUNT_GENDER message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading student gender response from client!");
        return false;
    }
    if (readBuffer[0] == 'M' || readBuffer[0] == 'F' || readBuffer[0] == 'O')
        strcpy(newstudent.gender, readBuffer);
    else
    {
        writeBytes = write(connFD, ADMIN_ADD_STUDENT_WRONG_GENDER, strlen(ADMIN_ADD_STUDENT_WRONG_GENDER));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); 
        return false;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, ADMIN_ADD_STUDENT_AGE);
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing ADMIN_ADD_STUDENT_AGE message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading student age response from client!");
        return false;
    }

    int studentAge = atoi(readBuffer);
    if (studentAge == 0)
    {
        // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    newstudent.age = studentAge;

    strcpy(newstudent.login, newstudent.name);
    strcat(newstudent.login, "-");
    sprintf(writeBuffer, "%d", newstudent.id);
    strcat(newstudent.login, writeBuffer);

    char hashedPassword[100] = "spookytime";
    strcpy(newstudent.password, hashedPassword);

    studentFD = open("./database/student_db.txt", O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (studentFD == -1)
    {
        perror("Error while creating / opening student file!");
        return false;
    }
    writeBytes = write(studentFD, &newstudent, sizeof(newstudent));
    if (writeBytes == -1)
    {
        perror("Error while writing Student record to file!");
        return false;
    }

    close(studentFD);

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "%s%s-%d\n%s%s\n%s%d", ADMIN_ADD_STUDENT_AUTOGEN_LOGIN, newstudent.name, newstudent.id, ADMIN_ADD_STUDENT_AUTOGEN_PASSWORD, AUTOGEN_PASSWORD,STUDENT_ID,newstudent.id);
    strcat(writeBuffer, "^");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error sending student loginID and password to the client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return newstudent.id;
}

bool activate_deactivate_student(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Student student;

    writeBytes = write(connFD, ADMIN_ACTIVATE_DEACTIVATE_PROMPT, strlen(ADMIN_ACTIVATE_DEACTIVATE_PROMPT));

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer,sizeof(readBuffer));

    int activate = atoi(readBuffer);

    if(activate == 0){
        writeBytes = write(connFD, ADMIN_DEACTIVATE_STUDENT, strlen(ADMIN_DEACTIVATE_STUDENT));
        if (writeBytes == -1)
        {
            perror("Error writing ADMIN_DEACTIVATE_STUDENT to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading account number response from the client!");
            return false;
        }

        int deactID = atoi(readBuffer);

        int studentFD = open("./record_struct/Student.h", O_RDONLY);
        if (studentFD == -1)
        {
            // Student record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, STUDENT_ID_DOESNT_EXIT);
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing STUDENT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }


        int offset = lseek(studentFD, deactID * sizeof(struct Student), SEEK_SET);
        if (errno == EINVAL)
        {
            // Student record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, STUDENT_ID_DOESNT_EXIT);
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing STUDENT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        else if (offset == -1)
        {
            perror("Error while seeking to required student record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};
        int lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Account record!");
            return false;
        }

        readBytes = read(studentFD, &student, sizeof(struct Student));
        if (readBytes == -1)
        {
            perror("Error while reading Student record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(studentFD, F_SETLK, &lock);

        close(studentFD);

        bzero(writeBuffer, sizeof(writeBuffer));
        if (student.id != 0)
        {
        // No id, hence can close file
        
            studentFD = open("./database/student_db.txt", O_WRONLY);
            if (studentFD == -1)
            {
                perror("Error opening student file in write mode!");
                return false;
            }

            offset = lseek(studentFD, deactID * sizeof(struct Student), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the file!");
                return false;
            }

            lock.l_type = F_WRLCK;
            lock.l_start = offset;

            int lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on the student file!");
                return false;
            }
            strcpy(student.isActive, "false");
            writeBytes = write(studentFD, &student, sizeof(struct Student));
            if (writeBytes == -1)
            {
                perror("Error deleting student record!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(studentFD, F_SETLK, &lock);

            strcpy(writeBuffer, ADMIN_DEACTIVATE_STUDENT_SUCCESS);
        }
        else{
            strcpy(writeBuffer, ADMIN_DEACTIVATE_STUDENT_FAILURE);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing final DEL message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        }
        return true;
    }
    else if(activate == 1){
        
        writeBytes = write(connFD, ADMIN_ACTIVATE_STUDENT, strlen(ADMIN_ACTIVATE_STUDENT));
        if (writeBytes == -1)
        {
            perror("Error writing ADMIN_ACTIVATE_STUDENT to client!");
            return false;
            }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {   
            perror("Error reading account number response from the client!");
            return false;
        }

        int actID = atoi(readBuffer);

        int studentFD = open("./record_struct/Student.h", O_RDONLY);
        if (studentFD == -1)
        {
            // Student record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, STUDENT_ID_DOESNT_EXIT);
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing STUDENT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }


        int offset = lseek(studentFD, actID * sizeof(struct Student), SEEK_SET);
        if (errno == EINVAL)
        {
            // Student record doesn't exist
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, STUDENT_ID_DOESNT_EXIT);
            strcat(writeBuffer, "^");
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing STUDENT_ID_DOESNT_EXIT message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        else if (offset == -1)
        {
            perror("Error while seeking to required student record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};
        int lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Account record!");
            return false;
        }

        readBytes = read(studentFD, &student, sizeof(struct Student));
        if (readBytes == -1)
        {
            perror("Error while reading Student record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(studentFD, F_SETLK, &lock);

        close(studentFD);

        bzero(writeBuffer, sizeof(writeBuffer));
        if (student.id != 0)
        {
            // No id, hence can close file
        
            studentFD = open("./database/student_db.txt", O_WRONLY);
            if (studentFD == -1)
            {
                perror("Error opening student file in write mode!");
                return false;
            }

            offset = lseek(studentFD, actID * sizeof(struct Student), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the file!");
                return false;
            }

            lock.l_type = F_WRLCK;
            lock.l_start = offset;

            int    lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on the student file!");
                return false;
            }
            strcpy(student.isActive, "true");
            writeBytes = write(studentFD, &student, sizeof(struct Student));
            if (writeBytes == -1)
            {
                perror("Error activating student record!");
                return false;
            }

            lock.l_type = F_UNLCK;
            fcntl(studentFD, F_SETLK, &lock);

            strcpy(writeBuffer, ADMIN_ACTIVATE_STUDENT_SUCCESS);
        }
        else{
            strcpy(writeBuffer, ADMIN_ACTIVATE_STUDENT_FAILURE);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing final ACTIVE message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            }
            return true;
        }
}

bool update_faculty_details(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Faculty faculty;

    int facultyID;

    off_t offset;
    int lockingStatus;

    writeBytes = write(connFD, ADMIN_UP_FACULTY_ID, strlen(ADMIN_UP_FACULTY_ID));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_UP_FACULTY_ID message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while reading faculty ID from client!");
        return false;
    }

    facultyID = atoi(readBuffer);

    int facultyFD = open("./database/faculty_db.txt", O_RDONLY);
    if (facultyFD == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, FACULTY_ID_DOESNT_EXIST);
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing FACULTY_DOESNT_EXIST message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    
    offset = lseek(facultyFD, facultyID * sizeof(struct Faculty), SEEK_SET);
    if (errno == EINVAL)
    {
        // Faculty record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, FACULTY_ID_DOESNT_EXIST);
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing FACULTY_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required faculty record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};

    // Lock the record to be read
    lockingStatus = fcntl(facultyFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on faculty record!");
        return false;
    }

    readBytes = read(facultyFD, &faculty, sizeof(struct Faculty));
    if (readBytes == -1)
    {
        perror("Error while reading faculty record from the file!");
        return false;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(facultyFD, F_SETLK, &lock);

    close(facultyFD);

    writeBytes = write(connFD, ADMIN_UP_FACULTY_MENU, strlen(ADMIN_UP_FACULTY_MENU));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_UP_FACULTY_MENU message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while getting faculty modification menu choice from client!");
        return false;
    }

    int choice = atoi(readBuffer);
    if (choice == 0)
    { // A non-numeric string was passed to atoi
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    switch (choice)
    {
    case 1:
        writeBytes = write(connFD, ADMIN_UP_FACULTY_NEW_NAME, strlen(ADMIN_UP_FACULTY_NEW_NAME));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_FACULTY_NEW_NAME message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new name from client!");
            return false;
        }
        strcpy(faculty.name, readBuffer);
        break;
    case 2:
        writeBytes = write(connFD, ADMIN_UP_FACULTY_NEW_AGE, strlen(ADMIN_UP_FACULTY_NEW_AGE));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_FACULTY_NEW_AGE message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new age from client!");
            return false;
        }
        int updatedAge = atoi(readBuffer);
        if (updatedAge == 0)
        {
            // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        faculty.age = updatedAge;
        break;
    case 3:
        writeBytes = write(connFD, ADMIN_UP_FACULTY_COURSE_OLD_ID, strlen(ADMIN_UP_FACULTY_COURSE_OLD_ID));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_FACULTY_COURSE_ID message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        int oldID = atoi(readBuffer);
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's old course id from client!");
            return false;
        }
        bzero(writeBuffer,sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        writeBytes = write(connFD, ADMIN_UP_FACULTY_COURSE_ID, strlen(ADMIN_UP_FACULTY_COURSE_ID));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_FACULTY_COURSE_ID message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new course id from client!");
            return false;
        }
        for(int i = 0; i < faculty.numCoursesf; i++){
            if(faculty.courseID[i] == oldID){
                faculty.courseID[i] = atoi(readBuffer);
            }
        }
        break;
    default:
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, INVALID_MENU_CHOICE);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing INVALID_MENU_CHOICE message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    facultyFD = open("./database/faculty_db.txt", O_WRONLY);
    if (facultyFD == -1)
    {
        perror("Error while opening faculty file");
        return false;
    }
    offset = lseek(facultyFD, facultyID * sizeof(struct Faculty), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required faculty record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(facultyFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on faculty record!");
        return false;
    }

    writeBytes = write(facultyFD, &faculty, sizeof(struct Faculty));
    if (writeBytes == -1)
    {
        perror("Error while writing update faculty info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(facultyFD, F_SETLKW, &lock);

    close(facultyFD);

    writeBytes = write(connFD, ADMIN_UP_FACULTY_SUCCESS, strlen(ADMIN_UP_FACULTY_SUCCESS));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_UP_FACULTY_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}

bool update_student_details(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];

    struct Student student;

    int studentID;

    off_t offset;
    int lockingStatus;

    writeBytes = write(connFD, ADMIN_UP_STUDENT_ID, strlen(ADMIN_UP_STUDENT_ID));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_UP_STUDENT_ID message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while reading student ID from client!");
        return false;
    }

    studentID = atoi(readBuffer);

    int studentFD = open("./database/student_db.txt", O_RDONLY);
    if (studentFD == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, STUDENT_ID_DOESNT_EXIST);
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing STUDENT_DOESNT_EXIST message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    
    offset = lseek(studentFD, studentID * sizeof(struct Student), SEEK_SET);
    if (errno == EINVAL)
    {
        // Student record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, STUDENT_ID_DOESNT_EXIST);
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing STUDENT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required student record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};

    // Lock the record to be read
    lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on student record!");
        return false;
    }

    readBytes = read(studentFD, &student, sizeof(struct Student));
    if (readBytes == -1)
    {
        perror("Error while reading student record from the file!");
        return false;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(studentFD, F_SETLK, &lock);

    close(studentFD);

    writeBytes = write(connFD, ADMIN_UP_STUDENT_MENU, strlen(ADMIN_UP_STUDENT_MENU));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_UP_STUDENT_MENU message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while getting student modification menu choice from client!");
        return false;
    }

    int choice = atoi(readBuffer);
    if (choice == 0)
    { // A non-numeric string was passed to atoi
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    switch (choice)
    {
    case 1:
        writeBytes = write(connFD, ADMIN_UP_STUDENT_NEW_NAME, strlen(ADMIN_UP_STUDENT_NEW_NAME));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_STUDENT_NEW_NAME message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new name from client!");
            return false;
        }
        strcpy(student.name, readBuffer);
        break;
    case 2:
        writeBytes = write(connFD, ADMIN_UP_STUDENT_NEW_AGE, strlen(ADMIN_UP_STUDENT_NEW_AGE));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_STUDENT_NEW_AGE message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new age from client!");
            return false;
        }
        int updatedAge = atoi(readBuffer);
        if (updatedAge == 0)
        {
            // Either client has sent age as 0 (which is invalid) or has entered a non-numeric string
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
                return false;
            }
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
            return false;
        }
        student.age = updatedAge;
        break;
    case 3:
        writeBytes = write(connFD, ADMIN_UP_STUDENT_COURSE_NO, strlen(ADMIN_UP_STUDENT_COURSE_NO));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_STUDENT_COURSE_ID message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new course id from client!");
            return false;
        }
        student.numCourses = readBuffer[0];
        break;
    case 4:
        writeBytes = write(connFD, ADMIN_UP_STUDENT_COURSE_ID, strlen(ADMIN_UP_STUDENT_COURSE_ID));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_UP_STUDENT_COURSE_NO message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new course id from client!");
            return false;
        }

        student.courseID[student.numCourses] = atoi(readBuffer);
        break;
    default:
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, INVALID_MENU_CHOICE);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing INVALID_MENU_CHOICE message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

    studentFD = open("./database/student_db.txt", O_WRONLY);
    if (studentFD == -1)
    {
        perror("Error while opening student file");
        return false;
    }
    offset = lseek(studentFD, studentID * sizeof(struct Student), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required student record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(studentFD, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on student record!");
        return false;
    }

    writeBytes = write(studentFD, &student, sizeof(struct Student));
    if (writeBytes == -1)
    {
        perror("Error while writing update student info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(studentFD, F_SETLKW, &lock);

    close(studentFD);

    writeBytes = write(connFD, ADMIN_UP_STUDENT_SUCCESS, strlen(ADMIN_UP_STUDENT_SUCCESS));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_UP_STUDENT_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

    return true;
}


#endif