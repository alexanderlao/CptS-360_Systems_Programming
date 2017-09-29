// Alexander Lao
// CptS 360
// LAB1

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

struct partition {
	unsigned char drive;             /* 0x80 - active */

	unsigned char  head;             /* starting head */
	unsigned char  sector;           /* starting sector */
	unsigned char  cylinder;         /* starting cylinder */

	unsigned char  sys_type;         /* partition type */

	unsigned char  end_head;         /* end head */
	unsigned char  end_sector;       /* end sector */
	unsigned char  end_cylinder;     /* end cylinder */

	unsigned int start_sector;     /* starting sector counting from 0 */
	unsigned int nr_sectors;       /* number of of sectors in partition */
};

void printPartitions (struct partition *p);	// foward declaration of the function

// global variables used for reading from the vdisk
char buf[512];
int sector;
int fd, r;

main()
{
   struct partition *p;			  // local variable to hold the partition
   sector = 10;                           // sector # 10 (count from 0)  

   // reading from the vdisk
   fd = open("vdisk", O_RDONLY);          // open vdisk for READ           
   r = read(fd, buf, 512);                // read 512 bytes into buf
   p = (struct partition *)(buf + 0x1BE); // set p as the partition table
   printPartitions (p);			  // call the printPartitions function on p
   close(fd);                             // close opened file
}

void printPartitions (struct partition *p)
{
	// local variables to hold relevent data
	unsigned char totalHeads = 0;
	unsigned char totalCylinders = 0;
	unsigned int totalSectors = 0;
	int count = 1;

	printf ("-----RAW FORM-----\n");
	
	// loop through the first four partitions
	while (p -> drive == 0)
	{
		totalHeads += p -> head;
		totalCylinders += p -> cylinder;
		totalSectors += p -> nr_sectors;
		
		// if we're not in an extended disk
		if (count <= 4)
		{
			printf ("\n******vdisk %d******\n", count);
		}	

		// print out the relevant information
		printf ("drive: %u\n", p -> drive);
		printf ("head: %u\n", p -> head);
		printf ("sector: %u\n", p -> sector);
		printf ("cylinder: %u\n", p -> cylinder);
		printf ("sys_type: %u\n", p -> sys_type);
		printf ("end_head: %u\n", p -> end_head);
		printf ("end_sector: %u\n", p -> end_sector);
		printf ("end_cylinder: %u\n", p -> end_cylinder);
		printf ("start_sector: %u\n", p -> start_sector);
		printf ("nr_sectors: %u\n\n", p -> nr_sectors);
		
		// looking for extended partitions
		if (p -> sys_type == 5)
		{
			// only print this once
			if (count == 4)
			{
				printf ("******Extended Partitions******\n");
			}

			// seek to the next extended partition
			lseek(fd, (long)(p -> start_sector)*512, SEEK_SET);
			r = read(fd, buf, 512);
  			p = (struct partition *)(buf + 0x1BE);
			
			printf ("drive: %u\n", p -> drive);
			printf ("head: %u\n", p -> head);
			printf ("sector: %u\n", p -> sector);
			printf ("cylinder: %u\n", p -> cylinder);
			printf ("sys_type: %u\n", p -> sys_type);
			printf ("end_head: %u\n", p -> end_head);
			printf ("end_sector: %u\n", p -> end_sector);
			printf ("end_cylinder: %u\n", p -> end_cylinder);
			printf ("start_sector: %u\n", p -> start_sector);
			printf ("nr_sectors: %u\n\n", p -> nr_sectors);
		}
		
		// increment to the next partition
		p++;
		count++;
	}
	
	printf ("-----LINUX FORM-----\n\n");
	printf ("%u heads, %u cylinders, total %u sectors\n", 
		totalHeads, totalCylinders, totalSectors);
}
