
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#define MAXSIZE 4088
#define MIN_BLK_SIZE 8
/*
* This structure serves as the header for each allocated and free block
* It also serves as the footer for each free block
* The blocks are ordered in the increasing order of addresses
*/
typedef struct blk_hdr {
	int size_status;

	/*
	* Size of the block is always a multiple of 8
	* => last two bits are always zero - can be used to store other information
	*
	* LSB -> Least Significant Bit (Last Bit)
	* SLB -> Second Last Bit
	* LSB = 0 => free block
	* LSB = 1 => allocated/busy block
	* SLB = 0 => previous block is free
	* SLB = 1 => previous block is allocated/busy
	*
	* When used as the footer the last two bits should be zero
	*/

	/*
	* Examples:
	*
	* For a busy block with a payload of 20 bytes (i.e. 20 bytes data + an additional 4 bytes for header)
	* Header:
	* If the previous block is allocated, size_status should be set to 27
	* If the previous block is free, size_status should be set to 25
	*
	* For a free block of size 24 bytes (including 4 bytes for header + 4 bytes for footer)
	* Header:
	* If the previous block is allocated, size_status should be set to 26
	* If the previous block is free, size_status should be set to 24
	* Footer:
	* size_status should be 24
	*
	*/
} blk_hdr;

/* Global variable - This will always point to the first block
* i.e. the block with the lowest address */
blk_hdr *first_blk = NULL;

/*
* Note:
*  The end of the available memory can be determined using end_mark
*  The size_status of end_mark has a value of 1
*
*/

/* Function that creates a footer
* Argument - ptr: Location of block that needs a footer
*/
void createFooter(blk_hdr *ptr) {
	//Mask two LSBs to find the size of the block
	int size = (ptr->size_status) & 0xFFFFFFFC;

	//Move pointer to footer location and initializes footer's size_status
	ptr = (blk_hdr *)((char*)ptr + size - sizeof(blk_hdr));
	ptr->size_status = size;
}

/*
* Function for allocating 'size' bytes
* Returns address of allocated block on success
* Returns NULL on failure
* - Check for sanity of size - Return NULL when appropriate
* - Round up size to a multiple of 8
* - Traverse the list of blocks and allocate the best free block which can accommodate the requested size
* - Also, when allocating a block - split it into two blocks
*/
void* Mem_Alloc(int size) {
	int best_size = INT_MAX;	//Variable to hold the size of the best fitting block
	blk_hdr *best_blk = NULL;	//Pointer to best fit block
	blk_hdr *curr_blk = first_blk;	//Pointer to current block
	blk_hdr *next_blk;
	blk_hdr *split_blk_hdr;		//Pointer to splitted block's header
	int curr_blk_size;			//Size of current block
	int curr_blk_status;		//Status of current block (0 = free, 1 = allocated)
	int prev_blk_status = 0;	//Status of block previous to best block
	int size_diff;

	if (size < 1 || size > MAXSIZE) {	//Invalid size input; return NULL
		return NULL;
	}

	size += sizeof(blk_hdr);	//Block header is always used, therefore its size must be added

	if (size % 8 != 0) {	//Round size up to a multiple of 8
		size += (8 - size % 8);
	}

	//Search for a suitable mark until the end of the heap is reached
	while (curr_blk->size_status != 1) {

		//Mask the 2 least significant bits to get the size of the current block
		curr_blk_size = (curr_blk->size_status) & 0xFFFFFFFC;
		//Mask everything except the LSB to determine whether the block is free
		curr_blk_status = (curr_blk->size_status) & 1;

		//Determine if the current block is a new best by checking if it is:
		// - large enough to accomodate
		// - smaller than the current best
		// - free
		if ((curr_blk_size >= size) && (curr_blk_size < best_size) && (curr_blk_status == 0)) {
			best_blk = curr_blk;
			best_size = curr_blk_size;

			//Remember previous block's status
			if (((best_blk->size_status) & 2) == 2) {
				prev_blk_status = 1;
			}

			//No need to continue searching if block size is perfect fit
			if (best_size == size) {
				break;
			}
		}
		//Move to the next block
		curr_blk = (blk_hdr *)((char*)curr_blk + curr_blk_size);
	}

	//If best_size is unchanged, no suitable block found; failure.
	if (best_size == INT_MAX) {
		return NULL;
	}

	size_diff = best_size - size;

	//Begin allocation and splitting

	//Splitting is unnecessary if the size of the splitted block would be too small;
	//treat as a perfect fit
	if (size_diff < MIN_BLK_SIZE) {

		best_blk->size_status = size;
		best_blk->size_status += 1;	//Change last bit to indicate the block is busy

		//If previous block was busy, update best block's size_status
		if (prev_blk_status == 1) {
			best_blk->size_status += 2;
		}

		//Since the current block is a perfect fit, the immediate next block's SLB must be updated	
		next_blk = (blk_hdr *)((char*)best_blk + best_size);
		if (next_blk->size_status != 1) {
			next_blk->size_status += 2;	//Change SLB to indicate the previous block is busy
		}
	}

	//Splitting required
	else {

		//Split the block based on the free space left after allocation
		//Update LSB to 1 to reflect busy status
		best_blk->size_status = size;
		best_blk->size_status += 1;
		//If previous block was busy, update best block's size_status
		if (prev_blk_status == 1) {
			best_blk->size_status += 2;
		}

		//Move the split block header pointer to just after the newly allocated block
		//Update its size_status to reflect its size/status and the previous block's busy status
		split_blk_hdr = (blk_hdr *)((char*)best_blk + size);
		split_blk_hdr->size_status = size_diff;
		split_blk_hdr->size_status += 2;

		//Create a footer for the split block
		createFooter(split_blk_hdr);
	}

	//Return address right after block header
	return (char*)best_blk + sizeof(blk_hdr);
}

/*
* Function for freeing up a previously allocated block
* Argument - ptr: Address of the block to be freed up
* Returns 0 on success
* Returns -1 on failure
* - Return -1 if ptr is NULL
* - Return -1 if ptr is not 8 byte aligned or if the block is already freed
* - Mark the block as free
* - Coalesce if one or both of the immediate neighbours are free
*/
int Mem_Free(void *ptr) {

	blk_hdr *free_blk = NULL;	//Block to be freed
	blk_hdr *prev_blk = NULL;	//Block previous to the one to be freed
	blk_hdr *prev_blk_ftr = NULL;	
	blk_hdr *next_blk = NULL;	//Block following the one to be freed
	int free_blk_status = 0;	//Status of block to be freed
	int prev_blk_status = 0;	//Status of previous block
	int next_blk_status = 0;	//Status of next block
	int free_size = 0;
	int prev_size = 0;
	int next_size = 0;

	//Return error if ptr is null or misaligned
	if (ptr == NULL || ((int) ptr % 8) != 0) {	
		return -1;
	}

	//Move pointer backwards to point to header 
	free_blk = (blk_hdr *)((char *)ptr - sizeof(blk_hdr));

	//Return error if block is not busy
	free_blk_status = (free_blk->size_status) & 1;
	if (free_blk_status != 1) {
		return -1;
	}
	
	//Mask two LSBs to find the size of block
	free_size = (free_blk->size_status) & 0xFFFFFFFC;

	//Check if previous block is allocated by bitmasking
	if ((free_blk->size_status & 2) == 2) {
		prev_blk_status = 1;	
	}
	else {
		//Point to the footer of the previous block and get its size
		prev_blk_ftr = free_blk - 1;
		prev_size = prev_blk_ftr->size_status;	

		//Point to the header of the previous block
		prev_blk = (blk_hdr *)((char*)prev_blk_ftr - (prev_size - sizeof(blk_hdr)));	
	}

	//Determine status of next block
	next_blk = (blk_hdr *)((char*)free_blk + free_size);
	next_blk_status = (next_blk->size_status) & 1;
	//Mask two LSBs to find the size of next block if block is free
	next_size = (next_blk->size_status) & 0xFFFFFFFC;
	
	//Coalescing

	if ((next_blk_status == 1) && (prev_blk_status == 1)) {	
		//Both neighbors are allocated so coalescing is unnecessary
		//Simply set the LSB of the block's size status to 0 and the SLB to 1,
		//freeing the block and indicating the previous block's allocation
		free_blk->size_status = free_size + 2;

		//Must also update the next block to reflect the current block's free status
		next_blk->size_status = next_size + next_blk_status;
		//Create a footer
		createFooter(free_blk);

	} else if ((next_blk_status == 1) && (prev_blk_status == 0)) {
		//Only the previous block is free, and needs to be coalesced

		//Update prev block's header & footer to indicate the new size after merging
		prev_blk->size_status += free_size;
		createFooter(prev_blk);

		//Update next block's size status to indicate the previous merged block is free
		next_blk->size_status = next_size + next_blk_status;

		//Update middle block to ensure it cannot be read as allocated
		free_blk->size_status = free_size + 2;

	} else if ((next_blk_status == 0) && (prev_blk_status == 1)) {
		//Only the next block is free, and needs to be coalesced

		//Update middle/freed block's header & footer to indicate merged size
		free_blk->size_status = free_size + next_size + 2;
		createFooter(free_blk);

	} else if ((next_blk_status == 0) && (prev_blk_status == 0)) {
		//Both neighbors are free so all must be coalesced
		prev_blk->size_status += (free_size + next_size);
		createFooter(prev_blk);

		//Update middle block to ensure it cannot be read as allocated
		free_blk->size_status = free_size + 2;
	}

	else {
		printf("Error in Mem_Free.");
		return -1;
	}

	return 0;
}

/*
* Function used to initialize the memory allocator
* Not intended to be called more than once by a program
* Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated
* Returns 0 on success and -1 on failure
*/
int Mem_Init(int sizeOfRegion) {
	int pagesize;
	int padsize;
	int alloc_size;
	void* space_ptr;
	blk_hdr* end_mark;
	static int allocated_once = 0;

	if (0 != allocated_once) {
		fprintf(stderr,
			"Error:mem.c: Mem_Init has allocated space during a previous call\n");
		return -1;
	}
	if (sizeOfRegion <= 0) {
		fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
		return -1;
	}

	// Get the pagesize
	pagesize = 4024;

	// Calculate padsize as the padding required to round up sizeOfRegion 
	// to a multiple of pagesize
	padsize = sizeOfRegion % pagesize;
	padsize = (pagesize - padsize) % pagesize;

	alloc_size = sizeOfRegion + padsize;

	space_ptr = malloc(alloc_size);

	allocated_once = 1;

	// for double word alignement and end mark
	alloc_size -= 8;

	// To begin with there is only one big free block
	// initialize heap so that first block meets 
	// double word alignement requirement
	first_blk = (blk_hdr*)space_ptr + 1;
	end_mark = (blk_hdr*)((char*)first_blk + alloc_size); // changed from void to char

														  // Setting up the header
	first_blk->size_status = alloc_size;

	// Marking the previous block as busy
	first_blk->size_status += 2;

	// Setting up the end mark and marking it as busy
	end_mark->size_status = 1;

	// Setting up the footer
	blk_hdr *footer = (blk_hdr*)((char*)first_blk + alloc_size - 4);
	footer->size_status = alloc_size;

	return 0;
}

/*
* Function to be used for debugging
* Prints out a list of all the blocks along with the following information i
* for each block
* No.      : serial number of the block
* Status   : free/busy
* Prev     : status of previous block free/busy
* t_Begin  : address of the first byte in the block (this is where the header starts)
* t_End    : address of the last byte in the block
* t_Size   : size of the block (as stored in the block header) (including the header/footer)
*/
void Mem_Dump() {
	int counter;
	char status[5];
	char p_status[5];
	char *t_begin = NULL;
	char *t_end = NULL;
	int t_size;

	blk_hdr *current = first_blk;
	counter = 1;

	int busy_size = 0;
	int free_size = 0;
	int is_busy = -1;

	fprintf(stdout, "************************************Block list***\
					                    ********************************\n");
	fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
	fprintf(stdout, "-------------------------------------------------\
					                    --------------------------------\n");

	while (current->size_status != 1) {
		t_begin = (char*)current;
		t_size = current->size_status;

		if (t_size & 1) {
			// LSB = 1 => busy block
			strcpy_s(status, 5, "Busy");
			is_busy = 1;
			t_size = t_size - 1;
		}
		else {
			strcpy_s(status, 5, "Free");
			is_busy = 0;
		}

		if (t_size & 2) {
			strcpy_s(p_status, 5, "Busy");
			t_size = t_size - 2;
		}
		else {
			strcpy_s(p_status, 5, "Free");
		}

		if (is_busy)
			busy_size += t_size;
		else
			free_size += t_size;

		t_end = t_begin + t_size - 1;

		fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status,
			p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);

		current = (blk_hdr*)((char*)current + t_size);
		counter = counter + 1;
	}

	fprintf(stdout, "---------------------------------------------------\
					                    ------------------------------\n");
	fprintf(stdout, "***************************************************\
					                    ******************************\n");
	fprintf(stdout, "Total busy size = %d\n", busy_size);
	fprintf(stdout, "Total free size = %d\n", free_size);
	fprintf(stdout, "Total size = %d\n", busy_size + free_size);
	fprintf(stdout, "***************************************************\
					                    ******************************\n");
	fflush(stdout);

	return;
}

int _tmain(int argc, _TCHAR* argv[])
{

	if (Mem_Init(600 * 1024) == -1) {
		fprintf(stderr, "ERROR\n");
		exit(1);
	}


#define MAXPTRS 10
#define MAXALLOCSIZE 513

	void* ptr[MAXPTRS];

	int i, j, size = 0, allocs = 0, frees = 0, allocerrors = 0;

	for (i = 0; i < MAXPTRS; i++) {
		ptr[i] = NULL;
	}

	Mem_Dump();

	for (j = 0; j < 1000000; j++) {
		printf("$$$$$ Iteration # %d\n\n", j);
		Mem_Dump();

		// Choose a random index
		i = rand() % MAXPTRS;
		size = rand() % MAXALLOCSIZE + 1;

		if (ptr[i] == NULL) {
			// Allocate of some size
			if ((ptr[i] = Mem_Alloc(size)) == NULL) {
				// printf("Error in allocating\n");
				allocerrors++;
				continue;
			}
			allocs++;
		}
		else {
			if (Mem_Free(ptr[i]) != 0) {
				printf("Error in freeing\n");
				break;
			}
			else if (Mem_Free(ptr[i]) == 0) {
				break;
			}
			else {
				ptr[i] = NULL;
				frees++;
			}
		}
		if (j % 100000 == 0) {
			printf("Iteration # %d; allocs = %d, frees = %d, allocerrors = %d\n", j, allocs, frees, allocerrors);
		}
	}

	Mem_Dump();

	for (i = 0; i < MAXPTRS; i++) {
		if (ptr[i] != NULL) {
			if (Mem_Free(ptr[i]) != 0) {
				printf("Error in freeing\n");
				break;
			}
			else if (Mem_Free(ptr[i]) == 0) {
				printf("ERROR\n");
			}
		}
	}

	Mem_Dump();

	Mem_Dump();
}
