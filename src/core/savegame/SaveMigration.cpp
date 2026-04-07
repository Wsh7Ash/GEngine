#include "../savegame/SaveMigration.h"
#include <algorithm>
#include <vector>
#include <string>
#include <utility>
#include <functional>
#include <memory>

namespace nlohmann {
    class json;
}

namespace ge {
namespace savegame {

SaveMigrationRegistry& SaveMigrationRegistry::Get() {
    static SaveMigrationRegistry instance;
    return instance;
}

void SaveMigrationRegistry::RegisterMigration(uint32_t fromVersion, uint32_t toVersion, MigrationFactory factory) {
    migrations_.push_back({{fromVersion, toVersion}, std::move(factory)});
}

bool SaveMigrationRegistry::HasMigration(uint32_t fromVersion, uint32_t toVersion) const {
    for (const auto& [versions, _] : migrations_) {
        if (versions.first == fromVersion && versions.second == toVersion) {
            return true;
        }
    }
    return false;
}

std::vector<uint32_t> SaveMigrationRegistry::GetMigrationPath(uint32_t fromVersion, uint32_t toVersion) const {
    std::vector<uint32_t> path;
    uint32_t current = fromVersion;
    
    while (current < toVersion) {
        path.push_back(current);
        bool found = false;
        for (const auto& [versions, _] : migrations_) {
            if (versions.first == current) {
                current = versions.second;
                found = true;
                break;
            }
        }
        if (!found) break;
    }
    
    if (current == toVersion) {
        path.push_back(toVersion);
    }
    
    return path;
}

bool SaveMigrationRegistry::Migrate(nlohmann::json& data, uint32_t fromVersion, uint32_t toVersion) {
    auto path = GetMigrationPath(fromVersion, toVersion);
    if (path.empty() || path.back() != toVersion) {
        return false;
    }

    for (size_t i = 0; i < path.size() - 1; ++i) {
        uint32_t src = path[i];
        uint32_t dst = path[i + 1];
        
        for (const auto& [versions, factory] : migrations_) {
            if (versions.first == src && versions.second == dst) {
                auto migration = factory();
                if (!migration->Migrate(data)) {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

void SaveMigrationRegistry::RegisterDefaultMigrations() {
}

void SaveMigrationChain::AddMigration(std::unique_ptr<ISaveMigration> migration) {
    stepDescriptions_.push_back(migration->GetDescription());
    migrations_.push_back(std::move(migration));
}

bool SaveMigrationChain::Execute(nlohmann::json& data, uint32_t fromVersion, uint32_t toVersion) {
    uint32_t currentVersion = fromVersion;
    
    for (auto& migration : migrations_) {
        if (migration->GetFromVersion() != currentVersion) {
            return false;
        }
        if (!migration->Migrate(data)) {
            return false;
        }
        currentVersion = migration->GetToVersion();
    }
    
    return currentVersion == toVersion;
}

std::vector<std::string> SaveMigrationChain::GetMigrationSteps() const {
    return stepDescriptions_;
}

SaveMigrationManager::SaveMigrationManager() {
    SaveMigrationRegistry::Get().RegisterDefaultMigrations();
}

bool SaveMigrationManager::MigrateSave(nlohmann::json& data, uint32_t currentVersion, uint32_t targetVersion) {
    return SaveMigrationRegistry::Get().Migrate(data, currentVersion, targetVersion);
}

std::vector<uint32_t> SaveMigrationManager::GetAvailableVersions() const {
    std::vector<uint32_t> versions;
    for (uint32_t v = 1; v <= SAVEGAME_CURRENT_VERSION; ++v) {
        versions.push_back(v);
    }
    return versions;
}

std::unique_ptr<SaveMigrationChain> SaveMigrationManager::BuildMigrationChain(uint32_t from, uint32_t to) {
    auto chain = std::make_unique<SaveMigrationChain>();
    auto path = SaveMigrationRegistry::Get().GetMigrationPath(from, to);
    
    for (size_t i = 0; i < path.size() - 1; ++i) {
        uint32_t src = path[i];
        uint32_t dst = path[i + 1];
        
        for (const auto& [versions, factory] : SaveMigrationRegistry::Get().migrations_) {
            if (versions.first == src && versions.second == dst) {
                chain->AddMigration(factory());
                break;
            }
        }
    }
    
    return chain;
}

} // namespace savegame
} // namespace ge