#include "Utilities.h"

namespace Utilities
{
	bool InitializeActorInstant(RE::Actor& a_actor, bool a_update3D)
	{
		using func_t = decltype(&InitializeActorInstant);
		static REL::Relocation<func_t> func{ GetFallout4BaseAddress() + 0xD8E180 };
		return func(a_actor, a_update3D);
	}

	RE::TESForm* Utilities::GetFormFromMod(std::string modname, uint32_t formid) {
		if (!modname.length())
			return nullptr;

		return RE::TESDataHandler::GetSingleton()->LookupForm(formid, modname);
	}

	uintptr_t GetFallout4BaseAddress()
	{
		HMODULE hModule = GetModuleHandle("Fallout4.exe");

		if (!hModule) {
			return 0;
		}

		return reinterpret_cast<uintptr_t>(hModule);
	}
}