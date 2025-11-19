#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "crt.h"

int main(void)
{
	return EXIT_SUCCESS;
}


void *bigmalloc(size_t size)
{
	return malloc(size);
}

uint32_t get_uticks()
{
	// FIXME: Monotonic clock source
	static uint32_t fake_ticks = 0u;
	return fake_ticks++;
}

void CtrlC()
{
	abort();
}

void ResetKeyboardInput()
{

}

void CaptureKeyboardInput()
{

}

int ReadKBByte()
{
	// FIXME!
	return 0;
}

int IsKBHit()
{
	// FIXME!
	return 0;
}