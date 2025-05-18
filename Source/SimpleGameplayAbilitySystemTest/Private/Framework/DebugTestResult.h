#pragma once

#if defined(_MSC_VER)
	#define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__)
	#define DEBUG_BREAK() __builtin_trap()
#else
	#include <cstdlib>
	#define DEBUG_BREAK() std::abort()
#endif

class FDebugTestResult
{
public:
	FDebugTestResult(bool bInitial = true)
		: bValue(bInitial)
	{
	}

	// Overload the &= operator so that if bValue is true and the new value is false, we break.
	FDebugTestResult& operator&=(bool bOther)
	{
		// Only break on the transition from true to false.
		if (bValue && !bOther)
		{
			DEBUG_BREAK(); // Break immediately when a test fails.
		}
		bValue = bValue && bOther;
		return *this;
	}

	// Allow implicit conversion to bool.
	operator bool() const { return bValue; }

private:
	bool bValue;
};