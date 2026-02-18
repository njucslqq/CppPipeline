#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace memory_tracer {
namespace capture {

struct AllocationInfo {
    uint64_t timestamp;      // 纳秒时间戳
    void* address;           // 内存地址
    size_t size;             // 分配大小
    std::string function;    // 调用函数
    std::string file;        // 源文件
    int line;                // 行号
    uint32_t thread_id;      // 线程ID
    std::vector<std::string> stack_trace;  // 调用栈

    AllocationInfo() : address(nullptr), size(0), line(0), thread_id(0) {}
};

class Capture {
public:
    static Capture& GetInstance();

    // 初始化捕获模块
    void Initialize();

    // 关闭捕获模块
    void Shutdown();

    // 开始捕获
    void StartCapture();

    // 停止捕获
    void StopCapture();

    // 是否正在捕获
    bool IsCapturing() const;

    // 获取捕获到的内存分配信息
    const std::vector<AllocationInfo>& GetAllocations() const;

    // 清空捕获信息
    void Clear();

    // 设置回调函数，当有新的内存分配时调用
    using AllocationCallback = void(*)(const AllocationInfo&);
    void SetAllocationCallback(AllocationCallback callback);

private:
    Capture();
    ~Capture();
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

// 便捷宏
#define MT_CAPTURE_INIT()    memory_tracer::capture::Capture::GetInstance().Initialize()
#define MT_CAPTURE_START()    memory_tracer::capture::Capture::GetInstance().StartCapture()
#define MT_CAPTURE_STOP()     memory_tracer::capture::Capture::GetInstance().StopCapture()
#define MT_CAPTURE_SHUTDOWN() memory_tracer::capture::Capture::GetInstance().Shutdown()

} // namespace capture
} // namespace memory_tracer
