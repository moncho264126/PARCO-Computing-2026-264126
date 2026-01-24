// gcc -O2 -o vector_generator vector_generator.c
// ./vector_generator

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LENGTH 6400000
#define FILENAME "WSV8.mtx"

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