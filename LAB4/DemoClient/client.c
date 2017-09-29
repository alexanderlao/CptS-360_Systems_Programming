// The echo client client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX 256

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr; 

char * cwd[128];

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";


int server_sock, r;
int SERVER_IP, SERVER_PORT; 


// clinet initialization code

int client_init(char *argv[])
{
  printf("======= clinet init ==========\n");

  printf("1 : get server info\n");
  hp = gethostbyname(argv[1]);
  if (hp==0){
     printf("unknown host %s\n", argv[1]);
     exit(1);
  }

  SERVER_IP   = *(long *)hp->h_addr;
  SERVER_PORT = atoi(argv[2]);

  printf("2 : create a TCP socket\n");
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock<0){
     printf("socket call failed\n");
     exit(2);
  }

  printf("3 : fill server_addr with server's IP and PORT#\n");
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = SERVER_IP;
  server_addr.sin_port = htons(SERVER_PORT);

  // Connect to server
  printf("4 : connecting to server ....\n");
  r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
  if (r < 0){
     printf("connect failed\n");
     exit(1);
  }

  printf("5 : connected OK to \007\n"); 
  printf("---------------------------------------------------------\n");
  printf("hostname=%s  IP=%s  PORT=%d\n", 
          hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
  printf("---------------------------------------------------------\n");

  printf("========= init done ==========\n");
}

char** parseInput(char* input)
 {
    int count = 0;
    char *str = NULL;
    char *tmpStr = NULL;
    // Allocate char* inputArr[20]
    char** inputArr = (char**)malloc(sizeof(char*)*20);
    str = strtok(input, " ");
    while(str)
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

main(int argc, char *argv[ ])
{
  int n;
  char line[MAX], ans[MAX];
  char **inputArray;

  if (argc < 3){
     printf("Usage : client ServerName SeverPort\n");
     exit(1);
  }

  client_init(argv);
  // sock <---> server
  printf("********  processing loop  *********\n");
  while (1){
    printf("input a line : ");
    bzero(line, MAX);                // zero out line[ ]
    fgets(line, MAX, stdin);         // get a line (end with \n) from stdin
    char temp[MAX];

    line[strlen(line)-1] = 0;        // kill \n at end
    strcpy(temp,line);
    if (line[0]==0)                  // exit if NULL line
       exit(0);
   
     	/***Local process ***/
    	inputArray = parseInput(line);
	printf("\n");
	if(strcmp(inputArray[0],"lpwd") == 0)
    	{
		getcwd(cwd,MAX);
		printf("%s\n", cwd);
    	}
	else if(strcmp(inputArray[0],"lls") == 0)
	{
		localls();
	}
	else if(strcmp(inputArray[0],"lmkdir") == 0)
	{
		localmkdir(inputArray);
	}
	else if(strcmp(inputArray[0],"lrmdir") == 0)
	{
		localrmdir(inputArray);
	}
	else if(strcmp(inputArray[0],"lrm") == 0)
	{
		localrm(inputArray);
	}
    	else if(strcmp(inputArray[0],"lcat") == 0)
    	{
		localcat(inputArray);
    	}
	else if(strcmp(inputArray[0],"lcd") == 0)
	{
		localcd(inputArray);
	}
	else
	{	
		n = write(server_sock, temp, MAX);	// 1
    		printf("client: wrote n=%d bytes; line=(%s)\n\n", n, temp);
		n = read(server_sock,ans,MAX);		// 2
		printf ("ans: %s", ans);
		if(strcmp(inputArray[0],"ls") == 0)
		{
			serverls(ans);
			//printf("client: read  n=%d bytes; echo=\n(%s)\n",n, ans);
		}
		else if(strcmp(inputArray[0],"get") == 0)
		{
			char data[MAX];
			FILE * destFD = fopen(inputArray[1], "w+");
			if(destFD != NULL)
			{
				read(server_sock,data,MAX);
				while(strcmp(data,"end") != 0)
				{
					fputs(data,destFD);
					read(server_sock,data,MAX);
				}
			}
			printf("File Copied!\n\n");	
			fclose(destFD);	
		}
		else if(strcmp(inputArray[0],"put") == 0)
		{
			char data[MAX];
			FILE * sourceFD = fopen(inputArray[1],"r");
			while(fgets(data,MAX,sourceFD) != NULL)
			{
				write(server_sock,data,MAX);
			}
			write(server_sock,"end",MAX);		
			fclose(sourceFD);
			printf("File Copied!\n\n");	
		}
		else
		{	
			printf("%s\n",ans);
		}
   
	}
  }
}

void localmkdir(char **inputArray)
{
	if(mkdir(inputArray[1], 0777) < 0)
	{
		printf("Error\n\n");
	}
	else
	{
		printf("Created directory %s\n\n", inputArray[1]);
	}
}

void localrmdir(char **inputArray)
{
		printf("Error\n\n");
	}
	else
	if(rmdir(inputArray[1], 0777) < 0)
	{
	{
		printf("Removed directory %s\n\n", inputArray[1]);
	}
}

void localrm(char **inputArray)
{
	if(remove(inputArray[1]) < 0)
	{
		printf("Error\n\n");
	}
	else
	{
		printf("Removed %s\n\n", inputArray[1]);
	}
}

void localcd(char **inputArray)
{
	chdir(inputArray[1]);
	printf("The current working directory: %s\n\n",getcwd(cwd,MAX));
}

void localcat(char **inputArray)
{
	FILE* fr = fopen(inputArray[1], "r");
	char buf[1024];
	if(!fr)
	{
		printf("Invalid filename\n");
		return -1;
	}
	while(fgets(buf, 1024, fr) != NULL)
	{
		printf("%s", buf);
	}
	fclose(fr);
}

void serverls(char * servercwd)
{
	DIR * dir;
	struct dirent * myfile;
	struct stat mystat, *sp;
	char sans[MAX];

	dir = opendir(servercwd);
	if(dir == NULL)
	{
		perror("Error");
	}
	else
	{
		while(myfile = readdir(dir))
		{
			read(server_sock,sans,MAX);
			printf("%s\n",sans);

		}
		printf("\n");
	}
	close(dir);	
}

void localls()
{
	DIR * dir;
	struct dirent * myfile;
	struct stat mystat, *sp;
	
	char buf[512];
	dir = opendir(getcwd(cwd,MAX));
	if(dir == NULL)
	{
		perror("Error");
	}
	else
	{
		while(myfile = readdir(dir))
		{
			sp = &mystat;
			int r, i;
			char ftime[64];

			if((r = lstat(myfile->d_name,&mystat))<0)
			{
				printf("error %s",myfile->d_name);
				exit(1);
			}
			// RWX
			for (i=8; i >= 0; i--)
			{
  				if (sp->st_mode & (1<<i))
  				{
    					printf("%c", t1[i]);
  				}
  				else
  				{
    					printf("%c", t2[i]);
  				}
			}
			printf("%4d ", sp->st_nlink);
			printf("%4d ", sp->st_gid);
			printf("%4d ", sp->st_uid);
			printf("%8d ", sp->st_size);

			//print time
			strcpy(ftime, ctime(&sp->st_ctime));
			ftime[strlen(ftime)-1] = 0;
			printf("%26s  ",ftime);

			//print name
			printf("%s\n", myfile->d_name);

		}
		printf("\n");
	}
	close(dir);	
}
