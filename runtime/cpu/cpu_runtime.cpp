#include <stdlib.h>
#include <iostream>

#include "thorin_runtime.h"

// helper functions
void thorin_init() { }

#ifdef _MSC_VER

#include <malloc.h>

void *thorin_malloc(size_t size) {
	void *mem = _aligned_malloc(size, 64);
	std::cerr << " * malloc(" << size << ") -> " << mem << std::endl;
	return mem;
}

void thorin_free(void *ptr) {
    _aligned_free(ptr);
}

#else // _MSC_VER

void *thorin_malloc(size_t size) {
    void *mem;
    posix_memalign(&mem, 64, size);
    std::cerr << " * malloc(" << size << ") -> " << mem << std::endl;
    return mem;
}

void thorin_free(void *ptr) {
    free(ptr);
}

#endif // _MSC_VER

void thorin_print_total_timing() { }

