#include "stdafx.h"
#include "ntdll.hpp"

process::operator bool()
{
	return static_cast<bool>(this->handle);
}

process process::current_process()
{
	return process(reinterpret_cast<HANDLE>(-1));
}

uint32_t process::from_name(const std::string& process_name)
{
	DWORD process_list[516], bytes_needed;
	if (EnumProcesses(process_list, sizeof(process_list), &bytes_needed))
	{
		for (size_t index = 0; index < bytes_needed / sizeof(uint32_t); index++)
		{
			auto proc = process(process_list[index], PROCESS_ALL_ACCESS);

			if (!proc)
				continue;

			if (process_name == proc.get_name())
				return process_list[index];
		}
	}

	return 0;
}

MEMORY_BASIC_INFORMATION process::virtual_query(const uintptr_t address)
{
	MEMORY_BASIC_INFORMATION mbi;

	VirtualQueryEx(this->handle.get_handle(), reinterpret_cast<LPCVOID>(address), &mbi, sizeof(MEMORY_BASIC_INFORMATION));

	return mbi;
}

uintptr_t process::raw_allocate(const SIZE_T virtual_size, const uintptr_t address)
{
	return reinterpret_cast<uintptr_t>(
		VirtualAllocEx(this->handle.get_handle(), reinterpret_cast<LPVOID>(address), virtual_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE)
	);
}

bool process::free_memory(const uintptr_t address)
{
	auto query = virtual_query(address);
	return VirtualFreeEx(this->handle.get_handle(), reinterpret_cast<LPVOID>(address), query.RegionSize, MEM_RELEASE);
}


bool process::read_raw_memory(void* buffer, const uintptr_t address, const SIZE_T size)
{
	return ReadProcessMemory(this->handle.get_handle(), reinterpret_cast<LPCVOID>(address), buffer, size, nullptr);
}

bool process::write_raw_memory(void* buffer, const SIZE_T size, const uintptr_t address)
{
	return WriteProcessMemory(this->handle.get_handle(), reinterpret_cast<LPVOID>(address), buffer, size, nullptr);
}

uintptr_t process::map(memory_section& section)
{
	void* base_address = nullptr;
	SIZE_T view_size = section.size;
	auto result = ntdll::NtMapViewOfSection(section.handle.get_handle(), this->handle.get_handle(), &base_address, NULL, NULL, NULL, &view_size, 2, 0, section.protection);
	
	if (!NT_SUCCESS(result))
	{
		logger::log_error("NtMapViewOfSection failed");
		logger::log_formatted("Error code", result, true);
	}

	return reinterpret_cast<uintptr_t>(base_address);
}

std::unordered_map<std::string, uintptr_t> process::get_modules()
{
	auto result = std::unordered_map<std::string, uintptr_t>();

	HMODULE module_handles[1024];
	DWORD size_needed;


	if (EnumProcessModules(this->handle.get_handle(), module_handles, sizeof(module_handles), &size_needed))
	{
		for (auto i = 0; i < size_needed / sizeof(HMODULE); i++)
		{
			CHAR szModName[MAX_PATH];
			GetModuleBaseNameA(this->handle.get_handle(), module_handles[i], szModName, MAX_PATH);

			std::string new_name = szModName;
			std::transform(new_name.begin(), new_name.end(), new_name.begin(), ::tolower);

			result[new_name] = reinterpret_cast<uintptr_t>(module_handles[i]);
		}
	}



	return result;
}

std::string process::get_name()
{
	char buffer[MAX_PATH];
	GetModuleBaseNameA(handle.get_handle(), nullptr, buffer, MAX_PATH);

	return std::string(buffer);
}

HANDLE process::create_thread(const uintptr_t address, const uintptr_t argument)
{
	return CreateRemoteThread(this->handle.get_handle(), nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(address), reinterpret_cast<LPVOID>(argument), 0, nullptr);
}
