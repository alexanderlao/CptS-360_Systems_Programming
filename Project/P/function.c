#include "function.h"
#include "utility.h"

// initializes the processes and MINODE array
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
		proc[1] -> fd[j] = 0;
	}
	
	// set the root to 0
	root = 0;
	printf("Initialized!\n");
}

// initializes the root by checking for a valid disk
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

	// initialize the FD array
	for (int i = 0; i < NFD; i++)
	{
		running -> fd[i] = 0;
	}
		
	// Print.
	printf("mount : %s  mounted on %s\n", root->mountptr->mount_name,root->mountptr->name);
}

// prints out the content of a directory
void ls()
{
	// List disk.
	// Set up the variables.
	int ino;
	MINODE * backup = running->cwd;
	MINODE * mip = running->cwd;
	int tempdev = running->cwd->dev;

	// If the user enters a pathname.
	if(pathname[0])
	{
		// If pathname is /
		if(pathname[0] == '/' && pathname[1] == '\0')
		{	
			// Set the minode to root.
			mip = iget(running->cwd->dev,ROOT_INODE);
		}
		// Otherwise, get the minode that correspond to the pathname.
		else if(pathname[0] == '/' && pathname[1] != '\0')
		{
			int i;
			token_path(pathname);
			
			// traverse to the pathname
			for(i = 0; i < 128; i++)
			{
				if(name[i] != NULL)
				{
					// Set root as the corresponding minode.;
					int inotemp = getino(&running->cwd->dev,name[i]);
					MINODE * temp = iget(running->cwd->dev,inotemp);
					if(S_ISDIR(temp->INODE.i_mode))
					{
						running->cwd = temp;
					}
				}
				else
				{	
					break;
				}
			}
			mip = running->cwd;
		}
		// otherwise we just want to ls the current working directory
		else
		{
			ino = getino(&running->cwd->dev,path);
			mip = iget(running->cwd->dev,ino);
		}	
	}	
	// if the basename is a dir, just print out it's content
	if(S_ISDIR(mip->INODE.i_mode))
	{
		// Print the minode.
		printdir(mip);
	}
	// otherwise it's not a dir and something went wrong
	else
	{
		printf("%s is not a directory!\n",pathname);
	}

	// restore the original cwd
	running->cwd = backup;
}

// changes the current working directory
void cd()
{
	char buf[1024];
	int i;
	
	// If there is no pathname.
	if(!pathname[0] || pathname[0] == '/' && pathname[1] == NULL)
	{	
		// Set root as default.
		running->cwd = iget(running->cwd->dev,ROOT_INODE);
	}
	// Otherwise,
	else
	{
		token_path(pathname);

		// traverse to the pathname
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
	}
}

// prints the current working directory
void mypwd()
{
	// base case is once we reach the root
	if(running->cwd->ino == ROOT_INODE)
	{
		printf("CWD: /\n");
	}
	else
	{
		printf("CWD: ");
		mypwdHelper(running->cwd);
		printf("\n");
	}
}

// recursive helper for the pwd function
void mypwdHelper(MINODE * mip)
{
	int ino, parentino;
	char * name;

	// once we hit the root, return
	if(mip->ino == ROOT_INODE)
	{
		return;
	}
	else
	{
		// find the child ino in its parent
		findino(mip,&ino,&parentino);

		// read in the parent's MINODE
		mip = iget(mip->dev,parentino);
		mip->ino = parentino;

		// make the recursive call on the parent
		mypwdHelper(mip);

		// find the child's name in the parent
		name = findname(mip,ino);

		// print out the name
		if(name != NULL)
			printf("/%s", name);

		// clean up
		iput(mip);
	}
}

// changes the mode of a file
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
	MINODE * mip = iget(running->cwd->dev,ino);

	// check that the basename exists
	if (ino == 0)
	{
		printf ("%s doesn't exist!\n", name[count]);
		return;
	}

	// calculate the permission bits with left shifts
	// and OR operations
	int mode = 1 << 15;
	mode |= 0 << 12;
	mode |= permission[0] - 48 << 9;
	mode |= permission[1] - 48 << 6;
	mode |= permission[2] - 48 << 3;
	mode |= permission[3] - 48;

	// reset the MINODE's current mode
	// and set the new mode
	mip->INODE.i_mode &= 0xFF000;
	mip->INODE.i_mode |= mode;

	// restore the original cwd
	running->cwd = backup;
}

// updates the time field of a file
void mytouch (char* path)
{
	// read the path's MINODE into memory
	int ino = getino(&running->cwd->dev,path);
	MINODE * mip = iget(running->cwd->dev,ino);

	// reset its time fields
	mip -> INODE.i_atime = time (0L);
	mip -> INODE.i_ctime = time (0L);
	mip -> INODE.i_mtime = time (0L);

	// clean up
	iput(mip);
}

// returns the information of a file in a STAT struct
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

// prints out a directory
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

			// loop to the end of the block
			while ((((char*)dp - buf) < 1024))
			{	
				// read in the MINODE
				int temp = dp->inode;
				MINODE * tmip = iget(running->cwd->dev,temp);

				// check for a symlink
				if(S_ISLNK(tmip->INODE.i_mode))
				{
					printf("  %s(SYMLINK: %s)",dp->name,tmip->INODE.i_block);
				}
				// otherwise just print out the name
				else
				{
					printf("  %s", dp->name);
				}
	
				// iterate to the next entry
				cp += dp->rec_len;
				dp = (DIR *) cp;
			}
		}
	}
	printf("\n");
}

// use this to find commands for main
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
	
	else if(strcmp(command,"open") == 0)
	{
		return 11;
	}
	else if(strcmp(command,"pfd") == 0)
	{
		return 12;
	}
	else if(strcmp(command,"close") == 0)
	{
		return 13;
	}
	else if(strcmp(command,"write") == 0)
	{
		return 14;
	}
	else if(strcmp(command,"read") == 0)
	{
		return 15;
	}
	else if(strcmp(command,"cat") == 0)
	{
		return 16;
	}
	else if(strcmp(command,"chmod") == 0)
	{
		return 17;
	}
	else if(strcmp(command,"touch") == 0)
	{
		return 18;
	}
	else if(strcmp(command,"cp") == 0)
	{
		return 19;
	}
	else if(strcmp(command,"lseek") == 0)
	{
		return 20;
	}
	else if(strcmp(command,"mv") == 0)
	{
		return 21;
	}
	else if(strcmp(command,"rm") == 0)
	{
		return 22;
	}
	else if(strcmp(command,"quit") == 0)
	{
		return 23;
	}	
}

// removes a directory
void myrmdir (char * path)
{
	MINODE * pmip;
	MINODE * mip;
	int ino;
	int num;
	MINODE * backup = running->cwd;
	
	// check that a pathname was passed it
	if (path[0] == 0)
	{
		printf("No pathname for a directory to remove given!\n");
		return;
	}
	token_path(path);

	// traverse to the basename
	for(int i = 0; i< 128; i++)
	{
		if(name[i] != NULL && name[i+1] != NULL)
		{
			int inotemp = getino(&running->cwd->dev,name[i]);
			MINODE * temp = iget(running->cwd->dev,inotemp);
			if(S_ISDIR(temp->INODE.i_mode))
			{
				running->cwd = temp;
			}
		}
		else
		{	
			num = i;
			break;
		}
	}

	// load the basename into a MINODE
	ino = getino(&running->cwd->dev,name[num]);
	mip = iget(running->cwd->dev,ino);

	// check if it's a dir
	if(!S_ISDIR(mip->INODE.i_mode))
	{
		//printf("\t\t\t\t\tLINE 9\n");
		printf("Invalid pathname!\n");
		iput(mip);
		return;
	}
	// check if we're trying to remove the dir
	// that we're currently in
	else if(running->cwd == mip)
	{
		//printf("\t\t\t\t\tLINE 10\n");
		printf("DIR is being used!\n");
		iput(mip);
		return;
	}
	// check if the dir is empty
	else if(mip->INODE.i_links_count > 2)
	{
		printf("DIR not empty!\n");
		iput(mip);
		return;
	}
	// check if the dir exists
	else if(ino == 0)
	{
		printf("DIR not found!\n");
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

	// deallocate the INODE and clean it up
	idealloc(mip->dev,mip->ino);
	iput(mip);

	// load the parent MINODE into memory
	// and remove the removed dir from the parent
	pmip = iget(running->cwd->dev,running->cwd->ino);
	rm_child(pmip,name[num]);

	// decrement the parent's link count
	// and update its timefield
	pmip->INODE.i_links_count--; 
	pmip->INODE.i_atime = pmip->INODE.i_mtime = time(0L);

	// clean up
	pmip->dirty = 1; 
	iput(pmip);

	// restore the current working directory
	running->cwd = backup;
}

// removes a directory from its parent
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
				namebuf[dp->name_len] = '\0';
				total_length += dp->rec_len;

				// found my_name? then we must erase it
				if(!strcmp(namebuf, my_name))
				{
					// if not first entry in data block
					if(j)
					{
						// if my_name is the last entry in the data block...
						if(total_length == BLKSIZE)
						{
							// remove the last entry
							removed_length = dp->rec_len;
							cp -= previous_length;
							dp =(DIR *)cp;

							// give the predecessor the removed length
							dp->rec_len += removed_length;

							// clean up
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
							// shifting all entries to the left
							total_length += dNext->rec_len;
							next_length = dNext->rec_len;

							// copy the next entry
							dp->inode = dNext->inode;
							dp->rec_len = dNext->rec_len;
							dp->name_len = dNext->name_len;

							// copy the name over to the left
							strcpy(dp->name, dNext->name);

							// iterate to the next entry
							cNext += next_length;
							dNext = (DIR *)cNext;
							cp+= next_length;
							dp = (DIR *)cp;
						}
						// for the last entry
						dp->inode = dNext->inode;
						
						// add removed rec_len to the last entry of the block
						dp->rec_len = dNext->rec_len + removed_length;
						dp->name_len = dNext->name_len;
			
						// copy the name over to the left
						strncpy(dp->name, dNext->name, dNext->name_len);
						dp->name[dNext->name_len] = '\0';

						// clean up
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

				// we didn't find the correct dir to remove
				// so keep iterating
				j++;
				previous_length = dp->rec_len;
				cp+=dp->rec_len;
				dp = (DIR *)cp;
			}
		}
	}
}

// creates a new directory
void mymkdir (char* path)
{
	MINODE * backup = running->cwd;
	MINODE * pmip;

	// check that a pathname was passed in
	if (path[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}	

	token_path(path);

	// traverse to the pathname
	for(int i = 0; i< 128; i++)
	{
		if(name[i] != NULL)
		{
			pmip = iget(running->cwd->dev,running->cwd->ino);
			if(getino(&pmip->dev,name[i]) == 0)
			{
				// check if it's a valid dir to go into
				if(!S_ISDIR(pmip->INODE.i_mode))
				{
					//printf("\t\t\t\t\tLINE 9\n");
					printf("%s is not a directory.\n", dirname(name[i]));
					iput(pmip);
					return;
				}	
				
				// found the basename so we can create the dir
				kmkdir(pmip,name[i]);
			}
			// traversal
			int ino = getino(&running->cwd->dev,name[i]);
			MINODE * temp = iget(running->cwd->dev,ino);
			if(S_ISDIR(temp->INODE.i_mode))
			{
				running->cwd = temp;
			}
		}
	}

	// restore the original current working directory
	running->cwd = backup;
}

// initalizes the new dir entry
kmkdir (MINODE * pmip, char* newBasename)
{
	char * cp;
	int ino, bno, i;
	MINODE * mip;
	char buf[BLKSIZE];

	// allocate a new INODE and block
	ino = ialloc (pmip->dev);
	bno = balloc (pmip->dev);

	// load the new INODE into a MINODE
	// and initialize its fields
	mip = iget (pmip->dev, ino);		
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

	// initializing the data blocks
	for (i = 1; i < 15; i++)
	{
		mip -> INODE.i_block[i] = 0;
	}
	mip -> INODE.i_block[0] = bno;
	iput (mip);
	
	// reset the buf
	memset (buf, 0, BLKSIZE);

	// need to add the . and .. entries to the new dir
	dp = (DIR*)buf;
	dp -> inode = ino;			// inode number
	strncpy (dp -> name, ".",1);		// file name
	dp -> name_len = 1;			// name length
	dp -> rec_len = 12;

	// directory entry length
	// interate to the next entry
	cp = buf + dp->rec_len;			
	dp = (DIR*)cp;

	dp -> inode = pmip -> ino;		// inode number
	strncpy (dp -> name, "..",2);		// file name
	dp -> name_len = 2;			// name length
	dp -> rec_len = BLKSIZE - 12;		// directory entry length + last entry

	// write the block back to the disk
	put_block (pmip->dev, bno, buf);
	
	// enter the new dir in its parent
	enter_name (pmip, ino, newBasename);
	
	// increment the parent's link count
	// and clean up
	pmip -> INODE.i_links_count++;
	pmip -> dirty = 1;
	iput(pmip);
}

// enters a new directory or file into its parent
void enter_name (MINODE * pmip, int ino, char* newBasename)
{
	int i = 0;
	int rec_length;
	char buf[BLKSIZE];
	int need_length,ideal_length;

	// find the last non-empty block
	while(pmip->INODE.i_block[i])
	{
		i++;
	}
	i--;

	// read the block into memory
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
	ideal_length = (4 *((8 + dp->name_len + 3) / 4) + 1);
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
}

// creates a new regular file
void mycreat (char* path)
{
	MINODE * backup = running->cwd;
	MINODE * pmip;

	// check that a pathname was passed in correctly
	if (path[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}	
	int j = 0;
	token_path(path);

	// traverse to the basename
	for(int i = 0; i< 128; i++)
	{
		if(name[i] != NULL)
		{
			pmip = iget(running->cwd->dev,running->cwd->ino);

			// if the current part of the path already exists
			if(getino(&pmip->dev,name[i]) != 0)
			{
				int ino = getino(&running->cwd->dev,name[i]);
				MINODE * temp = iget(running->cwd->dev,ino);

				// check for valid dirs to go into
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
					// found the file to create
					kcreat(pmip,name[i]);
				}
			}	
		}
	}

	// restore the original current working directory
	running->cwd = backup;
}

// initializes a new file entry
kcreat (MINODE * pmip, char* newBasename)
{
	char * cp;
	int ino, bno, i;
	MINODE * mip;
	char buf[BLKSIZE];

	// allocate a new INODE
	ino = ialloc (pmip->dev);
	//bno = balloc (pmip->dev);

	// load the new INODE into a MINODE
	// and initialize its fields
	mip = iget (pmip->dev, ino);		
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

	// initialize the data blocks
	for (i = 0; i < 15; i++)
	{
		mip -> INODE.i_block[i] = 0;
	}
	
	// reset the buf
	memset (buf, 0, BLKSIZE);

	// enter the file into the parent
	enter_name (pmip, ino, newBasename);
	
	// clean up
	pmip -> dirty = 1;
	iput(pmip);
}

// links two regular files
void link(char * old, char * newone)
{
	int oino;
	MINODE * omip;
	int nino;
	int pino;
	MINODE * pmip;
	int ino;

	// check for valid input
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
	// check which device to use
	if(pathname[0] == '/')
	{
		device = root->dev;
	}
	else
	{
		device = running->cwd->dev;
	}
	
	// read in the parent MINODE
	pmip = iget(running->cwd->dev,running->cwd->ino);

	// read in the old inumber/MINODE and the new inumber
	oino = getino(&running->cwd->dev,old);
	omip = iget(running->cwd->dev, oino);
	nino = getino(&running->cwd->dev,newone);

	// check that the new file doesn't already exist
	if(nino != 0)
	{
		printf("File already exists!\n");
		return;
	}
	// check that the old file exists
	if(getino(&running->cwd->dev,old) == 0)
	{
		printf("No file with the entered name availabe!\n");
		return;
	}
	// check for dirs
	if(S_ISDIR(omip->INODE.i_mode))
	{
		printf("Can't link to a dir!\n");
		return;
	}

	// enter the new file into the parent
	enter_name(pmip,oino,newone);

	// mark the old MINODE as dirty and increment its link count
	omip -> dirty = 1;
	omip ->INODE.i_links_count++;

	// load the new MINODE into memory
	nino = getino(&running->cwd->dev,newone);
	MINODE * temp = iget(running->cwd->dev,nino);

	// clean up and reset the parameter
	iput (pmip);
	memset(parameter,0,64);
}

// unlink files
void unlink(char * path)
{
	int pino;
	MINODE * pmip;
	MINODE * mip;
	int ino;
	int parentino;
	char  parent[256];
	char child[256];
	
	// check that a pathname was passed in
	if (path[0] == 0)
	{
		printf("No pathname for a directory to remove given!\n");
		return;
	}
	
	// read its MINODE into memory
	ino = getino(&running->cwd->dev,path);
	mip = iget(device,ino);

	// if the MINODE is not a regular file or a symbolic link
	// there is a problem
	if(!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode))
	{
		printf("Invalid pathname\n");
		iput(mip);
		return;
	}
	// decrement the parent's link count
	mip->INODE.i_links_count--;
	
	// check if the MINODE is still being used
	if(mip->INODE.i_links_count > 0)
	{
		mip->dirty = 1;
		iput (mip);
	}
	// check for regular files
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
	
	// remove the child and clean up
	iput(mip);
	pmip = iget(running->cwd->dev,running->cwd->ino);
	rm_child(pmip,path);
	pmip->dirty = 1;
	iput(pmip);
}

// searches for a name in a MINODE
// returns the inumber if found or 0 if not found
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

// creates a symbolic link
void mysymlink(char * old, char * newone)
{
	int oino;
	MINODE * omip;
	MINODE * mip;
	int nino;
	int pino;
	MINODE * pmip;
	int ino;

	// check that parameters were passed in
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

	// get the old and new inumbers
	oino = getino(&running->cwd->dev,old);
	nino = getino(&running->cwd->dev,newone);

	// check that the old file exists
	if(oino == 0)
	{
		printf("File doesn't exist!\n");
		return;
	}
	// check that the new files doesn't already exist
	if(nino != 0)
	{
		printf("File already exists!\n");
		return;
	}

	// load each MINODE into memory
	pmip = iget(running->cwd->dev,running->cwd->ino);
	omip = iget(running->cwd->dev, oino);

	// check whether we need to symlink a dir or regular file
	// and create the appropriate one
	if(S_ISDIR(omip->INODE.i_mode))
	{
		kmkdir(pmip,newone);
	}
	else if(S_ISREG(omip->INODE.i_mode))
	{
		kcreat (pmip,newone);
	}

	// reload the new MINODE and set its mode
	// to a symbolic link
	nino = getino(&running->cwd->dev,newone);
	mip = iget(running->cwd->dev,nino);
	mip->INODE.i_mode = 0xA1FF; 

	// copy the name over to the MINODE's
	// data block field
	strcpy((char *)(mip->INODE.i_block), old);
	mip->INODE.i_size = strlen(old);

	// clean up and reset the parameter
	mip->dirty = 1;
	iput(mip);
	memset(parameter,0,64);
}

// reads the data in the INODE's data block field
char * readlink(char * path)
{
	int ino;
	MINODE * mip;

	// check that a pathname was passed in correctly
	if (path[0] == 0)
	{
		printf("No pathname for a new directory given!\n");
		return;
	}
	
	// read the MINODE into memory
	ino = getino(&running->cwd->dev,path);
	mip = iget(running->cwd->dev,ino);
	
	// check for a symbolic link
	if(!S_ISLNK(mip->INODE.i_mode))
	{
		printf("Invalid pathname\n");
		iput(mip);
		return NULL;
	}

	// print out the MINODE's INODE's data in the data block field
	printf("SYMLINK: %s\n", (char *)(mip->INODE.i_block));

	// return its length
	return (char *)(mip->INODE.i_block);
}

// open a file for read, write, read/write, or append
OFT * myopen(char * path, int * param)
{
	int ino;
	MINODE * mip;

	// check that parameters were passed in correctly
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

	// check for paths and how we need to parse it
	if(path[0] == '/')
	{
		token_path(path);
		ino = getino(&running->cwd->dev,name[0]);
		mip = iget(running->cwd->dev,ino);
	}
	else
	{
		ino = getino(&running->cwd->dev,path);
		mip = iget(running->cwd->dev,ino);
	}
	
	// check if the file is regular
	if(!S_ISREG(mip->INODE.i_mode))	
	{
		printf("File is not a regular file!\n");
		iput(mip);
		return NULL;
	}
	
	// search for a free fd entry with the lowest index
	for(int i = 0; i < 10; i++)
	{
		if(running->fd[i] != 0  && running->fd[i]->inodeptr == mip && running->fd[i]->mode != 0)
		{
			printf("File is in use\n");
			iput(mip);
			return NULL;
		}
	}

	// initialize a new openTable entry
	OFT * oftp = malloc(sizeof(OFT));
	int ofin = falloc(oftp);
	if (ofin == -1)
	{
		printf("No available slots!\n");
		return NULL;
	}

	// initialize its fields
	oftp->mode = param;

	// check where to set the offset
	// R, W, or R/W starts at zero
	if (oftp->mode == 0 || oftp->mode == 1 || oftp->mode == 2)
	{
		oftp->offset = 0;
	}
	// append mode starts at the end of the file
	else if (oftp->mode == 3)
	{
		oftp->offset = mip->INODE.i_size;
	}
	// otherwise its invalid
	else
	{
		printf("Invalid mode!\n");
		running->fd[ofin] = 0;
		return NULL;
	}

	// update the time fields
	// and store the new openTable entry
	// in the fd array
	mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
	mip->dirty = 1;
	oftp->inodeptr = mip;
	oftp->refCount++;
	running->fd[ofin] = oftp;
	return running->fd[ofin];		
}

// close a file
void close(OFT * currentFD)
{
	// check for a valid FD
	if(currentFD == NULL)
	{
		printf("Not found!\n");
		return;
	}
	// look for the current FD in the fd array
	for(int i = 0; i < 10; i++)
	{
		if(currentFD == running->fd[i])
		{
			// reset everything to 0
			running->fd[i]->inodeptr = 0;
			running->fd[i]->refCount--;
			running->fd[i] = 0;
		}
	}
}

// prints the contents of the FD array
void pfd()
{
	int ino;
	int pino;
	MINODE * mip;
	int i;
	int check = 0;

	// loop through each fd in the fd array
	for(i = 0; i < 10; i++)
	{
		// look for used fds
		if(running->fd[i] != 0)
		{
			check = 1;

			// print out its name
			printf("Opened file: ");
			MINODE * mip = iget(running->cwd->dev,running->cwd->ino);
			printf("%s\n",findname(mip,running->fd[i]->inodeptr->ino));

			// print outs its mode
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
	// if no files are opened
	if(check == 0)
	{
		printf("There is no opened file!\n");
	}
}

// prints out names in a MINODE
void pfdprint(MINODE * mip)
{
	char buf[BLKSIZE];
	char *cp;
	
	// loop through each data block
	for(int i = 0; i < 12;i++)
	{
		// check if the data block is being used
		if(mip->INODE.i_block[i])
		{	
			// Read data into buf.
			get_block(mip->dev,mip->INODE.i_block[i], buf);
			// Set up dp base on buf.
			dp = (DIR*)buf;
			cp = buf;

			// skip the . and ..
			cp += dp->rec_len;
			dp = (DIR *) cp;
			cp += dp->rec_len;
			dp = (DIR *) cp;

			// loop to the end of the file
			while ((((char*)dp - buf) < 1024))
			{
				// print out the name and
				// iterate to the next entry
				printf("  %s", dp->name);
				cp += dp->rec_len;
				dp = (DIR *) cp;
			}
		}
	}
	printf("\n");	
}

// writes to a file
int mywrite(OFT * fdcurrent, char * param)
{
	// check if the fd is opened for writing mode
	if(fdcurrent->mode == 0)
	{
		printf("File is not in write mode!\n");
		return;
	}
	char text[1024];
	int i;

	// set up the text to be written in a char arr
	if(param[0] == '\"')
	{
		for(i = 1; i < strlen(param); i++)
		{
			text[i-1] = param[i];	
		}
		text[strlen(param) - 2] = '\0';
	}
	else
	{
		for(i = 0; i < strlen(param); i++)
		{
			text[i] = param[i];	
		}
		text[strlen(param)] = '\0';
	}

	// length is the number of bytes to be written
	int length = strlen(text);
	int lbk;
	int blk;
	int start;
	MINODE * mip = fdcurrent->inodeptr;
	/*
	if(mip -> INODE.i_size == 0)
	{
		mip -> INODE.i_size = 1024;	
	}*/
	int flag = 0;
	char buf[BLKSIZE];
	char indirbuf[BLKSIZE];
	int indirect;
	char * cp;
	int remain;
	char * cq = (char *)text;
	int count = 0;

	// while there are still bytes to be written
	while(length > 0)
	{
		// calculate the logical block and start postition
		// using mailman's algorithm
		lbk = fdcurrent->offset / BLKSIZE;
		start = fdcurrent->offset % BLKSIZE;

		// convert the logical block to the physical block
		// check direct block
		if (lbk < 12)
		{
			if(mip->INODE.i_block[lbk] == 0)
			{
				mip->INODE.i_block[lbk] = balloc(mip->dev);
			}
			blk = mip->INODE.i_block[lbk];
		}

		// check for indirect or double indirect block
		else if(lbk >= 12 && lbk < 256+12)
		{
			if(flag == 0)
			{
				flag = 1;
				get_block(mip->dev,mip->INODE.i_block[12],indirbuf);
			}
			blk = buf + lbk -12;		
		}
		else
		{

		}

		// read the physical block into buf
		get_block(mip->dev,blk,buf);

		// if we're in write or read/write mode
		// reset the buf
		if(fdcurrent->mode == 1 && fdcurrent->mode == 2)
		{
			memset(buf,0,1024);
		}
		
		// OPTIMIZED!
		cp = buf + start;
		remain = BLKSIZE - start;
		while (remain > 0)
		{
			// cq hold the text that we want to write
			// just append it to the text in the file
			strcat(cp,cq);

			// calculate the new offset
			fdcurrent->offset = fdcurrent->offset + length;
			count = count + length;

			if(fdcurrent->offset > mip->INODE.i_size)
			{
				// set the new size of the file
				mip->INODE.i_size = mip->INODE.i_size + length;
			}
			remain = 0;
			length = length - strlen(cp);
			if(length <=0)
			{
				break;
			}
		}
	}

	// write the block back to the disk
	// clean up and reset the parameter
	put_block(mip->dev,blk,buf);
	memset(parameter,0,64);
	iput(mip);
	return count;
}

// reads bytes from a file
int * myread(OFT * fdcurrent, int length)
{
	// check that the mode is not in reader mode
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
	int minimum;
	
	// while there are still more bytes to read
	while(length > 0)
	{
		// calculate the logical block and starting postition
		// using mailman's algorithm
		lbk = fdcurrent->offset / BLKSIZE;
		start = fdcurrent->offset % BLKSIZE;

		// convert the logical block to a physical block
		// check for direct block
		if (lbk < 12)
		{
			if(mip->INODE.i_block[lbk] == 0)
			{
				break;
			}
			blk = mip->INODE.i_block[lbk];
		}
		else if(lbk >= 12 && lbk < 256+12)
		{
			/**
			if(flag == 0)
			{
				flag = 1;
				//get_block(mip->dev,mip->INODE.i_block[13],indirbuf);
			}
			blk = buf + lbk -12;*/
		}
		else
		{

		}

		// read the physical block into memory
		get_block(mip->dev,blk,buf);
		cq = result;
		cp = buf + start;

		// calulate the remaining space in the block
		// and the available number of bytes we can read
		remain = BLKSIZE - start;
		int avil = fdcurrent->inodeptr->INODE.i_size - start;

		// if we want to read more bytes than there are available to read
		if(avil < length)
		{
			printf("Length is higher than the remain character!\n");
			memset(parameter,0,64);
			return;
		}
		// if there are enough bytes to read	
		else if (avil >= length)
		{
			// set the minimum to the number of bytes we want to read
			minimum = length;
		}
		// while there are still bytes to read
		while (remain > 0)
		{
			// copy every byte that we want to read into cq
			strncpy(cq,cp,minimum);
			cq[minimum] = '\0';

			// set the new offset
			fdcurrent->offset = fdcurrent->offset + minimum;
			count = count + minimum;
			length = 0;

			// calculate the new remaining and available bytes
			remain = remain - minimum;
			avil = avil - minimum;
			count = count + minimum;
			if(length <=0 || avil <= 0)
			{
				break;
			}
		}
	}

	// clean up, reset the paramter, and print out the result
	iput(mip);
	printf("READED: %s\n", result);
	memset(parameter,0,64);
	return count;
}

// jumps to a new starting location in a block
void mylseek (OFT * fdNo, int seekTo)
{
    // verify the seekTo value is a valid value
    // i.e. the seekTo is not negative or greater
    // than the file size
    if (seekTo < 0 || seekTo > (fdNo -> inodeptr -> INODE.i_size))
    {
        printf ("Invalid seek value!\n");
        return -1;
    }
    // set the offset to the new position
    fdNo -> offset = seekTo;
}

// prints out the content of a file to the screen
char * cat(char * path)
{
	int ino;
	int blk;
	char buf[BLKSIZE];
	token_path(path);
	OFT * cur = myopen(name[0],0);
	MINODE * mip = running->cwd;
	ino = getino(&mip->dev,name[0]);
	mip = iget(mip->dev,ino);
	int i;

	// check that the file opened correctly and that
	// it's a regular file
	if (cur == NULL || !S_ISREG(mip->INODE.i_mode))
	{
		return;
	}
	// check if we're in the correct spot
	if(cur->inodeptr->ino == mip->ino)
	{
		// read the data into memory
		blk = mip->INODE.i_block[0];
		get_block(mip->dev,blk,buf);

		// print out the data
		printf("Inside file:\n");
		printf("%s",buf);
	}
	printf("\n");

	// close the file when we're done
	// and return the content that we printed
	close(cur);
	char * result = buf;
	return result;
}

// returns an open spot in the fd array
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

// copies a file from the source to the destination
void cp(char * path, char * des)
{
	// tokenize the path and save a copy of its origin
	token_path(path);
	char strbackuppathone[128];
	strcpy(strbackuppathone,name[0]);
	strbackuppathone[strlen(name[0])] = '\0';

	// tokenize the destination and save a copy of its origin
	token_path(des);
	char strbackupdesone[128];
	strcpy(strbackupdesone,name[0]);
	strbackupdesone[strlen(name[0])] = '\0';

	// get the inumber of the path origin
	int inos = getino(&running->cwd->dev,strbackuppathone);
	if(inos == 0)
	{
		printf("Invalid source!\n");
		return;
	}

	// get the inumber of the destination origin
	int inod = getino(&running->cwd->dev,strbackupdesone);

	// if the destination doesn't exists
	if(inod == 0)
	{
		// create a new file for it
		mycreat(strbackupdesone);
	}

	// retrieve the data from the source
	// using the cat function
	char * data = cat(strbackuppathone);

	// open the destination for writer mode
	OFT * copying = myopen(strbackupdesone,1);

	// write the data to the destination
	// and close the file
	mywrite(copying,data);
	close(copying);	
}

// moves a file to a new destination
// or renames a file
void mv(char * path, char * param)
{
	// save a copy of the destination
	char strbackupdesone[128];
	strcpy(strbackupdesone,param);
	strbackupdesone[strlen(param)] = '\0';
		
	MINODE * backup = running->cwd;
	int num;

	// check that parameters were passed in correctly
	if(path[0] == 0)
	{
		printf("No pathname for the source was given!\n");
		return;
	}
	else if(param[0] == 0)
	{
		printf("No path/name for the dest. was given!\n");
		return;
	}
	token_path(param);

	// if we're just changing the name of the file
	if(name[1] == NULL)
	{
		// save a copy of the des
		char strbackupdes[128];
		strcpy(strbackupdes,name[0]);
		strbackupdes[strlen(name[0])] = '\0';
		token_path(path);

		// link the files and delete the original
		link(name[0],strbackupdes);
		myrm(name[0]);
	}
	// otherwise we want to move a file to a new destination
	else
	{	
		token_path(path);
		char strbackuppath[128];
		strcpy(strbackuppath,name[0]);
		strbackuppath[strlen(name[0])] = '\0';
		int inos = getino(&running->cwd->dev,name[0]);
		if(inos == 0)
		{
			printf("Invalid source!\n");
			return;
		}
		char data[BLKSIZE];
		strcpy(data,cat(name[0]));
		token_path(strbackupdesone);
		int num;

		// traverse to the source
		for(int i = 0; i< 128; i++)
		{
			if(name[i] != NULL && name[i+1] != NULL)
			{
				int inotemp = getino(&running->cwd->dev,name[i]);
				MINODE * temp = iget(running->cwd->dev,inotemp);
				if(S_ISDIR(temp->INODE.i_mode))
				{
					running->cwd = temp;
					iput(temp);
				}
				else
				{
					printf("Not valid path!\n");
					return;
				}
			}
			else
			{	
				num = i;
				break;
			}
		}

		// save a copy of the destinations's name
		char strbackupdestwo[128];
		strcpy(strbackupdestwo,name[num]);
		strbackupdestwo[strlen(name[num])] = '\0';

		int inod = getino(&running->cwd->dev,name[num]);

		// if the destination doesn't already exist
		if(inod == 0)
		{
			// create file for it
			mycreat(name[num]);
		}

		// open the destination up for writing mode,
		// write the data to it, and close it when we're done
		OFT * copying = myopen(strbackupdestwo,1);
		mywrite(copying,data);
		close(copying);

		// increment the cwd's refcount,
		// restore the original current working directory
		// and remove the source file
		running->cwd->refCount++;
		running->cwd = backup;
		myrm(strbackuppath);
	}

	// reset the parameter
	memset(param,0,64);
}

// removes a file for a path
void myrm(char *path)
{	
	MINODE * pmip;
	MINODE * mip;
	int ino;
	int num;
	MINODE * backup = running->cwd;
	
	// check that a pathname was passed in correctly
	if (path[0] == 0)
	{
		printf("No pathname for a directory to remove given!\n");
		return;
	}
	token_path(path);

	// traverse to the path
	for(int i = 0; i< 128; i++)
	{
		if(name[i] != NULL && name[i+1] != NULL)
		{
			int inotemp = getino(&running->cwd->dev,name[i]);
			MINODE * temp = iget(running->cwd->dev,inotemp);

			// go into each dir
			if(S_ISDIR(temp->INODE.i_mode))
			{
				running->cwd = temp;
			}
		}
		else
		{	
			num = i;
			break;
		}
	}

	// load the MINODE into memory
	ino = getino(&running->cwd->dev,name[num]);
	mip = iget(running->cwd->dev,ino);

	// check for a regular file
	if(!S_ISREG(mip->INODE.i_mode))
	{
		//printf("\t\t\t\t\tLINE 9\n");
		printf("Invalid pathname!\n");
		iput(mip);
		return;
	}
	// look in the fd array for files in use
	for(int i = 0; i < 10; i++)
	{
		if(currentfd != NULL && currentfd->inodeptr == mip)
		{
			printf("File is being used!\n");	
			return;
		}
	}
	// check if the file is linked
	if(mip->INODE.i_links_count > 2)
	{
		printf("File is linked!\n");
		iput(mip);
		return;
	}
	// check if the file exists
	else if(ino == 0)
	{
		printf("File not found!\n");
		iput(mip);
		return;
	}

	// deallocate the inode and clean it up
	idealloc(mip->dev,mip->ino);
	iput(mip); 

	// load the parent MINODE into memory and remove the child from it
	pmip = iget(running->cwd->dev,running->cwd->ino);
	rm_child(pmip,name[num]);

	// update the parent and clean up
	pmip->INODE.i_links_count--; 
	pmip->INODE.i_atime = pmip->INODE.i_mtime = time(0L); 
	pmip->dirty = 1; 
	iput(pmip); 

	// restore the original current working directory
	running->cwd = backup;
}

int quit()
{
	// Quit.
	for(int i =0; i < NMINODES; i++)
	{
		// clean up any dirty MINODES
		if(minode[i]->refCount > 0 && minode[i]->dirty ==1)
		{
			iput(minode[i]);
			minode[i] = 0;
		}
	}
	printf("Quit!\n");
	exit(0);
}
