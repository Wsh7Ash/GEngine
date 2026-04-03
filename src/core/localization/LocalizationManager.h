#pragma once

// ================================================================
//  LocalizationManager.h
//  Central localization registry for i18n strings.
// ================================================================

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace ge {
namespace localization {

struct LocaleInfo {
    std::string code;
    std::string name;
    std::string nativeName;
    bool isRightToLeft = false;
    
    LocaleInfo() = default;
    LocaleInfo(const std::string& c, const std::string& n, const std::string& native, bool rtl = false)
        : code(c), name(n), nativeName(native), isRightToLeft(rtl) {}
};

using TranslationCallback = std::function<void(const std::string& key, const std::string& value)>;

class LocaleDictionary {
public:
    LocaleDictionary() = default;
    explicit LocaleDictionary(const std::string& locale) : locale_(locale) {}
    
    void SetLocale(const std::string& locale) { locale_ = locale; }
    const std::string& GetLocale() const { return locale_; }
    
    void SetString(const std::string& key, const std::string& value);
    const std::string* GetString(const std::string& key) const;
    
    bool HasString(const std::string& key) const;
    void RemoveString(const std::string& key);
    
    void MergeFrom(const LocaleDictionary& other);
    
    size_t GetStringCount() const { return strings_.size(); }
    void Clear() { strings_.clear(); }
    
    const std::unordered_map<std::string, std::string>& GetAllStrings() const { return strings_; }
    
private:
    std::string locale_;
    std::unordered_map<std::string, std::string> strings_;
};

class LocalizationManager {
public:
    static LocalizationManager& Get();
    
    LocalizationManager();
    ~LocalizationManager();
    
    void Initialize();
    void Shutdown();
    
    bool LoadDictionary(const std::string& locale, const std::string& filepath);
    bool LoadDictionaryFromJSON(const std::string& locale, const std::string& jsonContent);
    bool LoadDictionaryFromCSV(const std::string& locale, const std::string& csvContent);
    
    void UnloadDictionary(const std::string& locale);
    
    void SetCurrentLocale(const std::string& locale);
    const std::string& GetCurrentLocale() const { return currentLocale_; }
    
    std::string Get(const std::string& key) const;
    std::string Get(const std::string& key, const std::vector<std::string>& args) const;
    
    template<typename... Args>
    std::string GetFormatted(const std::string& key, Args&&... args) const {
        std::vector<std::string> argsVec = { std::forward<Args>(args)... };
        return Get(key, argsVec);
    }
    
    bool HasKey(const std::string& key) const;
    
    void AddFallbackLocale(const std::string& locale);
    void ClearFallbackLocales();
    
    std::vector<std::string> GetAvailableLocales() const;
    const LocaleInfo* GetLocaleInfo(const std::string& locale) const;
    
    void SetMissingKeyHandler(std::function<void(const std::string&)> handler);
    
    bool IsRightToLeft() const;
    
    void SetUpdateCallback(TranslationCallback callback);
    void NotifyUpdate(const std::string& key, const std::string& value);

private:
    const std::string* FindString(const std::string& key) const;
    
    std::unordered_map<std::string, LocaleDictionary> dictionaries_;
    std::unordered_map<std::string, LocaleInfo> localeInfos_;
    std::vector<std::string> fallbackLocales_;
    std::string currentLocale_ = "en-US";
    
    std::function<void(const std::string&)> missingKeyHandler_;
    TranslationCallback updateCallback_;
    
    bool isInitialized_ = false;
};

std::string FormatString(const std::string& templateStr, const std::vector<std::string>& args);

} // namespace localization
} // namespace ge
