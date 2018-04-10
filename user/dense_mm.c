/******************************************************************************
* 
* dense_mm.c
* 
* This program implements a dense matrix multiply and can be used as a
* hypothetical workload. 
*
* Usage: This program takes a single input describing the size of the matrices
*        to multiply. For an input of size N, it computes A*B = C where each
*        of A, B, and C are matrices of size N*N. Matrices A and B are filled
*        with random values. 
*
* Written Sept 6, 2015 by David Ferry
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sched_monitor.h>


const int num_expected_args = 2;
const unsigned sqrt_of_UINT32_MAX = 65536;

int main( int argc, char* argv[] ){
    FILE *results;
    int fd, status, i, print;
	preemption_info_t buf;
    fd = open(DEV_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Could not open %s: %s\n", DEV_NAME, strerror(errno));
        return -1; 
    }

	unsigned index, row, col; //loop indicies
	unsigned matrix_size, squared_size;
	double *A, *B, *C;
	
	print = 0;
	if( argc > 2){
		if(strcmp(argv[2], "-p\n")){
			print = 1;
		
		}
		else{
			printf("arg[2]: %s\n", argv[2]);
                        printf("Print Option Usage: ./dense_mm <size_of_matrices> -p \n");
                        exit(-1);
		}
	}


	if( argc < num_expected_args ){
		printf("Usage: ./dense_mm <size of matrices>\n");
		printf("Print Option Usage: ./dense_mm <size_of_matrices> -p");
		exit(-1);
	}

	matrix_size = atoi(argv[1]);
	
	if( matrix_size > sqrt_of_UINT32_MAX ){
		printf("ERROR: Matrix size must be between zero and 65536!\n");
		exit(-1);
	}

	squared_size = matrix_size * matrix_size;
	
	results = fopen("preemptions.csv", "w");
        if(results == NULL){

		printf("Unable to open output CSV: %s \n", strerror(errno));
		exit(-1);

	}
 
        fprintf(results, "EVENT, TIME_ON, TIME_OFF, CPU, PREEMPTED_BY ");

	printf("Generating matrices...\n");

	A = (double*) malloc( sizeof(double) * squared_size );
	B = (double*) malloc( sizeof(double) * squared_size );
	C = (double*) malloc( sizeof(double) * squared_size );

	for( index = 0; index < squared_size; index++ ){
		A[index] = (double) rand();
		B[index] = (double) rand();
		C[index] = 0.0;
	}

	printf("Multiplying matrices...\n");

    status = enable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not enable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1; 
    }
	
	for( row = 0; row < matrix_size; row++ ){
		for( col = 0; col < matrix_size; col++ ){
			for( index = 0; index < matrix_size; index++){
			C[row*matrix_size + col] += A[row*matrix_size + index] *B[index*matrix_size + col];
			}	
		}
	}

	printf("Multiplication done!\n");
    status = disable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not disable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1; 
    }   
	
	i = 1;
    while(read(fd, &buf, sizeof(buf)) > 0) {
        
	if(print){
		printf("Event %d\n", i); 
        	printf("\tTime off: %'llu ns. Time on: %'llu ns.\n", buf.time_off,buf.time_on);
        	printf("\tScheduled on core %d\n", buf.cpu);
        	printf("\tPreempted by %s\n", &buf.preempted_by);
	}
        fprintf(results, "\n%d", i);
	fprintf(results, ",%llu", buf.time_on);
	fprintf(results, ",%llu", buf.time_off);
	fprintf(results, ",%d", buf.cpu);
	fprintf(results, ",%s", &buf.preempted_by);
        ++i;
    }   

    close(results);
    close(fd);

	return 0;
}
