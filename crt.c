#include "crt.h"
#include "pc.h"

#include <stdio.h>

#if defined(USE_CRT_OBJPOOLS) && (USE_CRT_OBJPOOLS != 0)
size_t crt_objpool_claim(crt_objpool_alloc_state_t *map, size_t nobjs)
{
	for (size_t i = 0u; i < nobjs; ++i)
	{
		bool was_in_use = false;
		if (atomic_compare_exchange_strong(&map[i], &was_in_use, true))
		{
			/* We claimed the object */
			assert (!was_in_use);
			return i;
		}
	}

	/* No free objects found */
	return SIZE_MAX;
}

void crt_objpool_release(crt_objpool_alloc_state_t *map, size_t idx)
{
	bool was_allocated = atomic_exchange(&map[idx], false);
	assert (was_allocated);
}
#endif

#if defined(USE_CRT_HEAP) && (USE_CRT_HEAP != 0)
void* crt_heap_alloc(crt_heap_t *heap, size_t n)
{
	size_t n_alloc = CRT_HEAP_ALIGN * ((n + CRT_HEAP_ALIGN - 1) / CRT_HEAP_ALIGN);
	size_t n_avail = heap->heap_size - (heap->heap_brk - heap->heap_base);
	void* p = NULL;

	// TODO: Atomic support
	if (n_alloc <= n_avail)
	{
		p = (void *) heap->heap_brk;
		heap->heap_brk += n_alloc;

		memset(p, 0, n_alloc);
	}
	else
	{
		fprintf(stderr, "out of heap memory\n");
		abort();
	}
	return p;
}

void crt_heap_free(crt_heap_t *heap, void *p)
{
	/* No deallocation implementated */
}

CRT_HEAP_DEFINE(gen_heap, gen_heap_alloc, gen_heap_free, 1024u * 1024u)
#endif

#ifndef BUILD_ESP32
void *pcmalloc(long size)
{
	return gen_heap_alloc(size);
}

void *psmalloc(size_t size)
{
	return gen_heap_alloc(size);	
}
#endif
