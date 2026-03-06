#include "Core/Core.h"

void MessageCallback(F4SE::MessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case F4SE::MessagingInterface::kGameDataReady:
		Core::GameDataReady();
		break;
	case F4SE::MessagingInterface::kPreLoadGame:
		Core::PreLoadGame();
		break;
	default:
		break;
	}
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);
	F4SE::AllocTrampoline(8 * 8);
	Core::F4SELoaded();

	if (!F4SE::GetMessagingInterface()->RegisterListener(MessageCallback))
	{
		REX::WARN("Cannot register listener!");
		return false;
	}

	return true;
}
