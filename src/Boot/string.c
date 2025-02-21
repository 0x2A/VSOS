
#include <stdint.h>

int memcmp(void const* _Buf1, void const* _Buf2, size_t _Size)
{
	uintptr_t* ptr1_64 = (uintptr_t*)_Buf1;
	uintptr_t* ptr2_64 = (uintptr_t*)_Buf2;

	size_t i = 0;
	while (i < _Size / sizeof(uintptr_t))
	{
		if (*ptr1_64 < *ptr2_64)
			return -1;
		else if (*ptr1_64 > *ptr2_64)
			return 1;

		i++;
		ptr1_64++;
		ptr2_64++;
	}

	uint8_t* ptr1_8 = (uint8_t*)ptr1_64;
	uint8_t* ptr2_8 = (uint8_t*)ptr2_64;

	i = 0;
	while (i < _Size % sizeof(uintptr_t))
	{
		if (*ptr1_8 < *ptr2_8)
			return -1;
		else if (*ptr1_8 > *ptr2_8)
			return 1;

		i++;
		ptr1_8++;
		ptr2_8++;
	}

	return 0;
}

int strcmp(const char* str1, const char* str2)
{
	while ((*str1) && (*str1 == *str2))
	{
		str1++;
		str2++;
	}
	return *(unsigned char*)str1 - *(unsigned char*)str2;
}

size_t strlen(const char* str)
{
	size_t length = 0;
	while (*str != '\0')
	{
		length++;
		str++;
	}

	return length;
}

char* strncpy(char* _Destination, char const* _Source, size_t _Count)
{
	memcpy(_Destination, _Source, _Count * sizeof(char));
	return _Destination;
}

char* strcpy(char* _Destination, char const* _Source)
{
	return strncpy(_Destination, _Source, strlen(_Source));
}