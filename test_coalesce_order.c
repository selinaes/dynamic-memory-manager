/*
 * Test Cases 2 - Coalesce Reverse Order 
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

#define STEP 8

//Test Case 2: Allocate entire heap in multiple steps and free in reverse order
// return 0 if fails and 1 if passes
int test_case2(double *nbytes) {
	printf("TC2: Start\n");
	fflush(stdout);
	
	int size = MAX_HEAP_SIZE / STEP;
	void *ptr[STEP];
	int nb = 0;
	int i;

	//allocate size bytes (STEP-1) time
	for (i = 0; i < STEP - 1; i++) {
		ptr[i] = dmalloc(size);
		if (!ptr[i]) {
			printf("TC2:Fail:[%d]th dmalloc(%d) failed\n", i + 1, size);
			fflush(stdout);
			return 0;
		}
		nb += size;
		*nbytes = (double) nb / (double) MAX_HEAP_SIZE;
		printf("TC2:dmalloc(%d)\n", size);
		fflush(stdout);
		PFL;
	}

	//try to allocate size byte for last step (it should fail due to rounding and metadata)
	ptr[STEP - 1] = dmalloc(size);
	if (ptr[STEP - 1]) {
		printf("TC2:Fail:[%d]th dmalloc(%d) wrongly succeeded\n", STEP, size);
		fflush(stdout);
		return 0;
	}
	printf("TC2:dmalloc(%d) failed correctly\n", size);
	fflush(stdout);
	PFL;

	//free allocated memory in order
	for (i = STEP - 2; i >= 0; i--) {
		dfree(ptr[i]);
		printf("TC2:dfree step %d\n", i + 1);
		fflush(stdout);
		PFL;
	}

	//finally try to allocate big chunk (90%) of memory to check functionality of free
	size = MAX_HEAP_SIZE * 0.9;
	ptr[0] = dmalloc(size);
	if (!ptr[0]) {
		printf("TC2:Fail:dmalloc(%d) failed\n", size);
		fflush(stdout);
		return 0;
	}
	*nbytes = 0.9;
	printf("TC2:dmalloc(%d)\n", size);
	fflush(stdout);
	PFL;

	dfree(ptr[0]);
	return 1;
}

int main(int argc, char** argv) {

	clock_t begin, end;

	double nbytes = 0.0; //successful bytes (%) allocation in a test case
	double time = 0.0; //execution time of a test case

	//Test Case 2
	begin = clock();
	int rc = test_case2(&nbytes);
	end = clock();
	time = (double) (end - begin) / CLOCKS_PER_SEC;
	if(rc == 1)
		fprintf(stderr, "TC2 passed!\n");
	//Format: TC2: Success ExecTime MaxBytes
	fprintf(stderr,"TC2: ExecTime MaxBytes\n");
	fprintf(stderr," %f %f\n",time,nbytes);
	fflush (stderr);

	return 0;
}
