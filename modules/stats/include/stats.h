#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "storage/storage.h"

namespace memory_tracer {
namespace stats {

struct FunctionStats {
    std::string function_name;
    size_t allocation_count;      // 分配次数
    size_t total_allocated;       // 总分配大小
    size_t current_allocated;     // 当前分配大小（未释放）
    size_t peak_allocated;        // 峰值分配大小
    double avg_size;              // 平均分配大小
    std::map<size_t, size_t> size_distribution;  // 大小分布

    FunctionStats() : allocation_count(0), total_allocated(0), current_allocated(0), peak_allocated(0), avg_size(0.0) {}
};

struct FileStats {
    std::string file_path;
    size_t allocation_count;
    size_t total_allocated;
    size_t current_allocated;
    std::map<std::string, size_t> function_counts;  // 各函数分配次数
};

struct SizeBucketStats {
    size_t min_size;
    size_t max_size;
    size_t count;
    size_t total_size;

    SizeBucketStats() : min_size(0), max_size(0), count(0), total_size(0) {}
    SizeBucketStats(size_t min, size_t max) : min_size(min), max_size(max), count(0), total_size(0) {}
};

class Stats {
public:
    static Stats& GetInstance();

    // 初始化统计模块
    void Initialize();

    // 关闭统计模块
    void Shutdown();

    // 添加内存分配记录
    void AddAllocation(const capture::AllocationInfo& info);

    // 批量添加内存分配记录
    void AddAllocations(const std::vector<capture::AllocationInfo>& allocations);

    // 按函数统计
    std::vector<FunctionStats> GetFunctionStats(int limit = 0);

    // 获取指定函数的统计信息
    FunctionStats GetFunctionStats(const std::string& function_name);

    // 按文件统计
    std::vector<FileStats> GetFileStats(int limit = 0);

    // 按大小分布统计
    std::vector<SizeBucketStats> GetSizeDistributionStats();

    // 获取内存热点（分配最多的地方）
    std::vector<std::pair<std::string, size_t>> GetMemoryHotspots(int limit = 10);

    // 获取调用栈统计
    std::map<std::string, size_t> GetCallStackStats();

    // 生成统计报告
    std::string GenerateReport();

    // 获取摘要信息
    std::string GetSummary();

    // 重置统计数据
    void Reset();

private:
    Stats();
    ~Stats();
    Stats(const Stats&) = delete;
    Stats& operator=(const Stats&) = delete;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace stats
} // namespace memory_tracer
