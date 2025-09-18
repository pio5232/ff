#include "LibraryPch.h"
#include "SerializationBuffer.h"
#include "Memory.h"

using namespace jh_utility;

//PacketPtr MakeSharedBuffer(jh_memory::MemoryAllocator* allocator, size_t bufferSize)
//{
//    if (allocator == nullptr)
//        return nullptr;
//
//    // nullptr이 나오는 경우가 없다. malloc에 실패하면 crash
//    void* tmp = allocator->Alloc(sizeof(SerializationBuffer));
//
//    SerializationBuffer* sb = new (tmp) SerializationBuffer(allocator, bufferSize);
//
//    auto deleter = [allocator](SerializationBuffer* _sb)
//        {
//            _sb->~SerializationBuffer();
//
//            allocator->Free(_sb);
//        };
//
//    // raw 포인터와 삭제자를 등록 -> 생성
//    return std::shared_ptr<SerializationBuffer>(sb, deleter);
//}
PacketPtr MakeSharedBuffer(jh_memory::MemoryAllocator* allocator, size_t bufferSize)
{
    return MakeShared<jh_utility::SerializationBuffer>(allocator, allocator, bufferSize);
}