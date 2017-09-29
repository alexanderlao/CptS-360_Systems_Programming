#include "function.h"
#include "utility.h"

void init()
{
	// declare the process
	// and minode
	PROC *p;
	MINODE *mip;
	int i, j;

	printf ("Initializing...\n");

	proc[0] = malloc(sizeof(PROC));
	proc[1] = malloc(sizeof(PROC));

	// initialize the values in the minode array
	for (i = 0; i < NMINODES; i++)
	{
		// set the local minode to the
		// address of the global minode
		// at the current index
		minode[i] = malloc(sizeof(MINODE));
		mip = &minode[i];
		mip -> dev = 0;
		mip -> ino = 0;
		mip -> refCount = 0;
		mip -> dirty = 0;
		mip -> mounted = 0;
		mip -> mountptr = 0;
	}

	proc[0] -> pid = 0;
	proc[0] -> uid = 0;
	proc[0] -> cwd = 0;
	proc[0] -> status = FREE;
	proc[0] -> cwd = malloc(sizeof(MINODE));
	// loop through the proc's fd and set them to 0
	for (j = 0; j < NFD; j++)
	{
		proc[0] -> fd[j] = 0;
	}

	proc[1] -> pid = 0;
	proc[1] -> uid = 1;
	proc[1] -> cwd = 0;
	proc[1] -> status = FREE;
	proc[1] -> cwd = malloc(sizeof(MINODE));
	// loop through the proc's fd and set them to 0
	for (j = 0; j < NFD; j++)
	{
		proc[0] -> fd[j] = 0;
	}
	
	// set the root to 0
	root = 0;
	printf("Initialized!\n");
}

void mount_root(char rootDevice[64])
{
	char * buf[BLKSIZE];
	int magic, nblocks, bfree, ninodes, ifree;

	if(dev < 0)
	{
		printf("Panic: Can't open root device\n");
		exit(0);
	}


	// Get the super block.
	get_block(dev,SUPERBLOCK,buf);
	sp = (SUPER*)buf;
	magic = sp->s_magic;
	nblocks = sp->s_blocks_count;
	bfree = sp->s_free_blocks_count;
	ninodes = sp->s_inodes_count;
	ifree = sp->s_free_inodes_count;

	// Check magic number.
	if(sp->s_magic != SUPER_MAGIC)
	{
		printf("super magic=%x : %s is not a valid Ext2 filesys\n",sp->s_magic, rootDevice);
     		exit(0);
	}

	// Read GD block.
	get_block(dev,GDBLOCK,buf);
	gp = (GD*)buf;
	inode_start = gp->bg_inode_table;
	root = iget(dev,ROOT_INODE);
	root->mountptr = malloc(sizeof(MOUNT));
	strcpy(root->mountptr-> name,"/");
	strcpy(root->mountptr->mount_name,rootDevice);
	root->mountptr->nblocks = nblocks;
	root->mountptr->ninodes = ninodes;
	root->mountptr->bmap = gp->bg_block_bitmap;
	root->mountptr->imap = gp->bg_inode_bitmap;
	imap = gp->bg_inode_bitmap;
	bmap = gp->bg_block_bitmap;
	backup = root;
	proc[0]->cwd = root;
	proc[1]->cwd = root;
	running = &proc[0];
	running->cwd = root;

	// Print.
	printf("mount : %s  mounted on %s\n", root->mountptr->mount_name,root->mountptr->name);
}

void ls()
{
	// List disk.
	// Set up the variables.
	int ino;
	printf("\t\t\t\t\t****INSIDE LS****\n");
	printf("\t\t\t\t\tLINE 1\n");
	MINODE * mip = running->cwd;
	printf("\t\t\t\t\tdevice: %d\n",mip->dev);
	// If the user enter a pathname.
	if(pathname[0])
	{
		// If pathname is /
		if(pathname[0] == '/')
		{	
			printf("\t\t\t\t\tLINE 2 IF\n");
			// Set the minode to root.
			mip = root;
		}
		// Otherwise, get the minode that correspond to the pathname.
		else
		{
			printf("\t\t\t\t\tLINE 2 ELSE\n");
			ino = getino(&running->cwd->dev,pathname);
			mip = iget(running->cwd->dev,ino);
		}
			
	}	
	if(S_ISDIR(mip->INODE.i_mode))
	{
		printf("\t\t\t\t\tLINE 3\n");
		// Print the minode.
		printdir(mip);
	}
	else
	{
		printf("\t\t\t\t\tLINE 4\n");
		printf("%s is not a directory!\n",pathname);
	}
	printf("\t\t\t\t\t****DONE LS****\n");
}

void cd()
{
	char buf[1024];
	int i;
	// If there is no pathname.
	if(!pathname[0] || (pathname[0] == '/' && !pathname[1]))
	{	
		// Set root as default.
		running->cwd = backup;
		//root = running->cwd;
		root->refCount++;
	}
	// Otherwise,
	else
	{
		token_path(pathname);
		for(i = 0; i < 128; i++)
		{
			if(name[i] != NULL)
			{
				
				// Set root as the corresponding minode.;
				int ino = getino(&running->cwd->dev,name[i]);
				MINODE * temp = iget(running->cwd->dev,ino);
				if(S_ISDIR(temp->INODE.i_mode))
				{
					running->cwd = temp;
					iput(temp);
					strcpy(cwdname,pathname);
				}
				else
				{
					printf("%s is not a directory!\n",name[i]);
				}
			}
		}
		for(int i = 0; i< 128; i++)
		{
			name[i] = NULL;
		}
	}
}

void mypwd(MINODE * mip)
{
	if(mip->ino == ROOT_INODE)
	{
		printf("CWD: /\n");
	}
	else
	{
		printf("CWD: ");
		mypwdHelper(mip);
		printf("\n");
	}
}

void mypwdHelper(MINODE * mip)
{
	int ino, parentino;
	char * name;
	if(mip->ino == ROOT_INODE)
	{
		return;
	}
	else
	{
		findino(mip,&ino,&parentino);
		mip = iget(mip->dev,parentino);
		mip->ino = parentino;
		mypwdHelper(mip);
		name = findname(mip,ino);
		if(name != NULL)
			printf("/%s", name);
		iput(mip);
	}
}


void mystat()
{
	// Set up variables.
	int ino;
	MINODE * mip = malloc(sizeof(MINODE));
	// If there is no pathname, print error.
	if(!pathname[0])
	{
		printf("No pathname!");
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

void printdir(MINODE * mip)
{
	// Use buf to store data.
	char buf[1024];
	char * cp;
	for(int i = 0; i < 12;i++)
	{
		if(mip->INODE.i_block[i])
		{
			// Read data into buf.
			get_block(mip->dev,mip->INODE.i_block[i], buf);
			// Set up dp base on buf.
			dp = (DIR*)buf;
			cp = buf;

			while ((((char*)dp - buf) < 1024))
			{
				printf("  %s", dp->name);
				cp += dp->rec_len;
				dp = (DIR *) cp;
			}
		}
	}
	printf("\n");
}

int findCmd(char * command)
{
	// Find the appropriate command.
	if(strcmp(command,"ls") == 0)
	{
		return 0;
	}
	else if(strcmp(command,"cd") == 0)
	{
		return 1;
	}
	else if(strcmp(command,"stat") == 0)
	{
		return 2;
	}
	else if(strcmp(command,"pwd") == 0)
	{
		return 3;
	}
	else if(strcmp(command,"mkdir") == 0)
	{
		return 4;
	}
	else if(strcmp(command,"rmdir") == 0)
	{
		return 5;
	}
	else if(strcmp(command,"creat") == 0)
	{
		return 6;
	}
	else if(strcmp(command,"link") == 0)
	{
		return 7;
	}
	else if(strcmp(command,"unlink") == 0)
	{
		return 8;
	}
	else if(strcmp(command,"symlink") == 0)
	{
		return 9;
	}
	else if(strcmp(command,"readlink") == 0)
	{
		return 10;
	}
	else if(strcmp(command,"chmod") == 0)
	{
		return 11;
	}
	else if(strcmp(command,"touch") == 0)
	{
		return 12;
	}
	else if(strcmp(command,"open") == 0)
	{
		return 13;
	}
	else if(strcmp(command,"pfd") == 0)
	{
		return 14;
	}
	else if(strcmp(command,"quit") == 0)
	{
		return 15;
	}	
}

void myrmdir (char * pathname)
{
	int pino;
	MINODE * pmip;
	MINODE * mip;
	int ino;
	int parentino;
	char * name;

	printf("\t\t\t\t\t****INSIDE RMDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	
	if (pathname[0] == 0)
	{
		printf("No pathname for a directory to remove given!\n");
		return;
	}

	if(pathname[0] == '/')
	{
		printf("\t\t\t\t\tLINE 2 If\n");
		device = root->dev;
	}
	else
	{
		printf("\t\t\t\t\tLINE 2 Else\n");
		device = running->cwd->dev;
	}
	
	ino = getino(&device,pathname);
	mip = iget(device,ino);

	if(!S_ISDIR(mip->INODE.i_mode))
	{
		printf("\t\t\t\t\tLINE 9\n");
		printf("Invalid pathname\n");
		iput(mip);
		return;
	}
	else if(running->cwd == mip)
	{
		printf("\t\t\t\t\tLINE 10\n");
		printf("DIR is being used.\n");
		iput(mip);
		return;
	}
	else if(mip->INODE.i_links_count > 2)
	{
		printf("DIR not empty\n");
		iput(mip);
		return;
	}

	// deallocate its block and inode
	for(int i = 0; i < 12; i++)
	{
		if(mip->INODE.i_block[i] == 0)
			continue;
		bdealloc(mip->dev, mip->INODE.i_block[i]);
	}
	printf("%s\n",pathname);
	idealloc(mip->dev,mip->ino);
	iput(mip); 

	pmip = iget(running->cwd->dev,running->cwd->ino);
	rm_child(pmip,pathname);
	pmip->INODE.i_links_count--; // decrement pip's link count
	pmip->INODE.i_atime = pmip->INODE.i_mtime = time(0L); // touch pips time fields
	pmip->dirty = 1; // mark as dirty
	iput(pmip); //cleanup
	
}

void rm_child(MINODE * parent, char * my_name)
{
	int i, j, total_length, next_length, removed_length, previous_length;
	DIR *dNext;
	char buf[BLKSIZE], namebuf[256], temp[BLKSIZE], *cp, *cNext;

	// search parent inode's data blocks for the entry of my_name
	for(i = 0; i < 12; i++)
	{
		if(parent->INODE.i_block[i])
		{
			get_block(parent->dev, parent->INODE.i_block[i], buf);
			dp = (DIR *)buf;
			cp = buf;
			j = 0;
			total_length = 0;
			while(cp < &buf[BLKSIZE])
			{
				strcpy(namebuf, dp->name);
				namebuf[dp->name_len] = 0;
				total_length += dp->rec_len;

				// found my_name
				if(!strcmp(namebuf, my_name))
				{
					// if not first entry in data block
					if(j)
					{
						// if my_name is the last entry in the data block...
						if(total_length == BLKSIZE)
						{
							removed_length = dp->rec_len;
							cp -= previous_length;
							dp =(DIR *)cp;
							dp->rec_len += removed_length;
							put_block(parent->dev, parent->INODE.i_block[i], buf);
							parent->dirty = 1;
							return;
						}
						// otherwise, we must move all entries after this one left
						removed_length = dp->rec_len;
						cNext = cp + dp->rec_len;
						dNext = (DIR *)cNext;
						while(total_length + dNext->rec_len < BLKSIZE)
						{ 
							total_length += dNext->rec_len;
							next_length = dNext->rec_len;
							dp->inode = dNext->inode;
							dp->rec_len = dNext->rec_len;
							dp->name_len = dNext->name_len;
							strcpy(dp->name, dNext->name);
							cNext += next_length;
							dNext = (DIR *)cNext;
							cp+= next_length;
							dp = (DIR *)cp;
						}
						dp->inode = dNext->inode;
						// add removed rec_len to the last entry of the block
						dp->rec_len = dNext->rec_len + removed_length;
						dp->name_len = dNext->name_len;
						strncpy(dp->name, dNext->name, dNext->name_len);
						put_block(parent->dev, parent->INODE.i_block[i], buf); // save
						parent->dirty = 1;
						return;
					}
					// if first entry in a data block
					else
					{
						// deallocate the data block and modify the parent's file size
						bdealloc(parent->dev, parent->INODE.i_block[i]);
						memset(temp, 0, BLKSIZE);
						put_block(parent->dev, parent->INODE.i_block[i], temp);
						parent->INODE.i_size -= BLKSIZE;
						parent->INODE.i_block[i] = 0;
						parent->dirty = 1;
						return;
					}
				}
				j++;
				previous_length = dp->rec_len;
				cp+=dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
}


void mymkdir (char* path)
{
	MINODE * backup = running->cwd;
	MINODE * pmip;

	printf("\t\t\t\t\t****INSIDE MKDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");

	if (path[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}	

	token_path(path);
	for(int i = 0; i< 128; i++)
	{
		if(name[i] != NULL)
		{
			pmip = iget(running->cwd->dev,running->cwd->ino);
			if(getino(&pmip->dev,name[i]) == 0)
			{
				
				if(!S_ISDIR(pmip->INODE.i_mode))
				{
					printf("\t\t\t\t\tLINE 9\n");
					printf("%s is not a directory.\n", dirname(name[i]));
					iput(pmip);
					return;
				}	
				printf("\t\t\t\t\tLINE 11\n");
				kmkdir(pmip,name[i]);
				printf("\t\t\t\t\t****DONE MKDIR****\n");
				
			}
			int ino = getino(&running->cwd->dev,name[i]);
			MINODE * temp = iget(running->cwd->dev,ino);
			if(S_ISDIR(temp->INODE.i_mode))
			{
				running->cwd = temp;
				iput(temp);
			}
		}
	}
	for(int i = 0; i < 128; i++)
	{	
		pathname[i] = NULL;
		name[i] = NULL;
	}
	running->cwd = backup;
}

kmkdir (MINODE * pmip, char* newBasename)
{
	char * cp;
	int ino, bno, i;
	MINODE * mip;
	char buf[BLKSIZE];

	printf("\t\t\t\t\t****INSIDE KMKDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	ino = ialloc (pmip->dev);
	printf("\t\t\t\t\tLINE 2\n");
	bno = balloc (pmip->dev);
	printf("\t\t\t\t\tLINE 3\n");
	mip = iget (pmip->dev, ino);
	printf("\t\t\t\t\tLINE 4\n");			
	mip -> INODE.i_mode = 0x41ED;		
	mip -> INODE.i_uid = pmip -> INODE.i_uid;	
	mip -> INODE.i_gid = pmip -> INODE.i_gid;	
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
	
	//iput (mip);
	
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
	put_block (pmip->dev, bno, buf);
	printf("\t\t\t\t\tLINE 12\n");	
	enter_name (pmip, ino, newBasename);
	printf("\t\t\t\t\tLINE 13\n");
	pmip -> INODE.i_links_count++;
	printf("\t\t\t\t\tLINE 14\n");
	pmip -> dirty = 1;
	
	//iput(pmip);
	
	printf("\t\t\t\t\tLINE 16\n");
	printf("\t\t\t\t\t****DONE KMKDIR****\n");
}

enter_name (MINODE * pmip, int ino, char* newBasename)
{

	printf("\t\t\t\t\t****ENTER NAME****\n");
	int i = 0;
	int rec_length;
	char buf[BLKSIZE];
	int need_length,ideal_length;

	while(pmip->INODE.i_block[i])
	{
		i++;
	}
	i--;

	get_block(pmip->dev, pmip->INODE.i_block[i], buf);
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
	ideal_length = (4 *((8 + dp->name_len + 3) / 4)) + 1;
	rec_length = dp->rec_len; // store rec_len for a bit easier code writing

	// check if it can enter the new entry as the last entry
	if((rec_length - ideal_length) >= need_length)
	{
		// trim the previous entry to its ideal_length
		dp->rec_len = ideal_length;
		cp+= dp->rec_len;
		dp = (DIR *)cp;
		dp->rec_len = rec_length - ideal_length;
		dp->name_len = strlen(newBasename);
		strcpy(dp->name, newBasename);
		dp->inode = ino;
		// write the new block back to the disk
		put_block(pmip->dev, pmip->INODE.i_block[i], buf);
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
		put_block(pmip->dev, pmip->INODE.i_block[i], buf);
	}
	printf("\t\t\t\t\t****DONE ENTER NAME****\n");
}

void mycreat (char* path)
{
	MINODE * backup = running->cwd;
	MINODE * pmip;

	//printf("\t\t\t\t\t****INSIDE CREAT****\n");
	//printf("\t\t\t\t\tProcessing\n");
	//printf("\t\t\t\t\tLINE 1\n");

	if (path[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}	
	int j = 0;
	token_path(path);
	for(int i = 0; i< 128; i++)
	{
		if(name[i] != NULL)
		{
			pmip = iget(running->cwd->dev,running->cwd->ino);
			if(getino(&pmip->dev,name[i]) != 0)
			{
				int ino = getino(&running->cwd->dev,name[i]);
				MINODE * temp = iget(running->cwd->dev,ino);
				if(S_ISDIR(temp->INODE.i_mode))
				{
					running->cwd = temp;
					iput(temp);
				}
			}
			else
			{	
				if(name[i+1] != NULL)
				{
					printf("Directory is not available!\n");
					break;	
				}
				else if(name[i+1] == NULL)
				{
					//printf("\t\t\t\t\tLINE 11\n");
					kcreat(pmip,name[i]);
					//printf("\t\t\t\t\t****DONE CREAT****\n");	
				}
			}	
		}
	}
	for(int i = 0; i < 128; i++)
	{	
		pathname[i] = NULL;
		name[i] = NULL;
	}
	running->cwd = backup;
}

kcreat (MINODE * pmip, char* newBasename)
{
	char * cp;
	int ino, bno, i;
	MINODE * mip;
	char buf[BLKSIZE];

	//printf("\t\t\t\t\t****INSIDE KCREAT****\n");
	//printf("\t\t\t\t\tProcessing\n");
	//printf("\t\t\t\t\tLINE 1\n");
	ino = ialloc (pmip->dev);
	//printf("\t\t\t\t\tLINE 2\n");
	//bno = balloc (pmip->dev);
	//printf("\t\t\t\t\tLINE 3\n");
	mip = iget (pmip->dev, ino);
	//printf("\t\t\t\t\tLINE 4\n");			
	mip -> INODE.i_mode = 0x81A4;		
	mip -> INODE.i_uid = pmip -> INODE.i_uid;	
	mip -> INODE.i_gid = pmip -> INODE.i_gid;	
	mip -> INODE.i_size = 0;		
	mip -> INODE.i_links_count = 2;		
	mip -> INODE.i_atime = time (0L);
	mip -> INODE.i_ctime = time (0L);
	mip -> INODE.i_mtime = time (0L);
	mip -> INODE.i_blocks = 2;		
	mip -> dirty = 1;
	mip -> refCount = 1;

	//printf("\t\t\t\t\tLINE 5\n");
	/*for (i = 1; i < 15; i++)
	{
		mip -> INODE.i_block[i] = 0;
	}
	mip -> INODE.i_block[0] = bno;*/

	//printf("\t\t\t\t\tLINE 6\n");
	
	//iput (mip);
	
	//printf("\t\t\t\t\tLINE 7\n");
	memset (buf, 0, BLKSIZE);

	//printf("\t\t\t\t\tLINE 8\n");
	dp = (DIR*)buf;
	dp -> inode = ino;			// inode number
	strncpy (dp -> name, ".",1);		// file name
	dp -> name_len = 1;			// name length
	dp -> rec_len = 12;

	// directory entry length
	//printf("\t\t\t\t\tLINE 9\n");
	cp = buf + dp->rec_len;			
	dp = (DIR*)cp;

	//printf("\t\t\t\t\tLINE 10\n");
	dp -> inode = pmip -> ino;		// inode number
	strncpy (dp -> name, "..",2);		// file name
	dp -> name_len = 2;			// name length
	dp -> rec_len = BLKSIZE - 12;		// directory entry length + last entry

	//printf("\t\t\t\t\tLINE 11\n");
	put_block (pmip->dev, bno, buf);
	//printf("\t\t\t\t\tLINE 12\n");	
	enter_name (pmip, ino, newBasename);
	//printf("\t\t\t\t\tLINE 13\n");
	pmip -> INODE.i_links_count++;
	//printf("\t\t\t\t\tLINE 14\n");
	pmip -> dirty = 1;
	
	//iput(pmip);
	
	//printf("\t\t\t\t\tLINE 16\n");
	//printf("\t\t\t\t\t****DONE KCREAT****\n");
}

void link(char * old, char * newone)
{
	int oino;
	MINODE * omip;
	int nino;
	int pino;
	MINODE * pmip;
	int ino;

	printf("\t\t\t\t\t****INSIDE LINK****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	if (old[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}
	if (newone[0] == 0)
	{
		printf("No parameter for a new directory given!\n");
		return;
	}
	
	if(pathname[0] == '/')
	{
		printf("\t\t\t\t\tLINE 2 If\n");
		device = root->dev;
	}
	else
	{
		printf("\t\t\t\t\tLINE 2 Else\n");
		device = running->cwd->dev;
	}
	
	printf("\t\t\t\t\tLINE 6\n");
	pmip = iget(running->cwd->dev,running->cwd->ino);
	oino = getino(&running->cwd->dev,old);
	omip = iget(running->cwd->dev, oino);
	nino = getino(&running->cwd->dev,newone);

	if(nino != 0)
	{
		printf("File already exists!\n");
		return;
	}
	if(S_ISDIR(omip->INODE.i_mode))
	{
		printf("Can't link to a dir!\n");
		return;
	}

	printf("\t\t\t\t\tLINE 11\n");
	enter_name(pmip,oino,newone);
	printf("\t\t\t\t\tLINE 14\n");
	omip -> dirty = 1;
	omip ->INODE.i_links_count++;
	printf("\t\t\t\t\tLINE 16\n");
	iput (pmip);
	printf("\t\t\t\t\t****DONE LINK****\n");
	mystat();
	
	
}

void unlink(char * path)
{
	int pino;
	MINODE * pmip;
	MINODE * mip;
	int ino;
	int parentino;
	char  parent[256];
	char child[256];
	

	printf("\t\t\t\t\t****INSIDE RMDIR****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	
	if (path[0] == 0)
	{
		printf("No pathname for a directory to remove given!\n");
		return;
	}

	if(path[0] == '/')
	{
		printf("\t\t\t\t\tLINE 2 If\n");
		device = root->dev;
	}
	else
	{
		printf("\t\t\t\t\tLINE 2 Else\n");
		device = running->cwd->dev;
	}
	
	ino = getino(&device,path);
	mip = iget(device,ino);

	if(!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode))
	{
		printf("\t\t\t\t\tLINE 9\n");
		printf("Invalid pathname\n");
		iput(mip);
		return;
	}
	if(mip->INODE.i_links_count > 2)
	{
		printf("File is in used!\n");
		iput(mip);
		return;
	}
	mip->INODE.i_links_count--;
	if(mip->INODE.i_links_count > 0)
	{
		mip->dirty = 1;
		iput (mip);
	}
	if(!S_ISLNK(mip->INODE.i_mode))
	{
		// deallocate its block and inode
		for(int i = 0; i < 12; i++)
		{
			if(mip->INODE.i_block[i] == 0)
				continue;
			bdealloc(mip->dev, mip->INODE.i_block[i]);
		}
		idealloc(mip->dev,mip->ino);
		iput(mip); 
	}
	iput(mip);
	pmip = iget(running->cwd->dev,running->cwd->ino);
	rm_child(pmip,path);
	pmip->dirty = 1;
	iput(pmip);
}

int search (MINODE * mip, char *searchName)
{
	char buf[BLKSIZE];
	int i;
	char * cp;
	// loop through the direct blocks
	for (i = 0; i < 15; i++)
	{
		if (mip->INODE.i_block[i] != 0)
		{
			get_block (mip->dev, mip->INODE.i_block[i], buf);

			// have dp and cp both
			// both point at buf
			dp = (DIR *)buf;
			cp = (char *)buf;
		
			while (cp < buf + BLKSIZE)
			{	
				// looking for the directory name
				if (strcmp (dp -> name, searchName) == 0)
				{
					return dp -> inode;
				}
			
				// iterate to the next entry
				cp += dp -> rec_len;
				dp = (DIR *)cp;
			}
		}
	}
	
	// return 0 if we didn't find the ino
	return 0;
}

void mysymlink(char * old, char * newone)
{
	int oino;
	MINODE * omip;
	MINODE * mip;
	int nino;
	int pino;
	MINODE * pmip;
	int ino;

	printf("\t\t\t\t\t****INSIDE SYMLINK****\n");
	printf("\t\t\t\t\tProcessing\n");
	printf("\t\t\t\t\tLINE 1\n");
	if (old[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}
	if (newone[0] == 0)
	{
		printf("No parameter for a new directory given!\n");
		return;
	}

	if(pathname[0] == '/')
	{
		printf("\t\t\t\t\tLINE 2 If\n");
		device = root->dev;
	}
	else
	{
		printf("\t\t\t\t\tLINE 2 Else\n");
		device = running->cwd->dev;
	}
	
	printf("\t\t\t\t\tLINE 6\n");
	oino = getino(&running->cwd->dev,old);
	nino = getino(&running->cwd->dev,newone);
	if(oino == 0)
	{
		printf("File doesn't exist!\n");
		return;
	}
	if(nino != 0)
	{
		printf("File already exists!\n");
		return;
	}
	pmip = iget(running->cwd->dev,running->cwd->ino);
	omip = iget(running->cwd->dev, oino);
	kcreat (pmip,newone);
	nino = getino(&device,newone);
	mip = iget(device,nino);
	mip->INODE.i_mode = 0xA1FF; 
	strcpy((char *)(mip->INODE.i_block), old);
	mip->INODE.i_size = strlen(old);
	mip->dirty = 1;
	iput(mip);
	mystat();
}

char * readlink(char * path)
{
	int ino;
	MINODE * mip;
	if (path[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}
	if(pathname[0] == '/')
	{
		printf("\t\t\t\t\tLINE 2 If\n");
		device = root->dev;
	}
	else
	{
		printf("\t\t\t\t\tLINE 2 Else\n");
		device = running->cwd->dev;
	}
	ino = getino(&device,path);
	mip = iget(device,ino);
	
	if(!S_ISLNK(mip->INODE.i_mode))
	{
		printf("\t\t\t\t\tLINE 9\n");
		printf("Invalid pathname\n");
		iput(mip);
		return;
	}
	printf("SYMLINK: %s\n", (char *)(mip->INODE.i_block));
	return (char *)(mip->INODE.i_block);
	
}

void mychmod (char* path, char* permission)
{
	int i, j, r, w, x, count = 0;
	char cur;
	MINODE * backup = running->cwd;
	MINODE * pmip;

	// check if a path was passed in
	if (path[0] == 0)
	{
		printf("No pathname for chmod was given!\n");
		return;
	}

	// tokenize the path and store in the global name array
	token_path(path);

	printf ("name[0] = %s, name[1] = %s, name[2] = %s\n", name[0], name[1], name[2]);

	// loop through each index of the name array
	// need to loop to the dir containing the basename
	for(i = 0; i< 128; i++)
	{
		// if the name array contains something
		if(name[i] != NULL && name[i + 1] != NULL)
		{
			// read in the MINODE
			int ino = getino(&running->cwd->dev,name[i]);
			MINODE * temp = iget(running->cwd->dev,ino);

			// check for a dir
			if(!S_ISDIR(temp->INODE.i_mode))
			{
				break;
			}

			// go into the dir
			running->cwd = temp;
			iput(temp);

			count++;
		}
	}

	// read in the MINODE of whatever we're changing the mode of
	int ino = getino(&running->cwd->dev,name[count]);
	MINODE * temp = iget(running->cwd->dev,ino);

	// check that the basename exists
	if (ino == 0)
	{
		printf ("%s doesn't exist!\n", name[count]);
		return;
	}

	printf ("Previous permission = %o\n", temp->INODE.i_mode);
	
	// loop through each bit in the temp's current permission
	for (j = 0; j < 9; j++)
	{
		// reset each bit
		temp -> INODE.i_mode &= ~(1 << j);
	}

	for (j = 0; j < 3; j++)
	{
		r = 8 - 3 * j;
		j = 7 - 3 * j;
		x = 6 - 3 * j;

		cur = permission[j + 1];
	
		if (cur == '1' || cur == '3' || cur == '5' || cur == '7')
		{
			// shift the bits of x left by one
			// then binary OR it
			temp -> INODE.i_mode |= (1 << x);
		}
		else if (cur == '2' || cur == '3' || cur == '6' || cur == '7')
		{
			// shift the bits of w left by one
			// then binary OR it
			temp -> INODE.i_mode |= (1 << w);
		}
		else if (cur == '4' || cur == '5' || cur == '6' || cur == '7')
		{
			// shift the bits of r left by one
			// then binary OR it
			temp -> INODE.i_mode |= (1 << r);
		}
	}

	printf ("New permission = %o\n", temp->INODE.i_mode);
	
	// mark the minode as dirty
	temp -> dirty = 1;
	iput (temp);

	// reset the name and pathname arrays
	for(int i = 0; i < 128; i++)
	{	
		pathname[i] = NULL;
		name[i] = NULL;
	}
	
	// restore the original cwd
	running->cwd = backup;
}

void mytouch (char* path)
{
	int i, count = 0;
	char cur;
	MINODE * backup = running->cwd;
	MINODE * pmip;

	// check if a path was passed in
	if (path[0] == 0)
	{
		printf("No pathname for chmod was given!\n");
		return;
	}

	// tokenize the path and store in the global name array
	token_path(path);

	printf ("name[0] = %s, name[1] = %s, name[2] = %s\n", name[0], name[1], name[2]);

	// loop through each index of the name array
	// need to loop to the dir containing the basename
	for(i = 0; i< 128; i++)
	{
		// if the name array contains something
		if(name[i] != NULL && name[i + 1] != NULL)
		{
			// read in the MINODE
			int ino = getino(&running->cwd->dev,name[i]);
			MINODE * temp = iget(running->cwd->dev,ino);

			// check for a dir
			if(!S_ISDIR(temp->INODE.i_mode))
			{
				break;
			}

			// go into the dir
			running->cwd = temp;
			iput(temp);

			count++;
		}
	}

	// read in the MINODE of whatever we're changing the mode of
	int ino = getino(&running->cwd->dev,name[count]);
	MINODE * temp = iget(running->cwd->dev,ino);

	// check that the basename exists
	if (ino == 0)
	{
		printf ("%s doesn't exist!\n", name[count]);
		return;
	}

	printf ("Previous time = %s\n", ctime(&(temp->INODE.i_ctime)));
	printf ("Previous atime = %s\n", ctime(&(temp->INODE.i_atime)));
	printf ("Previous mtime = %s\n", ctime(&(temp->INODE.i_mtime)));
	
	// update the inode's time to the current time
	temp -> INODE.i_atime = time (0L);
	temp -> INODE.i_mtime = time (0L);

	printf ("New time = %s\n", ctime(&(temp->INODE.i_ctime)));
	printf ("New atime = %s\n", ctime(&(temp->INODE.i_atime)));
	printf ("New mtime = %s\n", ctime(&(temp->INODE.i_mtime)));
	
	// mark the minode as dirty
	temp -> dirty = 1;
	iput (temp);

	// reset the name and pathname arrays
	for(int i = 0; i < 128; i++)
	{	
		pathname[i] = NULL;
		name[i] = NULL;
	}
	
	// restore the original cwd
	running->cwd = backup;
}

OFT * myopen(char * path, int * param)
{
	int ino;
	MINODE * mip;

	if (path[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}
	if (param < 0 || param > 3)
	{
		printf("No permission was given!\n");
		return;
	}
	if(path[0] == '/')
	{
		token_path(path);
		ino = getino(&running->cwd->dev,name[0]);
		mip = iget(running->cwd->dev,ino);
		for(int i = 0; i< 128; i++)
		{
			name[i] = NULL;
		}
	}
	else
	{
		ino = getino(&running->cwd->dev,path);
		mip = iget(running->cwd->dev,ino);
	}
	
	if(!S_ISREG(mip->INODE.i_mode))	
	{
		printf("File is not a regular file!\n");
		iput(mip);
		return NULL;
	}
	
	for(int i = 0; i < 10; i++)
	{
		
		if(running->fd[i] != 0  && running->fd[i]->inodeptr == mip && running->fd[i]->mode != 0)
		{
			printf("File is in use\n");
			iput(mip);
			return NULL;
		}
	}

	OFT * oftp = malloc(sizeof(OFT));
	int ofin = falloc(oftp);
	if (ofin == -1)
	{
		printf("No available slots!\n");
		return NULL;
	}
	oftp->mode = param;

	if (oftp->mode == 0 || oftp->mode == 1 || oftp->mode == 2)
	{
		oftp->offset = 0;
	}
	else if (oftp->mode == 3)
	{
		oftp->offset = mip->INODE.i_size;
	}
	else
	{
		printf("Invalid mode!\n");
		running->fd[ofin] = 0;
		return NULL;
	}
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	mip->dirty = 1;
	oftp->inodeptr = mip;
	running->fd[ofin] =oftp;
	return running->fd[ofin];		
}

void close(OFT * currentFD)
{
	if(currentFD == NULL)
	{
		printf("Not found!\n");
		return;
	}
	for(int i = 0; i < 10; i++)
	{
		if(currentFD == running->fd[i])
		{
			running->fd[i]->refCount--;
			running->fd[i] = 0;
		}
	}
}

void pfd()
{
	int ino;
	int pino;
	MINODE * mip;
	int i;
	int check = 0;
	for(i = 0; i < 10; i++)
	{
		if(running->fd[i] != 0)
		{
			check = 1;
			printf("Opened file: ");
			MINODE * mip = iget(running->cwd->dev,running->cwd->ino);
			printf("%s\n",findname(mip,running->fd[i]->inodeptr->ino));

			printf("MODE: ");
			if(running->fd[i]->mode == 0)
			{
				printf("READ\n");
			}
			else if(running->fd[i]->mode == 1)
			{
				printf("WRITE\n");
			}
			else if(running->fd[i]->mode == 2)
			{
				printf("RDWR\n");
			}
			if(running->fd[i]->mode == 3)
			{
				printf("APPEND\n");
			}
			/**
			if(S_ISDIR(running->fd[i]->inodeptr->INODE.i_mode))
			{
				printf("Inside the file: ");
				pfdprint(running->fd[i]->inodeptr);
			}*/
		}
	}
	if(check == 0)
	{
		printf("There is no opened file!\n");
	}
}

void pfdprint(MINODE * mip)
{
	char buf[BLKSIZE];
	char *cp;
	for(int i = 0; i < 12;i++)
	{
		if(mip->INODE.i_block[i])
		{	
			// Read data into buf.
			get_block(mip->dev,mip->INODE.i_block[i], buf);
			// Set up dp base on buf.
			dp = (DIR*)buf;
			cp = buf;
			cp += dp->rec_len;
			dp = (DIR *) cp;
			cp += dp->rec_len;
			dp = (DIR *) cp;
			while ((((char*)dp - buf) < 1024))
			{
				printf("  %s", dp->name);
				cp += dp->rec_len;
				dp = (DIR *) cp;
			}
		}
	}
	printf("\n");	
}

int mywrite(OFT * fdcurrent, char * param)
{
	if(fdcurrent->mode == 0)
	{
		printf("File is not in write mode!\n");
		return;
	}
	char text[1024];
	int i;
	for(i = 1; i < strlen(param); i++)
	{
		text[i-1] = param[i];	
	}
	text[strlen(param) - 2] = '\0';
	printf("param: %s\n", text);
	int length = strlen(text);
	int lbk;
	int blk;
	int start;
	MINODE * mip = fdcurrent->inodeptr;

	char buf[BLKSIZE];
	char indirbuf[BLKSIZE];
	int indirect;
	char * cp;
	int remain;
	char * cq = (char *)text;
	int count = 0;

	while(length > 0)
	{
		lbk = fdcurrent->offset / BLKSIZE;
		start = fdcurrent->offset % BLKSIZE;
		if(mip->INODE.i_block[lbk] == 0)
		{
			mip->INODE.i_block[lbk] = balloc(mip->dev);
		}
		blk = mip->INODE.i_block[lbk];
		get_block(mip->dev,blk,buf);
		if(fdcurrent->mode == 1)
		{
			memset(buf,0,1024);
		}
		cp = buf + start;
		remain = BLKSIZE - start;
		while (remain > 0)
		{
			*cp++ = *cq++;
			length--;
			remain--;
			fdcurrent->offset++;
			count++;
			if(fdcurrent->offset > mip->INODE.i_size)
			{
				mip->INODE.i_size++;
			}
			if(length <=0)
			{
				break;
			}
		}
		put_block(mip->dev,blk,buf);
		OFT * backup = fdcurrent;
		close(fdcurrent);
		mip = iget(running->cwd->dev,running->cwd->ino);
		currentfd = myopen(findname(mip,backup->inodeptr->ino),backup->mode);
		iput(mip);
	}
	return count;
}

int myread(OFT * fdcurrent, int length)
{
	if(fdcurrent->mode == 1)
	{
		printf("File is not in read mode!\n");
		return;
	}
	char text[1024];
	int i;
	int lbk;
	int blk;
	int start;
	MINODE * mip = fdcurrent->inodeptr;
	char buf[BLKSIZE];
	char result[BLKSIZE];
	char * cp;
	int remain;
	char * cq;
	int count = 0;
	
	while(length > 0)
	{
		lbk = fdcurrent->offset / BLKSIZE;
		start = fdcurrent->offset % BLKSIZE;
		if(mip->INODE.i_block[lbk] == 0)
		{
			break;
		}
		blk = mip->INODE.i_block[lbk];
		get_block(mip->dev,blk,buf);
		cq = result;
		cp = buf + start;
		remain = BLKSIZE - start;	
		while (remain > 0)
		{
			*cq++ = *cp++;
			length--;
			remain--;
			fdcurrent->offset++;
			count++;
			if(length ==0)
			{
				break;
			}
		}
	}
	iput(mip);
	printf("READED: %s\n", result);
	OFT * backup = fdcurrent;
	close(fdcurrent);
	mip = iget(running->cwd->dev,running->cwd->ino);
	currentfd = myopen(findname(mip,backup->inodeptr->ino),backup->mode);
	iput(mip);
	return count;
}

void cat(char * path)
{
	int ino;
	int blk;
	char * buf[BLKSIZE];
	OFT * cur = myopen(path,0);
	MINODE * mip = running->cwd;
	ino = getino(&mip->dev,path);
	mip = iget(mip->dev,ino);
	if (cur == NULL || !S_ISREG(mip->INODE.i_mode))
	{
		return;
	}
	if(cur->inodeptr->ino == mip->ino)
	{
		int lbk = cur->offset / BLKSIZE;
		int start = cur->offset % BLKSIZE;
		blk = mip->INODE.i_block[lbk];
		get_block(mip->dev,blk,buf);
		printf("Inside file:\n");
		printf("%s",buf);
	}
	printf("\n");
	close(cur);
}

int falloc(OFT * oftp)
{
	int i;
	for(i = 0; i < 10; i++)
	{
		if(running->fd[i] == 0)
		{
			break;
		}
	}
	if (i == 10)
	{
		return -1;
	}
	//running->fd[i] = oftp;
	return i;
}

void mylseek (int fdNo, int seekTo)
{
	// verify the seekTo value is a valid value
	// i.e. the seekTo is not negative or greater
	// than the file size
	if (seekTo < 0 || seekTo > (running -> fd[fdNo] -> inodeptr -> INODE.i_size))
	{
		printf ("Invalid seek value!\n");
		return -1;
	}

	// set the offset to the new position
	running -> fd[fdNo] -> offset = seekTo;
}

int quit()
{
	// Quit.
	printf("Quit!\n");
	exit(0);
}

