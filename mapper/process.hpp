#pragma once
#include "stdafx.h"
#include "safe_handle.hpp"
#include "memory_section.hpp"

class process
{
public:
	process(HANDLE handle) : handle(handle) {}
	process(uint32_t id, DWORD desired_access) : handle(safe_handle(OpenProcess(desired_access, false, id))) { }
	process() : handle() { }

	operator bool() const;

#pragma region Statics
	static process current_process();
	static uint32_t from_name(std::string process_name);
#pragma endregion

#pragma region Memory
	MEMORY_BASIC_INFORMATION virtual_query(const uintptr_t address);
	uintptr_t raw_allocate(const SIZE_T virtual_size, const uintptr_t address = 0);
	bool free_memory(const uintptr_t address);
	bool read_raw_memory(void* buffer, const uintptr_t address, const SIZE_T size);
	bool write_raw_memory(void* buffer, const SIZE_T size, const uintptr_t address);

	uintptr_t map(memory_section& section);

	template <class T>
	inline uintptr_t allocate_and_write(T buffer)
	{
		auto buffer_pointer = allocate(buffer);
		write_memory(buffer, buffer_pointer);
		return buffer_pointer;
	}

	template <class T>
	inline uintptr_t allocate(T buffer)
	{
		return raw_allocate(sizeof(T));
	}

	template<class T>
	inline bool read_memory(T* buffer, const uintptr_t address)
	{
		return read_raw_memory(buffer, address, sizeof(T));
	}

	template<class T>
	inline bool write_memory(T buffer, const uintptr_t address)
	{
		return write_raw_memory(reinterpret_cast<unsigned char*>(&buffer), sizeof(T), address);
	}
#pragma endregion

#pragma region Information
	std::unordered_map<std::string, uintptr_t> get_modules();
	std::string get_name();
#pragma endregion

#pragma region Thread
	HANDLE create_thread(const uintptr_t address, const uintptr_t argument = 0);
#pragma endregion

private:
	safe_handle handle;
};



