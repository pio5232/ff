#pragma once


namespace jh_memory
{
	template <typename T>
	class ObjectPool
	{
	public:
		/// <summary>
		///  ������ ȣ�� �� �����͸� ��ȯ�Ѵ�
		/// </summary>
		/// <param name="...args"></param>
		/// <returns> ������ ��ȯ </returns>
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
		/// �Ҹ��� ȣ�� �� Ǯ�� ��ȯ�Ѵ�
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

