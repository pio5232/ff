#include "LibraryPch.h"
#include "SerializationBuffer.h"
#include "Memory.h"

using namespace jh_utility;

//PacketPtr MakeSharedBuffer(jh_memory::MemoryAllocator* allocator, size_t bufferSize)
//{
//    if (allocator == nullptr)
//        return nullptr;
//
//    // nullptr�� ������ ��찡 ����. malloc�� �����ϸ� crash
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
//    // raw �����Ϳ� �����ڸ� ��� -> ����
//    return std::shared_ptr<SerializationBuffer>(sb, deleter);
//}
PacketPtr MakeSharedBuffer(jh_memory::MemoryAllocator* allocator, size_t bufferSize)
{
    return MakeShared<jh_utility::SerializationBuffer>(allocator, allocator, bufferSize);
}