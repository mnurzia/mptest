#include "mptest_internal.h"

#if MPTEST_USE_LEAKCHECK

/* Set the guard bytes in `header`. */
MN_INTERNAL void mptest__leakcheck_header_set_guard(
    struct mptest__leakcheck_header* header)
{
    /* Currently we choose 0xCC as the guard byte, it's a stripe of ones and
     * zeroes that looks like 11001100b */
    size_t i;
    for (i = 0; i < MPTEST__LEAKCHECK_GUARD_BYTES_COUNT; i++) {
        header->guard_bytes[i] = 0xCC;
    }
}

/* Ensure that `header` has valid guard bytes. */
MN_INTERNAL int mptest__leakcheck_header_check_guard(
    struct mptest__leakcheck_header* header)
{
    size_t i;
    for (i = 0; i < MPTEST__LEAKCHECK_GUARD_BYTES_COUNT; i++) {
        if (header->guard_bytes[i] != 0xCC) {
            return 0;
        }
    }
    return 1;
}

/* Determine if `block` contains a `free()`able pointer. */
MN_INTERNAL int mptest__leakcheck_block_has_freeable(
    struct mptest__leakcheck_block* block)
{
    /* We can free the pointer if it was not freed or was not reallocated
     * earlier. */
    return !(block->flags
             & (MPTEST__LEAKCHECK_BLOCK_FLAG_FREED
                 | MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD));
}

/* Initialize a `struct mptest__leakcheck_block`.
 * If `prev` is NULL, then this function will not attempt to link `block` to
 * any previous element in the malloc linked list. */
MN_INTERNAL void mptest__leakcheck_block_init(
    struct mptest__leakcheck_block* block, size_t size,
    struct mptest__leakcheck_block*    prev,
    enum mptest__leakcheck_block_flags flags, const char* file, int line)
{
    block->block_size = size;
    /* Link current block to previous block */
    block->prev = prev;
    /* Link previous block to current block */
    if (prev) {
        block->prev->next = block;
    }
    block->next = NULL;
    /* Keep `realloc()` fields unpopulated for now */
    block->realloc_next = NULL;
    block->realloc_prev = NULL;
    block->flags        = flags;
    /* Save source info */
    block->file = file;
    block->line = line;
}

/* Link a block to its respective header. */
MN_INTERNAL void mptest__leakcheck_block_link_header(
    struct mptest__leakcheck_block*  block,
    struct mptest__leakcheck_header* header)
{
    block->header = header;
    header->block = block;
}

/* Initialize malloc-checking state. */
MN_INTERNAL void mptest__leakcheck_init(struct mptest__state* state)
{
    state->test_leak_checking = 0;
    state->first_block        = NULL;
    state->top_block          = NULL;
    state->total_allocations = 0;
}

/* Destroy malloc-checking state. */
MN_INTERNAL void mptest__leakcheck_destroy(struct mptest__state* state)
{
    /* Walk the malloc list, destroying everything */
    struct mptest__leakcheck_block* current = state->first_block;
    while (current) {
        struct mptest__leakcheck_block* prev = current;
        if (mptest__leakcheck_block_has_freeable(current)) {
            MN_FREE(current->header);
        }
        current = current->next;
        MN_FREE(prev);
    }
}

/* Reset (NOT destroy) malloc-checking state. */
/* For now, this is equivalent to a destruction. This may not be the case in
 * the future. */
MN_INTERNAL void mptest__leakcheck_reset(struct mptest__state* state)
{
    /* Preserve `test_leak_checking` */
    int test_leak_checking = state->test_leak_checking;
    mptest__leakcheck_destroy(state);
    mptest__leakcheck_init(state);
    state->test_leak_checking = test_leak_checking;
}

/* Check the block record for leaks, returning 1 if there are any. */
MN_INTERNAL int mptest__leakcheck_has_leaks(struct mptest__state* state)
{
    struct mptest__leakcheck_block* current = state->first_block;
    while (current) {
        if (mptest__leakcheck_block_has_freeable(current)) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

MN_API void* mptest__leakcheck_hook_malloc(struct mptest__state* state,
    const char* file, int line, size_t size)
{
    /* Header + actual memory block */
    char* base_ptr;
    /* Identical to `base_ptr` */
    struct mptest__leakcheck_header* header;
    /* Current block*/
    struct mptest__leakcheck_block* block_info;
    /* Pointer to return to the user */
    char* out_ptr;
    if (!state->test_leak_checking) {
        return (char*)MN_MALLOC(size);
    }
    /* Allocate the memory the user requested + space for the header */
    base_ptr = (char*)MN_MALLOC(size + MPTEST__LEAKCHECK_HEADER_SIZEOF);
    if (base_ptr == NULL) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state,
            MPTEST__LONGJMP_REASON_MALLOC_REALLY_RETURNED_NULL, file, line,
            NULL);
    }
    /* Allocate memory for the block_info structure */
    block_info = (struct mptest__leakcheck_block*)MN_MALLOC(
        sizeof(struct mptest__leakcheck_block));
    if (block_info == NULL) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state,
            MPTEST__LONGJMP_REASON_MALLOC_REALLY_RETURNED_NULL, file, line,
            NULL);
    }
    /* Setup the header */
    header = (struct mptest__leakcheck_header*)base_ptr;
    mptest__leakcheck_header_set_guard(header);
    /* Setup the block_info */
    if (state->first_block == NULL) {
        /* If `state->first_block == NULL`, then this is the first allocation.
         * Use NULL as the previous value, and then set the `first_block` and
         * `top_block` to the new block. */
        mptest__leakcheck_block_init(block_info, size, NULL,
            MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL, file, line);
        state->first_block = block_info;
        state->top_block   = block_info;
    } else {
        /* If this isn't the first allocation, use `state->top_block` as the
         * previous block. */
        mptest__leakcheck_block_init(block_info, size, state->top_block,
            MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL, file, line);
        state->top_block = block_info;
    }
    /* Link the header and block_info together */
    mptest__leakcheck_block_link_header(block_info, header);
    /* Return the base pointer offset by the header amount */
    out_ptr = base_ptr + MPTEST__LEAKCHECK_HEADER_SIZEOF;
    /* Increment the total number of allocations */
    state->total_allocations++;
    return out_ptr;
}

MN_API void mptest__leakcheck_hook_free(struct mptest__state* state,
    const char* file, int line, void* ptr)
{
    struct mptest__leakcheck_header* header;
    struct mptest__leakcheck_block*  block_info;
    if (!state->test_leak_checking) {
        MN_FREE(ptr);
        return;
    }
    if (ptr == NULL) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_FREE_OF_NULL, file,
            line, NULL);
    }
    /* Retrieve header by subtracting header size from pointer */
    header = (struct
        mptest__leakcheck_header*)((char*)ptr
                                   - MPTEST__LEAKCHECK_HEADER_SIZEOF);
    /* TODO: check for SIGSEGV here */
    if (!mptest__leakcheck_header_check_guard(header)) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_FREE_OF_INVALID,
            file, line, NULL);
    }
    block_info = header->block;
    /* Ensure that the pointer has not been freed or reallocated already */
    if (block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_FREED) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_FREE_OF_FREED, file,
            line, NULL);
    }
    if (block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_FREE_OF_REALLOCED,
            file, line, NULL);
    }
    /* We can finally `free()` the pointer */
    MN_FREE(header);
    block_info->flags |= MPTEST__LEAKCHECK_BLOCK_FLAG_FREED;
    /* Decrement the total number of allocations */
    state->total_allocations--;
}

MN_API void* mptest__leakcheck_hook_realloc(struct mptest__state* state,
    const char* file, int line, void* old_ptr, size_t new_size)
{
    /* New header + memory */
    char*                           base_ptr;
    struct mptest__leakcheck_header* old_header;
    struct mptest__leakcheck_header* new_header;
    struct mptest__leakcheck_block* old_block_info;
    struct mptest__leakcheck_block*  new_block_info;
    /* Pointer to return to the user */
    char* out_ptr;
    if (!state->test_leak_checking) {
        return (void*)MN_REALLOC(old_ptr, new_size);
    }
    old_header = (struct mptest__leakcheck_header*)((char*)old_ptr
                                            - MPTEST__LEAKCHECK_HEADER_SIZEOF);
    old_block_info = old_header->block;
    if (old_ptr == NULL) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_REALLOC_OF_NULL,
            file, line, NULL);
    }
    if (!mptest__leakcheck_header_check_guard(old_header)) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_REALLOC_OF_INVALID,
            file, line, NULL);
    }
    if (old_block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_FREED) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state, MPTEST__LONGJMP_REASON_REALLOC_OF_FREED,
            file, line, NULL);
    }
    if (old_block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state,
            MPTEST__LONGJMP_REASON_REALLOC_OF_REALLOCED, file, line, NULL);
    }
    /* Allocate the memory the user requested + space for the header */
    base_ptr = (char*)MN_REALLOC(old_header,
        new_size + MPTEST__LEAKCHECK_HEADER_SIZEOF);
    if (base_ptr == NULL) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state,
            MPTEST__LONGJMP_REASON_MALLOC_REALLY_RETURNED_NULL, file, line,
            NULL);
    }
    /* Allocate memory for the new block_info structure */
    new_block_info = (struct mptest__leakcheck_block*)MN_MALLOC(
        sizeof(struct mptest__leakcheck_block));
    if (new_block_info == NULL) {
        state->fail_data.memory_block = NULL;
        mptest__longjmp_exec(state,
            MPTEST__LONGJMP_REASON_MALLOC_REALLY_RETURNED_NULL, file, line,
            NULL);
    }
    /* Setup the header */
    new_header = (struct mptest__leakcheck_header*)base_ptr;
    /* Set the guard again (double bag it per se) */
    mptest__leakcheck_header_set_guard(new_header);
    /* Setup the block_info */
    if (state->first_block == NULL) {
        mptest__leakcheck_block_init(new_block_info, new_size, NULL,
            MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW, file, line);
        state->first_block = new_block_info;
        state->top_block   = new_block_info;
    } else {
        mptest__leakcheck_block_init(new_block_info, new_size,
            state->top_block, MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW, file,
            line);
        state->top_block = new_block_info;
    }
    /* Mark `old_block_info` as reallocation target */
    old_block_info->flags |= MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD;
    /* Link the block with its respective header */
    mptest__leakcheck_block_link_header(new_block_info, new_header);
    /* Finally, indicate the new allocation in the realloc chain */
    old_block_info->realloc_next = new_block_info;
    new_block_info->realloc_prev = old_block_info;
    out_ptr                      = base_ptr + MPTEST__LEAKCHECK_HEADER_SIZEOF;
    return out_ptr;
}

#endif