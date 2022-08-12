/*
 * Test Cases 4 - Stress
 * CPS 310 (OS) Lab 1 - Heap Manager
 * Author: Nisarg Raval
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

/*
 * The LOUD flag shouts a lot about malloc failures that are expected and normal.
 * We leave it undefined to keep down the piazza questions about it.
 * But you can turn it on to see how many malloc calls are returning NULL, i.e.,
 * how often your heap says it is out of space.
 * Please do not ask about it.  If you want to understand it, read the code.
 * Generally, if the stress test runs without crashing or complaining, you are OK.
 */
//#DEFINE LOUD

#include "dmm.h"

// PFL prints the free list if DBFLAG is set
//#define DBFLAG

#ifdef DBFLAG
#define PFL print_freelist()
#else
#define PFL
#endif

#ifdef HAVE_DRAND48
#define RAND() (drand48())
#define SEED(x) (srand48((x)))
#else
#define RAND() ((double)random()/RAND_MAX)
#define SEED(x) (srandom((x)))
#endif

#define PSEDUO_RND_SEED 0 // 1 for non-deterministic seeding
#define BUFLEN 1000
#define LOOPCNT 50000
#define ALLOC_SIZE MAX_HEAP_SIZE/100
#define ALLOC_CONST 0.6

//Test Case 4: allocate and free in random order and check for correctness
// return 0 if fails and 1 if passes
int test_case4(double *nbytes, double *acc, int *nbytes_abs, int *acc_abs) {
	printf("TC4: Start\n");
	fflush(stdout);

	int size;
	int itr;
	void *ptr[BUFLEN];
	int datasize[BUFLEN];
	int i, j;
	double randvar;
	int fail = 0;
	int nb = 0;
	int max_nb = 0;
	int nalloc = 0;

	long int global[LOOPCNT][2]; //store pointers to allocated memory segments
	for (j = 0; j < LOOPCNT; j++) {
		global[j][0] = -1;
		global[j][1] = -1;
	}
	for (i = 0; i < BUFLEN; i++) {
		ptr[i] = NULL;
	}

	/* Set the PSEUDO_RANDOM_SEED for pseduo random seed initialization based on time, i.e.,
	 * the random values changes after each execution 
	 */
	if (PSEDUO_RND_SEED)
		SEED(time(NULL));

	assert(
			MAX_HEAP_SIZE >= 1024 * 1024
					&& "MAX_HEAP_SIZE is too low; Recommended setting is at least 1MB for TC4 (test_stress)");

	for (i = 0; i < LOOPCNT; i++) {
		itr = (int) (RAND() * BUFLEN); //randomly choose an index for alloc/free

		randvar = RAND(); //flip a coin to decide alloc/free

		if (randvar < ALLOC_CONST && ptr[itr] == NULL) { //if the index is not already allocated allocate random size memory
			size = (int) (RAND() * ALLOC_SIZE);
			if (size > 0) {
				ptr[itr] = dmalloc(size);
				nalloc++;
			} else
				continue;
			if (ptr[itr] == NULL) {
#ifdef LOUD
				printf("TC4:Fail:[%d]th dmalloc(%d) failed\n", i, size);
				//fflush(stdout);
				fail++;
#endif
				continue;
			}

			// Range check for correctness
			for (j = 0; j < i; j++) {
				if (global[j][0] == -1) {
					continue;
				}
				if ((((long int) ptr[itr] >= global[j][0])
						&& ((long int) ptr[itr] <= global[j][1]))
						|| (((long int) ptr[itr] + size >= global[j][0])
								&& ((long int) ptr[itr] + size <= global[j][1]))) {

					printf(
							"TC4:Fail:Correctness fail [s] = %ld, [e] = %ld, [p] = %ld, [itr] = %d, [size] = %d\n",
							global[j][0], global[j][1], (long int) ptr[itr], i,
							size);
					fflush(stdout);
					return 0;
				}
			}
			global[i][0] = (long int) ptr[itr];
			global[i][1] = (long int) ptr[itr] + size;
			//printf(
			//		"TC4:Assigned: [s] = %ld, [e] = %ld, [p] = %ld, [itr] = %d, [size] = %d\n",
			//		global[i][0], global[i][1], (long int) ptr[itr], i, size);
			//fflush(stdout);
			PFL;
			//udate high water mark (max allocated heap)
			datasize[itr] = size;
			nb = nb + size;
			if (max_nb < nb){
				//printf("TC4: Updating Max Bytes Old: %d New: %d\n",max_nb,nb);
				//fflush(stdout);
				max_nb = nb;
				*nbytes_abs = max_nb;
				*nbytes = (double) max_nb / (double) MAX_HEAP_SIZE;
			}

		} else if (randvar >= ALLOC_CONST && ptr[itr] != NULL) { //free memory
			for (j = 0; j < i; j++) {
				if (global[j][0] == (long int) ptr[itr]) {
					global[j][0] = -1;
					global[j][1] = -1;
				}
			}
			dfree(ptr[itr]);
			ptr[itr] = NULL;
			nb = nb - datasize[itr];
			datasize[itr] = 0;
		}
	}

	//free all memory
	for (i = 0; i < BUFLEN; i++) {
		if (ptr[i] != NULL) {
			dfree(ptr[i]);
			ptr[i] = NULL;
		}
	}

	//*acc = (double) (nalloc-fail)/ (double) nalloc; //acc shows the percentage of successful dmalloc
	*acc = (double) (LOOPCNT-fail)/ (double) LOOPCNT; //acc shows the percentage of successful dmalloc/free
	*acc_abs = LOOPCNT-fail;
	return 1;
}

int main(int argc, char** argv) {

	clock_t begin, end;

	double nbytes = 0.0; //successful bytes (%) allocation in a test case
	double time = 0.0; //execution time of a test case
	double acc = 0.0; //percentage of correct alloc/free
	int nbytes_abs = 0; //absolute value of nbytes
	int acc_abs = 0;  // absolute value of acc
	
	//Test Case 4
	nbytes = 0.0;
	acc = 0.0;
	begin = clock();
	int rc = test_case4(&nbytes,&acc,&nbytes_abs,&acc_abs);
	end = clock();
	time = (double) (end - begin) / CLOCKS_PER_SEC;
	if(rc == 1)
		fprintf(stderr, "TC4 passed!\n");
	//Format: TC4: Success MaxBytes Accuracy MaxBytesABS AccuracyABS
	fprintf(stderr, "Runtime: %lf\n", time);
#ifdef LOUD
	fprintf(stderr,"TC4: MaxBytes Accuracy MaxBytesABS AccuracyABS\n");
	fprintf(stderr," %f %f %d %d\n",nbytes,acc,nbytes_abs,acc_abs);
#endif
	fflush(stderr);

	return 0;
}
