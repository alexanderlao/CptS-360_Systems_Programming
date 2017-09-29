#include "mysh.h"

main (int argc, char *argv[], char *env[])
{
	char** pathDirs;
	char* dirs = (char*)malloc (sizeof (char) * strlen (env[21]) + 1);
	int i = 0;

	strcpy (dirs, env[21]);
	dirs += 5;			// eliminate the "PATH=" from env[21]
	pathDirs = parseDirs (dirs);
	
	printf ("1. PATH:\n %s\n\n", env[21]);
	printf ("2. Decompose PATH into directory strings:\n");

	while (pathDirs[i] != NULL)
	{
		printf ("%s  ", pathDirs[i]);
		i++;
	}	
	
	printf ("\n\n3. HOME:\n %s\n", env[32]);
	
	runSH (argc, pathDirs, env);
}
