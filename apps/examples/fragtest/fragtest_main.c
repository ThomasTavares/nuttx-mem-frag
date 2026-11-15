/****************************************************************************
 * apps/examples/fragtest/fragtest_main.c
 * 
 * Developed by Thomas Tavares Tomaz
 * 
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <nuttx/mm/mm.h>

/* Default values if no arguments are provided by the user */
#define DEFAULT_NUM_BLOCKS 40
#define DEFAULT_BLOCK_SIZE 2048

int main(int argc, FAR char *argv[])
{
    void **blocks;
    size_t frag_percentage;
    int num_blocks = DEFAULT_NUM_BLOCKS;
    int block_size = DEFAULT_BLOCK_SIZE;
    int i;

    /* Parse command line arguments if provided */
    if (argc >= 2) num_blocks = atoi(argv[1]);
    if (argc >= 3) block_size = atoi(argv[2]);

    /* Validate parsed arguments */
    if (num_blocks <= 0 || block_size <= 0) {
        printf("Error: Invalid arguments.\n");
        printf("Usage: fragtest <num_blocks> <block_size>\n\n");
        return -1;
    }

    /* Dynamically allocate the array of pointers to avoid stack overflow */
    blocks = (void **)malloc(num_blocks * sizeof(void *));
    if (blocks == NULL) {
        printf("Error: Failed to allocate memory for the block pointers array\n\n");
        return -1;
    }

    printf("\nStarting RAM fragmentation test...\n");
    printf("Configuration: %d blocks of %d bytes.\n", num_blocks, block_size);

    /* Baseline check */
    frag_percentage = heap_get_fragmentation_stats();
    printf("Initial fragmentation: %zu%%\n\n", frag_percentage);

    /* Validate output limits */
    if (frag_percentage > 100) {
        printf("Error: Syscall returned invalid value (%zu%%)\n\n", frag_percentage);
        free(blocks);
        return -1;
    }

    /* Allocate memory blocks to populate the heap */
    printf("Allocating %d blocks of %d bytes...\n", num_blocks, block_size);
    for (i = 0; i < num_blocks; i++) {
        blocks[i] = malloc(block_size);
        if (blocks[i] == NULL) {
            printf("Warning: Failed to allocate block %d\n", i);
        }
    }

    frag_percentage = heap_get_fragmentation_stats();
    printf("Fragmentation after allocations: %zu%%\n", frag_percentage);

    /* Free alternate blocks to induce memory fragmentation */
    printf("Freeing alternate blocks to create heap holes...\n");
    for (i = 0; i < num_blocks; i += 2) {
        if (blocks[i] != NULL) {
            free(blocks[i]);
            blocks[i] = NULL;
        }
    }

    frag_percentage = heap_get_fragmentation_stats();
    printf("Fragmentation after inducing holes: %zu%%\n", frag_percentage);

    /* Cleanup and final check */
    printf("Freeing remaining blocks...\n");
    for (i = 1; i < num_blocks; i += 2) {
        if (blocks[i] != NULL) free(blocks[i]);
    }

    /* Free the dynamically allocated array of pointers */
    free(blocks);

    frag_percentage = heap_get_fragmentation_stats();
    printf("Final fragmentation: %zu%%\n", frag_percentage);
    printf("Test finished.\n\n");
    return 0;
}