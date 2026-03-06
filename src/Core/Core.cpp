#include "Core.h"
#include "../Data/Data.h"
#include "../Hooks/Hooks.h"

namespace Core
{
	void F4SELoaded()
	{
		Hooks::InitializeOnLaunch();
	}

	void GameDataReady()
	{
		Data::Settings::Load(false);
		Hooks::Initialize();
	}

	void PreLoadGame()
	{
		Data::Settings::Load(true);
	}
}