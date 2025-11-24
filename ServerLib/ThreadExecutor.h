#pragma once

namespace jh_utility
{
	using MainFunc = unsigned (WINAPI*)(LPVOID);

	class ThreadExecutor
	{
	public :
		static HANDLE Execute(MainFunc func, LPVOID lparam)
		{
			HANDLE handle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, func, lparam, 0, nullptr));

			if (nullptr == handle)
				jh_utility::CrashDump::Crash();

			return handle;
		}
	};
}

