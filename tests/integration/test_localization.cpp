#include "../catch_amalgamated.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>

TEST_CASE("Localization - Basic Get", "[localization]")
{
    std::unordered_map<std::string, std::string> dict = {
        {"greeting", "Hello"},
        {"farewell", "Goodbye"}
    };
    
    auto it = dict.find("greeting");
    REQUIRE(it != dict.end());
    REQUIRE(it->second == "Hello");
    
    it = dict.find("farewell");
    REQUIRE(it != dict.end());
    REQUIRE(it->second == "Goodbye");
}

TEST_CASE("Localization - Placeholder Replacement", "[localization]")
{
    std::string templateStr = "Hello {0}, you have {1} messages";
    std::vector<std::string> args = {"Alice", "3"};
    
    std::string result = templateStr;
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = result.find(placeholder);
        if (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), args[i]);
        }
    }
    
    REQUIRE(result == "Hello Alice, you have 3 messages");
}

TEST_CASE("Localization - Missing Placeholders", "[localization]")
{
    std::string templateStr = "Hello {0}, welcome to {1}";
    std::vector<std::string> args = {"Bob"};
    
    std::string result = templateStr;
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = result.find(placeholder);
        if (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), args[i]);
        }
    }
    
    REQUIRE(result == "Hello Bob, welcome to {1}");
}

TEST_CASE("Localization - Pluralization Simple", "[localization]")
{
    auto selectPlural = [](int count) -> const char* {
        return (count == 1) ? "one" : "other";
    };
    
    std::unordered_map<std::string, std::string> plurals = {
        {"one", "{n} item"},
        {"other", "{n} items"}
    };
    
    std::string form1 = plurals[selectPlural(1)];
    std::string form5 = plurals[selectPlural(5)];
    std::string form0 = plurals[selectPlural(0)];
    
    REQUIRE(form1 == "{n} item");
    REQUIRE(form5 == "{n} items");
    REQUIRE(form0 == "{n} items");
}

TEST_CASE("Localization - Pluralization Zero Case", "[localization]")
{
    auto selectPlural = [](int count) -> const char* {
        if (count == 0) return "zero";
        if (count == 1) return "one";
        return "other";
    };
    
    std::unordered_map<std::string, std::string> plurals = {
        {"zero", "No items"},
        {"one", "1 item"},
        {"other", "{n} items"}
    };
    
    REQUIRE(plurals[selectPlural(0)] == "No items");
    REQUIRE(plurals[selectPlural(1)] == "1 item");
    REQUIRE(plurals[selectPlural(5)] == "{n} items");
}

TEST_CASE("Localization - Pluralization Replace N", "[localization]")
{
    std::string templateStr = "{n} items";
    int count = 42;
    
    size_t pos = templateStr.find("{n}");
    if (pos != std::string::npos) {
        templateStr.replace(pos, 3, std::to_string(count));
    }
    
    REQUIRE(templateStr == "42 items");
}

TEST_CASE("Localization - Number Formatting", "[localization]")
{
    auto formatNumber = [](double value, int decimals) -> std::string {
        std::string result = std::to_string(value);
        size_t dotPos = result.find('.');
        if (dotPos != std::string::npos && decimals > 0) {
            result = result.substr(0, dotPos + 1 + decimals);
        } else if (decimals == 0) {
            result = result.substr(0, dotPos);
        }
        return result;
    };
    
    REQUIRE(formatNumber(3.14159, 2) == "3.14");
    REQUIRE(formatNumber(1000.0, 0) == "1000");
    REQUIRE(formatNumber(42.5, 1) == "42.5");
}

TEST_CASE("Localization - Currency Formatting", "[localization]")
{
    auto formatCurrency = [](double amount, const std::string& symbol) -> std::string {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << amount;

        std::string value = stream.str();
        if (value.find('.') != std::string::npos) {
            while (!value.empty() && value.back() == '0') {
                value.pop_back();
            }
            if (!value.empty() && value.back() == '.') {
                value.pop_back();
            }
        }

        return symbol + value;
    };
    
    REQUIRE(formatCurrency(9.99, "$") == "$9.99");
    REQUIRE(formatCurrency(100.0, "€") == "€100");
}

TEST_CASE("Localization - Date Formatting Basic", "[localization]")
{
    struct Date {
        int year, month, day;
    };
    
    auto formatDate = [](const Date& d, const std::string& style) -> std::string {
        if (style == "short") {
            return std::to_string(d.month) + "/" + std::to_string(d.day) + "/" + std::to_string(d.year % 100);
        } else if (style == "medium") {
            std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            return months[d.month - 1] + " " + std::to_string(d.day) + ", " + std::to_string(d.year);
        }
        return "";
    };
    
    Date d = {2024, 12, 25};
    REQUIRE(formatDate(d, "short") == "12/25/24");
    REQUIRE(formatDate(d, "medium") == "Dec 25, 2024");
}

TEST_CASE("Localization - Gender Select", "[localization]")
{
    std::unordered_map<std::string, std::string> genderForms = {
        {"male", "He is a developer"},
        {"female", "She is a developer"},
        {"other", "They are a developer"}
    };
    
    REQUIRE(genderForms["male"] == "He is a developer");
    REQUIRE(genderForms["female"] == "She is a developer");
    REQUIRE(genderForms["other"] == "They are a developer");
}

TEST_CASE("Localization - Fallback Chain", "[localization]")
{
    std::vector<std::string> fallbackChain = {"en-GB", "en", "en-US"};
    
    std::unordered_map<std::string, std::string> enGB = {{"colour", "colour"}};
    std::unordered_map<std::string, std::string> en = {{"color", "color"}, {"colour", "color"}};
    std::unordered_map<std::string, std::string> enUS = {{"color", "color"}};
    
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> dictionaries = {
        {"en-GB", enGB},
        {"en", en},
        {"en-US", enUS}
    };
    
    auto findString = [&](const std::string& key) -> std::string {
        for (const auto& locale : fallbackChain) {
            auto it = dictionaries[locale].find(key);
            if (it != dictionaries[locale].end()) {
                return it->second;
            }
        }
        return "";
    };
    
    REQUIRE(findString("colour") == "colour");
    REQUIRE(findString("color") == "color");
    REQUIRE(findString("missing") == "");
}

TEST_CASE("Localization - RTL Detection", "[localization]")
{
    auto isRTL = [](const std::string& locale) -> bool {
        std::vector<std::string> rtlLocales = {"ar", "he", "fa", "ur", "yi"};
        for (const auto& rtl : rtlLocales) {
            if (locale.find(rtl) == 0) return true;
        }
        return false;
    };
    
    REQUIRE(isRTL("ar-SA") == true);
    REQUIRE(isRTL("he-IL") == true);
    REQUIRE(isRTL("en-US") == false);
    REQUIRE(isRTL("fr-FR") == false);
    REQUIRE(isRTL("fa-IR") == true);
}

TEST_CASE("Localization - Escape Sequences", "[localization]")
{
    auto unescape = [](const std::string& str) -> std::string {
        std::string result;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                if (str[i + 1] == 'n') {
                    result += '\n';
                    ++i;
                } else if (str[i + 1] == 't') {
                    result += '\t';
                    ++i;
                } else if (str[i + 1] == '\\') {
                    result += '\\';
                    ++i;
                } else {
                    result += str[i];
                }
            } else {
                result += str[i];
            }
        }
        return result;
    };
    
    REQUIRE(unescape("Hello\\nWorld") == "Hello\nWorld");
    REQUIRE(unescape("Tab:\\there") == "Tab:\there");
    REQUIRE(unescape("Literal\\\\text") == "Literal\\text");
}

TEST_CASE("Localization - Unicode Handling", "[localization]")
{
    std::string english = "Hello";
    std::string japanese = reinterpret_cast<const char*>(u8"\u3053\u3093\u306b\u3061\u306f");
    std::string arabic = reinterpret_cast<const char*>(u8"\u0627\u0644\u0633\u0644\u0627\u0645");
    
    REQUIRE(english.size() == 5);
    REQUIRE(japanese.size() > 5);
    REQUIRE(arabic.size() > 5);
    
    bool isASCII = true;
    for (char c : english) {
        if (static_cast<unsigned char>(c) > 127) {
            isASCII = false;
            break;
        }
    }
    REQUIRE(isASCII == true);
    
    isASCII = true;
    for (char c : japanese) {
        if (static_cast<unsigned char>(c) > 127) {
            isASCII = false;
            break;
        }
    }
    REQUIRE(isASCII == false);
}

TEST_CASE("Localization - Case Sensitivity", "[localization]")
{
    std::unordered_map<std::string, std::string> dict = {
        {"Hello", "Hello"},
        {"HELLO", "HELLO"},
        {"hello", "hello"}
    };
    
    REQUIRE(dict.find("Hello") != dict.end());
    REQUIRE(dict.find("HELLO") != dict.end());
    REQUIRE(dict.find("hello") != dict.end());
    
    REQUIRE(dict["Hello"] != dict["hello"]);
}

TEST_CASE("Localization - Empty String Fallback", "[localization]")
{
    std::unordered_map<std::string, std::string> dict = {};
    
    auto findWithFallback = [&](const std::string& key) -> std::string {
        auto it = dict.find(key);
        if (it != dict.end() && !it->second.empty()) {
            return it->second;
        }
        return "[missing: " + key + "]";
    };
    
    REQUIRE(findWithFallback("exists") == "[missing: exists]");
    dict["exists"] = "";
    REQUIRE(findWithFallback("exists") == "[missing: exists]");
    dict["exists"] = "found";
    REQUIRE(findWithFallback("exists") == "found");
}

TEST_CASE("Localization - Format String Injection Prevention", "[localization]")
{
    auto safeFormat = [](const std::string& str) -> std::string {
        std::string result;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '{') {
                size_t close = str.find('}', i);
                if (close == std::string::npos) {
                    result += str[i];
                } else {
                    std::string inner = str.substr(i + 1, close - i - 1);
                    bool isNumber = true;
                    for (char c : inner) {
                        if (!std::isdigit(static_cast<unsigned char>(c))) {
                            isNumber = false;
                            break;
                        }
                    }
                    if (isNumber) {
                        result += str.substr(i, close - i + 1);
                    }
                    i = close;
                }
            } else {
                result += str[i];
            }
        }
        return result;
    };
    
    std::string malicious = "Hello {0} {malicious<script>}";
    std::string safe = safeFormat(malicious);
    REQUIRE(safe.find("<script>") == std::string::npos);
}

TEST_CASE("Localization - Nested Formatting", "[localization]")
{
    std::unordered_map<std::string, std::string> templates = {
        {"greeting", "Hello {name}"},
        {"full", "{greeting}, welcome to {place}!"}
    };
    
    auto expand = [&](const std::string& key, const std::unordered_map<std::string, std::string>& values) -> std::string {
        std::string result = templates[key];
        for (const auto& [k, v] : values) {
            std::string placeholder = "{" + k + "}";
            size_t pos = result.find(placeholder);
            if (pos != std::string::npos) {
                result.replace(pos, placeholder.length(), v);
            }
        }
        return result;
    };
    
    std::unordered_map<std::string, std::string> values = {
        {"name", "Alice"},
        {"place", "Wonderland"}
    };
    
    std::string expanded = expand("full", values);
    REQUIRE(expanded == "{greeting}, welcome to Wonderland!");
    
    values["greeting"] = "Hello Alice";
    expanded = expand("full", values);
    REQUIRE(expanded == "Hello Alice, welcome to Wonderland!");
}

TEST_CASE("Localization - Argument Type Coercion", "[localization]")
{
    auto formatArgument = [](const std::string& arg) -> std::string {
        if (arg == "{n}") return "42";
        if (arg == "{s}") return "test";
        if (arg == "{d}") return "3.14";
        return arg;
    };
    
    std::string templateStr = "Count: {n}, Text: {s}, Decimal: {d}";
    std::vector<std::string> args = {"{n}", "{s}", "{d}"};
    
    for (auto& arg : args) {
        arg = formatArgument(arg);
    }
    
    std::string result = templateStr;
    result.replace(result.find("{n}"), 3, args[0]);
    result.replace(result.find("{s}"), 3, args[1]);
    result.replace(result.find("{d}"), 3, args[2]);
    
    REQUIRE(result == "Count: 42, Text: test, Decimal: 3.14");
}
