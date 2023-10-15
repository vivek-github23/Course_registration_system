#ifndef FACULTY_RECORD
#define FACULTY_RECORD

struct Faculty
{
    int id;
    char name[25];
    char gender;
    int age;
    char login[32];
    int numCoursesf;
    char password[32];
    int courseID[6];
};
#endif