#pragma once

// ================================================================
//  Thread.h
//  Cross-platform threading abstraction.
// ================================================================

#include <functional>
#include <memory>
#include <vector>
#include <cstdint>
#include <atomic>
#include <string>

namespace ge {
namespace platform {

class Thread {
public:
    using Func = std::function<void(void*)>;
    using ID = uint64_t;

    static std::unique_ptr<Thread> Create(Func func, void* userData = nullptr, const std::string& name = "");
    static ID GetCurrentThreadID();
    static void Sleep(uint32_t milliseconds);
    static void Yield();

    virtual ~Thread() = default;

    virtual void Start() = 0;
    virtual void Join() = 0;
    virtual void Detach() = 0;
    virtual ID GetID() const = 0;
    virtual bool IsRunning() const = 0;
    virtual void SetAffinity(int core) = 0;
    virtual void SetPriority(int priority) = 0;

    virtual const std::string& GetName() const { return name_; }

protected:
    Thread() = default;
    Thread(Func func, void* userData, const std::string& name);

    Func func_;
    void* userData_;
    std::string name_;
};

class Mutex {
public:
    static std::unique_ptr<Mutex> Create(bool recursive = false);

    virtual ~Mutex() = default;

    virtual void Lock() = 0;
    virtual bool TryLock() = 0;
    virtual void Unlock() = 0;

    virtual void* GetNativeHandle() = 0;

protected:
    Mutex() = default;
};

class ReadWriteMutex {
public:
    static std::unique_ptr<ReadWriteMutex> Create();

    virtual ~ReadWriteMutex() = default;

    virtual void ReadLock() = 0;
    virtual bool TryReadLock() = 0;
    virtual void ReadUnlock() = 0;

    virtual void WriteLock() = 0;
    virtual bool TryWriteLock() = 0;
    virtual void WriteUnlock() = 0;

protected:
    ReadWriteMutex() = default;
};

class Semaphore {
public:
    static std::unique_ptr<Semaphore> Create(int32_t initialCount = 0, int32_t maxCount = 1);

    virtual ~Semaphore() = default;

    virtual void Signal() = 0;
    virtual void Signal(int32_t count) = 0;
    virtual bool Wait(int32_t timeoutMs = -1) = 0;

protected:
    Semaphore() = default;
};

class Atomic32 {
public:
    Atomic32(int32_t initial = 0) : value_(initial) {}
    ~Atomic32() = default;

    int32_t Load() const { return value_.load(std::memory_order_acquire); }
    void Store(int32_t v) { value_.store(v, std::memory_order_release); }
    int32_t FetchAdd(int32_t delta) { return value_.fetch_add(delta, std::memory_order_acq_rel); }
    int32_t FetchSub(int32_t delta) { return value_.fetch_sub(delta, std::memory_order_acq_rel); }
    int32_t FetchAnd(int32_t mask) { return value_.fetch_and(mask, std::memory_order_acq_rel); }
    int32_t FetchOr(int32_t mask) { return value_.fetch_or(mask, std::memory_order_acq_rel); }
    bool CompareAndSwap(int32_t expected, int32_t desired) {
        return value_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel);
    }

private:
    std::atomic<int32_t> value_;
};

class Atomic64 {
public:
    Atomic64(int64_t initial = 0) : value_(initial) {}
    ~Atomic64() = default;

    int64_t Load() const { return value_.load(std::memory_order_acquire); }
    void Store(int64_t v) { value_.store(v, std::memory_order_release); }
    int64_t FetchAdd(int64_t delta) { return value_.fetch_add(delta, std::memory_order_acq_rel); }
    int64_t FetchSub(int64_t delta) { return value_.fetch_sub(delta, std::memory_order_acq_rel); }
    bool CompareAndSwap(int64_t expected, int64_t desired) {
        return value_.compare_exchange_strong(expected, desired, std::memory_order_acq_rel);
    }

private:
    std::atomic<int64_t> value_;
};

class ThreadLocal {
public:
    ThreadLocal() = default;
    explicit ThreadLocal(void* value);
    ~ThreadLocal();

    void* Get() const;
    void Set(void* value);

private:
    void* value_ = nullptr;
};

class ThreadGuard {
public:
    ThreadGuard(Thread* thread);
    ~ThreadGuard();

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    ThreadGuard(ThreadGuard&& other) noexcept;
    ThreadGuard& operator=(ThreadGuard&& other) noexcept;

    void* GetUserData() const { return userData_; }

private:
    Thread* thread_ = nullptr;
    void* userData_ = nullptr;
};

class ScopedLock {
public:
    explicit ScopedLock(Mutex* mutex);
    ~ScopedLock();

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    Mutex* mutex_;
};

} // namespace platform
} // namespace ge
