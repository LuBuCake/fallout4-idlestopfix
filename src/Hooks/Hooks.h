#pragma once

namespace Hooks
{
	bool hook_func(RE::AIProcess* a_ai, RE::Actor& a_actor, RE::DEFAULT_OBJECT a_obj, RE::TESIdleForm* a_idle, bool a_bool, RE::TESObjectREFR* a_target);

	class Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent
	{
	public:
		typedef RE::BSEventNotifyControl(Hook_PlayerCharacter_BSTEventSink_BSAnimationGraphEvent::* OriginalProcessEventFunction)(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);
		RE::BSEventNotifyControl ProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);
		void Sink();

	protected:
		static std::unordered_map<uintptr_t, OriginalProcessEventFunction> FunctionMap;
	};

	class MenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		void Initialize();

	public:
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;
	};

	void Initialize();
	void InitializeOnLaunch();
}