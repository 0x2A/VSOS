#pragma once
#include <cstdint>


//Time
typedef uint64_t milli_t;//Time in milliseconds
typedef uint64_t nano_t;//Time in nanoseconds
static constexpr nano_t Second = 1'000'000'000; //1billion nano seconds
constexpr nano_t ToNano(const milli_t time)
{
	return time * Second / 1000;
}
//typedef uint64_t nano100_t;//Time in 100 nanoseconds
//static constexpr nano100_t Second100Ns = Second / 100; //# of 100ns segments in a second

