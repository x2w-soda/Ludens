#pragma once

#include <memory>

namespace LD {
	
	///
	/// HEAP MEMORY
	///
	
	void* MemoryAlloc(size_t size);
	void* MemoryRealloc(void* mem, size_t size);
	void MemoryFree(void* mem);

	struct MemoryPlacementData
	{
		size_t Count;
	};

	template <typename T>
	T* MemoryPlacementAlloc(size_t count)
	{
		MemoryPlacementData* header = (MemoryPlacementData*)MemoryAlloc(sizeof(MemoryPlacementData) + sizeof(T) * count);
		header->Count = count;
		T* mem = (T*)(header + 1);

		for (size_t i = 0; i < count; i++)
			new (mem + i) T{};
		return mem;
	}
	
	template <typename T>
	void MemoryPlacementFree(T* mem)
	{
		MemoryPlacementData* header = ((MemoryPlacementData*)mem) - 1;

		for (size_t i = 0; i < header->Count; i++)
			(mem + i)->~T();

		MemoryFree((void*)header);
	}

	template <typename T>
	T* MemoryPlacementRealloc(T* oldMem, size_t count)
	{
		if (oldMem == nullptr)
			return MemoryPlacementAlloc<T>(count);

		MemoryPlacementData* oldHeader = ((MemoryPlacementData*)oldMem) - 1;
		size_t oldCount = oldHeader->Count;
		size_t minCount = oldCount < count ? oldCount : count;

		T* newMem = MemoryPlacementAlloc<T>(count);

		for (size_t i = 0; i < minCount; i++)
			newMem[i] = oldMem[i];

		MemoryPlacementFree<T>(oldMem);

		return newMem;
	}

	///
	/// SMART POINTERS
	///

	template<typename T>
	using Own = std::unique_ptr<T>;

	template<typename T, typename... TArgs>
	Own<T> MakeOwn(TArgs&&... args)
	{
		return std::make_unique<T>(std::forward<TArgs>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T, typename... TArgs>
	Ref<T> MakeRef(TArgs&&... args)
	{
		return std::make_shared<T>(std::forward<TArgs>(args)...);
	}

} // namespace LD
