#include <cstdlib>
#include <time.h>

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <iostream>

#include "thorin_utils.h"


// common implementations of runtime utility functions

#ifdef _MSC_VER // VS does not even have type struct timespec

#include "Windows.h"

struct clock_sync {
    long long TICKS_PER_MICRO_SECOND;

    clock_sync() {
        LARGE_INTEGER f;
        if (!QueryPerformanceFrequency(&f)) {
            TICKS_PER_MICRO_SECOND = 1;
            return;
        }
        TICKS_PER_MICRO_SECOND = f.QuadPart / 1000000LL;
    }
};

static clock_sync CLOCK;

long long thorin_get_micro_time() {
    LARGE_INTEGER tc;
    if (!QueryPerformanceCounter(&tc))
        return 0;
    return tc.QuadPart / CLOCK.TICKS_PER_MICRO_SECOND;
}

#else // _MSC_VER

long long thorin_get_micro_time() {
    struct timespec now;
    #ifdef __APPLE__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    now.tv_sec = mts.tv_sec;
    now.tv_nsec = mts.tv_nsec;
    #else
    clock_gettime(CLOCK_MONOTONIC, &now);
    #endif

    long long time = now.tv_sec*1000000LL + now.tv_nsec / 1000LL;
    return time;
}
#endif // _MSC_VER

void thorin_print_micro_time(long long time) {
    std::cerr << "   timing: " << time * 1.0e-3f << "(ms)" << std::endl;
}
void thorin_print_gflops(float f) { printf("GFLOPS: %f\n", f); }

#ifdef _MSC_VER
double drand48() {
    return ((double)rand() / (RAND_MAX + 1));
}
#endif // _MSC_VER

float thorin_random_val(int max) {
#ifdef _MSC_VER
	return ((float)rand() / RAND_MAX) * max;
#else // _MSC_VER
    return ((float)random() / RAND_MAX) * max;
#endif // _MS_VER
}
