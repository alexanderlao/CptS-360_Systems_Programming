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
			temp -> INODE.i_mode |= (1 << x);
		}
		else if (cur == '2' || cur == '3' || cur == '6' || cur == '7')
		{
			temp -> INODE.i_mode |= (1 << w);
		}
		else if (cur == '4' || cur == '5' || cur == '6' || cur == '7')
		{
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
