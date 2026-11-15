/****************************************************************************
 * mm/mm_heap/mm_fragmentation.c
 * 
 * Developed by Thomas Tavares Tomaz
 * 
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/mm/mm.h>
#include "mm_heap/mm.h"

/****************************************************************************
 * Private Functions and Structures
 ****************************************************************************/

/* State structure to hold fragmentation data during iteration */
struct mm_frag_state_s {
    size_t total_free;
    size_t largest_free;
};

/* Callback function to inspect each node in the heap */
static void fragmentation_handler(FAR struct mm_allocnode_s *node, FAR void *arg)
{
    FAR struct mm_frag_state_s *state = (FAR struct mm_frag_state_s *)arg;
    size_t nodesize = MM_SIZEOF_NODE(node);

    /* Check if the current memory node is free */
    if (MM_NODE_IS_FREE(node)) {
        /* Accumulate the total free memory */
        state->total_free += nodesize;

        /* Keep track of the largest contiguous free block */
        if (nodesize > state->largest_free) state->largest_free = nodesize;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* System call implementation to calculate memory fragmentation */
size_t heap_get_fragmentation_stats(void)
{
    struct mm_frag_state_s state = {0, 0};
    size_t fragmented_memory, frag_percentage = 0;

/* Obtain the global kernel heap pointer */
#ifdef CONFIG_MM_KERNEL_HEAP
  FAR struct mm_heap_s *heap = g_kmmheap;
#else
  FAR struct mm_heap_s *heap = g_mmheap;
#endif

    if (heap != NULL) {
        /* Safely iterate over all memory nodes using the internal OS mechanism */
        mm_foreach(heap, fragmentation_handler, &state);

        /* Calculate fragmentation percentage */
        if (state.total_free > 0) {
            fragmented_memory = state.total_free - state.largest_free;
            frag_percentage = (fragmented_memory * 100) / state.total_free;
        }
    }

    return frag_percentage;
}