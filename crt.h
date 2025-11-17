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

/* Simplified object pool implementation (allocation from named heap, nobjs is currently ignored) */
#define CRT_DEFINE_OBJPOOL(prefix,objtype,nobjs,heap_name) \
	static objtype* prefix##_objpool_alloc(void) \
	{ \
		return heap_name##_heap_alloc(sizeof(objtype)); \
	} \
	static void prefix##_objpool_free(objtype *obj) \
	{ \
		heap_name##_heap_free(obj); \
	}

#define g_new(t, n) bigmalloc(sizeof(t) * (n))
#define g_free(p) /*free(p)*/

#endif /* CRT_H */
