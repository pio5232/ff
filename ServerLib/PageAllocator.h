#pragma once


namespace jh_memory
{
	class PageAllocator
	{
	public:
		PageAllocator();
		~PageAllocator();

		PageAllocator(const PageAllocator& other) = delete;
		PageAllocator(PageAllocator&& other) = delete;

		PageAllocator& operator=(const PageAllocator& other) = delete;
		PageAllocator& operator=(PageAllocator&& other) = delete;

		void* AllocPage(size_t blockSize);
		void ReleasePage(void* p);
	private:
		SRWLOCK m_lock;

		std::vector<void*> m_pAddressList;

	};
}
