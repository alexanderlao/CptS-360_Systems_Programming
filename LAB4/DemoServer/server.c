// This is the echo SERVER server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>


#include <sys/socket.h>
#include <netdb.h>

#define  MAX 256

// Define variables:
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

int  mysock, client_sock;              // socket descriptors
int  serverPort;                     // server port number
int  r, length, n;                   // help variables

char * cwd[128];

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

// Server initialization code:

int server_init(char *name)
{
   printf("==================== server init ======================\n");   
   // get DOT name and IP address of this host

   printf("1 : get and show server host info\n");
   hp = gethostbyname(name);
   if (hp == 0){
      printf("unknown host\n");
      exit(1);
   }
   printf("    hostname=%s  IP=%s\n",
               hp->h_name,  inet_ntoa(*(long *)hp->h_addr));
  
   //  create a TCP socket by socket() syscall
   printf("2 : create a socket\n");
   mysock = socket(AF_INET, SOCK_STREAM, 0);
   if (mysock < 0){
      printf("socket call failed\n");
      exit(2);
   }

   printf("3 : fill server_addr with host IP and PORT# info\n");
   // initialize the server_addr structure
   server_addr.sin_family = AF_INET;                  // for TCP/IP
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
   server_addr.sin_port = 0;   // let kernel assign port

   printf("4 : bind socket to host info\n");
   // bind syscall: bind the socket to server_addr info
   r = bind(mysock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   if (r < 0){
       printf("bind failed\n");
       exit(3);
   }

   printf("5 : find out Kernel assigned PORT# and show it\n");
   // find out socket port number (assigned by kernel)
   length = sizeof(name_addr);
   r = getsockname(mysock, (struct sockaddr *)&name_addr, &length);
   if (r < 0){
      printf("get socketname error\n");
      exit(4);
   }

   // show port number
   serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
   printf("    Port=%d\n", serverPort);

   // listen at port with a max. queue of 5 (waiting clients) 
   printf("5 : server is listening ....\n");
   listen(mysock, 5);
   printf("===================== init done =======================\n");
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

main(int argc, char *argv[])
{
   char *hostname;
   char line[MAX];
   char **inputArray;
   char buf[MAX];
   int total = 0;

   if (argc < 2)
      hostname = "localhost";
   else
      hostname = argv[1];
 
   server_init(hostname); 

   // Try to accept a client request
   while(1){
     printf("server: accepting new connection ....\n"); 

     // Try to accept a client connection as descriptor newsock
     length = sizeof(client_addr);
     client_sock = accept(mysock, (struct sockaddr *)&client_addr, &length);
     if (client_sock < 0){
        printf("server: accept error\n");
        exit(1);
     }
     printf("server: accepted a client connection from\n");
     printf("-----------------------------------------------\n");
     printf("        IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr),
                                        ntohs(client_addr.sin_port));
     printf("-----------------------------------------------\n");

     // Processing loop: newsock <----> client
     while(1){
       n = read(client_sock, line, MAX);
       if (n==0){
           printf("server: client died, server loops\n");
           close(client_sock);
           break;
      }

	// show the line string
      	printf("server: read  n=%d bytes; line=[%s]\n", n, line);

	inputArray = parseInput(line);
	printf("\n");
	if(strcmp(inputArray[0],"pwd") == 0)
    	{
		getcwd(cwd,MAX);
		sprintf(line,"Current Working Directory = %s", cwd);
		write(client_sock,line,MAX);
    	}
	else if(strcmp(inputArray[0],"ls") == 0)
	{
		n = write(client_sock,getcwd(cwd,MAX),MAX);
		sels();
	}
	else if(strcmp(inputArray[0],"mkdir") == 0)
	{
		semkdir(inputArray);
	}
	else if(strcmp(inputArray[0],"rmdir") == 0)
	{
		sermdir(inputArray);
	}
	else if(strcmp(inputArray[0],"rm") == 0)
	{
		serm(inputArray);
	}
	else if(strcmp(inputArray[0],"cd") == 0)
	{
		secd(inputArray);
	}
	else if(strcmp(inputArray[0],"get") == 0)
	{
		char data[MAX];
		FILE * sourceFD = fopen(inputArray[1],"r");
		while(fgets(data,MAX,sourceFD) != NULL)
		{
			write(client_sock,data,MAX);
		}
		write(client_sock,"end",MAX);		
		fclose(sourceFD);
		printf("File Copied!");	
	}
	else if(strcmp(inputArray[0],"put") == 0)
	{
		char data[MAX];
		FILE * destFD = fopen(inputArray[1], "w+");
		if(destFD != NULL)
		{
			write(client_sock,"Open Success!",MAX);
			read(client_sock,data,MAX);
			while(strcmp(data,"end") != 0)
			{
				fputs(data,destFD);
				read(client_sock,data,MAX);
			}
		}
		printf("File Copied!\n\n");	
		fclose(destFD);	
	}

      printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
      printf("server: ready for next request\n");
    }
 }
}

void semkdir(char **inputArray)
{
	if(mkdir(inputArray[1], 0777) < 0)
	{
		n = write(client_sock,"Create Directory Failed",MAX);
	}
	else
	{
		char result[MAX];
		result[0] = '\0';
		strcat(result,"Created directory ");
		result[strlen(result)] = '\0';
		strcat(result,inputArray[1]);
		result[strlen(result)] = '\0';
		strcat(result,"\n");
		result[strlen(result)] = '\0';
		n = write(client_sock,result,MAX);
	}
}

void sermdir(char **inputArray)
{
	if(rmdir(inputArray[1], 0777) < 0)
	{
		n = write(client_sock,"Remove Directory Failed",MAX);
	}
	else
	{
		char result[MAX];
		result[0] = '\0';
		strcat(result,"Removed directory ");
		result[strlen(result)] = '\0';
		strcat(result,inputArray[1]);
		result[strlen(result)] = '\0';
		strcat(result,"\n");
		result[strlen(result)] = '\0';
		n = write(client_sock,result,MAX);
	}
}

void serm(char **inputArray)
{
	if(remove(inputArray[1]) < 0)
	{
		n = write(client_sock,"Error\n\n",MAX);
	}
	else
	{
		char result[MAX];
		result[0] = '\0';
		strcat(result,"Removed directory ");
		result[strlen(result)] = '\0';
		strcat(result,inputArray[1]);
		result[strlen(result)] = '\0';
		strcat(result,"\n");
		result[strlen(result)] = '\0';
		n = write(client_sock,result,MAX);
	}
}

void secd(char **inputArray)
{
	chdir(inputArray[1]);
	char result[MAX];
	result[0] = '\0';
	strcat(result,"The current working directory: ");
	result[strlen(result)] = '\0';
	strcat(result,getcwd(cwd,MAX));
	result[strlen(result)] = '\0';
	strcat(result,"\n");
	result[strlen(result)] = '\0';
	n = write(client_sock,result,MAX);
}

void sels()
{
	DIR * dir;
	struct dirent * myfile;
	struct stat mystat, *sp;
	char result[MAX];
	result[0] = '\0';
	

	dir = opendir(getcwd(cwd,MAX));
	if(dir == NULL)
	{
		strcat(result,"error");
		result[strlen(result)] = '\0';
	}
	else
	{
		while(myfile = readdir(dir))
		{
			memset(result,0,MAX);
			sp = &mystat;
			int r, i;
			char ftime[64];

			if((r = lstat(myfile->d_name,&mystat))<0)
			{
				strcat(result,"error ");
				result[strlen(result)] = '\0';
				strcat(result,myfile->d_name);
				result[strlen(result)] = '\0';
				exit(1);
			}
			// RWX
			for (i=8; i >= 0; i--)
			{
  				if (sp->st_mode & (1<<i))
  				{
					append(result,t1[i]);
  				}
  				else
  				{
					append(result,t2[i]);
  				}
			}
			char convertedIntLink[5];
			sprintf(convertedIntLink,"%4d ", sp->st_nlink);
			strcat(result,convertedIntLink);
			result[strlen(result)] = '\0';			
			
			char convertedIntGID[5];
			sprintf(convertedIntGID,"%4d ", sp->st_gid);
			strcat(result,convertedIntGID);
			result[strlen(result)] = '\0';

			char convertedIntUID[5];
			sprintf(convertedIntUID,"%4d ", sp->st_uid);
			strcat(result,convertedIntUID);
			result[strlen(result)] = '\0';

			char convertedIntSIZE[9];
			sprintf(convertedIntSIZE,"%8d ", sp->st_size);
			strcat(result,convertedIntSIZE);
			result[strlen(result)] = '\0';

			//print time
			char time[27];
			strcpy(ftime, ctime(&sp->st_ctime));
			ftime[strlen(ftime)-1] = 0;
			strcat(result,ftime);
			result[strlen(result)] = '\0';

			//print name
			strcat(result," ");
			result[strlen(result)] = '\0';
			strcat(result,myfile->d_name);
			result[strlen(result)] = '\0';
			printf("ls_file ./%s\n", myfile->d_name);
			write(client_sock,result,MAX);	
		}
		printf("\n");
	}
	close(dir);
}

void append(char * s,char c)
{
	int len = strlen(s);
	s[len] = c;
	s[len + 1] = '\0';
}

