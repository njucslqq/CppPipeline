#include "capture/capture.h"
#include "logger/logger.h"
#include <backward.hpp>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <dlfcn.h>
#include <cstring>

namespace memory_tracer {
namespace capture {

// 全局 hook 函数指针
static void* (*real_malloc)(size_t) = nullptr;
static void (*real_free)(void*) = nullptr;
static void* (*real_realloc)(void*, size_t) = nullptr;

class Capture::Impl {
public:
    Impl() : capturing_(false) {
        // 初始化 backward-cpp
        backward::StackTrace st;
    }

    void Initialize() {
        std::lock_guard<std::mutex> lock(mutex_);

        // 加载原始 malloc/free 函数
        real_malloc = reinterpret_cast<void*(*)(size_t)>(dlsym(RTLD_NEXT, "malloc"));
        real_free = reinterpret_cast<void(*)(void*)>(dlsym(RTLD_NEXT, "free"));
        real_realloc = reinterpret_cast<void*(*)(void*, size_t)>(dlsym(RTLD_NEXT, "realloc"));

        if (!real_malloc || !real_free || !real_realloc) {
            LOG_ERROR("Failed to load original memory functions");
            return;
        }

        LOG_INFO("Memory capture module initialized");
    }

    void Shutdown() {
        StopCapture();
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.clear();
        active_allocations_.clear();
        LOG_INFO("Memory capture module shutdown");
    }

    void StartCapture() {
        std::lock_guard<std::mutex> lock(mutex_);
        capturing_ = true;
        LOG_INFO("Memory capture started");
    }

    void StopCapture() {
        std::lock_guard<std::mutex> lock(mutex_);
        capturing_ = false;
        LOG_INFO("Memory capture stopped");
    }

    bool IsCapturing() const {
        return capturing_;
    }

    const std::vector<AllocationInfo>& GetAllocations() const {
        return allocations_;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.clear();
        active_allocations_.clear();
    }

    void SetAllocationCallback(AllocationCallback callback) {
        allocation_callback_ = callback;
    }

    void RecordAllocation(void* address, size_t size, const char* function, const char* file, int line) {
        if (!capturing_) return;

        AllocationInfo info;
        info.timestamp = GetTimestamp();
        info.address = address;
        info.size = size;
        info.function = function ? function : "unknown";
        info.file = file ? file : "unknown";
        info.line = line;
        info.thread_id = GetThreadId();

        CaptureStackTrace(info);

        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.push_back(info);
        active_allocations_[address] = allocations_.size() - 1;

        if (allocation_callback_) {
            allocation_callback_(info);
        }
    }

    void RecordDeallocation(void* address) {
        if (!capturing_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = active_allocations_.find(address);
        if (it != active_allocations_.end()) {
            // 标记为已释放
            allocations_[it->second].address = nullptr;
            active_allocations_.erase(it);
        }
    }

private:
    uint64_t GetTimestamp() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }

    uint32_t GetThreadId() const {
        std::hash<std::thread::id> hasher;
        return static_cast<uint32_t>(hasher(std::this_thread::get_id()));
    }

    void CaptureStackTrace(AllocationInfo& info) {
        backward::StackTrace st;
        st.load_here(32);

        backward::Resolver resolver;
        resolver.load_stacktrace(st);

        for (size_t i = 0; i < st.size(); ++i) {
            backward::ResolvedTrace trace = resolver.resolve(st[i]);
            std::string frame = trace.object_function;
            if (!frame.empty()) {
                info.stack_trace.push_back(frame);
            }
        }
    }

    bool capturing_;
    std::vector<AllocationInfo> allocations_;
    std::map<void*, size_t> active_allocations_;
    mutable std::mutex mutex_;
    AllocationCallback allocation_callback_;
};

// 全局实例
Capture::Impl* g_capture_impl = nullptr;

// Hook 的 malloc 实现
extern "C" void* malloc(size_t size) {
    if (!real_malloc) {
        // 初始化期间直接调用原始函数
        return __libc_malloc(size);
    }

    void* ptr = real_malloc(size);

    if (g_capture_impl) {
        g_capture_impl->RecordAllocation(ptr, size, "malloc", nullptr, 0);
    }

    return ptr;
}

// Hook 的 free 实现
extern "C" void free(void* ptr) {
    if (!real_free) {
        __libc_free(ptr);
        return;
    }

    if (g_capture_impl && ptr) {
        g_capture_impl->RecordDeallocation(ptr);
    }

    real_free(ptr);
}

// Hook 的 realloc 实现
extern "C" void* realloc(void* ptr, size_t size) {
    if (!real_realloc) {
        return __libc_realloc(ptr, size);
    }

    void* new_ptr = real_realloc(ptr, size);

    if (g_capture_impl) {
        if (ptr) {
            g_capture_impl->RecordDeallocation(ptr);
        }
        if (new_ptr) {
            g_capture_impl->RecordAllocation(new_ptr, size, "realloc", nullptr, 0);
        }
    }

    return new_ptr;
}

Capture::Capture() : pimpl_(std::make_unique<Impl>()) {
    g_capture_impl = pimpl_.get();
}

Capture::~Capture() {
    Shutdown();
    g_capture_impl = nullptr;
}

Capture& Capture::GetInstance() {
    static Capture instance;
    return instance;
}

void Capture::Initialize() { pimpl_->Initialize(); }
void Capture::Shutdown() { pimpl_->Shutdown(); }
void Capture::StartCapture() { pimpl_->StartCapture(); }
void Capture::StopCapture() { pimpl_->StopCapture(); }
bool Capture::IsCapturing() const { return pimpl_->IsCapturing(); }
const std::vector<AllocationInfo>& Capture::GetAllocations() const { return pimpl_->GetAllocations(); }
void Capture::Clear() { pimpl_->Clear(); }
void Capture::SetAllocationCallback(AllocationCallback callback) { pimpl_->SetAllocationCallback(callback); }

} // namespace capture
} // namespace memory_tracer
