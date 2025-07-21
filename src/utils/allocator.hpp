#pragma once

using namespace std;

class Allocator {
    public:
        inline Allocator(size_t bytes): _size(bytes) {
            _buffer = static_cast<char*>(malloc(_size));
            _offset = _buffer;
        }

        template<typename T>
        inline T* allocate() {
            void* offset = _offset;
            _offset += sizeof(T);
            return new (offset) T();
        }

        inline Allocator(const Allocator&) = delete;
        inline Allocator operator=(const Allocator&) = delete;
        inline ~Allocator() { free(_buffer); }

    private:
        size_t _size;
        char* _buffer;
        char* _offset;
};