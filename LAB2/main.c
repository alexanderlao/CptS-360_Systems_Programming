#include "MyPrint.h"

int main (void)
{
	char c = 'q';
	//printc (c);

	char* string = "Hello World!\n";
	//prints (string);

	unsigned int u = 0xFFFFFFFF;
	unsigned int q = 55555;
	//printu (u);

	int d = 123;
	//printi (d);

	//printf ("int: %x\n", q);

	//printo (q);
	//printh (q);

	//myprintf("testing a=%d", 123);
	//myprintf("testing a=%d b=%x c=%c s=%s\n", 123, 123, 'a', "testing");
	//myprintf("testing a=%d b=%x\n", 123, 123);
	myprintf("string=%s, a=%d  b=%u  c=%o  d=%x\n",
          	 "testing string", -1024, 1024, 1024, 1024);
}
