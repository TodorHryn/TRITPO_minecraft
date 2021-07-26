#pragma once

template<class T>
class PoolAllocator {
	static_assert(sizeof(T) >= sizeof(T*), "Only data types with sizeof(T*) >= sizeof(T) are supported");
	
	public:
		explicit PoolAllocator(T *memory, int size) : m_memory(memory), m_size(size) {
			m_first_free_block = nullptr;

			free(memory, size);
		}

		T* malloc() {
			T* tmp = m_first_free_block;
			T** header = (T**) m_first_free_block;
			m_first_free_block = *header;

			return tmp;
		}

		T* malloc(int size) {
			T** pointer_to_edit = &m_first_free_block;
			T* start_block = m_first_free_block;
			T* end_block = m_first_free_block;
			int consecutive_blocks_found = 1;

			while (consecutive_blocks_found < size && end_block != nullptr) {
				T** end_header = (T**) end_block;
				T* next_block = *end_header;

				if (end_block + 1 == next_block) {
					consecutive_blocks_found++;
					end_block = next_block;
				}
				else {
					pointer_to_edit = (T**) end_block;
					consecutive_blocks_found = 1;
					start_block = next_block;
					end_block = next_block;
				}
			}

			if (consecutive_blocks_found == size) {
				*pointer_to_edit = *((T**) end_block);
				return start_block;
			}
			else
				return nullptr;
		}

		void free(T *ptr) {
			T** header = (T**) ptr;
			*header = m_first_free_block;
			m_first_free_block = ptr;
		}

		void free(T *ptr, int size) {
			for (int i = size - 1; i >= 0; --i)
				free(&ptr[i]);
		}

	private:
		T* m_memory;
		const int m_size;

		T* m_first_free_block;
};
