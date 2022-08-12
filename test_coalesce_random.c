/*
 * Test Cases 3 - Coalesce Random Oder
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

int step_perm[STEP] = { 3, 4, 1, 2, 0, 5, 6 }; //permutation of length step [0,STEP-1]

//Test Case 3: Allocate entire heap in multiple steps and free in random order (step_perm)
// return 0 if fails and 1 if passes
int test_case3(double *nbytes) {
	printf("TC3: Start\n");
	fflush(stdout);
	
	int size = MAX_HEAP_SIZE / STEP;
	void *ptr[STEP];
	int nb = 0;
	int i;

	//allocate size bytes (STEP-1) time
	for (i = 0; i < STEP - 1; i++) {
		ptr[i] = dmalloc(size);
		if (!ptr[i]) {
			printf("TC3:Fail:[%d]th dmalloc(%d) failed\n", i + 1, size);
			fflush(stdout);
			return 0;
		}
		nb += size;
		*nbytes = (double) nb / (double) MAX_HEAP_SIZE;
		printf("TC3:dmalloc(%d)\n", size);
		fflush(stdout);
		PFL;
	}

	//try to allocate size byte for last step (it should fail due to rounding and metadata)
	ptr[STEP - 1] = dmalloc(size);
	if (ptr[STEP - 1]) {
		printf("TC3:Fail:[%d]th dmalloc(%d) wrongly succeeded\n", STEP, size);
		fflush(stdout);
		return 0;
	}
	printf("TC3:dmalloc(%d) failed correctly\n", size);
	fflush(stdout);
	PFL;

	//free allocated memory by permutation
	for (i = 0; i < STEP - 1; i++) {
		dfree(ptr[step_perm[i]]);
		printf("TC3:dfree step %d\n", step_perm[i] + 1);
		fflush(stdout);
		PFL;
	}

	//finally try to allocate big chunk (90%) of memory to check functionality of free
	size = MAX_HEAP_SIZE * 0.9;
	ptr[0] = dmalloc(size);
	if (!ptr[0]) {
		printf("TC3:Fail:dmalloc(%d) failed\n", size);
		fflush(stdout);
		return 0;
	}
	*nbytes = 0.9;
	printf("TC3:dmalloc(%d)\n", size);
	fflush(stdout);
	PFL;

	dfree(ptr[0]);
	return 1;
}


int main(int argc, char** argv) {

	clock_t begin, end;

	double nbytes = 0.0; //successful bytes (%) allocation in a test case
	double time = 0.0; //execution time of a test case

	//Test Case 3
	nbytes = 0.0;
	begin = clock();
	int rc = test_case3(&nbytes);
	end = clock();
	time = (double) (end - begin) / CLOCKS_PER_SEC;
	if(rc == 1)
		fprintf(stderr, "TC3 Passed!\n");
	//Format: TC3: Success ExecTime MaxBytes
	fprintf(stderr,"TC3: ExecTime MaxBytes\n");
	fprintf(stderr," %f %f\n",time,nbytes);
	fflush (stderr);

	return 0;
}
