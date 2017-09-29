#ifndef FUNCTION_H
#define FUNCTION_H

#include "type.h"

void init();
void mount_root(char rootDevice[64]);
void ls();
void cd();
void mystat();
void mypwd();
void printdir(MINODE * mip);
int findCmd(char * command);
void mypwdHelper(MINODE * mip);
void mymkdir (char * path);
void kmkdir (MINODE *pmip, char* newBasename);
void enter_name (MINODE *pmip, int ino, char* newBasename);
int search (MINODE * mip, char *searchName);
void decFreeBlocks(int dev);
void myrmdir (char * path);
void rm_child(MINODE * pmip, char * my_name);
void kcreat (MINODE * pmip, char* newBasename);
void mycreat (char* path);
void link(char * old, char * newone);
void unlink(char * path);
void mysymlink(char * old, char * newone);
char * readlink(char * path);
OFT * myopen(char * path, int * param);
int falloc(OFT * oftp);
void pfd();
void pfdprint(MINODE * mip);
void close(OFT * fdindex);
int mywrite(OFT * fdcurrent, char * param);
int * myread(OFT * fdcurrent, int length);
char * cat(char * path);
void mychmod (char* path, char* permission);
void mytouch (char* path);
void cp (char * path,char * des);
void mylseek (OFT * fdNo, int seekTo);
void mv(char * path, char * param);
void myrm (char * pathname);
int quit();

#endif
