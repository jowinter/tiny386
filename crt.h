#ifndef CRT_H
#define CRT_H

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdlib.h>
#include <string.h>

#define CRT_HEAP_DECLARE(heap_name)    \
	void* heap_name##_alloc(size_t n); \
	void heap_name##_free(void *p);


/* Heap for generic (small) object allocations (pcalloc) */
CRT_HEAP_DECLARE(gen_heap)

/* Heap for big allocations (bigmalloc, psmalloc) */
CRT_HEAP_DECLARE(big_heap)

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
