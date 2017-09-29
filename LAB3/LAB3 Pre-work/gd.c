// Alexander Lao
// 11481444
// CptS 360
// Lab 3 Pre-Work

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE 1024
char buf[BLKSIZE];
int fd;

int get_block(int fd, int blk, char buf[ ])
{
   lseek(fd, (long)blk*BLKSIZE, 0);
   read(fd, buf, BLKSIZE);
}

void gd ()
{
   // read the gd block
   get_block (fd, 2, buf);
   gp = (GD *)buf;

   // print out the contents
   // of the gd block
   printf ("b_map block number: %d\n", gp -> bg_block_bitmap);
   printf ("i_map block number: %d\n", gp -> bg_inode_bitmap);
   printf ("i_node's beginning block number: %d\n", gp -> bg_inode_table);
   printf ("free blocks count: %d\n", gp -> bg_free_blocks_count);
   printf ("free inodes count: %d\n", gp -> bg_free_inodes_count);
   printf ("used dirs count: %d\n", gp -> bg_used_dirs_count);
}

char *disk = "mydisk";

main(int argc, char *argv[ ])

{ 
  if (argc > 1)
    disk = argv[1];
  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open failed\n");
    exit(1);
  }

  gd ();
}
