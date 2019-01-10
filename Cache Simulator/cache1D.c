///////////////////////////////////////////////////////////////////////////////
// Main File:        cache1D.c
// This File:        cache1D.c
// Other Files:      cache2Dcols.c, cache2Drows.c
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
#include <stdio.h>
#include <stdlib.h>

#define ARR_LENGTH 100000

int arr[ARR_LENGTH];

int main() {
	//Initialize arr
	for (int i = 0; i < ARR_LENGTH; i++) {
		arr[i] = i;
	}

	return 0;
}
