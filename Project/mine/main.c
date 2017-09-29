#include "type.h"

MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;

int dev;
int nblocks, ninodes, bmap, imap, inode_start, rootblock;
char names[64][128], *name[64];
char *cp, *disk = "mydisk", line[256], cmd[32], pathname[256], parameter[256];

init ()
{
	// declare the process
	// and minode
	PROC *p;
	MINODE *mip;
	int i, j;

	printf ("init ()\n");

	// initialize the values in the minode array
	for (i = 0; i < NMINODE; i++)
	{
		// set the local minode to the
		// address of the global minode
		// at the current index
		mip = &minode[i];

		mip -> dev = 0;
		mip -> ino = 0;
		mip -> refCount = 0;
		mip -> dirty = 0;
		mip -> mounted = 0;
		mip -> mptr = 0;
	}

	// initialize the values in the proc array
	for (i = 0; i < NPROC; i++)
	{
		// set the local proc to the
		// address of the global proc
		// at the current index
		p = &proc[i];

		p -> pid = i;
		p -> uid = 0;
		p -> cwd = 0;
		p -> status = FREE;

		// loop through the proc's fd and set them to 0
		for (j = 0; j < NFD; j++)
		{
			p -> fd[j] = 0;
		}
	}
	
	// set the root to 0
	root = 0;
}

int get_block (int fd, int blk, char buf[ ])
{
	lseek (fd, (long)blk*BLKSIZE, 0);
	read (fd, buf, BLKSIZE);
}

int put_block (int fd, int blk, char buf[ ])
{
	lseek (fd, (long)blk*BLKSIZE, 0);
	write (fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8;  j = bit % 8;
	return (buf[i] & (1 << j));
}

int clr_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] &= ~(1 << j);
	return 0;
}

int set_bit(char *buf, int bit)
{
	int i, j;
	i = bit / 8;
	j = bit % 8;
	buf[i] |= (1 << j);
	return 0;
}


int incFreeInodes(int dev)
{
	char buf[BLKSIZE];

	// inc free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count++;
	put_block(dev, 2, buf);
}

int decFreeInodes(int dev)
{
  	char buf[BLKSIZE];

	// dec free inodes count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
	char buf[BLKSIZE];

	// inc free block count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count++;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count++;
	put_block(dev, 2, buf);
}

int decFreeBlocks(int dev)
{
	char buf[BLKSIZE];

	// inc free block count in SUPER and GD
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(dev, 1, buf);

	get_block(dev, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, 2, buf);
}

u32 ialloc(int dev)
{
	int i;
	char buf[BLKSIZE];

	// get inode Bitmap into buf
	get_block(dev, imap, buf);

	for (i=0; i < ninodes; i++)
	{
		if (tst_bit(buf, i)==0)
		{
			set_bit(buf, i);
			put_block(dev, imap, buf);

			// update free inode count in SUPER and GD
			decFreeInodes(dev);

			printf("ialloc: ino = %d\n", i+1);
			return (i+1);
		}
	}
	
	return 0;
}

idealloc(int dev, int ino)
{
	int i;  
	char buf[BLKSIZE];

	if (ino > ninodes)
	{
		printf("inumber %d out of range\n", ino);
		return;
	}

	// get inode bitmap block
	get_block(dev, bmap, buf);
	clr_bit(buf, ino-1);

	// write buf back
	put_block(dev, imap, buf);

	// update free inode count in SUPER and GD
	incFreeInodes(dev);
}

u32 balloc(int dev)
{
	int i;
	char buf[BLKSIZE];

	// get inode bitmap block
	get_block (dev, bmap, buf);

	// loop through the inodes
	for (i = 0; i < ninodes; i++)
	{
		// look for the correct block
		if (tst_bit (buf, i) == 0)
		{
			set_bit(buf, i);
			put_block(dev, bmap, buf);

			printf("balloc: ino = %d\n", i + 1);
			return (i + 1);
		}
	}
	
	return 0;
}

int bdealloc(int dev, int bit)
{
	char buf[BLKSIZE];
	
	// get inode bitmap block into buf
	get_block (dev, bmap, buf);
	clr_bit (buf, bit - 1);

	// write buf back
	put_block(dev, imap, buf);
}

// determine the size of a char**
int arrSize (char** arr)
{
	int i = 0;
	
	while (arr[i]) i++;

	return i;
}

// prints the names in the names array
// used in pwd
void printNames (int size)
{
	for (size - 1; size >= 0; size--)
	{
		printf ("%s", names[size]);
	}

	printf ("\n");

	// reset the names array
	memset (names, 0, sizeof (names[0][0]) * 64 * 128);
}

char** parseInput(char *input)
 {
	int count = 0;
	char *str = NULL;
	char copy[256];
	char *tmpStr = NULL;
	char** inputArr = (char**)malloc (sizeof (char*) * 64);

	if (strcmp (input, "/") == 0)
	{
		inputArr[0] = "/";
		return inputArr;
	}
	
	// strtok doesn't like char* so i have to make it a char[]
	strncpy (copy, input, sizeof (copy) - 1);

	str = strtok (copy, "/");		
	
	// while strtok returned a valid string
	while (str)
	{
		// dynamically allocate memory for tmpStr
		// based on the length of the tokenized string
		tmpStr = (char*)malloc (sizeof (char) * strlen (str));
		strcpy (tmpStr, str);	
		
		inputArr[count] = tmpStr;

		count++;
		str = strtok(NULL, "/");
	}
	
	inputArr[count] = NULL;
	
	return inputArr;
}

int print_dir_entries (MINODE *mip, char *name)
{
	int i, ino;
	char *cp, sbuf[BLKSIZE];
	DIR *dp;
	INODE *ip;

	ip = &(mip->INODE);

	// ASSUME DIRs only has 12 direct blocks
	for (i = 0; i < 12; i++)
	{  
		if (ip -> i_block[i] == 0)
		  return 0;

		get_block (dev, ip -> i_block[i], sbuf);

		dp = (DIR *)sbuf;
		cp = sbuf;

		while (cp < sbuf + BLKSIZE)
		{
			printf ("inode = %d, rec_len = %d, name_len = %d, name = %s\n", 
			  	dp -> inode, dp -> rec_len, dp -> name_len, dp -> name);

			ino = search (ip, name);

			if (ino != 0) return ino;

			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}
	
	return 0;
}

int search (INODE *inodeptr, char *searchName)
{
	char buf[BLKSIZE];
	int i;
	
	// loop through the direct blocks
	for (i = 0; i < 12; i++)
	{
		if (inodeptr -> i_block[i] == 0)
		{
			return 0;
		}	
		
		get_block (dev, inodeptr -> i_block[i], buf);

		// have dp and cp both
		// both point at buf
		dp = (DIR *)buf;
		cp = (char *)buf;
		
		while (cp < buf + BLKSIZE)
		{	
			// looking for the directory name
			if (strcmp (dp -> name, searchName) == 0)
			{
				printf ("Found the correct inode!\n");
				return dp -> inode;
			}
			
			// iterate to the next entry
			cp += dp -> rec_len;
			dp = (DIR *)cp;
		}
	}
	
	// return 0 if we didn't find the ino
	return 0;
}

int getino (int *dev, char *pathname)
{
	char** paths = parseInput (pathname);
	char buf[BLKSIZE];
	int size = arrSize (paths);
	int i, inodeindex, blk;
	
	// start at the root
	INODE *cwd = (INODE*)malloc (sizeof (INODE));
	memcpy (cwd, &root -> INODE, sizeof (INODE));

	printf ("n = %d\n", size);
	for (i = 0; i < size; i++)
	{
		printf ("paths[%d] = %s\n", i, paths[i]);
		inodeindex = search (cwd, paths[i]);

		// if searching for /, inodeindex = 0

		if (inodeindex == 0 && paths[i] == "/") return 2;
		
		blk = ((inodeindex - 1) / 8 + gp -> bg_inode_table * BLKSIZE +
		       (inodeindex - 1) % 8 * 128);

		get_block (dev, blk, buf);
	}
	
	return inodeindex;
}

MINODE* iget (int dev, int ino)
{
	int i, offset, blk;
	MINODE *mip;
	char buf[BLKSIZE];

	// loop through the minode array
	for (i = 0; i < NMINODE; i++)
	{
		// search for an entry with its refCount > 1
		// and matching dev and ino
		if ((minode[i].refCount > 0) && (minode[i].dev == dev) &&
		    (minode[i].ino == ino))
		{
			// increment the refCount
			minode[i].refCount++;
			
			// return the address of the minode
			return &minode[i];
		}
	}

	// didn't find a matching minode
	// so look for one whose refCount is 0
	for (i = 0; i < NMINODE; i++)
	{
		if (minode[i].refCount == 0)
		{
			// set the local mip to the address
			// of the minode in the array
			mip = &minode[i];
			
			// use mailmain's algorithm
			blk = (ino - 1) / 8 + inode_start;
			offset = (ino - 1) % 8;

			// read blk into buf[]
			get_block (dev, blk, buf);
			ip = (INODE *)buf + offset;
			
			// copy ip into the minode's INODE
			mip -> INODE = *ip;
		
			// initialize all other fields of mip
			mip -> dev = dev;
			mip -> ino = ino;
			mip -> refCount = 1;
			mip -> dirty = 0;
			mip -> mounted = 0;
			mip -> mptr = 0;
			
			return mip;
		}
	}	
}

int iput (MINODE *mip)
{
	int offset, blk;
	char buf[BLKSIZE];

	// decrement the refCount
	mip -> refCount--;
	
	// if the refCount is still > 0 
	// after the decrement, return
	if (mip -> refCount > 0)
	{
		return;
	}
	// if the minode's dirty status is 0,
	// there is no need to write back so return
	else if (mip -> dirty == 0)
	{
		return;
	}
	else if ((mip -> refCount > 0) && (mip -> dirty == 1))
	{
		// have to write back
		// use mailman's algorithm to determine
		// the disk block and inode's offset in that block
		blk = (mip -> ino - 1) / 8 + inode_start;
		offset = (mip -> ino - 1) % 8;
		
		// read blk into buf[]
		get_block (dev, blk, buf);
		ip = (INODE *)buf + offset;
		
		// copy ip into the minode's INODE
		*ip = mip -> INODE;
		
		// write the block back to the disk
		put_block (dev, blk, buf);
	}
}

mount_root ()
{
	int offset, blk;
	char buf[BLKSIZE];

	printf ("mount_root ()\n");
	
	// open the dev for reading/writing
	dev = open (disk, O_RDWR);
	
	// check that the dev was opened correctly
	if (dev < 0)
	{
		printf ("Cannot open %s!\n", disk);
		exit (1);
	}

	// read the super block to verify ext2 fs
	get_block (dev, 1, buf);
	sp = (SUPER*)buf;

	// check the magic number
	printf ("super block's magic number = %x\n", sp -> s_magic);

	if (sp -> s_magic !=  0xEF53)
	{
		// magic number doesn't match
		printf ("%s is not a valid EXT2 file system!\n", disk);
		exit (1);
	}

	ninodes = sp -> s_inodes_count;
	nblocks = sp -> s_blocks_count;
	
	// read the GD block
	get_block (dev, 2, buf);
	gp = (GD*)buf;

	bmap = gp -> bg_block_bitmap;
	imap = gp -> bg_inode_bitmap;
	inode_start = gp -> bg_inode_table;
	printf ("bmap = %d imap = %d inode_start = %d\n", bmap, imap, inode_start);

	root = iget (dev, 2);
	proc[0].cwd = iget (dev, 2);
	proc[1].cwd = iget (dev, 2);

	printf ("root refCount = %d\n", root -> refCount);
	printf ("creating p[0] as the running process\n");

	// set running to proc[0]
	running = &proc[0];
	running -> status = READY;
	running -> cwd = iget (dev, 2);
	printf ("root refCount = %d\n", root -> refCount);
}

ls (char* pathname)
{
	int ino, dev = running -> cwd -> dev;
	MINODE *mip = running -> cwd;
	char buf[BLKSIZE];

	// check if a pathname was passed in
	if (!pathname[0] == '\0')
	{
		if (pathname[0] == '/')
		{
			dev = root -> dev;
		}
		
		ino = getino (&dev, pathname);
		mip = iget (dev, ino);
	}

	// mip points at minode
	// each data block of mip -> INODE contains dir entries
	// print the name strings of the DIR entries
	rootblock = mip -> INODE.i_block[0];
	get_block (dev, rootblock, buf);

	// have dp and cp both
	// both point at buf
	dp = (DIR *)buf;
	cp = (char *)buf;	

	while (cp < buf + BLKSIZE)
	{
		// print out the directories' info
		printf ("%d %s\n", dp -> rec_len, dp -> name);

		// iterate to the next entry
		cp += dp -> rec_len;
		dp = (DIR *)cp;
	}
}

cd (char *pathname)
{
	int ino;
	MINODE *mip, *old = running -> cwd;
	
	// if a pathname was passed in
	if (!pathname[0] == '\0')
	{
		printf ("cd to %s...\n", pathname);

		if (pathname[0] == '/')
		{
			dev = root -> dev;
		}
		
		ino = getino (&dev, pathname);
		mip = iget (dev, ino);

		// verify that the inode is a dir
		if (!S_ISDIR(mip -> INODE.i_mode))
		{
			// put the minode back to the disk
			iput (mip);

			printf ("%s is not a directory!\n", pathname);
			return 1;
		}
		
		running -> cwd = mip;
		iput (old);

		printf ("Now in ino = %d\n", running -> cwd -> ino);

		return 0;
	}
	// cd to the root
	else
	{
		printf ("no pathname was passed in so cd to the root...\n");
		running -> cwd = root;
		iput (old);

		printf ("Now in ino = %d\n", running -> cwd -> ino);

		return 0;
	}
}

pwd (MINODE *mip, int* count)
{
	// base case is once we hit the root
	if (mip == root)
	{	
		strcpy (names[(*count)++], "/\0");
		return;
	}	

	char buf[BLKSIZE], curName[256];
	int pIno;
	MINODE *parent;
	
	printf ("mip -> ino = %d\n", mip -> ino);
	printf ("rootIno = %d\n", root -> ino);
	
	// read the block into buf
	get_block (dev, mip -> INODE.i_block[0], buf);

	// have dp and cp both
	// both point at buf
	dp = (DIR *)buf;
	cp = (char *)buf + dp -> rec_len;

	// need dp to skip the . directory
	// and go straight to the parent directory ..
	dp = (DIR *)cp;

	// should be ..
	printf ("dp -> name = %s\n", dp -> name);
	
	// get the parent's inumber
	pIno = dp -> inode;

	// get the parent's minode
	parent = iget (dev, pIno);

	// read the parent's block into buf
	get_block (dev, parent -> INODE.i_block[0], buf);

	// have dp and cp both
	// both point at buf
	dp = (DIR *)buf;
	cp = (char *)buf;

	// search the parent for the current dir
	while (cp < buf + BLKSIZE)
	{
		// found the child in the parent
		if (dp -> inode == mip -> ino)
		{
			// copy the child's name to
			// the local variable curName
			strncpy (curName, dp -> name, dp -> name_len);
			curName[dp -> name_len] = '\0';
			strcpy (names[(*count)++], curName);
		}	
		
		// iterate to the next entry
		cp += dp -> rec_len;
		dp = (DIR *)cp;
	}
	
	// make the recursive call on the parent
	pwd (parent, count);
	
	return;
}

void mystat()
{
	// Set up variables.
	int ino;
	MINODE * mip = malloc(sizeof(MINODE));
	// If there is no pathname, print error.
	if(!pathname[0])
	{
		printf("No pathname!\n");
	}
	// Otherwise,
	else
	{	
		// Use buf to store data.
		char buf[1024];
		// Read data into buf.
		get_block(running->cwd->dev,running->cwd->INODE.i_block[0], buf);
		// Set up dp base on buf.
		dp = (DIR*)buf;
		// Print out dp name, which is the folder and file name.
		while ((dp->inode != 0) && (((char*)dp - buf) < 1024))
		{	
			// Use this for transversing.
			char * cpt = (char*)dp;
        		cpt += dp->rec_len;
			// Look for the file and print stat.
			if(strcmp(dp->name,pathname) == 0)
			{
				int temp = dp->inode;
				mip = iget(running->cwd->dev,dp->inode);
				printf("****************STAT****************\n");
				printf("dev=%d  ino=%d  mod=%4x\n",running->cwd->dev,dp->inode,mip->INODE.i_mode);
				printf("uid=%d  gid=%d  nlink=%d\n",mip->INODE.i_uid,mip->INODE.i_gid,mip->INODE.i_links_count);
				printf("size=%d time=%s",mip->INODE.i_size,ctime(&(mip->INODE.i_ctime)));
				printf("************************************\n");
				break;
			}
			// Move onto the next dp.
        		dp = (DIR*)cpt;
		}
	}
}

void mymkdir (char* pathname)
{
	char * dirName;
	char * baseName;
	int pino;
	MINODE * pmip;
	int ino;
	printf("\t\t\t\t\t****INSIDE MKDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	if (pathname[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}

	if(pathname[0] == '/')
	{
		printf("\t\t\t\t\tLINE 2 If\n");
		dev = root->dev;
	}
	else
	{
		printf("\t\t\t\t\tLINE 2 Else\n");
		dev = running->cwd->dev;
	}
	
	printf("\t\t\t\t\tLINE 3\n");
	dirName = dirname(pathname);
	printf("\t\t\t\t\tLINE 4\n");
	baseName = basename(pathname);
	printf("\t\t\t\t\tLINE 5\n");
	
	printf("\t\t\t\t\tLINE 6\n");
	pmip = iget(running->cwd->dev,running->cwd->ino);
	
	printf("\t\t\t\t\tLINE 8\n");
	printf ("dirname = %s basename = %s\n", dirName, baseName);

	if(!S_ISDIR(pmip->INODE.i_mode))
	{
		printf("\t\t\t\t\tLINE 9\n");
		printf("%s is not a directory.\n", dirName);
		iput(pmip);
		return;
	}
	else if(search(running->cwd,baseName) != 0)
	{
		printf("\t\t\t\t\tLINE 10\n");
		printf("%s already exists.\n", baseName);
		iput(pmip);
		return;
	}
	printf("\t\t\t\t\tLINE 11\n");
	kmkdir(pmip,baseName);
	printf("\t\t\t\t\t****DONE MKDIR****\n");
}

kmkdir (MINODE * pmip, char* newBasename)
{
	char * cp;
	int ino, bno, i;
	MINODE *mip;
	char buf[BLKSIZE];

	printf("\t\t\t\t\t****INSIDE KMKDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	ino = ialloc (dev);
	printf("\t\t\t\t\tLINE 2\n");
	bno = balloc (dev);
	printf("\t\t\t\t\tLINE 3\n");
	mip = iget (dev, ino);

	printf("\t\t\t\t\tLINE 4\n");			
	mip -> INODE.i_mode = 0x41ED;		
	mip -> INODE.i_uid = running->uid;	
	mip -> INODE.i_gid = running->gid;	
	mip -> INODE.i_size = 1024;		
	mip -> INODE.i_links_count = 2;		
	mip -> INODE.i_atime = time (0L);
	mip -> INODE.i_ctime = time (0L);
	mip -> INODE.i_mtime = time (0L);
	mip -> INODE.i_blocks = 2;		
	mip -> dirty = 1;
	mip -> refCount = 1;

	printf("\t\t\t\t\tLINE 5\n");
	for (i = 1; i < 15; i++)
	{
		mip -> INODE.i_block[i] = 0;
	}
	mip -> INODE.i_block[0] = bno;

	printf("\t\t\t\t\tLINE 6\n");
	iput (mip);
	printf("\t\t\t\t\tLINE 7\n");
	memset (buf, 0, BLKSIZE);

	printf("\t\t\t\t\tLINE 8\n");
	dp = (DIR*)buf;
	dp -> inode = ino;			// inode number
	strncpy (dp -> name, ".",1);		// file name
	dp -> name_len = 1;			// name length
	dp -> rec_len = 12;

	// directory entry length
	printf("\t\t\t\t\tLINE 9\n");
	cp = buf + dp->rec_len;			
	dp = (DIR*)cp;

	printf("\t\t\t\t\tLINE 10\n");
	dp -> inode = pmip -> ino;		// inode number
	strncpy (dp -> name, "..",2);		// file name
	dp -> name_len = 2;			// name length
	dp -> rec_len = BLKSIZE - 12;		// directory entry length + last entry

	printf("\t\t\t\t\tLINE 11\n");
	put_block (dev, bno, buf);
	printf("\t\t\tDevice %d PMIP %d\n",dev,pmip->dev);
	printf("\t\t\t\t\tLINE 12\n");	
	enter_name (pmip, ino, newBasename);
	printf("\t\t\t\t\tLINE 13\n");
	pmip -> INODE.i_links_count++;
	printf("\t\t\t\t\tLINE 14\n");
	pmip -> dirty = 1;
	printf("\t\t\t\t\tLINE 16\n");
	iput (pmip);
	printf("\t\t\t\t\t****DONE KMKDIR****\n");
}

enter_name (MINODE * pmip, int ino, char* newBasename)
{

	printf("\t\t\t\t\t****ENTER NAME****\n");

	int i = 0;
	int rec_length;
	char buf[BLKSIZE];
	int need_length,ideal_length;

	get_block(pmip->dev, pmip->INODE.i_block[0], buf);
	dp = (DIR *)buf;
	char * cp = buf;
	rec_length = 0;

	// step to the last entry in a data block
	while(dp->rec_len + rec_length < BLKSIZE)
	{
		rec_length += dp->rec_len;
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

	// when entering a new entry with name_len = n
	need_length = 4 * ((8 + strlen(name) + 3) / 4); // a multiple of 4
	// step to the last entry in a data block...its ideal length is...
	ideal_length = 4 *((8 + dp->name_len + 3) / 4);
	rec_length = dp->rec_len; // store rec_len for a bit easier code writing

	// check if it can enter the new entry as the last entry
	if((rec_length - ideal_length) >= need_length)
	{
		// trim the previous entry to its ideal_length
		dp->rec_len = ideal_length;
		cp+=dp->rec_len;
		dp = (DIR *)cp;
		dp->rec_len = rec_length - ideal_length;
		dp->name_len = strlen(newBasename);
		strcpy(dp->name, newBasename);
		dp->inode = ino;
		// write the new block back to the disk
		put_block(dev, pmip->INODE.i_block[i], buf);
	}
	else
	{
		// otherwise allocate a new data block 
		i++;
		int datab = balloc(pmip->dev);
		pmip->INODE.i_block[i] = datab;
		get_block(pmip->dev, datab, buf);

		// enter the new entry as the first entry in the new block
		dp = (DIR *)buf;
		dp->rec_len = BLKSIZE;
		dp->name_len = strlen(newBasename);
		strcpy(dp->name, newBasename);
		dp->inode = ino;

		pmip->INODE.i_size += BLKSIZE;
		
		// write the new block back to the disk
		put_block(dev, pmip->INODE.i_block[i], buf);
	}
	mystat();
	printf("\t\t\t\t\t****DONE ENTER NAME****\n");
}

int mycreat (char* pathname, char* type)
{
	char * dirName;
	char * baseName;
	int pino;
	MINODE * pmip;
	int ino;
	printf("\t\t\t\t\t****INSIDE MKDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	if (pathname[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}

	if(pathname[0] == '/')
	{
		printf("\t\t\t\t\tLINE 2 If\n");
		dev = root->dev;
	}
	else
	{
		printf("\t\t\t\t\tLINE 2 Else\n");
		dev = running->cwd->dev;
	}
	
	printf("\t\t\t\t\tLINE 3\n");
	dirName = dirname(pathname);
	printf("\t\t\t\t\tLINE 4\n");
	baseName = basename(pathname);
	printf("\t\t\t\t\tLINE 5\n");
	
	printf("\t\t\t\t\tLINE 6\n");
	pmip = iget(running->cwd->dev,running->cwd->ino);
	
	printf("\t\t\t\t\tLINE 8\n");
	printf ("dirname = %s basename = %s\n", dirName, baseName);

	if(!S_ISDIR(pmip->INODE.i_mode))
	{
		printf("\t\t\t\t\tLINE 9\n");
		printf("%s is not a directory.\n", dirName);
		iput(pmip);
		return;
	}
	else if(search(running->cwd,baseName) != 0)
	{
		printf("\t\t\t\t\tLINE 10\n");
		printf("%s already exists.\n", baseName);
		iput(pmip);
		return;
	}
	
	// check if we're calling creat
	if (strcmp (type, "creat") == 0)
	{
		// all checks are good so we can now make the file
		creatHelp (pmip, baseName);
	}
	// check if we're calling symlink
	else if (strcmp (type, "symlink") == 0)
	{
		symlinkHelp (pmip, baseName);
	}
	
}

creatHelp (MINODE *pmip, char* newBasename)
{
	char * cp;
	int ino, bno, i;
	MINODE *mip;
	char buf[BLKSIZE];

	printf("\t\t\t\t\t****INSIDE KMKDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	ino = ialloc (dev);
	printf("\t\t\t\t\tLINE 2\n");
	bno = balloc (dev);
	printf("\t\t\t\t\tLINE 3\n");
	mip = iget (dev, ino);

	printf("\t\t\t\t\tLINE 4\n");			
	mip -> INODE.i_mode = 0x81A4;		
	mip -> INODE.i_uid = running->uid;	
	mip -> INODE.i_gid = running->gid;	
	mip -> INODE.i_size = 0;		
	mip -> INODE.i_links_count = 2;		
	mip -> INODE.i_atime = time (0L);
	mip -> INODE.i_ctime = time (0L);
	mip -> INODE.i_mtime = time (0L);
	mip -> INODE.i_blocks = 2;		
	mip -> dirty = 1;
	mip -> refCount = 1;

	printf("\t\t\t\t\tLINE 5\n");
	for (i = 1; i < 15; i++)
	{
		mip -> INODE.i_block[i] = 0;
	}
	mip -> INODE.i_block[0] = bno;

	printf("\t\t\t\t\tLINE 6\n");
	iput (mip);
	printf("\t\t\t\t\tLINE 7\n");
	memset (buf, 0, BLKSIZE);

	printf("\t\t\t\t\tLINE 11\n");
	put_block (dev, bno, buf);
	printf("\t\t\tDevice %d PMIP %d\n",dev,pmip->dev);
	printf("\t\t\t\t\tLINE 12\n");	
	enter_name (pmip, ino, newBasename);
	printf("\t\t\t\t\tLINE 13\n");
	pmip -> INODE.i_links_count++;
	printf("\t\t\t\t\tLINE 14\n");
	pmip -> dirty = 1;
	printf("\t\t\t\t\tLINE 16\n");
	iput (pmip);
	printf("\t\t\t\t\t****DONE KMKDIR****\n");
}

// oldFile must exist, newFile must not exist
symlink (char* oldFile, char* newFile)
{
	int oino, nino;
	MINODE *omip, *nmip;
	
	// check that the old file exists
	oino = getino (&(running -> cwd -> dev), oldFile);

	if (oino == 0)
	{
		printf ("%s does not exist!\n", oldFile);
		return -1;
	}

	// check that the new file does not exist
	nino = getino (&dev, newFile);

	if (nino != 0)
	{
		printf ("%s already exists!\n", newFile);
		return -1;
	}

	// load the inode into a minode
	omip = iget (dev, oino);

	// create the newFile with SLINK type
	mycreat (newFile, "symlink");

	// load the inode into a minode
	nino = getino (&dev, newFile);
	nmip = iget (dev, nino);

	// store oldFile's name in newFile's INODE.i_block[] area
	memcpy (nmip -> INODE.i_block, oldFile, strlen (oldFile));
	
	// mark nmip as dirty
	nmip -> dirty = 1;
	iput (nmip);

	// mark omip as dirty
	omip -> dirty = 1;
	iput (omip);
}

symlinkHelp (MINODE *pmip, char* newBasename)
{
	char * cp;
	int ino, bno, i;
	MINODE *mip;
	char buf[BLKSIZE];

	printf("\t\t\t\t\t****INSIDE KMKDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	ino = ialloc (dev);
	printf("\t\t\t\t\tLINE 2\n");
	bno = balloc (dev);
	printf("\t\t\t\t\tLINE 3\n");
	mip = iget (dev, ino);

	printf("\t\t\t\t\tLINE 4\n");			
	mip -> INODE.i_mode = 0xA1A4;		
	mip -> INODE.i_uid = running->uid;	
	mip -> INODE.i_gid = running->gid;	
	mip -> INODE.i_size = 0;		
	mip -> INODE.i_links_count = 2;		
	mip -> INODE.i_atime = time (0L);
	mip -> INODE.i_ctime = time (0L);
	mip -> INODE.i_mtime = time (0L);
	mip -> INODE.i_blocks = 2;		
	mip -> dirty = 1;
	mip -> refCount = 1;

	printf("\t\t\t\t\tLINE 5\n");
	for (i = 1; i < 15; i++)
	{
		mip -> INODE.i_block[i] = 0;
	}
	mip -> INODE.i_block[0] = bno;

	printf("\t\t\t\t\tLINE 6\n");
	iput (mip);
	printf("\t\t\t\t\tLINE 7\n");
	memset (buf, 0, BLKSIZE);

	printf("\t\t\t\t\tLINE 11\n");
	put_block (dev, bno, buf);
	printf("\t\t\tDevice %d PMIP %d\n",dev,pmip->dev);
	printf("\t\t\t\t\tLINE 12\n");	
	enter_name (pmip, ino, newBasename);
	printf("\t\t\t\t\tLINE 13\n");
	pmip -> INODE.i_links_count++;
	printf("\t\t\t\t\tLINE 14\n");
	pmip -> dirty = 1;
	printf("\t\t\t\t\tLINE 16\n");
	iput (pmip);
	printf("\t\t\t\t\t****DONE KMKDIR****\n");
}

int readlink (char* file, char buffer[])
{
	int ino;
	MINODE *mip;
	
	// get file's MINODE into memory
	ino = getino (&dev, file);
	mip = iget (dev, ino);

	// verify it's a SLINK
	if (!(S_ISLNK (mip -> INODE.i_mode)))
	{
		printf ("%s is not a symbolic link!\n", file);
		return -1;
	}

	// copy the symlink's name into the buffer
	// (the target name was stored in the i_block in the symlink function)
	memcpy (buffer, mip -> INODE.i_block, strlen ((char*)mip -> INODE.i_block));

	printf ("buffer = %s\n", buffer);

	return strlen ((char*)mip -> INODE.i_block);
}

int main ()
{
	init ();
	mount_root ();

	while (1)
	{
		printf ("\ninput command: ");
		fgets (line, 128, stdin);
		
		line[strlen (line) - 1] = 0;
		
		if (line[0] == 0)
		{
			continue;
		}
		
		// reset the cmd, pathname, and parameter
		cmd[0] = 0, pathname[0] = 0, parameter[0] = 0;

		sscanf (line, "%s %s %s %s", cmd, pathname, parameter);

		printf ("cmd = %s, pathname = %s, parameter = %s\n", cmd, pathname, parameter);

		if (strcmp (cmd, "ls") == 0)
		{
			ls (pathname);
		}
		else if (strcmp (cmd, "cd") == 0)
		{
			cd (pathname);
		}
		else if (strcmp (cmd, "pwd") == 0)
		{	
			int size = 0;
			pwd (running -> cwd, &size);
			printNames (size);
		}
		else if (strcmp (cmd, "mkdir") == 0)
		{
			mymkdir (pathname);
		}
		else if (strcmp (cmd, "stat") == 0)
		{
			mystat ();
		}
		else if (strcmp (cmd, "creat") == 0)
		{
			mycreat (pathname, "creat");
		}
		else if (strcmp (cmd, "symlink") == 0)
		{
			symlink (pathname, parameter);
		}
		else if (strcmp (cmd, "readlink") == 0)
		{
			readlink (pathname, parameter);
		}
		else if (strcmp (cmd, "quit") == 0)
		{
			quit ();
		}
	}
}

int quit ()
{
	int i;
	MINODE *mip;

	// loop through each minode
	for (i = 0; i < NMINODE; i++)
	{
		// have mip point to the current minode
		mip = &minode[i];
		
		if (mip -> refCount > 0)
		{
			// release the minode
			iput (mip);
		}
	}
	
	exit (0);
}
