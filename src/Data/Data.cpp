#include "Data.h"
#include "..\Utilities\Utilities.h"

namespace Data
{
	/*
		TEMPLATE:
			PLUGIN,IDLE_FORMID

	*/

	Manager* Manager::Instance = nullptr;

	void Settings::Load(bool clearCache)
	{
        if (clearCache) {
            Manager::GetSingleton()->ResetMap();
        }

		std::string configsPath = "Data\\F4SE\\Plugins\\FO4IdleStopFix";
		std::filesystem::directory_entry configEntry{ configsPath };

		if (std::filesystem::exists(configsPath)) {
			for (auto& it : std::filesystem::directory_iterator(configEntry)) {
				if (it.path().extension().compare(".txt") == 0) {
					ParseConfigurations(it.path());
				}
			}
		}
	}

	void Settings::ParseConfigurations(const std::filesystem::path filePath)
	{
        std::ifstream file{ filePath };

        if (!file.is_open()) {
            return;
        }

        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }

            size_t commaPos = line.find(',');
            if (commaPos == std::string::npos) {
                continue;
            }

            std::string fileName = line.substr(0, commaPos);
            std::string numberStr = line.substr(commaPos + 1);
            int32_t number;

            try {
                number = std::stoi(numberStr, nullptr, 16);
            }
            catch (const std::invalid_argument& e) {
                logger::error("Error while parsing IdleFormID, error: {}", e.what());
                continue;
            }
            catch (const std::out_of_range& e) {
                logger::error("Error while parsing IdleFormID, error: {}", e.what());
                continue;
            }

            RE::TESIdleForm* idle = (RE::TESIdleForm*)Utilities::GetFormFromMod(fileName, number);
            if (!idle)
                continue;

            Manager::GetSingleton()->AddConfig(idle, fileName, number);
        }

        file.close();
	}

	void Manager::AddConfig(RE::TESIdleForm* idleForm, std::string pluginFile, int32_t idleFormID)
	{
		auto find = ConfigMap.find(idleForm);

        if (find == ConfigMap.end()) {
            ConfigMap.insert(std::pair<RE::TESIdleForm*, std::vector<IdleData>>(idleForm, std::vector<IdleData>()));
        }		

        ConfigMap[idleForm].push_back(IdleData(pluginFile, idleFormID));

        logger::warn("Added config: pluginFile->[{}]; idleFormID->[{}]", pluginFile, idleFormID);
	}
}