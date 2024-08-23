#pragma once

#include <cstddef>

class ArenaAllocator {
 private:
  size_t m_size;
  std::byte* m_buffer;
  std::byte* m_offset;

 public:
  ArenaAllocator(size_t bytes) : m_size(bytes) {
    m_buffer = static_cast<std::byte*>(malloc(bytes));
    m_offset = m_buffer;
  }

  template <typename T>
  T* alloc() {
    void* offset = m_offset;
    m_offset += sizeof(T);
    return static_cast<T*>(offset);
  }

  ~ArenaAllocator() { free(m_buffer); }
};
