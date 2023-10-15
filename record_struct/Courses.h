#ifndef COURSE_RECORD
#define COURSE_RECORD

struct Courses
{
    int id;
    bool isActive;
    int facultyID;
    int studentsEnrolled;
    int studentID[30];
};
#endif