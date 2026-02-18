#include "stats/stats.h"
#include "logger/logger.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <mutex>

namespace memory_tracer {
namespace stats {

class Stats::Impl {
public:
    void Initialize() {
        LOG_INFO("Stats module initialized");
    }

    void Shutdown() {
        Reset();
        LOG_INFO("Stats module shutdown");
    }

    void AddAllocation(const capture::AllocationInfo& info) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 按函数统计
        auto& func_stats = function_stats_[info.function];
        func_stats.function_name = info.function;
        func_stats.allocation_count++;
        func_stats.total_allocated += info.size;
        func_stats.current_allocated += info.size;
        func_stats.avg_size = static_cast<double>(func_stats.total_allocated) / func_stats.allocation_count;

        // 更新峰值
        if (info.size > func_stats.peak_allocated) {
            func_stats.peak_allocated = info.size;
        }

        // 大小分布
        size_t bucket = info.size;
        func_stats.size_distribution[bucket]++;

        // 按文件统计
        auto& file_stats = file_stats_[info.file];
        file_stats.file_path = info.file;
        file_stats.allocation_count++;
        file_stats.total_allocated += info.size;
        file_stats.function_counts[info.function]++;

        // 调用栈统计
        std::string stack_key = BuildStackKey(info.stack_trace);
        call_stack_stats_[stack_key]++;

        // 总体统计
        total_allocations_++;
        total_memory_allocated_ += info.size;

        // 记录分配用于追踪释放
        allocation_tracking_[info.address] = {
            info.function,
            info.size,
            info.stack_trace
        };
    }

    void RecordDeallocation(void* address) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocation_tracking_.find(address);
        if (it != allocation_tracking_.end()) {
            const auto& tracking = it->second;
            auto& func_stats = function_stats_[tracking.function];
            if (func_stats.current_allocated >= tracking.size) {
                func_stats.current_allocated -= tracking.size;
            }

            allocation_tracking_.erase(it);
        }
    }

    std::vector<FunctionStats> GetFunctionStats(int limit) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<FunctionStats> result;
        for (const auto& [name, stats] : function_stats_) {
            result.push_back(stats);
        }

        // 按总分配大小排序
        std::sort(result.begin(), result.end(),
            [](const FunctionStats& a, const FunctionStats& b) {
                return a.total_allocated > b.total_allocated;
            });

        if (limit > 0 && result.size() > static_cast<size_t>(limit)) {
            result.resize(limit);
        }

        return result;
    }

    FunctionStats GetFunctionStats(const std::string& function_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = function_stats_.find(function_name);
        if (it != function_stats_.end()) {
            return it->second;
        }
        return FunctionStats();
    }

    std::vector<FileStats> GetFileStats(int limit) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<FileStats> result;
        for (const auto& [path, stats] : file_stats_) {
            result.push_back(stats);
        }

        // 按总分配大小排序
        std::sort(result.begin(), result.end(),
            [](const FileStats& a, const FileStats& b) {
                return a.total_allocated > b.total_allocated;
            });

        if (limit > 0 && result.size() > static_cast<size_t>(limit)) {
            result.resize(limit);
        }

        return result;
    }

    std::vector<SizeBucketStats> GetSizeDistributionStats() {
        std::lock_guard<std::mutex> lock(mutex_);

        // 定义大小区间
        std::vector<SizeBucketStats> buckets = {
            SizeBucketStats(0, 16),
            SizeBucketStats(16, 32),
            SizeBucketStats(32, 64),
            SizeBucketStats(64, 128),
            SizeBucketStats(128, 256),
            SizeBucketStats(256, 512),
            SizeBucketStats(512, 1024),
            SizeBucketStats(1024, 4096),
            SizeBucketStats(4096, 16384),
            SizeBucketStats(16384, 65536),
            SizeBucketStats(65536, SIZE_MAX),
        };

        // 统计所有函数的大小分布
        for (const auto& [func, stats] : function_stats_) {
            for (const auto& [size, count] : stats.size_distribution) {
                for (auto& bucket : buckets) {
                    if (size >= bucket.min_size && size < bucket.max_size) {
                        bucket.count += count;
                        bucket.total_size += size * count;
                        break;
                    }
                }
            }
        }

        // 移除空区间
        buckets.erase(
            std::remove_if(buckets.begin(), buckets.end(),
                [](const SizeBucketStats& b) { return b.count == 0; }),
            buckets.end());

        return buckets;
    }

    std::vector<std::pair<std::string, size_t>> GetMemoryHotspots(int limit) {
        auto func_stats = GetFunctionStats(0);

        std::vector<std::pair<std::string, size_t>> hotspots;
        for (const auto& stats : func_stats) {
            hotspots.push_back({stats.function_name, stats.total_allocated});
        }

        std::sort(hotspots.begin(), hotspots.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        if (limit > 0 && hotspots.size() > static_cast<size_t>(limit)) {
            hotspots.resize(limit);
        }

        return hotspots;
    }

    std::map<std::string, size_t> GetCallStackStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        return call_stack_stats_;
    }

    std::string GenerateReport() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;

        oss << "======================================\n";
        oss << "       Memory Tracer Report\n";
        oss << "======================================\n\n";

        oss << "Total Allocations: " << total_allocations_ << "\n";
        oss << "Total Memory Allocated: " << FormatSize(total_memory_allocated_) << "\n";
        oss << "Unique Functions: " << function_stats_.size() << "\n";
        oss << "Unique Files: " << file_stats_.size() << "\n\n";

        oss << "--- Top 10 Functions by Allocation Size ---\n";
        auto func_stats = GetFunctionStats(10);
        for (size_t i = 0; i < func_stats.size(); ++i) {
            const auto& stats = func_stats[i];
            oss << (i + 1) << ". " << stats.function_name << "\n";
            oss << "   Allocations: " << stats.allocation_count << "\n";
            oss << "   Total: " << FormatSize(stats.total_allocated) << "\n";
            oss << "   Current: " << FormatSize(stats.current_allocated) << "\n";
            oss << "   Avg: " << FormatSize(static_cast<size_t>(stats.avg_size)) << "\n";
        }

        oss << "\n--- Size Distribution ---\n";
        auto size_dist = GetSizeDistributionStats();
        for (const auto& bucket : size_dist) {
            oss << "[" << FormatSize(bucket.min_size) << ", ";
            if (bucket.max_size == SIZE_MAX) {
                oss << "inf)";
            } else {
                oss << FormatSize(bucket.max_size) << ")";
            }
            oss << ": " << bucket.count << " allocs, " << FormatSize(bucket.total_size) << "\n";
        }

        oss << "\n======================================\n";

        return oss.str();
    }

    std::string GetSummary() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream oss;
        oss << "Total allocations: " << total_allocations_ << "\n";
        oss << "Total memory: " << FormatSize(total_memory_allocated_) << "\n";
        oss << "Functions: " << function_stats_.size() << "\n";

        return oss.str();
    }

    void Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        function_stats_.clear();
        file_stats_.clear();
        call_stack_stats_.clear();
        allocation_tracking_.clear();
        total_allocations_ = 0;
        total_memory_allocated_ = 0;
    }

private:
    std::string BuildStackKey(const std::vector<std::string>& stack_trace) {
        std::ostringstream oss;
        for (size_t i = 0; i < stack_trace.size() && i < 5; ++i) {
            if (i > 0) oss << " <- ";
            oss << stack_trace[i];
        }
        return oss.str();
    }

    std::string FormatSize(size_t size) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size_d = static_cast<double>(size);

        while (size_d >= 1024 && unit < 4) {
            size_d /= 1024;
            unit++;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size_d << " " << units[unit];
        return oss.str();
    }

    std::map<std::string, FunctionStats> function_stats_;
    std::map<std::string, FileStats> file_stats_;
    std::map<std::string, size_t> call_stack_stats_;

    struct AllocationTracking {
        std::string function;
        size_t size;
        std::vector<std::string> stack_trace;
    };
    std::map<void*, AllocationTracking> allocation_tracking_;

    size_t total_allocations_ = 0;
    size_t total_memory_allocated_ = 0;

    mutable std::mutex mutex_;
};

Stats::Stats() : pimpl_(std::make_unique<Impl>()) {}
Stats::~Stats() = default;

Stats& Stats::GetInstance() {
    static Stats instance;
    return instance;
}

void Stats::Initialize() { pimpl_->Initialize(); }
void Stats::Shutdown() { pimpl_->Shutdown(); }
void Stats::AddAllocation(const capture::AllocationInfo& info) { pimpl_->AddAllocation(info); }
void Stats::AddAllocations(const std::vector<capture::AllocationInfo>& allocations) {
    for (const auto& info : allocations) {
        pimpl_->AddAllocation(info);
    }
}
std::vector<FunctionStats> Stats::GetFunctionStats(int limit) { return pimpl_->GetFunctionStats(limit); }
FunctionStats Stats::GetFunctionStats(const std::string& function_name) {
    return pimpl_->GetFunctionStats(function_name);
}
std::vector<FileStats> Stats::GetFileStats(int limit) { return pimpl_->GetFileStats(limit); }
std::vector<SizeBucketStats> Stats::GetSizeDistributionStats() { return pimpl_->GetSizeDistributionStats(); }
std::vector<std::pair<std::string, size_t>> Stats::GetMemoryHotspots(int limit) {
    return pimpl_->GetMemoryHotspots(limit);
}
std::map<std::string, size_t> Stats::GetCallStackStats() { return pimpl_->GetCallStackStats(); }
std::string Stats::GenerateReport() { return pimpl_->GenerateReport(); }
std::string Stats::GetSummary() { return pimpl_->GetSummary(); }
void Stats::Reset() { pimpl_->Reset(); }

} // namespace stats
} // namespace memory_tracer
