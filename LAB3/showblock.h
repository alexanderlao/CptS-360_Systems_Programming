#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned int   u32;

#define BLKSIZE 1024

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

int fd;
int iblock;
int rootblock;
char *cp;

int get_block(int fd, int blk, char buf[ ]);
int tst_bit(char *buf, int bit);
int splitString (char splitThis[]);
int search (char* searchName);
void displayInfo (int inode);
