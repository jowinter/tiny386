#ifndef CRT_H
#define CRT_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdlib.h>
#include <string.h>

#define USE_CRT_HEAP 1

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

void* crt_heap_alloc(crt_heap_t *heap, size_t n);
void crt_heap_free(crt_heap_t *heap, void *p);

#define CRT_HEAP_DECLARE(heap_name,allocfn,freefn) \
	void* allocfn(size_t n); \
	void freefn(void *p);

#define CRT_HEAP_DEFINE(heap_name,allocfn,freefn,hsize) \
	static crt_heap_cell_t heap_name##_cells[(hsize + CRT_HEAP_ALIGN - 1) / CRT_HEAP_ALIGN]; \
	static crt_heap_t heap_name##_heap = { \
		.heap_base = (uintptr_t) (&heap_name##_cells), \
		.heap_size = sizeof(heap_name##_cells), \
		.heap_brk  = (uintptr_t) (&heap_name##_cells), \
	}; \
	void* allocfn(size_t n) \
	{ \
		return crt_heap_alloc(&heap_name##_heap, n); \
	} \
	void freefn(void *p) \
	{ \
		crt_heap_free(&heap_name##_heap, p); \
	}

#else

static inline void* crt_heap_alloc(crt_heap_t *heap, size_t n)
{
	(void) heap;
	return calloc(1u, sizeof(objtype));
}

static inline void crt_heap_free(crt_heap_t *heap, void *p)
{
	(void) heap;
	free(p);
}

#define CRT_HEAP_DECLARE(heap_name,allocfn,freefn) \
	static inline void* allocfn(size_t n) \
	{ \
		return crt_heap_alloc(NULL, n); \
	} \
	static inline void freefn(void *p) \
	{ \
		crt_heap_free(NULL, p); \
	}
	
#define CRT_HEAP_CREATE(heap_name,allocfn,freefn,heap_size)

#endif

CRT_HEAP_DECLARE(gen_heap, gen_heap_alloc, gen_heap_free)

#if defined(USE_CRT_OBJPOOLS) && (USE_CRT_OBJPOOLS != 0)

#include <stdatomic.h>
typedef volatile atomic_bool crt_objpool_alloc_state_t;

size_t crt_objpool_claim(crt_objpool_alloc_state_t *map, size_t nobjs);
void crt_objpool_release(crt_objpool_alloc_state_t *map, size_t idx);

/* Static object pools are in use */
#define CRT_DEFINE_OBJPOOL(prefix,objtype,nobjs) \
	static objtype prefix##_objpool_instances[(nobjs)]; \
	static crt_objpool_alloc_state_t prefix##_objpool_allocmap[(nobjs)]; \
	static objtype* prefix##_objpool_alloc(void) \
	{ \
		size_t i = crt_objpool_claim(&prefix##_objpool_allocmap[i], (size_t) (nobjs)); \
		objtype* obj = NULL; \
		if (i != SIZE_MAX) \
		{ \
			/* objects in the pool start as zero-initialized (.bss), and are cleared */ \
			/* cleared on free (need to clear twice).                                */ \
			/* memset(obj, 0, sizeof(objtype));                                      */ \
			obj = &prefix##_objpool_instances[i]; \
		} \
		return obj; \
	} \
	static inline size_t prefix##_objpool_index_of(objtype *obj) \
	{ \
		size_t idx = obj - &prefix##_objpool_instances[0u]; \
		assert (idx < (size_t) (nobjs)); \
		return idx; \
	} \
	static void prefix##_objpool_free(objtype *obj) \
	{ \
		if (obj) \
		{ \
			size_t i = prefix##_objpool_index_of(obj); \
			/* see the alloc() function; we ensure that objects are clean when */ \
			/* they are handed back to the pool. */ \
			memset(&prefix##_objpool_instances[i], 0, sizeof(objtype)); \
			crt_objpool_release(&prefix##_objpool_allocmap[i], i); \
		} \
	}

#else
/* No object pools in use (defer to calloc) */
#define CRT_DEFINE_OBJPOOL(prefix,objtype,nobjs) \
	static objtype* prefix##_objpool_alloc(void) \
	{ \
		return gen_heap_alloc(sizeof(objtype)); \
	} \
	static inline void prefix##_objpool_free(objtype *obj) \
	{ \
		gen_heap_free(obj); \
	}

#endif

#define g_new(t, n) bigmalloc(sizeof(t) * (n))
#define g_free(p) /*free(p)*/

#endif /* CRT_H */
