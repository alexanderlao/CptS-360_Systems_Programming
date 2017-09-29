#include "utility.h"

void get_block(int fd, int blk, char buf[BLKSIZE])
{
	lseek(fd,(long)blk * BLKSIZE,0);
	read(fd,buf,BLKSIZE);
}

int put_block (int fd, int blk, char buf[BLKSIZE])
{
	lseek (fd, (long)blk*BLKSIZE, 0);
	write (fd, buf, BLKSIZE);
}

int getino(int * thisDev,char * pathname)
{
	char buf[1024];
	char * cp;
	int inumber;
	MINODE * mip;

	// absolute or local path?
	if(root = running->cwd)
	{
		inumber = root->ino;
	}
	else // cwd
	{
		inumber = running->cwd->ino;
	}
	
		
	mip = iget(*thisDev,inumber);
	inumber = search(mip,pathname);
	if(inumber == 0 || !S_ISDIR(mip->INODE.i_mode))
	{
		iput(mip); // cleanup
		return 0; // return that we failed :(
	}
	iput(mip);
	return inumber;	
}

MINODE * iget(int thisDev,int ino)
{
	int i, offset, blk;
	
	char buf[BLKSIZE];
	//printf("\t\t\t\t\t****INSIDE IGET****\n");
	//printf("\t\t\t\t\tProcessing.\n");
	//printf("\t\t\t\t\tLINE 1\n");
	// loop through the minode array
	for (i = 0; i < NMINODES; i++)
	{
		// search for an entry with its refCount > 1
		// and matching dev and ino
		if ((minode[i]->dev == thisDev) && (minode[i]->ino == ino))
		{	
			//printf("\t\t\t\t\tLINE 2\n");
			// increment the refCount
			minode[i]->refCount++;
			// return the address of the minode
			//printf("\t\t\t\t\t****DONE IGET (1)****\n");	
			return minode[i];
		}
	}
	//printf("\t\t\t\t\tLINE 3\n");
	// didn't find a matching minode
	// so look for one whose refCount is 0
	for (i = 0; i < NMINODES; i++)
	{	
		if (minode[i]->refCount == 0)
		{	
			//printf("\t\t\t\t\tLINE 4\n");
			// use mailmain's algorithm
			blk = (ino - 1) / 8 + inode_start;
			offset = (ino - 1) % 8;

			// read blk into buf[]
			get_block (thisDev, blk, buf);
			ip = (INODE *)buf + offset;
			
			// copy ip into the minode's INODE
			minode[i] -> INODE = *ip;
		
			// initialize all other fields of mip
			minode[i] -> dev = thisDev;
			minode[i] -> ino = ino;
			minode[i] -> refCount = 1;
			minode[i] -> dirty = 0;
			minode[i] -> mounted = 0;
			minode[i] -> mountptr = 0;
			//printf("\t\t\t\t\t****DONE IGET****(2)\n");	
			return minode[i];
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
		get_block (mip->dev, blk, buf);
		ip = (INODE *)buf + offset;
		
		// copy ip into the minode's INODE
		*ip = mip -> INODE;
		
		// write the block back to the disk
		put_block (mip->dev, blk, buf);
	}
}

// get the inode
int findino(MINODE * mip, int *myino, int *parentino)
{
	int i;
	char buf[BLKSIZE], namebuf[256], *cp;
	//printf("Line 1\n");

	get_block(mip->dev, mip->INODE.i_block[0], buf);
	dp = (DIR *)buf;
	cp = buf;
	*myino = dp->inode;
	cp +=dp->rec_len;
	dp = (DIR *)cp;
	*parentino = dp->inode;	    	
	return 0;
}

char * findname(MINODE *parent, int ino)
{
	int i;
	char buf[BLKSIZE], namebuf[256], *cp;
	char * name;

	for(i = 0; i < 12 ; i ++)
	{
		if(parent->INODE.i_block[i] != 0)
		{
			get_block(parent->dev, parent->INODE.i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;
			while(cp < &buf[BLKSIZE])
			{
				if(dp->inode == ino)
				{
					return dp->name;
				}
				cp +=dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	return NULL;
}


// tokenize a path into its sub-components and return the number of pieces to the path
int token_path(char *path)
{
	for(int i = 0; i < 128; i++)
	{
		//printf("%s\n",name[i]);
		name[i] = NULL;
	}
	char *temp;
	int i = 0;
	
	temp = strtok(path, "/");
	while(temp != NULL)
	{
		name[i] = temp;
		temp = strtok(NULL, "/");
		i++;
	}
	return i;
}

int ialloc(int thedev)
{
	int i;
	char buf[BLKSIZE]; 
	SUPER *temp;

	get_block(thedev,SUPERBLOCK,buf);
	temp = (SUPER *)buf;
	ninodes = temp->s_inodes_count;
	put_block(thedev,SUPERBLOCK,buf);

	get_block(thedev, imap,buf);

	for(i = 0; i < ninodes ; i++) 
	{
		if(tst_bit(buf,i) == 0)
		{
			set_bit(buf,i);
			put_block(thedev,imap,buf); 
			decFreeInodes(thedev);
			return i+1;
		}
	}

	return 0; 
}

int balloc(int thedev)
{

	int i;
	char buf[BLKSIZE]; 
	SUPER *temp;

	get_block(thedev,SUPERBLOCK,buf);
	temp = (SUPER *)buf;
	nblocks = temp->s_blocks_count;
	put_block(thedev,SUPERBLOCK,buf);

	get_block(thedev, bmap,buf);

	for(i = 0; i < nblocks ; i++) 
	{	

		if(tst_bit(buf,i) == 0)
		{
			set_bit(buf,i);
			put_block(thedev,bmap,buf);
			decFreeBlocks(thedev);
			return i+1;
		}
	}

	return 0; // no more free inodes :(
}

void idealloc(int dev, int ino)
{
	int i;  
	char buf[BLKSIZE];

	if (ino > ninodes)
	{
		//printf("inumber %d out of range\n", ino);
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

void bdealloc(int dev, int bit)
{
	char buf[BLKSIZE];
	
	// get inode bitmap block into buf
	get_block (dev, bmap, buf);
	clr_bit (buf, bit - 1);

	// write buf back
	put_block(dev, imap, buf);
}

int clr_bit(char * buf, int bit)
{
	return buf[bit / 8] &= ~(1 << (bit % 8));
}

int tst_bit(char *buf, int bit)
{
	return buf[bit / 8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit)
{
	return buf[bit / 8] |= (1 << (bit % 8));
}

void incFreeInodes(int dev)
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


int decFreeInodes(int device)
{
  	char buf[BLKSIZE];

	// dec free inodes count in SUPER and GD
	get_block(device, 1, buf);
	sp = (SUPER *)buf;
	sp->s_free_inodes_count--;
	put_block(device, 1, buf);

	get_block(device, 2, buf);
	gp = (GD *)buf;
	gp->bg_free_inodes_count--;
	put_block(device, 2, buf);
}

// decrement free blocks on device
void decFreeBlocks(int dev)
{
	char buf[BLKSIZE];
	get_block(dev, SUPERBLOCK, buf);
	sp = (SUPER *)buf;
	sp->s_free_blocks_count--;
	put_block(dev, SUPERBLOCK,buf);

	get_block(dev, GDBLOCK,buf);
	gp = (GD *)buf;
	gp->bg_free_blocks_count--;
	put_block(dev, GDBLOCK,buf);
}

