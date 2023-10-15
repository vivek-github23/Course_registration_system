#ifndef STUDENT_RECORD
#define STUDENT_RECORD

struct Student
{
    int id;
    char isActive[8];
    char name[25];
    char gender[2];
    int age;
    int numCourses;
    int courseID[6];
    char login[32];
    char password[32];
   
};
#endif