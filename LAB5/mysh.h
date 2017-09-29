#ifndef MY_SH_H
#define MY_SH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX 256

char *cwd[128];

void runSH (int argc, char** pathDirs, char* env[]);
char** parseInput (char* input);
char** parseDirs (char* input);
void shcd(char **inputArray);

#endif
