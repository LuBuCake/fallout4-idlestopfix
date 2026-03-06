#include "Utilities.h"

namespace Utilities
{
	RE::TESForm* Utilities::GetFormFromMod(std::string modname, uint32_t formid) {
		if (!modname.length())
			return nullptr;

		return RE::TESDataHandler::GetSingleton()->LookupForm(formid, modname);
	}

	uintptr_t GetFallout4BaseAddress()
	{
		HMODULE hModule = GetModuleHandle(L"Fallout4.exe");

		if (!hModule) {
			return 0;
		}

		return reinterpret_cast<uintptr_t>(hModule);
	}
}