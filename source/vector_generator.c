/********************************************************************************************
 * Description:
 *    Random vector generator in Matrix Market (.mtx) array format.
 *
 *    This program:
 *        1. Generates a dense column vector of length LENGTH filled with
 *           uniformly distributed random values in the range [0,1].
 *        2. Writes the vector to a file in Matrix Market array format, compatible
 *           with CSR/SpMV programs for testing and benchmarking.
 *
 * Vector Format:
 *    - Output file: FILENAME ("v5.mtx")
 *    - Matrix Market header: array, real, general
 *    - Dimensions: LENGTH × 1
 *
 * Compilation:
 *        gcc -o vector_generator vector_generator.c
 *
 * Execution:
 *        ./vector_generator
 *
 * Output:
 *        - File "v.mtx" containing the generated vector
 *        - Console message with filename and vector length
 *
 * Notes:
 *    - LENGTH and FILENAME can be adjusted via preprocessor definitions.
 ********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LENGTH 128568730
#define FILENAME "v5.mtx"

int main() {
    srand(time(NULL));

    FILE *f = fopen(FILENAME, "w");
    if(!f){
        fprintf(stderr, "Error opening file %s\n", FILENAME);
        return 1;
    }

    // Matrix Market header for a dense vector
    fprintf(f, "%%%%MatrixMarket matrix array real general\n");
    fprintf(f, "%% Random vector with %d elements\n", LENGTH);
    fprintf(f, "%d %d\n", LENGTH, 1); // rows, cols

    for(int i = 0; i < LENGTH; i++){
        double val = (double) rand() / RAND_MAX;
        fprintf(f, "%.6f\n", val);
    }

    fclose(f);

    printf("Vector generated in Matrix Market format: %s\n", FILENAME);
    printf("Length: %d elements\n", LENGTH);

    return 0;
}