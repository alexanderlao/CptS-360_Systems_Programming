#include "mysh.h"

void runSH (int argc, char** pathDirs, char* env[])
{
	int running = 1;
	char line[MAX];
	char* checkPath = (char*)malloc (MAX);
	char* copyDir = (char*)malloc (MAX);
	char** inputArray;
	int i = 0, j = 0;
	pid_t pid;
	int pd[2];
	int status;

	printf ("********************** Welcome to mysh **********************\n");
	
	while (running)
	{
		printf ("mysh %: ");
	
		// reset the line to process a new command,
		// read in the line from stdin that ends 
		// with the null-terminating character,
		// kill the null-terminating character so
		// it doesn't mess up the parse
		bzero(line, MAX); 
    		fgets(line, MAX, stdin);
		line[strlen (line) - 1] = 0;
		printf ("\n");
		
		// parse the line and store it in the inputArray
		inputArray = parseInput (line);

		// update argc
		for (argc = 0; inputArray[argc] != NULL; argc++);
	
		// cd $HOME if arg1 was not provided
		if (strcmp (inputArray[0], "cd") == 0 && inputArray[1] == '\0')
	    	{	
			chdir (getenv ("HOME"));
			printf("The current working directory: %s\n\n", getcwd (cwd, MAX));
		}
		// cd to arg1
		else if (strcmp (inputArray[0], "cd") == 0 && inputArray[1] != '\0')
		{
			shcd (inputArray);
		}
		else if (strcmp (inputArray[0], "exit") == 0)
		{
			running = 0;
			break;
		}
		else
		{
			for (i = 0; i < argc; i++)
			{
				if (strcmp (inputArray[i], "<") == 0)
				{
					// handle reading inputs here
					// fork the child process
					pid = fork ();
					if (pid) {}
					else
					{
						inputArray[i] = NULL;
						close (0);
						open (inputArray[i + 1], O_RDONLY | O_CREAT);
						if (fork () == 0)
						{
							execve (strcat ("/bin/", inputArray[0]), inputArray, env);
						}
					}
					break;
				}
				else if (strcmp (inputArray[i], ">") == 0)
				{
					// handle sending inputs here
					// fork the child process
					pid = fork ();
					if (pid) {}
					else
					{
						inputArray[i] = NULL;
						close (1);
						open (inputArray[i + 1], O_WRONLY | O_CREAT);
						if (fork () == 0)
						{
							execve (strcat ("/bin/", inputArray[0]), inputArray, env);
						}
					}
					break;
				}
				else if (strcmp (inputArray[i], ">>") == 0)
				{
					// handle appending inputs here
					// fork the child process
					pid = fork ();
					if (pid) {}
					else
					{
						inputArray[i] = NULL;
						close (1);
						open (inputArray[i + 1], O_APPEND | O_CREAT);
						if (fork () == 0)
						{
							execve (strcat ("/bin/", inputArray[0]), inputArray, env);
						}
					}
					break;

				}
				else if (strcmp (inputArray[i], "|") == 0)
				{
					// handle pipes here
					inputArray[i] = NULL;
					pipe (pd);
					pid = fork ();
					if (pipe (pd) == -1)
					{
						perror ("pipe");
						exit (EXIT_FAILURE);
					}
	
					if (pid)	// parent as pipe writer
					{
						close(pd[0]);		// WRITER MUST close pd[0]
						close(1);    		// close 1 SEGMENTATION FAULT HERE
						dup2(pd[1], 1);  	// replace 1 with pd[1]
						execve (strcat ("/bin/", inputArray[i - 1]), inputArray, env);
					}
					else		// child as pipe reader
					{
						close(pd[1]); 		// READER MUST close pd[1]
						close(0);  
						dup2(pd[0], 0);   	// replace 0 with pd[0]
						execve (strcat ("/bin/", inputArray[i + 1]), inputArray, env);
					}	
					break;
				}
			}
			
			// fork the child process
			pid = fork ();

			// fork() may fail. e.g. no more PROC in Kernel
			if (pid < 0)
			{
	       			perror("Fork failed!");
	       			exit(1);
	   		}
	   		
			// PARENT EXECUTES THIS PART 
	  		if (pid)
			{
	          		printf("PARENT %d WAITS FOR CHILD %d TO DIE\n", getpid(),pid);
	  			pid = wait (&status);
	   			printf("DEAD CHILD = %d, HOW = %04x\n", pid, status);
	  		}
			// child executes this part
	   		else
			{
	         		printf("I am %d and my parent = %d\n", getpid(), getppid());
			
				// for each path directory
				for (i = 0; i <= 7; i++)
				{
					strcpy (checkPath, inputArray[0]);	// copy the command to checkPath
					strcat (pathDirs[i], "/");		// append a / to the pathDirs
					strcpy (copyDir, pathDirs[i]);		// copy the /-appended dirs to copyDir
					strcat (copyDir, checkPath);		// append the command to copyDir

					// try executing the image of the new path
					execve (copyDir, inputArray, env);

					if (execve (copyDir, inputArray, env) == -1)
					{
						printf ("Not in: %s\n", copyDir);
					}
				}

				printf("Child %d dies by exit(VALUE)\n", getpid());
				EXIT: exit(100);
	  		}
		}
	}
}

char** parseInput(char* input)
 {
	int count = 0;
	char *str = NULL;
	char *tmpStr = NULL;

	// Allocate char* inputArr[20]
	char** inputArr = (char**)malloc(sizeof(char*)*20);
	str = strtok(input, " ");

	while (str)
	{
		tmpStr = (char *)malloc(sizeof(char)*strlen(str));
		strcpy(tmpStr, str);
		inputArr[count] = tmpStr;
		count++;
		str = strtok(NULL, " ");
	}

	inputArr[count] = NULL;
	return inputArr;
}

char** parseDirs (char* input)
{
	int count = 0;
	char *str = NULL;
	char *tmpStr = NULL;

	// Allocate char* inputArr[20]
	char** inputArr = (char**)malloc(sizeof(char*)*20);
	str = strtok(input, ":");

	while (str)
	{
		tmpStr = (char *)malloc(sizeof(char)*strlen(str));
		strcpy(tmpStr, str);
		inputArr[count] = tmpStr;
		count++;
		str = strtok(NULL, ":");
	}

	inputArr[count] = NULL;
	return inputArr;
}

void shcd(char **inputArray)
{
	chdir (inputArray[1]);
	printf("The current working directory: %s\n\n", getcwd (cwd, MAX));
}
