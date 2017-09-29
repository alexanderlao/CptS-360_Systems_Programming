#include "type.h"
#include "function.h"
#include "utility.h"

int main(int argc,char * argv[])
{
	int i,cmd; 
  	char line[128], cname[64];
	char copy[64];
	if(argc > 1)
	{
		strcpy(rootdev,argv[1]);
		rootdev[strlen(argv[1])] = '\0';
	}
	else
	{
		strcpy(rootdev,"disk");
		rootdev[strlen("disk")] = '\0';
	}
	strcpy(copy,rootdev);
	// Open the device.
	dev = open(rootdev,O_RDWR);
  	init();
	mount_root(copy);
  	while(1)
	{
		printf("input command : ");
      		gets(line);
		if(line[0] == 0) continue;
		sscanf(line, "%s %s %64c", cname, pathname, parameter);
		cmd = findCmd(cname);
		switch(cmd)
		{
			case 0: ls(); break;
			case 1: cd(pathname); break;
			case 2: mystat(); break;
			case 3: mypwd(); break;
			case 4: mymkdir(pathname); break;
			case 5: myrmdir(pathname); break;
			case 6: mycreat(pathname); break;
			case 7: link(pathname,parameter); break;
			case 8: unlink(pathname); break;
			case 9: mysymlink(pathname,parameter); break;
			case 10: readlink(pathname); break;
			case 11: currentfd = myopen(pathname,atoi(parameter)); break;
			case 12: pfd(); break;
			case 13: close(currentfd); break;
			case 14: mywrite(currentfd,parameter); break;
			case 15: myread(currentfd,atoi(parameter)); break;
			case 16: cat(pathname); break;
			case 17: mychmod(pathname,parameter);break;
			case 18: mytouch (pathname);break;
			case 19: cp(pathname,parameter); break;
			case 20: mylseek(currentfd,atoi(parameter)); break;
			case 21: mv(pathname,parameter); break;
			case 22: myrm(pathname); break;
			case 23: quit(); break;
			default: printf("Invalid command\n"); break;
		}
		pathname[0] = 0;
  	}
	return 0;
}
