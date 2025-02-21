#pragma once

#include <stdint.h>

#ifndef __cplusplus
#define constexpr
#endif

//Convenient defines for all targets
constexpr size_t PageShift = 12;
constexpr size_t PageSize = (1 << PageShift);
#define PAGESIZE (1 << PageShift)
constexpr size_t PageMask = 0xFFF;

typedef uintptr_t paddr_t;

inline constexpr size_t DivRoundUp(const size_t x, const size_t y)
{
	return (x + y - 1) / y;
}

inline constexpr size_t SizeToPages(const size_t bytes)
{
	return DivRoundUp(bytes, PAGESIZE);
}

constexpr size_t ByteAlign(const size_t size, const size_t alignment)
{
	const size_t mask = alignment - 1;
	return ((size + mask) & ~(mask));
}

constexpr size_t PageAlign(const size_t size)
{
	return ByteAlign(size, PAGESIZE);
}

#ifdef __cplusplus
template<typename T>
constexpr T MakePointer(const void* base, const ptrdiff_t offset = 0)
{
	return reinterpret_cast<T>((char*)base + offset);
}
#endif