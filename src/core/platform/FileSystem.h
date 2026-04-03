#pragma once

// ================================================================
//  FileSystem.h
//  Cross-platform file system abstraction.
// ================================================================

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <chrono>

namespace ge {
namespace platform {

enum class FileMode {
    Read,
    Write,
    ReadWrite,
    Append,
    Truncate
};

enum class FileSeek {
    Begin,
    Current,
    End
};

class IFile {
public:
    virtual ~IFile() = default;

    virtual bool IsValid() const = 0;
    virtual void Close() = 0;

    virtual size_t Read(void* buffer, size_t size) = 0;
    virtual size_t Write(const void* buffer, size_t size) = 0;

    virtual bool Seek(int64_t offset, FileSeek origin) = 0;
    virtual int64_t Tell() const = 0;
    virtual int64_t GetSize() const = 0;

    virtual bool IsEOF() const = 0;
    virtual void Flush() = 0;

    virtual const std::string& GetPath() const = 0;
};

class FileReader : public IFile {
public:
    static std::unique_ptr<FileReader> Open(const std::string& path);

    FileReader();
    explicit FileReader(const std::string& path);
    ~FileReader() override;

    bool Open(const std::string& path);
    bool IsValid() const override;
    void Close() override;

    size_t Read(void* buffer, size_t size) override;
    size_t Write(const void* buffer, size_t size) override;

    bool Seek(int64_t offset, FileSeek origin) override;
    int64_t Tell() const override;
    int64_t GetSize() const override;

    bool IsEOF() const override;
    void Flush() override;

    const std::string& GetPath() const override;

    std::vector<uint8_t> ReadAll();
    std::string ReadAllText();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class FileWriter : public IFile {
public:
    static std::unique_ptr<FileWriter> Open(const std::string& path, bool binary = false);
    static std::unique_ptr<FileWriter> Create(const std::string& path, bool binary = false);
    static std::unique_ptr<FileWriter> Append(const std::string& path);

    FileWriter();
    explicit FileWriter(const std::string& path, bool binary = false);
    ~FileWriter() override;

    bool Open(const std::string& path, bool binary = false);
    bool IsValid() const override;
    void Close() override;

    size_t Read(void* buffer, size_t size) override;
    size_t Write(const void* buffer, size_t size) override;

    bool Seek(int64_t offset, FileSeek origin) override;
    int64_t Tell() const override;
    int64_t GetSize() const override;

    bool IsEOF() const override;
    void Flush() override;

    const std::string& GetPath() const override;

    void WriteAll(const std::vector<uint8_t>& data);
    void WriteAllText(const std::string& text);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class FileSystem {
public:
    static bool Exists(const std::string& path);
    static bool IsFile(const std::string& path);
    static bool IsDirectory(const std::string& path);
    static bool IsSymbolicLink(const std::string& path);

    static bool CreateDir(const std::string& path);
    static bool CreateDirs(const std::string& path);
    static bool RemoveFile(const std::string& path);
    static bool RemoveDir(const std::string& path);

    static std::string GetExtension(const std::string& path);
    static std::string GetFileName(const std::string& path);
    static std::string GetFileNameWithoutExtension(const std::string& path);
    static std::string GetDirectoryName(const std::string& path);
    static std::string GetFullPath(const std::string& path);
    static std::string GetParentPath(const std::string& path);
    static std::string Combine(const std::string& path1, const std::string& path2);

    static std::vector<std::string> ListFiles(const std::string& directory, const std::string& pattern = "*");
    static std::vector<std::string> ListDirectories(const std::string& directory);

    static std::int64_t GetFileSize(const std::string& path);
    static std::chrono::system_clock::time_point GetLastModifiedTime(const std::string& path);
    static std::chrono::system_clock::time_point GetCreationTime(const std::string& path);

    static bool Copy(const std::string& source, const std::string& dest);
    static bool Move(const std::string& source, const std::string& dest);

    static std::string GetWorkingDirectory();
    static bool SetWorkingDirectory(const std::string& path);

    static std::string GetTemporaryDirectory();

    static std::uint64_t GetDiskFreeSpace(const std::string& path = "");
    static std::uint64_t GetDiskTotalSpace(const std::string& path = "");
};

class DirectoryWatcher {
public:
    using Callback = std::function<void(const std::string& path, bool isDirectory)>;

    static std::unique_ptr<DirectoryWatcher> Create(const std::string& path, Callback callback);

    virtual ~DirectoryWatcher() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsWatching() const = 0;

    virtual const std::string& GetPath() const = 0;
};

class Path {
public:
    static std::string Normalize(const std::string& path);
    static std::string ToNative(const std::string& path);
    static bool IsAbsolute(const std::string& path);
    static std::string Join(const std::vector<std::string>& parts);
};

} // namespace platform
} // namespace ge
