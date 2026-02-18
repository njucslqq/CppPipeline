#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

#include "capture/capture.h"

namespace memory_tracer {
namespace storage {

using json = nlohmann::json;

struct QueryResult {
    std::vector<capture::AllocationInfo> allocations;
    size_t total_count;
    size_t total_size;
    size_t peak_usage;

    QueryResult() : total_count(0), total_size(0), peak_usage(0) {}
};

class Storage {
public:
    static Storage& GetInstance();

    // 初始化存储模块
    void Initialize(const std::string& data_dir = "./data");

    // 关闭存储模块
    void Shutdown();

    // 添加内存分配记录
    void AddAllocation(const capture::AllocationInfo& info);

    // 批量添加内存分配记录
    void AddAllocations(const std::vector<capture::AllocationInfo>& allocations);

    // 根据函数名查询
    QueryResult QueryByFunction(const std::string& function_name);

    // 根据文件名查询
    QueryResult QueryByFile(const std::string& file_path);

    // 根据大小范围查询
    QueryResult QueryBySizeRange(size_t min_size, size_t max_size);

    // 根据时间范围查询
    QueryResult QueryByTimeRange(uint64_t start_time, uint64_t end_time);

    // 获取所有内存泄漏（未释放的分配）
    std::vector<capture::AllocationInfo> GetLeaks();

    // 获取统计摘要
    json GetSummary();

    // 导出到 JSON 文件
    bool ExportToJson(const std::string& filepath);

    // 从 JSON 文件导入
    bool ImportFromJson(const std::string& filepath);

    // 获取分配时间线
    json GetAllocationTimeline(size_t bucket_size_ns = 1000000000);  // 默认 1秒

    // 设置存储上限（防止内存占用过大）
    void SetMaxAllocations(size_t max_allocations);

    // 清空所有存储
    void Clear();

private:
    Storage();
    ~Storage();
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace storage
} // namespace memory_tracer
