#include "crt.h"
#include "pc.h"

#include <stdio.h>

#ifndef NANO386_PLATFORM
#include <time.h>
#include <unistd.h>
#endif

void crt_usleep(unsigned long delay)
{
#ifndef NANO386_PLATFORM
	usleep(delay);
#else
	// FIXME: usleep(1) is present in 4.3BSD, POSIX.1-2001 (and removed in POSIX.1-2008).
	// Need a HAL wrapper
#endif
}

uint64_t crt_clock_get_ns(void)
{
#ifndef NANO386_PLATFORM
	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t) ts.tv_sec * 1000000000ull +
	    (uint64_t) ts.tv_nsec);
#else
	// FIXME: Monotonic clock source
	return 1000ull * get_uticks();
#endif
}

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

// TODO: Provide means to initialize the heap size at runtime.
// This could be done by adding an early crt_init call (either in main or in pc_new).
// For a freestanding environment we may want to resort to an alternative approach
// (could be from linker map, device tree, or similar mechanism, ...)
//
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
