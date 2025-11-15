#include "crt.h"

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
