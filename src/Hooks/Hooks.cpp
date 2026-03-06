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
			RE::BGSAnimationSystemUtils::InitializeActorInstant(*Player, false);
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

	RE::BSEventNotifyControl MenuWatcher::ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
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
		uintptr_t FunctionA = REL::Relocation<uintptr_t>{ Utilities::GetFallout4BaseAddress() + 0x4FCDAA }.address();
		uintptr_t FunctionB = REL::Relocation<uintptr_t>{ Utilities::GetFallout4BaseAddress() + 0x13864FC }.address();

		F4SE::Trampoline& trampoline = F4SE::GetTrampoline();
		SetupSpecialIdleOriginal = trampoline.write_call<5>(FunctionA, &hook_func);
		trampoline.write_call<5>(FunctionB, &hook_func);
	}
}
