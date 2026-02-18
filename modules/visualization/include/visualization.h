#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "stats/stats.h"
#include "storage/storage.h"

namespace memory_tracer {
namespace visualization {

enum class ChartType {
    BAR,
    HISTOGRAM,
    TIMELINE,
    PIE
};

class Visualization {
public:
    static Visualization& GetInstance();

    // 初始化可视化模块
    void Initialize();

    // 关闭可视化模块
    void Shutdown();

    // 绘制函数内存分配柱状图
    void DrawFunctionAllocationChart(int limit = 10);

    // 绘制大小分布直方图
    void DrawSizeDistributionHistogram();

    // 绘制内存使用时间线
    void DrawMemoryTimeline(size_t bucket_size_ns = 1000000000);

    // 绘制内存热点图表
    void DrawMemoryHotspotsChart(int limit = 10);

    // 绘制调用栈频率图
    void DrawCallStackFrequencyChart(int limit = 10);

    // 绘制文件级别的分配图
    void DrawFileAllocationChart(int limit = 10);

    // 实时监控模式
    void StartRealtimeMonitor(int refresh_interval_ms = 1000);
    void StopRealtimeMonitor();

    // 将图表导出为文本
    std::string ExportFunctionChartToText(int limit = 10);

    std::string ExportSizeDistributionToText();

    std::string ExportTimelineToText(size_t bucket_size_ns = 1000000000);

    std::string ExportReportToText();

    // 设置输出流
    void SetOutputStream(std::ostream& stream);

private:
    Visualization();
    ~Visualization();
    Visualization(const Visualization&) = delete;
    Visualization& operator=(const Visualization&) = delete;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace visualization
} // namespace memory_tracer
