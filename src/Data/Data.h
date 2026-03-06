#pragma once

#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

namespace Data
{
	class Settings
	{
	public:
		static void Load(bool clearCache);

	private:
		static void ParseConfigurations(const std::filesystem::path path);
	};

	struct IdleData
	{
		std::string PluginFile;
		int32_t IdleFormID;

		IdleData(std::string pluginFile, int32_t idleFormID) {
			this->PluginFile = pluginFile;
			this->IdleFormID = idleFormID;
		}
	};

	class Manager
	{
	protected:
		static Manager* Instance;

	public:
		std::unordered_map<RE::TESIdleForm*, std::vector<IdleData>> ConfigMap;

		Manager() = default;
		Manager(Manager&) = delete;

		void operator=(const Manager&) = delete;

		static Manager* GetSingleton()
		{
			if (!Instance)
				Instance = new Manager();

			return Instance;
		}

		inline void ResetMap() {
			ConfigMap.clear();
		}

		void AddConfig(RE::TESIdleForm* idleForm, std::string pluginFile, int32_t idleFormID);
	};
}