#pragma once


namespace jh_memory
{
	template <typename T>
	class ObjectPool
	{
	public:

		//  생성자 호출 후 포인터를 반환
		template <typename... Args>
		static T* Alloc(Args&&... args)
		{
			T* obj = static_cast<T*>(g_memSystem->Alloc(sizeof(T)));

			new (obj) T(std::forward<Args>(args)...);

			return obj;
		}

		// 소멸자 호출 후 풀에 반환
		static void Free(T* obj)
		{
			if (nullptr == obj)
				return;

			obj->~T();

			g_memSystem->Free(obj);
		}

	};

	template<typename T, typename... Args>
	std::shared_ptr<T> MakeShared(Args&&... args)
	{
		// 포인터와 삭제자를 등록 -> 생성
		std::shared_ptr<T> ret = std::shared_ptr<T>(ObjectPool<T>::Alloc(std::forward<Args>(args)...), ObjectPool<T>::Free);
		return ret;
	}
}

