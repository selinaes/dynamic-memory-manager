# You can compile with either gcc or g++
# CC = g++
CC = gcc

# Compile/link options

# You can set the heap size here: your solution should work for large and small heaps.
# You can also comment this out and define it in dmm.h.
HEAP_SIZE= -DMAX_HEAP_SIZE=1048576

# Use -lm (math library) if you wish, but leave it out if not in use.
#CFLAGS = -I. -Wall -lm $(HEAP_SIZE) -DNDEBUG
#Note: NDEBUG flag turns off asserts and other debugging keyed by this flag.

CFLAGS = -I. -Wall $(HEAP_SIZE)

# Optimization
# Add this flag if you want to optimize for speed-testing.
OPTFLAG = -O2

#Debugging: to build for debugging, add this.  (-g3 might be better)
DEBUGFLAG = -g
CFLAGS += $(DEBUGFLAG)

EXECUTABLES = basicdmmtest test_basic test_coalesce_order test_coalesce_random test_stress

all: ${EXECUTABLES}

test: ${EXECUTABLES}
	for exec in ${EXECUTABLES}; do \
		./$$exec ; \
	done

debug: CFLAGS += $(DEBUGFLAG)
debug: ${EXECUTABLES}

test_basic: test_basic.c dmm.o 
	$(CC) $(CFLAGS)  -o test_basic test_basic.c dmm.o
test_coalesce_order: test_coalesce_order.c dmm.o 
	$(CC) $(CFLAGS)  -o test_coalesce_order test_coalesce_order.c dmm.o
test_coalesce_random: test_coalesce_random.c dmm.o 
	$(CC) $(CFLAGS)  -o test_coalesce_random test_coalesce_random.c dmm.o
test_stress: test_stress.c dmm.o 
	$(CC) $(CFLAGS)  -o test_stress test_stress.c dmm.o
test_stress_time: test_stress_time.c dmm.o 
	$(CC) $(CFLAGS)  -o test_stress_time test_stress_time.c dmm.o

gradescope:
	zip submission.zip dmm.c dmm.h

basicdmmtest: basicdmmtest.c dmm.o
	$(CC) $(CFLAGS) -o basicdmmtest basicdmmtest.c dmm.o

dmm.o: dmm.c dmm.h
	$(CC) $(CFLAGS) -c dmm.c

clean:
	rm -rf *.o ${EXECUTABLES} *.dSYM a.out 
