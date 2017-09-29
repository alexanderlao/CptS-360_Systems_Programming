#ifndef UTILITY_H
#define UTILITY_H

#include "type.h"

void get_block(int fd, int blk, char buf[BLKSIZE]);
int put_block (int fd, int blk, char buf[BLKSIZE]);
int getino(int * thisDev,char * pathname);
MINODE * iget(int thisDev,int ino);
int iput (MINODE *mip);
int findino(MINODE *mip, int * myino, int * parentino);
char * findname(MINODE *parent, int ino);
int token_path(char *path);
int balloc(int thedev);
int ialloc(int thedev);
int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int decFreeInodes(int dev);
MINODE* igetmkdir (int dev, int ino);
void bdealloc(int dev, int bit);
int clr_bit(char *buf, int bit);
void idealloc(int dev, int ino);
void incFreeInodes(int dev);
int arrSize (char * path);

#endif
