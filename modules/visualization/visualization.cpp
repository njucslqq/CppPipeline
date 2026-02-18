#include "visualization/visualization.h"
#include "logger/logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <atomic>
#include <cmath>

namespace memory_tracer {
namespace visualization {

class Visualization::Impl {
public:
    Impl() : output_stream_(&std::cout), realtime_running_(false) {}

    void Initialize() {
        LOG_INFO("Visualization module initialized");
    }

    void Shutdown() {
        StopRealtimeMonitor();
        LOG_INFO("Visualization module shutdown");
    }

    void DrawFunctionAllocationChart(int limit) {
        auto func_stats = stats::Stats::GetInstance().GetFunctionStats(limit);

        if (func_stats.empty()) {
            *output_stream_ << "No allocation data available.\n";
            return;
        }

        size_t max_size = func_stats[0].total_allocated;

        *output_stream_ << "\n========================================\n";
        *output_stream_ << "  Function Memory Allocation Chart\n";
        *output_stream_ << "========================================\n\n";

        for (const auto& stats : func_stats) {
            double ratio = static_cast<double>(stats.total_allocated) / max_size;
            int bar_length = static_cast<int>(ratio * 50);

            *output_stream_ << std::left << std::setw(25) << stats.function_name.substr(0, 24);
            *output_stream_ << " |";

            // 绘制条形图
            for (int i = 0; i < bar_length; ++i) {
                *output_stream_ << "█";
            }
            for (int i = bar_length; i < 50; ++i) {
                *output_stream_ << " ";
            }

            *output_stream_ << "| " << FormatSize(stats.total_allocated) << "\n";
        }

        *output_stream_ << "\n";
    }

    void DrawSizeDistributionHistogram() {
        auto buckets = stats::Stats::GetInstance().GetSizeDistributionStats();

        if (buckets.empty()) {
            *output_stream_ << "No size distribution data available.\n";
            return;
        }

        size_t max_count = 0;
        for (const auto& bucket : buckets) {
            if (bucket.count > max_count) {
                max_count = bucket.count;
            }
        }

        *output_stream_ << "\n========================================\n";
        *output_stream_ << "  Size Distribution Histogram\n";
        *output_stream_ << "========================================\n\n";

        for (const auto& bucket : buckets) {
            double ratio = static_cast<double>(bucket.count) / max_count;
            int bar_length = static_cast<int>(ratio * 40);

            std::string label = FormatSize(bucket.min_size) + "-" +
                              (bucket.max_size == SIZE_MAX ? "inf" : FormatSize(bucket.max_size));
            *output_stream_ << std::left << std::setw(20) << label;
            *output_stream_ << " |";

            for (int i = 0; i < bar_length; ++i) {
                *output_stream_ << "█";
            }
            for (int i = bar_length; i < 40; ++i) {
                *output_stream_ << " ";
            }

            *output_stream_ << "| " << bucket.count << " allocs\n";
        }

        *output_stream_ << "\n";
    }

    void DrawMemoryTimeline(size_t bucket_size_ns) {
        auto timeline = storage::Storage::GetInstance().GetAllocationTimeline(bucket_size_ns);

        if (timeline.empty()) {
            *output_stream_ << "No timeline data available.\n";
            return;
        }

        // 找出最大值用于缩放
        uint64_t max_usage = 0;
        for (const auto& point : timeline) {
            if (point["memory_usage"] > max_usage) {
                max_usage = point["memory_usage"];
            }
        }

        *output_stream_ << "\n========================================\n";
        *output_stream_ << "  Memory Usage Timeline\n";
        *output_stream_ << "========================================\n\n";

        for (const auto& point : timeline) {
            uint64_t timestamp = point["timestamp"];
            uint64_t usage = point["memory_usage"];
            double ratio = static_cast<double>(usage) / max_usage;
            int bar_length = static_cast<int>(ratio * 40);

            *output_stream_ << std::setw(12) << FormatTimestamp(timestamp) << " |";

            for (int i = 0; i < bar_length; ++i) {
                *output_stream_ << "█";
            }
            for (int i = bar_length; i < 40; ++i) {
                *output_stream_ << " ";
            }

            *output_stream_ << "| " << FormatSize(usage) << "\n";
        }

        *output_stream_ << "\nPeak usage: " << FormatSize(max_usage) << "\n\n";
    }

    void DrawMemoryHotspotsChart(int limit) {
        auto hotspots = stats::Stats::GetInstance().GetMemoryHotspots(limit);

        if (hotspots.empty()) {
            *output_stream_ << "No hotspot data available.\n";
            return;
        }

        size_t max_size = hotspots[0].second;

        *output_stream_ << "\n========================================\n";
        *output_stream_ << "  Memory Hotspots\n";
        *output_stream_ << "========================================\n\n";

        for (size_t i = 0; i < hotspots.size(); ++i) {
            const auto& [func, size] = hotspots[i];
            double ratio = static_cast<double>(size) / max_size;
            int bar_length = static_cast<int>(ratio * 45);

            *output_stream_ << std::setw(2) << (i + 1) << ". ";
            *output_stream_ << std::left << std::setw(22) << func.substr(0, 21);
            *output_stream_ << " |";

            for (int j = 0; j < bar_length; ++j) {
                *output_stream_ << "█";
            }
            for (int j = bar_length; j < 45; ++j) {
                *output_stream_ << " ";
            }

            *output_stream_ << "| " << FormatSize(size) << "\n";
        }

        *output_stream_ << "\n";
    }

    void DrawCallStackFrequencyChart(int limit) {
        auto call_stack_stats = stats::Stats::GetInstance().GetCallStackStats();

        if (call_stack_stats.empty()) {
            *output_stream_ << "No call stack data available.\n";
            return;
        }

        // 转换为 vector 并排序
        std::vector<std::pair<std::string, size_t>> stacks;
        for (const auto& [stack, count] : call_stack_stats) {
            stacks.push_back({stack, count});
        }
        std::sort(stacks.begin(), stacks.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        if (limit > 0 && stacks.size() > static_cast<size_t>(limit)) {
            stacks.resize(limit);
        }

        size_t max_count = stacks[0].second;

        *output_stream_ << "\n========================================\n";
        *output_stream_ << "  Top Call Stacks by Frequency\n";
        *output_stream_ << "========================================\n\n";

        for (size_t i = 0; i < stacks.size(); ++i) {
            const auto& [stack, count] = stacks[i];
            double ratio = static_cast<double>(count) / max_count;
            int bar_length = static_cast<int>(ratio * 30);

            *output_stream_ << std::setw(3) << (i + 1) << ". ";

            // 简化的栈显示
            std::string simplified = SimplifyStack(stack);
            *output_stream_ << std::left << std::setw(30) << simplified;
            *output_stream_ << " |";

            for (int j = 0; j < bar_length; ++j) {
                *output_stream_ << "█";
            }
            for (int j = bar_length; j < 30; ++j) {
                *output_stream_ << " ";
            }

            *output_stream_ << "| " << count << "\n";
        }

        *output_stream_ << "\n";
    }

    void DrawFileAllocationChart(int limit) {
        auto file_stats = stats::Stats::GetInstance().GetFileStats(limit);

        if (file_stats.empty()) {
            *output_stream_ << "No file allocation data available.\n";
            return;
        }

        size_t max_size = file_stats[0].total_allocated;

        *output_stream_ << "\n========================================\n";
        *output_stream_ << "  File Memory Allocation Chart\n";
        *output_stream_ << "========================================\n\n";

        for (const auto& stats : file_stats) {
            // 只显示文件名，不显示路径
            std::string filename = ExtractFilename(stats.file_path);
            double ratio = static_cast<double>(stats.total_allocated) / max_size;
            int bar_length = static_cast<int>(ratio * 40);

            *output_stream_ << std::left << std::setw(28) << filename.substr(0, 27);
            *output_stream_ << " |";

            for (int i = 0; i < bar_length; ++i) {
                *output_stream_ << "█";
            }
            for (int i = bar_length; i < 40; ++i) {
                *output_stream_ << " ";
            }

            *output_stream_ << "| " << FormatSize(stats.total_allocated) << "\n";
        }

        *output_stream_ << "\n";
    }

    void StartRealtimeMonitor(int refresh_interval_ms) {
        if (realtime_running_) {
            return;
        }

        realtime_running_ = true;
        realtime_thread_ = std::thread([this, refresh_interval_ms]() {
            while (realtime_running_) {
                // 清屏
                *output_stream_ << "\033[2J\033[H";

                // 绘制实时图表
                DrawRealtimeDashboard();

                std::this_thread::sleep_for(std::chrono::milliseconds(refresh_interval_ms));
            }
        });

        LOG_INFO("Realtime monitor started");
    }

    void StopRealtimeMonitor() {
        if (realtime_running_) {
            realtime_running_ = false;
            if (realtime_thread_.joinable()) {
                realtime_thread_.join();
            }
            LOG_INFO("Realtime monitor stopped");
        }
    }

    std::string ExportFunctionChartToText(int limit) {
        std::ostringstream oss;
        std::streambuf* old = output_stream_->rdbuf(oss.rdbuf());
        DrawFunctionAllocationChart(limit);
        output_stream_->rdbuf(old);
        return oss.str();
    }

    std::string ExportSizeDistributionToText() {
        std::ostringstream oss;
        std::streambuf* old = output_stream_->rdbuf(oss.rdbuf());
        DrawSizeDistributionHistogram();
        output_stream_->rdbuf(old);
        return oss.str();
    }

    std::string ExportTimelineToText(size_t bucket_size_ns) {
        std::ostringstream oss;
        std::streambuf* old = output_stream_->rdbuf(oss.rdbuf());
        DrawMemoryTimeline(bucket_size_ns);
        output_stream_->rdbuf(old);
        return oss.str();
    }

    std::string ExportReportToText() {
        return stats::Stats::GetInstance().GenerateReport();
    }

    void SetOutputStream(std::ostream& stream) {
        output_stream_ = &stream;
    }

private:
    void DrawRealtimeDashboard() {
        auto summary = stats::Stats::GetInstance().GetSummary();

        *output_stream_ << "\n========================================\n";
        *output_stream_ << "  Realtime Memory Monitor\n";
        *output_stream_ << "========================================\n\n";
        *output_stream_ << summary << "\n";

        DrawMemoryHotspotsChart(5);
        DrawSizeDistributionHistogram();
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

    std::string FormatTimestamp(uint64_t ns) {
        double seconds = ns / 1e9;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << seconds << "s";
        return oss.str();
    }

    std::string SimplifyStack(const std::string& stack) {
        // 只保留最后一层调用
        size_t pos = stack.rfind(" <- ");
        if (pos == std::string::npos) {
            return stack;
        }
        return stack.substr(pos + 4);
    }

    std::string ExtractFilename(const std::string& filepath) {
        size_t pos = filepath.rfind('/');
        if (pos == std::string::npos) {
            pos = filepath.rfind('\\');
        }
        if (pos == std::string::npos) {
            return filepath;
        }
        return filepath.substr(pos + 1);
    }

    std::ostream* output_stream_;
    std::atomic<bool> realtime_running_;
    std::thread realtime_thread_;
};

Visualization::Visualization() : pimpl_(std::make_unique<Impl>()) {}
Visualization::~Visualization() = default;

Visualization& Visualization::GetInstance() {
    static Visualization instance;
    return instance;
}

void Visualization::Initialize() { pimpl_->Initialize(); }
void Visualization::Shutdown() { pimpl_->Shutdown(); }
void Visualization::DrawFunctionAllocationChart(int limit) { pimpl_->DrawFunctionAllocationChart(limit); }
void Visualization::DrawSizeDistributionHistogram() { pimpl_->DrawSizeDistributionHistogram(); }
void Visualization::DrawMemoryTimeline(size_t bucket_size_ns) { pimpl_->DrawMemoryTimeline(bucket_size_ns); }
void Visualization::DrawMemoryHotspotsChart(int limit) { pimpl_->DrawMemoryHotspotsChart(limit); }
void Visualization::DrawCallStackFrequencyChart(int limit) { pimpl_->DrawCallStackFrequencyChart(limit); }
void Visualization::DrawFileAllocationChart(int limit) { pimpl_->DrawFileAllocationChart(limit); }
void Visualization::StartRealtimeMonitor(int refresh_interval_ms) { pimpl_->StartRealtimeMonitor(refresh_interval_ms); }
void Visualization::StopRealtimeMonitor() { pimpl_->StopRealtimeMonitor(); }
std::string Visualization::ExportFunctionChartToText(int limit) { return pimpl_->ExportFunctionChartToText(limit); }
std::string Visualization::ExportSizeDistributionToText() { return pimpl_->ExportSizeDistributionToText(); }
std::string Visualization::ExportTimelineToText(size_t bucket_size_ns) { return pimpl_->ExportTimelineToText(bucket_size_ns); }
std::string Visualization::ExportReportToText() { return pimpl_->ExportReportToText(); }
void Visualization::SetOutputStream(std::ostream& stream) { pimpl_->SetOutputStream(stream); }

} // namespace visualization
} // namespace memory_tracer
