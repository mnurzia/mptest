#include "mptest_internal.h"

#if MPTEST_USE_LEAKCHECK

/* Set the guard bytes in `header`. */
MN_INTERNAL void
mptest__leakcheck_header_set_guard(struct mptest__leakcheck_header* header)
{
  /* Currently we choose 0xCC as the guard byte, it's a stripe of ones and
   * zeroes that looks like 11001100b */
  size_t i;
  for (i = 0; i < MPTEST__LEAKCHECK_GUARD_BYTES_COUNT; i++) {
    header->guard_bytes[i] = 0xCC;
  }
}

/* Ensure that `header` has valid guard bytes. */
MN_INTERNAL int
mptest__leakcheck_header_check_guard(struct mptest__leakcheck_header* header)
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
MN_INTERNAL int
mptest__leakcheck_block_has_freeable(struct mptest__leakcheck_block* block)
{
  /* We can free the pointer if it was not freed or was not reallocated
   * earlier. */
  return !(
      block->flags & (MPTEST__LEAKCHECK_BLOCK_FLAG_FREED |
                      MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD));
}

/* Initialize a `struct mptest__leakcheck_block`.
 * If `prev` is NULL, then this function will not attempt to link `block` to
 * any previous element in the malloc linked list. */
MN_INTERNAL void mptest__leakcheck_block_init(
    struct mptest__leakcheck_block* block, size_t size,
    struct mptest__leakcheck_block* prev,
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
  block->flags = flags;
  /* Save source info */
  block->file = file;
  block->line = line;
}

/* Link a block to its respective header. */
MN_INTERNAL void mptest__leakcheck_block_link_header(
    struct mptest__leakcheck_block* block,
    struct mptest__leakcheck_header* header)
{
  block->header = header;
  header->block = block;
}

/* Initialize malloc-checking state. */
MN_INTERNAL void mptest__leakcheck_init(struct mptest__state* state)
{
  mptest__leakcheck_state* leakcheck_state = &state->leakcheck_state;
  leakcheck_state->test_leak_checking = 0;
  leakcheck_state->first_block = NULL;
  leakcheck_state->top_block = NULL;
  leakcheck_state->total_allocations = 0;
  leakcheck_state->total_calls = 0;
  leakcheck_state->oom_failed = 0;
  leakcheck_state->oom_fail_call = -1;
  leakcheck_state->fall_through = 0;
}

/* Destroy malloc-checking state. */
MN_INTERNAL void mptest__leakcheck_destroy(struct mptest__state* state)
{
  /* Walk the malloc list, destroying everything */
  struct mptest__leakcheck_block* current = state->leakcheck_state.first_block;
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
  mptest__leakcheck_mode test_leak_checking =
      state->leakcheck_state.test_leak_checking;
  /* Preserve fall_through */
  int fall_through = state->leakcheck_state.fall_through;
  mptest__leakcheck_destroy(state);
  mptest__leakcheck_init(state);
  state->leakcheck_state.test_leak_checking = test_leak_checking;
  state->leakcheck_state.fall_through = fall_through;
}

/* Check the block record for leaks, returning 1 if there are any. */
MN_INTERNAL int mptest__leakcheck_has_leaks(struct mptest__state* state)
{
  struct mptest__leakcheck_block* current = state->leakcheck_state.first_block;
  while (current) {
    if (mptest__leakcheck_block_has_freeable(current)) {
      return 1;
    }
    current = current->next;
  }
  return 0;
}

MN_API void* mptest__leakcheck_hook_malloc(
    struct mptest__state* state, const char* file, int line, size_t size)
{
  /* Header + actual memory block */
  char* base_ptr;
  /* Identical to `base_ptr` */
  struct mptest__leakcheck_header* header;
  /* Current block*/
  struct mptest__leakcheck_block* block_info;
  /* Pointer to return to the user */
  char* out_ptr;
  struct mptest__leakcheck_state* leakcheck_state = &state->leakcheck_state;
  if (!leakcheck_state->test_leak_checking) {
    return (char*)MN_MALLOC(size);
  }
  if (leakcheck_state->test_leak_checking == MPTEST__LEAKCHECK_MODE_OOM_ONE) {
    if (leakcheck_state->total_calls == leakcheck_state->oom_fail_call) {
      leakcheck_state->total_calls++;
      mptest_ex_oom_inject();
      return NULL;
    }
  }
  if (leakcheck_state->test_leak_checking == MPTEST__LEAKCHECK_MODE_OOM_SET) {
    if (leakcheck_state->total_calls == leakcheck_state->oom_fail_call) {
      mptest_ex_oom_inject();
      return NULL;
    }
  }
  if (leakcheck_state->fall_through) {
    leakcheck_state->total_calls++;
    return (char*)MN_MALLOC(size);
  }
  /* Allocate the memory the user requested + space for the header */
  base_ptr = (char*)MN_MALLOC(size + MPTEST__LEAKCHECK_HEADER_SIZEOF);
  if (base_ptr == NULL) {
    state->fail_data.memory_block = NULL;
    mptest_ex_nomem();
    mptest__longjmp_exec(state, MPTEST__FAIL_REASON_NOMEM, file, line, NULL);
  }
  /* Allocate memory for the block_info structure */
  block_info = (struct mptest__leakcheck_block*)MN_MALLOC(
      sizeof(struct mptest__leakcheck_block));
  if (block_info == NULL) {
    state->fail_data.memory_block = NULL;
    mptest_ex_nomem();
    mptest__longjmp_exec(state, MPTEST__FAIL_REASON_NOMEM, file, line, NULL);
  }
  /* Setup the header */
  header = (struct mptest__leakcheck_header*)base_ptr;
  mptest__leakcheck_header_set_guard(header);
  /* Setup the block_info */
  if (leakcheck_state->first_block == NULL) {
    /* If `state->first_block == NULL`, then this is the first allocation.
     * Use NULL as the previous value, and then set the `first_block` and
     * `top_block` to the new block. */
    mptest__leakcheck_block_init(
        block_info, size, NULL, MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL, file,
        line);
    leakcheck_state->first_block = block_info;
    leakcheck_state->top_block = block_info;
  } else {
    /* If this isn't the first allocation, use `state->top_block` as the
     * previous block. */
    mptest__leakcheck_block_init(
        block_info, size, leakcheck_state->top_block,
        MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL, file, line);
    leakcheck_state->top_block = block_info;
  }
  /* Link the header and block_info together */
  mptest__leakcheck_block_link_header(block_info, header);
  /* Return the base pointer offset by the header amount */
  out_ptr = base_ptr + MPTEST__LEAKCHECK_HEADER_SIZEOF;
  /* Increment the total number of allocations */
  leakcheck_state->total_allocations++;
  /* Increment the total number of calls */
  leakcheck_state->total_calls++;
  return out_ptr;
}

MN_API void mptest__leakcheck_hook_free(
    struct mptest__state* state, const char* file, int line, void* ptr)
{
  struct mptest__leakcheck_header* header;
  struct mptest__leakcheck_block* block_info;
  struct mptest__leakcheck_state* leakcheck_state = &state->leakcheck_state;
  if (!leakcheck_state->test_leak_checking || leakcheck_state->fall_through) {
    MN_FREE(ptr);
    return;
  }
  if (ptr == NULL) {
    state->fail_data.memory_block = NULL;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_FREE_OF_NULL, file, line, NULL);
  }
  /* Retrieve header by subtracting header size from pointer */
  header =
      (struct
       mptest__leakcheck_header*)((char*)ptr - MPTEST__LEAKCHECK_HEADER_SIZEOF);
  /* TODO: check for SIGSEGV here */
  if (!mptest__leakcheck_header_check_guard(header)) {
    state->fail_data.memory_block = ptr;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_FREE_OF_INVALID, file, line, NULL);
  }
  block_info = header->block;
  /* Ensure that the pointer has not been freed or reallocated already */
  if (block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_FREED) {
    state->fail_data.memory_block = ptr;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_REALLOC_OF_FREED, file, line, NULL);
  }
  if (block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD) {
    state->fail_data.memory_block = ptr;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_FREE_OF_REALLOCED, file, line, NULL);
  }
  /* We can finally `free()` the pointer */
  MN_FREE(header);
  block_info->flags |= MPTEST__LEAKCHECK_BLOCK_FLAG_FREED;
  /* Decrement the total number of allocations */
  leakcheck_state->total_allocations--;
}

MN_API void* mptest__leakcheck_hook_realloc(
    struct mptest__state* state, const char* file, int line, void* old_ptr,
    size_t new_size)
{
  /* New header + memory */
  char* base_ptr;
  struct mptest__leakcheck_header* old_header;
  struct mptest__leakcheck_header* new_header;
  struct mptest__leakcheck_block* old_block_info;
  struct mptest__leakcheck_block* new_block_info;
  /* Pointer to return to the user */
  char* out_ptr;
  struct mptest__leakcheck_state* leakcheck_state = &state->leakcheck_state;
  if (!leakcheck_state->test_leak_checking) {
    return (void*)MN_REALLOC(old_ptr, new_size);
  }
  if (leakcheck_state->test_leak_checking == MPTEST__LEAKCHECK_MODE_OOM_ONE) {
    if (leakcheck_state->total_calls == leakcheck_state->oom_fail_call) {
      leakcheck_state->total_calls++;
      mptest_ex_oom_inject();
      return NULL;
    }
  }
  if (leakcheck_state->test_leak_checking == MPTEST__LEAKCHECK_MODE_OOM_SET) {
    if (leakcheck_state->total_calls == leakcheck_state->oom_fail_call) {
      mptest_ex_oom_inject();
      return NULL;
    }
  }
  if (leakcheck_state->fall_through) {
    leakcheck_state->total_calls++;
    return (char*)MN_REALLOC(old_ptr, new_size);
  }
  old_header =
      (struct
       mptest__leakcheck_header*)((char*)old_ptr - MPTEST__LEAKCHECK_HEADER_SIZEOF);
  old_block_info = old_header->block;
  if (old_ptr == NULL) {
    state->fail_data.memory_block = NULL;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_REALLOC_OF_NULL, file, line, NULL);
  }
  if (!mptest__leakcheck_header_check_guard(old_header)) {
    state->fail_data.memory_block = old_ptr;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_REALLOC_OF_INVALID, file, line, NULL);
  }
  if (old_block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_FREED) {
    state->fail_data.memory_block = old_ptr;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_REALLOC_OF_FREED, file, line, NULL);
  }
  if (old_block_info->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD) {
    state->fail_data.memory_block = old_ptr;
    mptest_ex_bad_alloc();
    mptest__longjmp_exec(
        state, MPTEST__FAIL_REASON_REALLOC_OF_REALLOCED, file, line, NULL);
  }
  /* Allocate the memory the user requested + space for the header */
  base_ptr =
      (char*)MN_REALLOC(old_header, new_size + MPTEST__LEAKCHECK_HEADER_SIZEOF);
  if (base_ptr == NULL) {
    state->fail_data.memory_block = old_ptr;
    mptest_ex_nomem();
    mptest__longjmp_exec(state, MPTEST__FAIL_REASON_NOMEM, file, line, NULL);
  }
  /* Allocate memory for the new block_info structure */
  new_block_info = (struct mptest__leakcheck_block*)MN_MALLOC(
      sizeof(struct mptest__leakcheck_block));
  if (new_block_info == NULL) {
    state->fail_data.memory_block = old_ptr;
    mptest_ex_nomem();
    mptest__longjmp_exec(state, MPTEST__FAIL_REASON_NOMEM, file, line, NULL);
  }
  /* Setup the header */
  new_header = (struct mptest__leakcheck_header*)base_ptr;
  /* Set the guard again (double bag it per se) */
  mptest__leakcheck_header_set_guard(new_header);
  /* Setup the block_info */
  if (leakcheck_state->first_block == NULL) {
    mptest__leakcheck_block_init(
        new_block_info, new_size, NULL,
        MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW, file, line);
    leakcheck_state->first_block = new_block_info;
    leakcheck_state->top_block = new_block_info;
  } else {
    mptest__leakcheck_block_init(
        new_block_info, new_size, leakcheck_state->top_block,
        MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW, file, line);
    leakcheck_state->top_block = new_block_info;
  }
  /* Mark `old_block_info` as reallocation target */
  old_block_info->flags |= MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD;
  /* Link the block with its respective header */
  mptest__leakcheck_block_link_header(new_block_info, new_header);
  /* Finally, indicate the new allocation in the realloc chain */
  old_block_info->realloc_next = new_block_info;
  new_block_info->realloc_prev = old_block_info;
  out_ptr = base_ptr + MPTEST__LEAKCHECK_HEADER_SIZEOF;
  /* Increment the total number of calls */
  leakcheck_state->total_calls++;
  return out_ptr;
}

MN_API void mptest__leakcheck_set(struct mptest__state* state, int on)
{
  state->leakcheck_state.test_leak_checking = on;
}

MN_INTERNAL mptest__result mptest__leakcheck_oom_run_test(
    struct mptest__state* state, mptest__test_func test_func)
{
  int max_iter = 0;
  int i;
  mptest__result res = MPTEST__RESULT_PASS;
  struct mptest__leakcheck_state* leakcheck_state = &state->leakcheck_state;
  leakcheck_state->oom_fail_call = -1;
  leakcheck_state->oom_failed = 0;
  mptest__leakcheck_reset(state);
  res = mptest__state_do_run_test(state, test_func);
  max_iter = leakcheck_state->total_calls;
  if (res) {
    /* Initial test failed. */
    return res;
  }
  for (i = 0; i < max_iter; i++) {
    int should_finish = 0;
    mptest__leakcheck_reset(state);
    leakcheck_state->oom_fail_call = i;
    res = mptest__state_do_run_test(state, test_func);
    if (res != MPTEST__RESULT_PASS) {
      should_finish = 1;
    }
    if (mptest__leakcheck_has_leaks(state)) {
      should_finish = 1;
    }
    if (should_finish) {
      /* Save fail context */
      leakcheck_state->oom_failed = 1;
      break;
    }
  }
  return res;
}

MN_API void mptest_ex_nomem(void) { mptest_ex(); }
MN_API void mptest_ex_oom_inject(void) { mptest_ex(); }
MN_API void mptest_ex_bad_alloc(void) { mptest_ex(); }

MN_API void mptest_malloc_dump(void)
{
  struct mptest__state* state = &mptest__state_g;
  struct mptest__leakcheck_state* leakcheck_state = &state->leakcheck_state;
  struct mptest__leakcheck_block* block = leakcheck_state->first_block;
  while (block) {
    printf(
        "%p: %u bytes at %s:%i: %s%s%s%s",
        (void*)(((char*)block->header) + MPTEST__LEAKCHECK_HEADER_SIZEOF),
        (unsigned int)block->block_size, block->file, block->line,
        (block->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_INITIAL) ? "I" : "-",
        (block->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_FREED) ? "F" : "-",
        (block->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_OLD) ? "O" : "-",
        (block->flags & MPTEST__LEAKCHECK_BLOCK_FLAG_REALLOC_NEW) ? "N" : "-");
    if (block->realloc_prev) {
      printf(
          " from %p",
          (void*)(((char*)block->realloc_prev->header) + MPTEST__LEAKCHECK_HEADER_SIZEOF));
    }
    printf("\n");
    block = block->next;
  }
}

#endif
