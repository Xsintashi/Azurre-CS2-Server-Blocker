#pragma once
#if !defined(RAVEN)
#define WIN32_LEAN_AND_MEAN
#undef min
#undef max
#include <Windows.h>
#include <algorithm>
#include <TlHelp32.h>
#include "Config.h"
#include <cstdint>
#include <string_view>
class Memory {
public:
	Memory() = default;
	Memory(const wchar_t* Target) {
		HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (snapshotHandle == INVALID_HANDLE_VALUE)
			return;
		
		PROCESSENTRY32W processEntry = { };
		processEntry.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(snapshotHandle, &processEntry)) {
			do {
				if (_wcsicmp(processEntry.szExeFile, Target) == 0) {
					CloseHandle(snapshotHandle);
					process = processEntry.th32ProcessID;
					handle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, process);
					return;
				}
			} while (Process32NextW(snapshotHandle, &processEntry));
		}
		CloseHandle(snapshotHandle);
		return;
	}

	template <typename type>
	type Read(uintptr_t pointerStatic) {
		type value = { };
		ReadProcessMemory(handle, (LPVOID)pointerStatic, &value, sizeof(type), NULL);
		return value;
	}

	uintptr_t GetModuleBaseAddress(const wchar_t* moduleName) {

		HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process);

		if (snapshotHandle == INVALID_HANDLE_VALUE)
			return NULL;

		MODULEENTRY32W moduleEntry = { };
		moduleEntry.dwSize = sizeof(MODULEENTRY32W);

		if (Module32FirstW(snapshotHandle, &moduleEntry)) {
			do {
				if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
					CloseHandle(snapshotHandle);
					return reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
				}

			} while (Module32NextW(snapshotHandle, &moduleEntry));
		}

		CloseHandle(snapshotHandle);
		return NULL;

	}

	const DWORD GetModuleSize(const wchar_t* moduleName) {
		::MODULEENTRY32 entry = { };
		entry.dwSize = sizeof(::MODULEENTRY32);

		const auto snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process);

		while (::Module32Next(snapShot, &entry))
			if (_wcsicmp(moduleName, (const wchar_t*)entry.szModule) == 0)
				return entry.modBaseSize;

		if (snapShot)
			::CloseHandle(snapShot);

		return 0;
	}

	LPVOID allocateMemoryNear(LPVOID baseAddress, size_t size) {
		uintptr_t minAddress = (uintptr_t)baseAddress - INT_MAX + 0x10000000; // 0x10000000 safezone
		uintptr_t maxAddress = (uintptr_t)baseAddress + INT_MAX - 0x10000000; // 0x10000000 safezone

		minAddress = std::max(minAddress, (uintptr_t)0x00010000);		// Avoid lower addresses
		maxAddress = std::min(maxAddress, (uintptr_t)0x7FFEFFFFF000);	// Stay within valid range

		constexpr ptrdiff_t step = 0x10000;		// Step size, aligned to 64KB for efficiency
		uintptr_t currentAddress = minAddress;

		LPVOID allocatedMemory = nullptr;

		while (!allocatedMemory && currentAddress < maxAddress) {
			allocatedMemory = VirtualAllocEx(
				handle,
				(LPVOID)currentAddress,
				size,
				MEM_COMMIT | MEM_RESERVE,
				PAGE_EXECUTE_READWRITE
			);

			if (allocatedMemory)
				return allocatedMemory;
			currentAddress += step;
		}
		return 0x0;
	}

	template <typename type>
	bool Write(uintptr_t pointerStatic, type value) {

		if (cfg->restrictions > RESTRICTION::DISABLE_RISK_FEATURES)
			return false;

		return WriteProcessMemory(handle, (LPVOID)pointerStatic, &value, sizeof(type), NULL);
	}

	bool WriteString(uintptr_t pointerStatic, const std::string& value) {

		if (cfg->restrictions > RESTRICTION::DISABLE_RISK_FEATURES)
			return false;

		return WriteProcessMemory(handle, (LPVOID)pointerStatic, value.c_str(), value.size() + 1, NULL);
	}

	bool writeBytes(uintptr_t pointerStatic, const unsigned char* arrayPointer, size_t size) {

		if (cfg->restrictions > RESTRICTION::DISABLE_RISK_FEATURES)
			return false;

		return WriteProcessMemory(handle, (LPVOID)pointerStatic, arrayPointer, size, NULL);
	}

	DWORD process;
	HANDLE handle;
};
#endif