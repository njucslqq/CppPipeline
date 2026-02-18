#include "storage/storage.h"
#include "logger/logger.h"
#include <fstream>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>

namespace memory_tracer {
namespace storage {

class Storage::Impl {
public:
    Impl() : max_allocations_(1000000) {}

    void Initialize(const std::string& data_dir) {
        data_dir_ = data_dir;

        // 创建数据目录
        mkdir(data_dir_.c_str(), 0755);

        LOG_INFO("Storage module initialized, data directory: {}", data_dir_);
    }

    void Shutdown() {
        SaveToFile();
        Clear();
        LOG_INFO("Storage module shutdown");
    }

    void AddAllocation(const capture::AllocationInfo& info) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (allocations_.size() >= max_allocations_) {
            // 达到上限，移除最旧的记录
            allocations_.erase(allocations_.begin());
        }

        allocations_.push_back(info);

        // 更新索引
        function_index_[info.function].push_back(allocations_.size() - 1);
        file_index_[info.file].push_back(allocations_.size() - 1);
        time_index_.push_back({info.timestamp, allocations_.size() - 1});

        // 按时间排序
        std::sort(time_index_.begin(), time_index_.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }

    void AddAllocations(const std::vector<capture::AllocationInfo>& allocations) {
        for (const auto& info : allocations) {
            AddAllocation(info);
        }
    }

    QueryResult QueryByFunction(const std::string& function_name) {
        QueryResult result;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = function_index_.find(function_name);
        if (it == function_index_.end()) {
            return result;
        }

        for (size_t idx : it->second) {
            if (idx < allocations_.size()) {
                const auto& info = allocations_[idx];
                if (info.address != nullptr) {  // 只统计未释放的
                    result.allocations.push_back(info);
                    result.total_count++;
                    result.total_size += info.size;
                }
            }
        }

        CalculatePeakUsage(result);
        return result;
    }

    QueryResult QueryByFile(const std::string& file_path) {
        QueryResult result;

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = file_index_.find(file_path);
        if (it == file_index_.end()) {
            return result;
        }

        for (size_t idx : it->second) {
            if (idx < allocations_.size()) {
                const auto& info = allocations_[idx];
                if (info.address != nullptr) {
                    result.allocations.push_back(info);
                    result.total_count++;
                    result.total_size += info.size;
                }
            }
        }

        CalculatePeakUsage(result);
        return result;
    }

    QueryResult QueryBySizeRange(size_t min_size, size_t max_size) {
        QueryResult result;

        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& info : allocations_) {
            if (info.address != nullptr && info.size >= min_size && info.size <= max_size) {
                result.allocations.push_back(info);
                result.total_count++;
                result.total_size += info.size;
            }
        }

        CalculatePeakUsage(result);
        return result;
    }

    QueryResult QueryByTimeRange(uint64_t start_time, uint64_t end_time) {
        QueryResult result;

        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& info : allocations_) {
            if (info.timestamp >= start_time && info.timestamp <= end_time) {
                result.allocations.push_back(info);
                result.total_count++;
                if (info.address != nullptr) {
                    result.total_size += info.size;
                }
            }
        }

        CalculatePeakUsage(result);
        return result;
    }

    std::vector<capture::AllocationInfo> GetLeaks() {
        std::vector<capture::AllocationInfo> leaks;

        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& info : allocations_) {
            if (info.address != nullptr) {
                leaks.push_back(info);
            }
        }

        return leaks;
    }

    json GetSummary() {
        std::lock_guard<std::mutex> lock(mutex_);

        json summary;
        summary["total_allocations"] = allocations_.size();
        summary["unique_functions"] = function_index_.size();
        summary["data_dir"] = data_dir_;

        // 统计每个函数的分配情况
        json functions = json::object();
        for (const auto& [func, indices] : function_index_) {
            size_t count = 0;
            size_t total_size = 0;
            for (size_t idx : indices) {
                if (idx < allocations_.size()) {
                    count++;
                    total_size += allocations_[idx].size;
                }
            }
            functions[func] = {
                {"count", count},
                {"total_size", total_size}
            };
        }
        summary["by_function"] = functions;

        return summary;
    }

    bool ExportToJson(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);

        try {
            json j;
            j["allocations"] = json::array();

            for (const auto& info : allocations_) {
                j["allocations"].push_back({
                    {"timestamp", info.timestamp},
                    {"address", reinterpret_cast<uint64_t>(info.address)},
                    {"size", info.size},
                    {"function", info.function},
                    {"file", info.file},
                    {"line", info.line},
                    {"thread_id", info.thread_id},
                    {"stack_trace", info.stack_trace}
                });
            }

            std::ofstream file(filepath);
            file << j.dump(2);
            file.close();

            LOG_INFO("Exported {} allocations to {}", allocations_.size(), filepath);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to export to JSON: {}", e.what());
            return false;
        }
    }

    bool ImportFromJson(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open file: {}", filepath);
                return false;
            }

            json j;
            file >> j;

            if (j.contains("allocations")) {
                for (const auto& item : j["allocations"]) {
                    capture::AllocationInfo info;
                    info.timestamp = item["timestamp"];
                    info.address = reinterpret_cast<void*>(static_cast<uintptr_t>(item["address"]));
                    info.size = item["size"];
                    info.function = item["function"];
                    info.file = item["file"];
                    info.line = item["line"];
                    info.thread_id = item["thread_id"];
                    info.stack_trace = item["stack_trace"].get<std::vector<std::string>>();

                    allocations_.push_back(info);

                    // 更新索引
                    function_index_[info.function].push_back(allocations_.size() - 1);
                    file_index_[info.file].push_back(allocations_.size() - 1);
                    time_index_.push_back({info.timestamp, allocations_.size() - 1});
                }
            }

            LOG_INFO("Imported allocations from {}", filepath);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to import from JSON: {}", e.what());
            return false;
        }
    }

    json GetAllocationTimeline(size_t bucket_size_ns) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (allocations_.empty()) {
            return json::array();
        }

        uint64_t min_time = std::min_element(allocations_.begin(), allocations_.end(),
            [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; })->timestamp;

        uint64_t max_time = std::max_element(allocations_.begin(), allocations_.end(),
            [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; })->timestamp;

        std::map<uint64_t, size_t> timeline;

        for (const auto& info : allocations_) {
            if (info.address != nullptr) {
                uint64_t bucket = ((info.timestamp - min_time) / bucket_size_ns) * bucket_size_ns + min_time;
                timeline[bucket] += info.size;
            }
        }

        json result = json::array();
        for (const auto& [time, size] : timeline) {
            result.push_back({
                {"timestamp", time},
                {"memory_usage", size}
            });
        }

        return result;
    }

    void SetMaxAllocations(size_t max_allocations) {
        max_allocations_ = max_allocations;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.clear();
        function_index_.clear();
        file_index_.clear();
        time_index_.clear();
    }

private:
    void CalculatePeakUsage(QueryResult& result) {
        size_t peak = 0;
        for (const auto& info : result.allocations) {
            if (info.size > peak) {
                peak = info.size;
            }
        }
        result.peak_usage = peak;
    }

    bool SaveToFile() {
        std::string filepath = data_dir_ + "/allocations.json";
        return ExportToJson(filepath);
    }

    std::string data_dir_;
    std::vector<capture::AllocationInfo> allocations_;
    std::unordered_map<std::string, std::vector<size_t>> function_index_;
    std::unordered_map<std::string, std::vector<size_t>> file_index_;
    std::vector<std::pair<uint64_t, size_t>> time_index_;
    size_t max_allocations_;
    mutable std::mutex mutex_;
};

Storage::Storage() : pimpl_(std::make_unique<Impl>()) {}
Storage::~Storage() = default;

Storage& Storage::GetInstance() {
    static Storage instance;
    return instance;
}

void Storage::Initialize(const std::string& data_dir) { pimpl_->Initialize(data_dir); }
void Storage::Shutdown() { pimpl_->Shutdown(); }
void Storage::AddAllocation(const capture::AllocationInfo& info) { pimpl_->AddAllocation(info); }
void Storage::AddAllocations(const std::vector<capture::AllocationInfo>& allocations) { pimpl_->AddAllocations(allocations); }
QueryResult Storage::QueryByFunction(const std::string& function_name) { return pimpl_->QueryByFunction(function_name); }
QueryResult Storage::QueryByFile(const std::string& file_path) { return pimpl_->QueryByFile(file_path); }
QueryResult Storage::QueryBySizeRange(size_t min_size, size_t max_size) { return pimpl_->QueryBySizeRange(min_size, max_size); }
QueryResult Storage::QueryByTimeRange(uint64_t start_time, uint64_t end_time) { return pimpl_->QueryByTimeRange(start_time, end_time); }
std::vector<capture::AllocationInfo> Storage::GetLeaks() { return pimpl_->GetLeaks(); }
json Storage::GetSummary() { return pimpl_->GetSummary(); }
bool Storage::ExportToJson(const std::string& filepath) { return pimpl_->ExportToJson(filepath); }
bool Storage::ImportFromJson(const std::string& filepath) { return pimpl_->ImportFromJson(filepath); }
json Storage::GetAllocationTimeline(size_t bucket_size_ns) { return pimpl_->GetAllocationTimeline(bucket_size_ns); }
void Storage::SetMaxAllocations(size_t max_allocations) { pimpl_->SetMaxAllocations(max_allocations); }
void Storage::Clear() { pimpl_->Clear(); }

} // namespace storage
} // namespace memory_tracer
