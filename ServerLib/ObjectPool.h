#pragma once


namespace jh_memory
{
	template <typename T>
	class ObjectPool
	{
	public:
		/// <summary>
		///  생성자 호출 후 포인터를 반환한다
		/// </summary>
		/// <param name="...args"></param>
		/// <returns> 포인터 반환 </returns>
		template <typename... Args>
		static T* Alloc(Args&&... args)
		{
			T* obj= static_cast<T*>(g_memAllocator->Alloc(sizeof(T)));

			if (nullptr == obj)
				return nullptr;

			new (obj) T(std::forward<Args>(args)...);

			return obj;
		}

		/// <summary>
		/// 소멸자 호출 후 풀에 반환한다
		/// </summary>
		/// <param name="obj"></param>
		static void Free(T* obj)
		{
			if (nullptr == obj)
				return;

			obj->~T();

			g_memAllocator->Free(obj);
		}
	};
}

