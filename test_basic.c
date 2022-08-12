/*
 * Test Cases 1 - Basic
 * CPS 310 (OS) Lab 1 - Heap Manager
 * Author: Nisarg Raval
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dmm.h"

// PFL prints the free list if DBFLAG is set
//#define DBFLAG

#ifdef DBFLAG
#define PFL print_freelist()
#else
#define PFL
#endif

#define NB 10 

//Test Case 1: Allocate memory - fill content - free memory
// return 0 if fails and 1 if passes
int test_case1() {
	printf("TC1: Start\n");
	fflush(stdout);
	
	//allocate NB bytes
	char *ptr = (char*) dmalloc(NB);
	if (!ptr) {
		printf("TC1:Fail:dmalloc(%d) failed\n", NB);
		fflush(stdout);
		return 0;
	}
	printf("TC1:dmalloc(%d)\n", NB);
	fflush(stdout);
	PFL; //prints the free list if DBFLAG is set

	//fill NB bytes with a
	int i;
	for (i = 0; i < NB - 1; i++)
		ptr[i] = 'a';
	ptr[NB - 1] = '\0';
	printf("TC1:%d bytes filled\n", NB);
	fflush(stdout);

	//free NB bytes
	dfree(ptr);
	printf("TC1:dfree(%d)\n", NB);
	fflush(stdout);
	PFL;

	return 1;
}

int main(int argc, char** argv) {

	clock_t begin, end;

	begin = clock();
	int rc = test_case1();
	end = clock();
	
	double time = (double) (end - begin) / CLOCKS_PER_SEC;
	if(rc == 1)
		fprintf(stderr, "TC1 passed!\n");
	//Format: TC1: Success ExecTime
	fprintf(stderr,"TC1: ExecTime\n");
	fprintf(stderr," %f\n",time);
	fflush (stderr);

	return 0;
}
