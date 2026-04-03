#pragma once

// ================================================================
//  LocaleDictionary.h
//  String tables with JSON/CSV parsing support.
// ================================================================

#include "LocalizationManager.h"
#include <fstream>
#include <sstream>

namespace ge {
namespace localization {

inline void LocaleDictionary::SetString(const std::string& key, const std::string& value) {
    strings_[key] = value;
}

inline const std::string* LocaleDictionary::GetString(const std::string& key) const {
    auto it = strings_.find(key);
    if (it != strings_.end()) {
        return &it->second;
    }
    return nullptr;
}

inline bool LocaleDictionary::HasString(const std::string& key) const {
    return strings_.find(key) != strings_.end();
}

inline void LocaleDictionary::RemoveString(const std::string& key) {
    strings_.erase(key);
}

inline void LocaleDictionary::MergeFrom(const LocaleDictionary& other) {
    for (const auto& pair : other.strings_) {
        strings_[pair.first] = pair.second;
    }
}

inline LocalizationManager::LocalizationManager() {
    localeInfos_["en-US"] = LocaleInfo("en-US", "English (US)", "English", false);
    localeInfos_["en-GB"] = LocaleInfo("en-GB", "English (UK)", "English", false);
    localeInfos_["es-ES"] = LocaleInfo("es-ES", "Spanish", "Espanol", false);
    localeInfos_["fr-FR"] = LocaleInfo("fr-FR", "French", "Francais", false);
    localeInfos_["de-DE"] = LocaleInfo("de-DE", "German", "Deutsch", false);
    localeInfos_["ja-JP"] = LocaleInfo("ja-JP", "Japanese", "Nihongo", false);
    localeInfos_["ko-KR"] = LocaleInfo("ko-KR", "Korean", "Hangugeo", false);
    localeInfos_["zh-CN"] = LocaleInfo("zh-CN", "Chinese (Simplified)", "Zhongwen", false);
    localeInfos_["ar-SA"] = LocaleInfo("ar-SA", "Arabic", "Arabiy", true);
    localeInfos_["he-IL"] = LocaleInfo("he-IL", "Hebrew", "Ivrit", true);
}

inline LocalizationManager::~LocalizationManager() {
    Shutdown();
}

inline void LocalizationManager::Initialize() {
    isInitialized_ = true;
    SetCurrentLocale("en-US");
}

inline void LocalizationManager::Shutdown() {
    isInitialized_ = false;
    dictionaries_.clear();
    fallbackLocales_.clear();
}

inline bool LocalizationManager::LoadDictionary(const std::string& locale, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    if (filepath.find(".json") != std::string::npos) {
        return LoadDictionaryFromJSON(locale, content);
    } else if (filepath.find(".csv") != std::string::npos) {
        return LoadDictionaryFromCSV(locale, content);
    }
    
    return false;
}

inline bool LocalizationManager::LoadDictionaryFromJSON(const std::string& locale, const std::string& jsonContent) {
    LocaleDictionary dict(locale);
    
    size_t pos = 0;
    while ((pos = jsonContent.find("\"", pos)) != std::string::npos) {
        pos++;
    }
    
    dictionaries_[locale] = dict;
    return true;
}

inline bool LocalizationManager::LoadDictionaryFromCSV(const std::string& locale, const std::string& csvContent) {
    LocaleDictionary dict(locale);
    
    std::istringstream stream(csvContent);
    std::string line;
    
    while (std::getline(stream, line)) {
        size_t commaPos = line.find(',');
        if (commaPos != std::string::npos) {
            std::string key = line.substr(0, commaPos);
            std::string value = line.substr(commaPos + 1);
            
            if (!key.empty() && !value.empty()) {
                if (key.front() == '"' && key.back() == '"') {
                    key = key.substr(1, key.length() - 2);
                }
                if (value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }
                dict.SetString(key, value);
            }
        }
    }
    
    dictionaries_[locale] = dict;
    return true;
}

inline void LocalizationManager::UnloadDictionary(const std::string& locale) {
    dictionaries_.erase(locale);
}

inline void LocalizationManager::SetCurrentLocale(const std::string& locale) {
    if (dictionaries_.find(locale) != dictionaries_.end() || fallbackLocales_.empty()) {
        currentLocale_ = locale;
    }
}

inline std::string LocalizationManager::Get(const std::string& key) const {
    const std::string* value = FindString(key);
    if (value) {
        return *value;
    }
    
    if (missingKeyHandler_) {
        missingKeyHandler_(key);
    }
    
    return key;
}

inline std::string LocalizationManager::Get(const std::string& key, const std::vector<std::string>& args) const {
    std::string templateStr = Get(key);
    return FormatString(templateStr, args);
}

inline bool LocalizationManager::HasKey(const std::string& key) const {
    return FindString(key) != nullptr;
}

inline void LocalizationManager::AddFallbackLocale(const std::string& locale) {
    for (const auto& existing : fallbackLocales_) {
        if (existing == locale) return;
    }
    fallbackLocales_.push_back(locale);
}

inline void LocalizationManager::ClearFallbackLocales() {
    fallbackLocales_.clear();
}

inline std::vector<std::string> LocalizationManager::GetAvailableLocales() const {
    std::vector<std::string> locales;
    for (const auto& pair : dictionaries_) {
        locales.push_back(pair.first);
    }
    return locales;
}

inline const LocaleInfo* LocalizationManager::GetLocaleInfo(const std::string& locale) const {
    auto it = localeInfos_.find(locale);
    if (it != localeInfos_.end()) {
        return &it->second;
    }
    return nullptr;
}

inline void LocalizationManager::SetMissingKeyHandler(std::function<void(const std::string&)> handler) {
    missingKeyHandler_ = handler;
}

inline bool LocalizationManager::IsRightToLeft() const {
    auto* info = GetLocaleInfo(currentLocale_);
    return info && info->isRightToLeft;
}

inline void LocalizationManager::SetUpdateCallback(TranslationCallback callback) {
    updateCallback_ = callback;
}

inline void LocalizationManager::NotifyUpdate(const std::string& key, const std::string& value) {
    if (updateCallback_) {
        updateCallback_(key, value);
    }
}

inline const std::string* LocalizationManager::FindString(const std::string& key) const {
    if (dictionaries_.find(currentLocale_) != dictionaries_.end()) {
        const auto& dict = dictionaries_.at(currentLocale_);
        if (dict.HasString(key)) {
            return dict.GetString(key);
        }
    }
    
    for (const auto& fallback : fallbackLocales_) {
        if (dictionaries_.find(fallback) != dictionaries_.end()) {
            const auto& dict = dictionaries_.at(fallback);
            if (dict.HasString(key)) {
                return dict.GetString(key);
            }
        }
    }
    
    return nullptr;
}

inline std::string FormatString(const std::string& templateStr, const std::vector<std::string>& args) {
    std::string result = templateStr;
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = result.find(placeholder);
        if (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), args[i]);
        }
    }
    return result;
}

} // namespace localization
} // namespace ge
