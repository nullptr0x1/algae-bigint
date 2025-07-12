#ifndef TEST_H_H
#define TEST_H_H
void test1();void test2();
#endif // TEST_H_H
#ifndef TEST_H2_H
#define TEST_H2_H
void test3() {}
#endif // TEST_H_H
void test2() {}
void test1() {}
#ifndef TEST_H3_H
#define TEST_H3_H
#endif
#include <iostream>
#include <ctime>
int A1; int A2; int A3; int A4;  int A5;
#define A6
#define A7 \
    A8\
    A9\

int main() {const char* S = "text";const char* RS = R"--(
    
)--"; int a10; const char* RS2 = R"(1234)",* RS3 = R"(5678)";if (a10)a10 = 0;else a10 = 1;}