#include "MyPrint.h"

int mymain(int argc, char *argv[ ], char *env[ ])
{
   int i = 0;
   int j = 0;
   
   myprintf("in mymain(): argc = %d\n", argc);

   while (argv[i])
   {
	myprintf ("argv[%d] = %s\n", i, argv[i]);
	i++;
   }

   while (env[j])
   {
	if (strncmp (env[j], "PATH", strlen ("PATH")) == 0 || 
	    strncmp (env[j], "HOME", strlen ("HOME")) == 0)
	{
		myprintf ("env[%d] = %s\n", j, env[j]);
	}

	j++;
   }

   myprintf("---------- testing YOUR myprintf() ---------\n");
   myprintf("this is a test\n");
   myprintf("testing a=%d b=%x c=%c s=%s\n", 123, 123, 'a', "testing");
   myprintf("string=%s, a=%d  b=%u  c=%o  d=%x\n", 
	    "testing string", -1024, 1024, 1024, 1024);
   myprintf("mymain() return to main() in assembly\n"); 
}
