#include "Hooks.h"
#include "../Data/Data.h"
#include "../Utilities/Utilities.h"

namespace Hooks
{
	RE::PlayerCharacter* Player;
	MenuWatcher* MenuWatcherInstance;

	static uintptr_t SetupSpecialIdleOriginal;
	static bool ShouldBlockIdleStop = false;

	bool hook_func(RE::AIProcess* a_ai, RE::Actor& a_actor, RE::DEFAULT_OBJECT a_obj, RE::TESIdleForm* a_idle, bool a_bool, RE::TESObjectREFR* a_target)
	{
		auto manager = Data::Manager::GetSingleton();
		auto find = manager->ConfigMap.find(a_idle);

		if (find != manager->ConfigMap.end() && a_idle) {
			ShouldBlockIdleStop = true;
		}

		typedef bool (*FnSetupSpecialIdle)(RE::AIProcess*, RE::Actor&, RE::DEFAULT_OBJECT, RE::TESIdleForm*, bool, RE::TESObjectREFR*);
		FnSetupSpecialIdle fn = (FnSetupSpecialIdle)SetupSpecialIdleOriginal;
		return fn ? (*fn)(a_ai, a_actor, a_obj, a_idle, a_bool, a_target) : false;
	}

	std::unordered_map<uintptr_t, Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent::OriginalProcessEventFunction> Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent::FunctionMap;

	RE::BSEventNotifyControl Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent::ProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source)
	{
		if (a_event.tag == "IdleStop" && ShouldBlockIdleStop) {
			Player->UpdateAnimation(1000.0f);
			ShouldBlockIdleStop = false;
		}

		OriginalProcessEventFunction _function = FunctionMap.at(*(uintptr_t*)this);
		return _function ? (this->*_function)(a_event, a_source) : RE::BSEventNotifyControl::kContinue;
	}

	void Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent::Sink()
	{
		uintptr_t vtable = *(uintptr_t*)this;
		auto it = FunctionMap.find(vtable);

		if (it == FunctionMap.end()) {
			OriginalProcessEventFunction _function = Utilities::SafeWrite64Function(vtable + 0x8, &Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent::ProcessEvent);
			FunctionMap.insert(std::pair<uintptr_t, OriginalProcessEventFunction>(vtable, _function));
		}
	}

	RE::BSEventNotifyControl MenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source)
	{
		if ((a_event.menuName == "LoadingMenu" || a_event.menuName == "PipboyMenu") && a_event.opening) {
			ShouldBlockIdleStop = false;
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	void MenuWatcher::Initialize()
	{
		RE::UI::GetSingleton()->GetEventSource<RE::MenuOpenCloseEvent>()->RegisterSink(this);
	}

	void Initialize()
	{
		Player = RE::PlayerCharacter::GetSingleton();

		MenuWatcherInstance = new MenuWatcher();
		MenuWatcherInstance->Initialize();
		
		((Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent*)((uint64_t)Player + 0x38))->Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent::Sink();
	}

	void InitializeOnLaunch()
	{
		// Both sites we redirect are calls to
		// AIProcess::SetupSpecialIdle(actor, kActionIdle, idle, true, target),
		// so each pattern covers the argument setup that immediately precedes the
		// call and ends on its E8 opcode. Anchoring to that shape instead of to a
		// raw offset keeps the plugin working across executable revisions.
		//
		// Site A:                              Site B:
		//   mov  r8d, 35h                        mov  byte ptr [rsp+20h], 1
		//   mov  byte ptr [rsp+20h], 1           mov  rcx, rax
		//   call AIProcess::SetupSpecialIdle     mov  r8d, 35h
		//                                        call AIProcess::SetupSpecialIdle
		//
		// 35h is DEFAULT_OBJECT::kActionIdle and [rsp+20h] is the fifth argument,
		// a_testConditions, both of which are part of the call's meaning rather
		// than of how the compiler happened to lay the function out that day.

		const auto code = Utilities::GetGameCodeRange();

		if (code.Empty()) {
			REX::ERROR("Could not locate the .text section of Fallout4.exe, no hooks were installed.");
			return;
		}

		const uintptr_t callSites[]{
			Utilities::FindTrailingCall<"41 B8 35 00 00 00 C6 44 24 20 01 E8">(code),
			Utilities::FindTrailingCall<"C6 44 24 20 01 ?? ?? ?? 41 B8 35 00 00 00 E8">(code),
		};

		const REL::Relocation<uintptr_t> setupSpecialIdle{ RE::ID::AIProcess::SetupSpecialIdle };

		REL::Trampoline& trampoline = REL::GetTrampoline();

		for (const uintptr_t site : callSites) {
			if (!site) {
				continue;  // FindTrailingCall already logged what went wrong
			}

			// A matching byte string is not proof on its own, so confirm the call
			// really does land on SetupSpecialIdle before rewriting it.
			const uintptr_t target = REL::ASM::CALL5::TARGET(site);

			if (target != setupSpecialIdle.address()) {
				REX::ERROR(
					"Call at Fallout4.exe+{:X} targets Fallout4.exe+{:X}, expected AIProcess::SetupSpecialIdle at Fallout4.exe+{:X}, skipping.",
					code.OffsetOf(site),
					code.OffsetOf(target),
					code.OffsetOf(setupSpecialIdle.address()));
				continue;
			}

			SetupSpecialIdleOriginal = trampoline.write_call<5>(site, &hook_func);
			REX::INFO("Hooked AIProcess::SetupSpecialIdle call at Fallout4.exe+{:X}.", code.OffsetOf(site));
		}

		if (!SetupSpecialIdleOriginal) {
			REX::ERROR("No SetupSpecialIdle call site could be hooked, this game version is not supported yet.");
		}
	}
}
