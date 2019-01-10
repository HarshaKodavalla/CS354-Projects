#include <stdio.h>
#include <stdlib.h>

#define ROW_SIZE 3000
#define COL_SIZE 500

int arr2D_col[ROW_SIZE][COL_SIZE];

int main() {
	//Iterate through columns
	for (int i = 0; i < COL_SIZE; i++) {
		//Iterate through rows
		for (int j = 0; j < ROW_SIZE; j++) {
			arr2D_col[j][i] = i + j;
		}
	
	}

	return 0;
}

