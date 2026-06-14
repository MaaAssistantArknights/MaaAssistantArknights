#include "UpdaterPlan.h"
#include "UpdaterFile.h"
#include "UpdaterHandle.h"

#include <cstring>

// ---------------------------------------------------------------------------
// File I/O helpers
// ---------------------------------------------------------------------------

std::wstring BuildFileIoFailureReason(const wchar_t* action, const std::wstring& path, DWORD errorCode)
{
    return std::wstring(action) + L": " + path + L" (error=" + std::to_wstring(errorCode) + L")";
}

bool TryReadUtf8File(const std::wstring& path, std::string& content, std::wstring& failureReason)
{
    content.clear();
    failureReason.clear();

    ScopedHandle hFile(CreateFileW(
        path.c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!hFile) {
        failureReason = BuildFileIoFailureReason(L"Failed to open file", path, GetLastError());
        return false;
    }

    std::string buf;
    char chunk[4096];
    while (true) {
        DWORD chunkRead = 0;
        if (!ReadFile(hFile.get(), chunk, static_cast<DWORD>(sizeof(chunk)), &chunkRead, nullptr)) {
            failureReason = BuildFileIoFailureReason(L"Failed to read file", path, GetLastError());
            return false;
        }

        if (chunkRead == 0) {
            break;
        }

        buf.append(chunk, chunkRead);
    }

    content.swap(buf);
    return true;
}

// ---------------------------------------------------------------------------
// JSON parsing
// ---------------------------------------------------------------------------

// Convert a UTF-8 JSON string value to std::wstring.
// Handles basic \uXXXX, \n, \r, \t, \\, \/
std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 1) return {};
    std::wstring out(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
}

// Parse a JSON string literal starting at pos (after the opening quote).
// Advances pos past the closing quote. Returns the raw UTF-8 content.
std::string ParseJsonString(const std::string& json, size_t& pos)
{
    std::string result;
    while (pos < json.size()) {
        char c = json[pos++];
        if (c == '"') break;
        if (c == '\\' && pos < json.size()) {
            char esc = json[pos++];
            switch (esc) {
            case '"':  result += '"';  break;
            case '\\': result += '\\'; break;
            case '/':  result += '/';  break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            case 'u': {
                if (pos + 4 <= json.size()) {
                    char hex[5] = {};
                    memcpy(hex, json.c_str() + pos, 4);
                    pos += 4;
                    unsigned cp = static_cast<unsigned>(strtoul(hex, nullptr, 16));
                    // Encode code point as UTF-8
                    if (cp < 0x80) {
                        result += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        result += static_cast<char>(0xC0 | (cp >> 6));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (cp >> 12));
                        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                }
                break;
            }
            default: result += esc; break;
            }
        } else {
            result += c;
        }
    }
    return result;
}

size_t SkipJsonWhitespace(const std::string& json, size_t pos)
{
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                 json[pos] == '\r' || json[pos] == '\n'))
    {
        ++pos;
    }
    return pos;
}

// Skips one JSON value starting at `pos` and returns the position right after it.
size_t SkipJsonValue(const std::string& json, size_t pos)
{
    pos = SkipJsonWhitespace(json, pos);
    if (pos >= json.size()) return pos;

    char c = json[pos];
    if (c == '"') {
        ++pos;
        ParseJsonString(json, pos);
        return pos;
    }

    if (c == '{' || c == '[') {
        char open = c;
        char close = (c == '{') ? '}' : ']';
        int depth = 0;
        bool inString = false;
        bool escaped = false;

        for (; pos < json.size(); ++pos) {
            char ch = json[pos];
            if (inString) {
                if (escaped) {
                    escaped = false;
                } else if (ch == '\\') {
                    escaped = true;
                } else if (ch == '"') {
                    inString = false;
                }
                continue;
            }

            if (ch == '"') {
                inString = true;
                continue;
            }

            if (ch == open) {
                ++depth;
            } else if (ch == close) {
                --depth;
                if (depth == 0) {
                    ++pos;
                    break;
                }
            }
        }

        return pos;
    }

    // number / true / false / null
    while (pos < json.size()) {
        char ch = json[pos];
        if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            break;
        }
        ++pos;
    }
    return pos;
}

// Finds `"key": <value>` in the top-level JSON object and returns the start of `<value>`.
size_t FindTopLevelJsonValueStartByKey(const std::string& json, const char* key)
{
    size_t pos = SkipJsonWhitespace(json, 0);
    if (pos >= json.size() || json[pos] != '{') return std::string::npos;
    ++pos;

    while (pos < json.size()) {
        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size()) return std::string::npos;

        if (json[pos] == ',') {
            ++pos;
            continue;
        }

        if (json[pos] == '}') {
            return std::string::npos;
        }

        if (json[pos] != '"') {
            ++pos;
            continue;
        }

        ++pos;
        std::string currentKey = ParseJsonString(json, pos);

        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            return std::string::npos;
        }

        ++pos;
        pos = SkipJsonWhitespace(json, pos);
        if (pos >= json.size()) return std::string::npos;

        if (currentKey == key) {
            return pos;
        }

        pos = SkipJsonValue(json, pos);
    }

    return std::string::npos;
}

// Find a JSON array by key name and return its string elements.
std::vector<std::wstring> ParseJsonStringArray(
    const std::string& json,
    const char* key)
{
    std::vector<std::wstring> result;

    size_t pos = FindTopLevelJsonValueStartByKey(json, key);
    if (pos == std::string::npos) return result;

    if (pos >= json.size() || json[pos] != '[') return result;
    pos++; // consume '['

    // Parse array elements
    while (pos < json.size()) {
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                      json[pos] == '\r' || json[pos] == '\n' ||
                                      json[pos] == ','))
            pos++;

        if (pos >= json.size()) break;
        if (json[pos] == ']') break;
        if (json[pos] == '"') {
            pos++; // consume opening quote
            std::string val = ParseJsonString(json, pos);
            result.push_back(Utf8ToWide(val));
        } else {
            pos++;
        }
    }

    return result;
}

std::wstring ParseJsonStringProperty(const std::string& json, const char* key)
{
    size_t pos = FindTopLevelJsonValueStartByKey(json, key);
    if (pos == std::string::npos) return {};

    if (pos >= json.size() || json[pos] != '"') return {};

    pos++;
    return Utf8ToWide(ParseJsonString(json, pos));
}

// ---------------------------------------------------------------------------
// Plan loading
// ---------------------------------------------------------------------------

bool LoadPendingUpdatePlan(
    const std::wstring& planFile,
    PendingUpdatePlan& outPlan,
    std::wstring& failureReason,
    std::string* rawJson)
{
    outPlan = {};
    failureReason.clear();

    if (!PathExistsW(planFile)) {
        failureReason = L"Plan file not found: " + planFile;
        return false;
    }

    std::string planJson;
    if (!TryReadUtf8File(planFile, planJson, failureReason)) {
        return false;
    }

    if (rawJson != nullptr) {
        *rawJson = planJson;
    }

    if (planJson.empty()) {
        failureReason = L"Plan file is empty: " + planFile;
        return false;
    }

    outPlan.packageType = ParseJsonStringProperty(planJson, "packageType");
    outPlan.removeList = ParseJsonStringArray(planJson, "removeList");
    outPlan.moveList = ParseJsonStringArray(planJson, "moveList");
    return true;
}



