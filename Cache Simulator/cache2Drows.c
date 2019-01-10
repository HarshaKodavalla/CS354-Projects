///////////////////////////////////////////////////////////////////////////////
// Main File:        cache2Drows.c
// This File:        cache2Drows.c
// Other Files:      cache2Dcols.c, cache1D.c
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

#define ROW_SIZE 3000
#define COL_SIZE 500

int arr2D_row[ROW_SIZE][COL_SIZE];

int main() {
	
	//Iterate through rows
	for (int i = 0; i < ROW_SIZE; i++) {
		//Iterate through columns
		for (int j = 0; j < COL_SIZE; j++) {
			arr2D_row[i][j] = i + j;
		}
	}

	return 0;
}


