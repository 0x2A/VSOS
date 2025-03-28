#include <stdint.h>
#include <exception>
#include <intrin.h>

//these are empty function definitions so linker doesn't complain about missing functions.

namespace std
{
	void __cdecl _Xbad_alloc()
	{

	}
	void __cdecl _Xinvalid_argument(const char*)
	{

	}
	void __cdecl _Xlength_error(const char*)
	{

	}
	void __cdecl _Xout_of_range(const char*)
	{

	}
	void __cdecl _Xoverflow_error(const char*)
	{

	}
	void __cdecl _Xruntime_error(const char*)
	{

	}

	_Prhand _Raise_handler = nullptr; // define raise handler pointer
}

extern "C" void _cdecl _invoke_watson(wchar_t const* _Expression, wchar_t const* _FunctionName, wchar_t const* _FileName, unsigned int _LineNo, uintptr_t _Reserved)
{

}

void __cdecl _invalid_parameter(char const*, char const*, char const*, unsigned int, int)
{

}


extern "C" void __cdecl _invalid_parameter(
	wchar_t const* const expression,
	wchar_t const* const function_name,
	wchar_t const* const file_name,
	unsigned int   const line_number,
	uintptr_t      const reserved
)
{

}

extern "C" __declspec(noreturn) void __cdecl _invalid_parameter_noinfo_noreturn()
{
	_invalid_parameter(nullptr, nullptr, nullptr, 0, 0);
	_invoke_watson(nullptr, nullptr, nullptr, 0, 0);
}

extern "C" int _cdecl _purecall(void)
{
	return 0;
}


struct _Find_traits_8 {
#ifndef _M_ARM64EC
	static __m256i _Set_avx(const uint64_t _Val) noexcept {
		return _mm256_set1_epi64x(_Val);
	}

	static __m128i _Set_sse(const uint64_t _Val) noexcept {
		return _mm_set1_epi64x(_Val);
	}

	static __m256i _Cmp_avx(const __m256i _Lhs, const __m256i _Rhs) noexcept {
		return _mm256_cmpeq_epi64(_Lhs, _Rhs);
	}

	static __m128i _Cmp_sse(const __m128i _Lhs, const __m128i _Rhs) noexcept {
		return _mm_cmpeq_epi64(_Lhs, _Rhs);
	}
#endif // !_M_ARM64EC
};


enum class _Find_one_predicate { _Equal, _Not_equal };

template <class _Traits, _Find_one_predicate _Pred, class _Ty>
const void* __stdcall __std_find_trivial_impl(const void* _First, const void* _Last, _Ty _Val) noexcept {

	auto _Ptr = static_cast<const _Ty*>(_First);
	if constexpr (_Pred == _Find_one_predicate::_Not_equal) {
		while (_Ptr != _Last && *_Ptr == _Val) {
			++_Ptr;
		}
	}
	else {
		while (_Ptr != _Last && *_Ptr != _Val) {
			++_Ptr;
		}
	}
	return _Ptr;
}

extern "C"
{
	const void* __stdcall __std_find_trivial_8(
		const void* const _First, const void* const _Last, const uint64_t _Val) noexcept {
		return __std_find_trivial_impl<_Find_traits_8, _Find_one_predicate::_Equal>(_First, _Last, _Val);
	}
}


extern "C" void _cdecl __std_exception_copy(const struct __std_exception_data* src,
	struct __std_exception_data* dst)
{

}

extern "C" void _cdecl __std_exception_destroy(struct __std_exception_data* data)
{

}

extern "C" int _CrtDbgReport(int _ReportType, char const* _FileName, int _Linenumber, char const* _ModuleName, char const* _Format, ...)
{
#ifdef _DEBUG
	//TODO
	return 0;
#else
	return 0;
#endif
}