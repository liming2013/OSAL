/*
 * Copyright (C) 2016 Alexandre Bustico, Gautier Hattenberger
 *
 * This file is part of paparazzi.
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file arch/chibios/modules/tlsf/tlsf_malloc_arch.c
 *
 * Dynamic memory allocation based on TLSF library.
 *
 */

#include <tlsf.h>
#include "tlsf_malloc.h"

typedef uint32_t mutex_t;

struct _tlsf_memory_heap_t {
  tlsf_t tlsf;
  mutex_t *mtx;
};

/**
 * @brief   Static mutex initializer.
 * @details Statically initialized mutexes require no explicit initialization
 *          using @p chMtxInit().
 *
 * @param[in] name      the name of the mutex variable
 */
#define MUTEX_DECL(name) mutex_t name;

#ifdef HEAP_CCM
#define HEAP_CCM_BUFFER HEAP_CCM ## _buffer
#define HEAP_CCM_MTX    HEAP_CCM ## _mtx
static uint8_t HEAP_CCM_BUFFER[HEAP_CCM_SIZE] ;
static MUTEX_DECL(HEAP_CCM_MTX);
tlsf_memory_heap_t HEAP_CCM;
#endif

#ifdef HEAP_SRAM
#define HEAP_SRAM_BUFFER HEAP_SRAM ## _buffer
#define HEAP_SRAM_MTX    HEAP_SRAM ## _mtx
static uint8_t HEAP_SRAM_BUFFER[HEAP_SRAM_SIZE] ;
static MUTEX_DECL(HEAP_SRAM_MTX);
tlsf_memory_heap_t HEAP_SRAM;
#endif

#ifdef HEAP_EXTERN
#define HEAP_EXTERN_BUFFER HEAP_EXTERN ## _buffer
#define HEAP_EXTERN_MTX    HEAP_EXTERN ## _mtx
static uint8_t HEAP_EXTERN_BUFFER[HEAP_EXTERN_SIZE] ;
static MUTEX_DECL(HEAP_EXTERN_MTX);
tlsf_memory_heap_t HEAP_EXTERN;
#endif

static void stat_tlsf_walker(void *ptr, size_t size, int used, void *user);
static void chMtxUnlock(mutex_t *mp);
static void chMtxLock(mutex_t *mp);

void tlsf_init_heaps(void)
{
#ifdef HEAP_CCM
  HEAP_CCM.mtx = &HEAP_CCM_MTX;
  HEAP_CCM.tlsf = tlsf_create_with_pool(HEAP_CCM_BUFFER, HEAP_CCM_SIZE);
#endif
#ifdef HEAP_SRAM
  HEAP_SRAM.mtx = &HEAP_SRAM_MTX;
  HEAP_SRAM.tlsf = tlsf_create_with_pool(HEAP_SRAM_BUFFER, HEAP_SRAM_SIZE);
#endif
#ifdef HEAP_EXTERN
  HEAP_EXTERN.mtx = &HEAP_EXTERN_MTX;
  HEAP_EXTERN.tlsf = tlsf_create_with_pool(HEAP_EXTERN_BUFFER, HEAP_EXTERN_SIZE);
#endif
}

void *tlsf_get_heap_addr(const tlsf_memory_heap_t *heap)
{
  return heap->tlsf;
}

void *tlsf_malloc_r(tlsf_memory_heap_t *heap, size_t bytes)
{
  chMtxLock(heap->mtx);
  void *ret = tlsf_malloc(heap->tlsf, bytes);
  chMtxUnlock(heap->mtx);
  return ret;
}

void *tlsf_memalign_r(tlsf_memory_heap_t *heap, size_t align, size_t bytes)
{
  chMtxLock(heap->mtx);
  void *ret = tlsf_memalign(heap->tlsf, align, bytes);
  chMtxUnlock(heap->mtx);
  return ret;
}

void *tlsf_realloc_r(tlsf_memory_heap_t *heap, void *ptr, size_t bytes)
{
  chMtxLock(heap->mtx);
  void *ret = tlsf_realloc(heap->tlsf, ptr, bytes);
  chMtxUnlock(heap->mtx);
  return ret;
}

void  tlsf_free_r(tlsf_memory_heap_t *heap, void *ptr)
{
  chMtxLock(heap->mtx);
  tlsf_free(heap->tlsf, ptr);
  chMtxUnlock(heap->mtx);
}


void tlsf_stat_r(tlsf_memory_heap_t *heap, struct tlsf_stat_t *stat)
{
  stat->mused = stat->mfree = 0;
  chMtxLock(heap->mtx);
  tlsf_walk_pool(tlsf_get_pool(heap->tlsf),  &stat_tlsf_walker, stat);
  chMtxUnlock(heap->mtx);
}


/* Returns nonzero if any internal consistency check fails. */
int tlsf_check_r(tlsf_memory_heap_t *heap)
{
  int ret = 0;
  chMtxLock(heap->mtx);
  ret = tlsf_check(heap->tlsf);
  if (ret == 0) {
    ret = tlsf_check_pool(tlsf_get_pool(heap->tlsf));
  }

  chMtxUnlock(heap->mtx);
  return ret;
}


static void stat_tlsf_walker(void *ptr, size_t size, int used, void *user)
{
  (void) ptr;
  struct tlsf_stat_t *tstat = (struct tlsf_stat_t *) user;
  if (used) {
    tstat->mused  += size;
  } else {
    tstat->mfree += size;
  }
}

/**
 * @brief   Locks the specified mutex.
 * @post    The mutex is locked and inserted in the per-thread stack of owned
 *          mutexes.
 *
 * @param[in] mp        pointer to the @p mutex_t structure
 *
 * @api
 */
static void chMtxLock(mutex_t *mp) {

}

/**
 * @brief   Unlocks the specified mutex.
 * @note    Mutexes must be unlocked in reverse lock order. Violating this
 *          rules will result in a panic if assertions are enabled.
 * @pre     The invoking thread <b>must</b> have at least one owned mutex.
 * @post    The mutex is unlocked and removed from the per-thread stack of
 *          owned mutexes.
 *
 * @param[in] mp        pointer to the @p mutex_t structure
 *
 * @api
 */
static void chMtxUnlock(mutex_t *mp) {

}
