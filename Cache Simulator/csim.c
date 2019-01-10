////////////////////////////////////////////////////////////////////////////////
// Main File:        csim.c
// This File:        csim.c
// Other Files:      
// Semester:         CS 354 Spring 2018
//
// Author:           Harsha Kodavalla
// Email:            kodavalla@wisc.edu
// CS Login:         harsha
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          Identify persons by name, relationship to you, and email.
//                   Describe in detail the the ideas and help they provided.
//
// Online sources:   avoid web searches to solve your problems, but if you do
//                   search, be sure to include Web URLs and description of 
//                   of any information you find.
//////////////////////////// 80 columns wide ///////////////////////////////////
/* Name: Harsha Kodavalla
* CS login: harsha
* Section(s):
*
* csim.c - A cache simulator that can replay traces from Valgrind
*     and output statistics such as number of hits, misses, and
*     evictions.  The replacement policy is LRU.
*
* Implementation and assumptions:
*  1. Each load/store can cause at most one cache miss plus a possible eviction.
*  2. Instruction loads (I) are ignored.
*  3. Data modify (M) is treated as a load followed by a store to the same
*  address. Hence, an M operation can result in two cache hits, or a miss and a
*  hit plus a possible eviction.
*
* The function printSummary() is given to print output.
* Please use this function to print the number of hits, misses and evictions.
* This is crucial for the driver to evaluate your work.
*/

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <math.h>

#define MEM_BITS 64		// Number of memory address bits

/****************************************************************************/
/***** DO NOT MODIFY THESE VARIABLE NAMES ***********************************/

/* Globals set by command line args */
int s = 0; /* set index bits */
int E = 0; /* associativity */
int b = 0; /* block offset bits */
int verbosity = 0; /* print trace if set */
char* trace_file = NULL;

/* Derived from command line args */
int B; /* block size (bytes) B = 2^b */
int S; /* number of sets S = 2^s In C, you can use the left shift operator */

	   /* Counters used to record cache statistics */
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;
/*****************************************************************************/

int cacheSize;

/* Type: Memory address
* Use this type whenever dealing with addresses or address masks
*/
typedef unsigned long long int mem_addr_t;

/* Type: Cache line
*
* next: pointer to the next line in the linked list LRU representation
* prev: pointer to the prev line in the linked list LRU representation
*/
typedef struct cache_line {
	char valid;
	mem_addr_t tag;
	struct cache_line * next;
	struct cache_line * prev;
} cache_line_t;

/* Type: Cache set
*
* lines: an array of all the lines in the set
* list: points to the head of a doubly linked list of lines in order of most recently used.
* list_size: the size of the list
*/
typedef struct cache_set {
	cache_line_t * lines;
	cache_line_t * list;
	int list_size;
} cache_set;

typedef cache_set* cache_t;		// Pointer to cache set = entire cache

cache_t cache;					/* The cache we are simulating */


/* updateList -
* Moves given line to head of the list located at the given set.
* Updates new tail and old head.
*/
void updateList(cache_line_t * curr_line, int set) {
	cache[set].list->prev = curr_line;
	curr_line->next = cache[set].list;
	curr_line->prev = NULL;
	cache[set].list = curr_line;
	return;
}

/* 
* initCache -
* Allocate data structures to hold info regrading the sets and cache lines
* use struct "cache_line_t" here
* Initialize valid and tag field with 0s.
* use S (= 2^s) and E while allocating the data structures here
*/
void initCache() {
	B = 1 << b;		// size of block = 2 ^ b
	S = 1 << s;		// # of sets = 2 ^ s

	// Initialize cache as an array of sets
	cache = (cache_set *)malloc(sizeof(cache_set) * S);

	// Initialize each set; 
	// - lines points to size E array of lines
	// - list points to NULL
	// - list_size to 0
	for (int i = 0; i < S; i++) {
		cache[i].lines = (cache_line_t*)malloc(sizeof(cache_line_t) * E);
		cache[i].list = NULL;
		cache[i].list_size = 0;
		// Initialize each line; valid and tag to 0, next and prev to NULL
		for (int j = 0; j < E; j++) {
			cache[i].lines[j].valid = 0;
			cache[i].lines[j].tag = 0;
			cache[i].lines[j].next = NULL;
			cache[i].lines[j].prev = NULL;
		}
	}
}

/* 
* freeCache - free each piece of memory you allocated using malloc
* inside initCache() function
*/
void freeCache() {
	// Free each set's line array
	for (int i = 0; i < S; i++) {
		free(cache[i].lines);
	}

	// Free the cache's set array
	free(cache);
	return;
}

/* 
* accessData - Access data at memory address addr.
*   If it is already in cache, increase hit_cnt
*   If it is not in cache, bring it in cache, increase miss count.
*   Also increase evict_cnt if a line is evicted.
*   you will manipulate data structures allocated in initCache() here
*/
void accessData(mem_addr_t addr) {
	// Bit mask the address to find the set.
	// Shift address to the right b bits then bitwise AND with (2 ^ s) - 1.
	int set = (addr >> b) & ((1 << s) - 1);

	// Shift the address to the right b + s bits.
	int tag = (addr >> (b + s));

	// Cold miss - empty set; change unused line's tag to address's tag. 
	// Move to head of empty list. The chosen line is the first line 
	// in the lines array as all lines are unused.
	if (cache[set].list_size == 0) {
		cache[set].list = &(cache[set].lines[0]);
		cache[set].list->tag = tag;
		cache[set].list->valid = 1;
		cache[set].list_size++;

		miss_cnt++;
		return;
	}

	// Pointer to the current line in the linked list
	cache_line_t *curr_line = cache[set].list;	

	// Traverse the list searching for a line with a valid bit of 1 and a matching tag
	for (int i = 0; i < cache[set].list_size; i++) {
		if ((curr_line->valid == 1) && (curr_line->tag == tag)) {
			// Hit 
			hit_cnt++;

			// Determine if current line is tail of list
			if ((curr_line->next == NULL) && (curr_line->prev != NULL)) {
				// Since current line is tail, no changes to next
				// Update prev neighbor
				curr_line->prev->next = NULL;

				// Move current to head
				updateList(curr_line, set);
			
			// Determine if current line is neither head nor tail
			} else if ((curr_line->next != NULL) && (curr_line->prev != NULL)) {
				// Must update both neighbors
				curr_line->prev->next = curr_line->next;
				curr_line->next->prev = curr_line->prev;

				// Move current to head
				updateList(curr_line, set);
			}
			return;
		}
		// Increment curr_line
		// If the for loop is about to exit, ensure curr_line will point to the tail
		if (i != cache[set].list_size - 1) {
			curr_line = curr_line->next;
		}
	}

	// If the function does not return inside of the for loop, a miss has occurred. 
	miss_cnt++;

	// Handle each type of miss accordingly

	// Check for a capacity miss
	if (cache[set].list_size == E) {
		// Eviction must occur. Line at tail of list must be evicted
		// as it is the LRU.
		curr_line->tag = tag;
		evict_cnt++;

		if (curr_line->prev != NULL) {
			// Update neighbor
			curr_line->prev->next = NULL;
		}

		// Move "new" line to head
		updateList(curr_line, set);
	
	// Check for a cold miss - nonempty set
	} else if (cache[set].list_size < E) {
		// No eviction is necessary. Simply initialize an unused line from 
		// the lines array.
		curr_line = &(cache[set].lines[cache[set].list_size]);
		curr_line->valid = 1;
		curr_line->tag = tag;
		cache[set].list_size++;

		// Move new line to head 
		updateList(curr_line, set);

	// This code should never be reached.
	} else {
		printf("Error occurred in accessData.");
		exit(0);
	}

	return;
}

/*
* replayTrace - replays the given trace file against the cache
* reads the input trace file line by line
* extracts the type of each memory access : L/S/M
* YOU MUST TRANSLATE one "L" as a load i.e. 1 memory access
* YOU MUST TRANSLATE one "S" as a store i.e. 1 memory access
* YOU MUST TRANSLATE one "M" as a load followed by a store i.e. 2 memory accesses
*/
void replayTrace(char* trace_fn) {
	char buf[1000];
	mem_addr_t addr = 0;
	unsigned int len = 0;
	FILE* trace_fp = fopen(trace_fn, "r");

	if (!trace_fp) {
		fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
		exit(1);
	}

	while (fgets(buf, 1000, trace_fp) != NULL) {
		if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
			sscanf(buf + 3, "%llx,%u", &addr, &len);

			if (verbosity)
				printf("%c %llx,%u ", buf[1], addr, len);

			// now you have: 
			// 1. address accessed in variable - addr 
			// 2. type of acccess(S/L/M)  in variable - buf[1] 
			// call accessData function here depending on type of access
			if (buf[1] == 'S' || buf[1] == 'L') {
				accessData(addr);
			}
			else if (buf[1] == 'M') {
				accessData(addr);
				accessData(addr);
			}

			if (verbosity)
				printf("\n");
		}
	}

	fclose(trace_fp);
}

/*
* printUsage - Print usage info
*/
void printUsage(char* argv[]) {
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
	printf("Options:\n");
	printf("  -h         Print this help message.\n");
	printf("  -v         Optional verbose flag.\n");
	printf("  -s <num>   Number of set index bits.\n");
	printf("  -E <num>   Number of lines per set.\n");
	printf("  -b <num>   Number of block offset bits.\n");
	printf("  -t <file>  Trace file.\n");
	printf("\nExamples:\n");
	printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
	printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
	exit(0);
}

/*
* printSummary - Summarize the cache simulation statistics. Student cache simulators
*                must call this function in order to be properly autograded.
*/
void printSummary(int hits, int misses, int evictions) {
	printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
	FILE* output_fp = fopen(".csim_results", "w");
	assert(output_fp);
	fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
	fclose(output_fp);
}

/*
* main - Main routine
*/
int main(int argc, char* argv[]) {
	char c;

	// Parse the command line arguments: -h, -v, -s, -E, -b, -t 
	while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
		switch (c) {
		case 'b':
			b = atoi(optarg);
			break;
		case 'E':
			E = atoi(optarg);
			break;
		case 'h':
			printUsage(argv);
			exit(0);
		case 's':
			s = atoi(optarg);
			break;
		case 't':
			trace_file = optarg;
			break;
		case 'v':
			verbosity = 1;
			break;
		default:
			printUsage(argv);
			exit(1);
		}
	}


	if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
		printf("%s: Missing required command line argument\n", argv[0]);
		printUsage(argv);
		exit(1);
	}


	initCache();

	replayTrace(trace_file);

	freeCache();

	printSummary(hit_cnt, miss_cnt, evict_cnt);
	return 0;
}

