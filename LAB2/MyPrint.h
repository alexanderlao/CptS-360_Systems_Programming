#ifndef MY_PRINT_H
#define MY_PRINT_H

#include <stdio.h>
#include <stdlib.h>

void printc (char c);
void prints (char* s);
int rpu (unsigned int u, int BASE);
int printu (unsigned int u);
void printi (int d);
void printo (unsigned int d);
void printh (unsigned int x);
void myprintf (char* fmt, ...);

#endif
