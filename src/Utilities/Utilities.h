#pragma once

namespace Utilities
{
	RE::TESForm* GetFormFromMod(std::string modname, uint32_t formid);
	uintptr_t GetFallout4BaseAddress();

	template <class FunctionPointer>
	FunctionPointer SafeWrite64Function(uintptr_t addr, FunctionPointer data)
	{
		DWORD oldProtect;
		void* _d[2];
		memcpy(_d, &data, sizeof(data));
		size_t len = sizeof(_d[0]);

		VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
		FunctionPointer olddata;
		memset(&olddata, 0, sizeof(FunctionPointer));
		memcpy(&olddata, (void*)addr, len);
		memcpy((void*)addr, &_d[0], len);
		VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
		return olddata;
	}
}