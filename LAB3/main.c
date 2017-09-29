#include "showblock.h"

main(int argc, char *argv[ ])
{
   char* disk = "mydisk";
   char* split;
   int i = -1, j = 0;
   int inode;

   // open the disk to read
   fd = open(disk, O_RDONLY);

   // check if the disk was opened successfully
   // -1 is unsuccessful
   if (fd < 0)
   {
     printf("open %s failed\n", disk);
     exit(1);
   }

   printf ("fd = %d\n", fd);

   // split the path name based on the /
   split = strtok (argv[1], "/");
   i++;

   printf ("split = %s\n", split);

   // loop to continue the split
   while (split != NULL)
   {
	printf ("search for directory: %s\n", split);
        inode = search (split);
	
	if (inode < 0)
	{
		printf ("cannot find %s\n", split);
		break;
	}

	displayInfo (inode);
	split = strtok (NULL, "/");
	i++;
   } 

   printf ("\nn = %d\n\n", i);
}
