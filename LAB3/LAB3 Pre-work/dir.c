#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

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

int get_block(int fd, int blk, char buf[ ])
{
   lseek(fd,(long)blk*BLKSIZE, 0);
   read(fd, buf, BLKSIZE);
}

void dir ()
{
   char buf[BLKSIZE];

   // read GD
   get_block(fd, 2, buf);
   gp = (GD *)buf;

   iblock = gp->bg_inode_table;   // get inode start block#
   printf("inode_block=%d\n", iblock);

   // get inode start block     
   get_block(fd, iblock, buf);

   ip = (INODE *)buf + 1;         // ip points at 2nd INODE

   // access the inode's i_block[0] 
   // to access the directory
   rootblock = ip -> i_block[0];
   get_block (fd, rootblock, buf);

   // have dp and cp both
   // both point at buf
   dp = (DIR *)buf;
   cp = (char *)buf;

   while (cp < buf + BLKSIZE)
   {
	// print out the relevant information
	printf ("inode: %d\n", dp -> inode);
	printf ("rec_len: %d\n", dp -> rec_len);
	printf ("name_len: %d\n", dp -> name_len);
	printf ("name: %s\n\n", dp -> name);

	// iterate to the next entry
	cp += dp -> rec_len;
	dp = (DIR *)cp;
   }
}

char *disk = "mydisk";
main(int argc, char *argv[])
{ 
  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dir();
}
