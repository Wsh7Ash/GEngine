#pragma once

// ================================================================
//  SaveMigration.h
//  Version migration system for save game files.
// ================================================================

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace nlohmann {
    class json;
}

namespace ge {
namespace savegame {

constexpr uint32_t SAVEGAME_CURRENT_VERSION = 1;

class ISaveMigration {
public:
    virtual ~ISaveMigration() = default;
    virtual uint32_t GetFromVersion() const = 0;
    virtual uint32_t GetToVersion() const = 0;
    virtual bool Migrate(nlohmann::json& data) = 0;
    virtual std::string GetDescription() const = 0;
};

using MigrationFactory = std::function<std::unique_ptr<ISaveMigration>()>;

class SaveMigrationRegistry {
public:
    static SaveMigrationRegistry& Get();

    void RegisterMigration(uint32_t fromVersion, uint32_t toVersion, MigrationFactory factory);
    bool HasMigration(uint32_t fromVersion, uint32_t toVersion) const;
    std::vector<uint32_t> GetMigrationPath(uint32_t fromVersion, uint32_t toVersion) const;
    bool Migrate(nlohmann::json& data, uint32_t fromVersion, uint32_t toVersion);
    void RegisterDefaultMigrations();

private:
    SaveMigrationRegistry() = default;
    std::vector<std::pair<std::pair<uint32_t, uint32_t>, MigrationFactory>> migrations_;
};

class SaveMigrationChain {
public:
    SaveMigrationChain() = default;
    void AddMigration(std::unique_ptr<ISaveMigration> migration);
    bool Execute(nlohmann::json& data, uint32_t fromVersion, uint32_t toVersion);
    std::vector<std::string> GetMigrationSteps() const;

private:
    std::vector<std::unique_ptr<ISaveMigration>> migrations_;
    std::vector<std::string> stepDescriptions_;
};

class SaveMigrationManager {
public:
    SaveMigrationManager();
    bool MigrateSave(nlohmann::json& data, uint32_t currentVersion, uint32_t targetVersion);
    bool NeedsMigration(uint32_t currentVersion) const {
        return currentVersion < SAVEGAME_CURRENT_VERSION;
    }
    uint32_t GetCurrentVersion() const { return SAVEGAME_CURRENT_VERSION; }
    std::vector<uint32_t> GetAvailableVersions() const;

private:
    std::unique_ptr<SaveMigrationChain> BuildMigrationChain(uint32_t from, uint32_t to);
};

} // namespace savegame
} // namespace ge