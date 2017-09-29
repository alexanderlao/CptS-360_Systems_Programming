#include "showblock.h"

int get_block(int fd, int blk, char buf[ ])
{
   lseek(fd,(long)blk*BLKSIZE, 0);
   read(fd, buf, BLKSIZE);
}

// mailman's algorithm
// 8 for number of bits in a byte
int tst_bit(char *buf, int bit)
{
   int i, j;
   i = bit / 8;  j = bit % 8;
   return (buf[i] & (1 << j));
}

int splitString (char splitThis[])
{
   char* split;
   int i = 0;

   split = strtok (splitThis, "/");
   i++;

   while (split != NULL)
   {
	printf ("%s\n", split);
	split = strtok (NULL, "/");
	i++;
   }

   return i;
}

int search (char* searchName)
{
   char buf[BLKSIZE], copy[BLKSIZE];

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
	// copy the name of the inode to a local char arr
	// manually add the null terminating character to the end
	strncpy (copy, dp -> name, dp -> rec_len);
	copy[sizeof (copy)] = '\0';

	// looking for the directory name
	if (strcmp (dp -> name, searchName) == 0)
	{
		printf ("found ino = %d\n", dp -> inode);
		return dp -> inode;
	}

	// iterate to the next entry
	cp += dp -> rec_len;
	dp = (DIR *)cp;
   }
}

void displayInfo (int inode)
{
  char buf[BLKSIZE], ibuf[BLKSIZE], dbuf[BLKSIZE];
  int i, blk, offset;
  u32 *up, *fun;

  blk = iblock + (inode - 1) / 8;
  offset = (inode - 1) % 8;

  printf ("iblock = %d, blk = %d, offset = %d\n", iblock, blk, offset);

  get_block (fd, blk, buf);
  ip = (INODE*)buf + offset;

  printf ("ip -> link count = %d\n", ip -> i_links_count);

  // print the direct blocks
  for (i = 1; i < 12; i++)
  {
	printf ("direct blocks: %d\n", ip -> i_block[i]);
  }

  // check for indirect blocks
  if (ip -> i_block[12])
  {
	printf ("indirect: ");
	get_block(fd, ip->i_block[12], ibuf);
        up = (u32*)ibuf;

        while(*up)
	{
           printf("%d ", *up);
           up++;
        }
	printf ("\n");
  }

  // check for double indirect blocks
  if (ip -> i_block[13])
  {
	printf ("double indirect: ");
	get_block(fd, ip->i_block[13], ibuf);
        fun = (u32*)dbuf;

        while(*fun)
	{
           printf("%d ", *fun);
           fun++;
        }
  }
}
