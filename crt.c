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


#define CRT_HEAP_DEFINE_CUSTOM(heap_name,allocfn,freefn) \
	void* heap_name##_alloc(size_t n) \
	{ \
		void *p = allocfn(n); \
		if (p) \
		{ \
			memset(p, 0, n); \
		} \
		return p; \
	} \
	void heap_name##_free(void *p) \
	{ \
		freefn(p); \
	}

#if defined(USE_CRT_HEAP) && (USE_CRT_HEAP != 0)
typedef struct crt_heap {
	uintptr_t heap_base;
	uintptr_t heap_size;
	uintptr_t heap_brk;
} crt_heap_t;

typedef union crt_heap_cell {
	uint64_t  i;
	uintptr_t u;
	double    d;
	void*     p;
} crt_heap_cell_t;

#define CRT_HEAP_ALIGN (sizeof(crt_heap_cell_t))

static void* crt_heap_alloc(crt_heap_t *heap, size_t n)
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

static void crt_heap_free(crt_heap_t *heap, void *p)
{
	/* No deallocation implementated */
}

#define CRT_HEAP_DEFINE(heap_name,hsize) \
	static crt_heap_cell_t heap_name##_cells[(hsize + CRT_HEAP_ALIGN - 1) / CRT_HEAP_ALIGN]; \
	static crt_heap_t heap_name##_heap = { \
		.heap_base = (uintptr_t) (&heap_name##_cells), \
		.heap_size = sizeof(heap_name##_cells), \
		.heap_brk  = (uintptr_t) (&heap_name##_cells), \
	}; \
	void* heap_name##_alloc(size_t n) \
	{ \
		return crt_heap_alloc(&heap_name##_heap, n); \
	} \
	void heap_name##_free(void *p) \
	{ \
		crt_heap_free(&heap_name##_heap, p); \
	}

CRT_HEAP_DEFINE(gen_heap,       1024u * 1024u) // 1M generic object heap
CRT_HEAP_DEFINE(big_heap, 32u * 1024u * 1024u) // 32M "big" heap

#else
/* Delegate to standard C heap */
CRT_HEAP_DEFINE_CUSTOM(gen_heap, malloc, free)
CRT_HEAP_DEFINE_CUSTOM(big_heap, malloc, free)
#endif

#ifndef BUILD_ESP32
void *pcmalloc(long size)
{
	return gen_heap_alloc(size);
}

void *psmalloc(size_t size)
{
	return big_heap_alloc(size);
}
#endif
