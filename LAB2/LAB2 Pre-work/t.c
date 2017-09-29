#include <stdio.h>
#include <stdlib.h>

int *FP;

main(int argc, char *argv[ ], char *env[ ])
{
  int a,b,c;
  printf("enter main\n");

  
  // PRINT ADDRESS


  a=1; b=2; c=3;
  A(a,b);
  printf("exit main\n");
}

int A(int x, int y)
{
  int d,e,f;
  printf("enter A\n");


  // PRINT ADDRESS OF d, e, f


  d=4; e=5; f=6;
  B(d,e);
  printf("exit A\n");
}

int B(int x, int y)
{
  int g,h,i;
  printf("enter B\n");
  // PRINT ADDRESS OF g,h,i
  printf("&g=%8x &h=%8x &i=%8x\n", &g, &h, &i);
  g=7; h=8; i=9;
  C(g,h);
  printf("exit B\n");
}

int C(int x, int y)
{
  int u, v, w, i, j, *p;
  int *fp0, k;

  printf("enter C\n");
  // PRINT ADDRESS OF u,v,w,i,p;
  printf("&u=%8x &v=%8x &w=%8x &i=%8x p=%8x\n", &u, &v, &w, &i, p);
  u=10; v=11; w=12; i=13, p=14;

  p = (int *)&p;
  printf("p = %8x\n", p);

  asm("movl %ebp, FP"); // set FP=CPUâ€™s %ebp register

  /*********** Write C code to DO ************
  1 to 4 AS SPECIFIED in the PROBLEM 3 of the TEXT
  ********************************************/
  /**
   *  (1). Print the stack frame link list.
   */
  printf("<stack frame link list>\n");
  p = FP;
  while (p != NULL)
  {
     printf ("%p\n", (void*) &p);
     p++;
  }
  printf("fp = %8x\n", p);
  printf("</stack frame link list>\n");

  /**
   * (2). Print the stack contents from p to the frame of main()
   *    YOU MAY JUST PRINT 128 entries of the stack contents.
   */

  printf("The 128 entries of the stack contents starting from main()\n");
     /* ===== Add your code here. ===== */
	for (j = 0; j <= 128; j++)
	{
		printf ("%8x\n", fp0);
		fp0++;
	}


  /**
   * (3). On a hard copy of the print out, identify the stack contents
   *    as LOCAL VARIABLES, PARAMETERS, stack frame pointer of each function.
   */

  /* Annotate the outfile to indicate the answer. */
  

  /**
   * (4). Find where are argc, argv and env located in the stack.
   *    What is exactly argv?
   */

  /**
   * PUT YOUR ANSWER IN THIS COMMENT AREA
   * Sort env, argv, argc,fpM by addresses.
   * _____ > _____ > ______ > ______
   * So they are in ahead of the fpM.
   * argv is _____ _____ _____ ______
   * , where argv[0] = 
   * ,       argv[1] = 
   * ,       argv[2] = 
   * ,       argv[3] = 
   */
}
