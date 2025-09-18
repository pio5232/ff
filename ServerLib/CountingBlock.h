#pragma once
#include <Windows.h>

namespace jh_utility
{
	struct CountingBlock
	{
		LONG m_lRefCnt;
	};
}