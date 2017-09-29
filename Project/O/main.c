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
		strcpy(rootdev,"mydisk");
		rootdev[strlen("mydisk")] = '\0';
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
			case 3: mypwd(running->cwd); break;
			case 4: mymkdir(pathname); break;
			case 5: myrmdir(pathname); break;
			case 6: mycreat(pathname); break;
			case 7: link(pathname,parameter); break;
			case 8: unlink(pathname); break;
			case 9: mysymlink(pathname,parameter); break;
			case 10: readlink(pathname); break;
			case 11: mychmod (pathname, parameter); break;
			case 12: mytouch (pathname); break;
			case 13: myopen (pathname, atoi (parameter)); break;
			case 14: pfd (); break;
			case 15: quit(); break;
			default: printf("Invalid command\n"); break;
		}
		pathname[0] = 0;
  	}
	return 0;
}
