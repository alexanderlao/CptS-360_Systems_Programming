#ifndef FUNCTION_H
#define FUNCTION_H

#include "type.h"

void init();
void mount_root(char rootDevice[64]);
void ls();
void cd();
void mystat();
void mypwd(MINODE * mip);
void printdir(MINODE * mip);
int findCmd(char * command);
void mypwdHelper(MINODE * mip);
void mymkdir (char * path);
void kmkdir (MINODE *pmip, char* newBasename);
void enter_name (MINODE *pmip, int ino, char* newBasename);
int search (MINODE * mip, char *searchName);
void decFreeBlocks(int dev);
void myrmdir (char * pathname);
void rm_child(MINODE * pmip, char * my_name);
kcreat (MINODE * pmip, char* newBasename);
void mycreat (char* path);
void link(char * old, char * newone);
void unlink(char * path);
void mysymlink(char * old, char * newone);
char * readlink(char * path);
void mychmod (char* path, char* permission);
void mytouch (char* path);
OFT * myopen(char * path, int * param);
int falloc(OFT * oftp);
void pfd();
void pfdprint(MINODE * mip);
void close(OFT * fdindex);
int mywrite(OFT * fdcurrent, char * param);
int myread(OFT * fdcurrent, int length);
void cat(char * path);
void mylseek (int fd, int seekTo);
int quit();

#endif
