#include "MyPrint.h"

// print a single character
void printc (char c)
{
	putchar (c);
}

// print a string
void prints (char* s)
{
	// loop through s while the char is not null
	while (*s)
	{
		// print the character
		putchar (*s);
		s++;
	}
}

// helper function for printing 
// an unsigned int
int rpu (unsigned int u, int BASE)
{
	char* table = "0123456789ABCDEF";
	char c;
	
	if (u)
	{
		c = table[u % BASE];
		rpu (u / BASE, BASE);
		putchar (c);
	}
}

// print an unsigned int
int printu (unsigned int u)
{
	// check for 0
	if (u == 0)
	{
		putchar ('0');
	}
	else
	{
		rpu (u, 10);
	}
	
	putchar (' ');
}

// print an int
void printi (int d)
{
	// check if d is negative
	if (d < 0)
	{
		putchar ('-');
		d = -d;
	}
	
	// check if d contains more
	// than one digit
	if (d > 9)
	{
		printi (d / 10);
	}
	
	putchar ('0' + (d % 10));
}

// print an unsigned int in oct
void printo (unsigned int d)
{
	if (d != 0)
	{
		printo (d / 8);
		printi (d % 8);
	}
}

// print an unsigned int in hex
void printh (unsigned int x)
{
	// check for 0
	if (x == 0)
	{
		putchar ('0');
	}
	else
	{
		rpu (x, 16);
	}
	
	putchar (' ');	
}

void myprintf (char* fmt, ...)
{
	char* cp = fmt;
	int* ip = &fmt + 1;

	// loop through the string
	while (*cp)
	{	
		// check for the %
		if (*cp == '%')
		{	
			// increment to the 
			// relevant character
			cp++;
			
			// call the appropriate print
			// function based on the
			// relevant char
			if (*cp == 'c') printc ((char)*ip);
			else if (*cp == 's') prints ((char*)*ip);
			else if (*cp == 'u') printu ((unsigned int)*ip);
			else if (*cp == 'd') printi ((int)*ip);
			else if (*cp == 'o') printo ((unsigned int)*ip);
			else if (*cp == 'x') printh ((unsigned int)*ip);
	
			// increment ip to the next item on the stack
			// increment cp to the next char after the relevant char
			ip++;
			cp++;
		}
		else
		{
			// print the irrelevant char and
			// increment to the next one
			putchar (*cp);		
			cp++;
		}
	}
}
