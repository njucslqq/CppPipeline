#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>

#include "capture/capture.h"
#include "logger/logger.h"
#include "storage/storage.h"
#include "stats/stats.h"
#include "visualization/visualization.h"

// 测试函数
void TestFunction1() {
    int* data = new int[100];
    std::vector<int> vec(1000);
    char* buffer = new char[1024];

    // 模拟一些操作
    for (int i = 0; i < 100; ++i) {
        data[i] = i;
    }

    delete[] data;
    delete[] buffer;
}

void TestFunction2() {
    std::vector<double> vec(5000);
    std::string str = "This is a test string with more data";
    char* large_buffer = new char[4096];

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    delete[] large_buffer;
}

void TestFunction3() {
    int* ptr = new int;
    *ptr = 42;

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    delete ptr;
}

void TestFunction4() {
    // 分配一些小内存
    for (int i = 0; i < 100; ++i) {
        int* p = new int;
        delete p;
    }

    // 分配一些中等大小的内存
    for (int i = 0; i < 10; ++i) {
        char* buf = new char[256];
        delete[] buf;
    }
}

void TestFunction5() {
    // 模拟内存泄漏
    int* leak = new int[50];
    (void)leak;  // 避免编译警告

    double* d = new double[100];
    delete[] d;
}

void WorkerThread(int id) {
    for (int i = 0; i < 5; ++i) {
        int* data = new int[100 + id * 10];
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        delete[] data;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Memory Tracer Test Program ===" << std::endl;
    std::cout << "This program demonstrates memory tracking capabilities." << std::endl;
    std::cout << std::endl;

    // 初始化所有模块
    LOG_INFO("Initializing Memory Tracer...");

    memory_tracer::logger::Logger::GetInstance().SetLogFile("memory_tracer.log");
    memory_tracer::logger::Logger::GetInstance().SetLogLevel(memory_tracer::logger::LogLevel::INFO);

    memory_tracer::storage::Storage::GetInstance().Initialize("./data");
    memory_tracer::stats::Stats::GetInstance().Initialize();
    memory_tracer::visualization::Visualization::GetInstance().Initialize();

    // 初始化捕获模块（这会 hook malloc/free）
    MT_CAPTURE_INIT();
    MT_CAPTURE_START();

    std::cout << "Starting memory capture..." << std::endl;

    // 模拟一些内存分配
    std::cout << "\nRunning test functions..." << std::endl;

    for (int i = 0; i < 3; ++i) {
        TestFunction1();
        TestFunction2();
        TestFunction3();
        TestFunction4();
        TestFunction5();
    }

    // 多线程测试
    std::cout << "Running multi-threaded test..." << std::endl;
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(WorkerThread, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    // 停止捕获
    std::cout << "\nStopping memory capture..." << std::endl;
    MT_CAPTURE_STOP();

    // 获取捕获的数据并添加到存储和统计模块
    std::cout << "Processing captured data..." << std::endl;
    const auto& allocations = memory_tracer::capture::Capture::GetInstance().GetAllocations();

    memory_tracer::storage::Storage::GetInstance().AddAllocations(allocations);
    memory_tracer::stats::Stats::GetInstance().AddAllocations(allocations);

    // 生成可视化图表
    std::cout << "\n=== Memory Statistics ===" << std::endl;

    // 1. 函数分配柱状图
    memory_tracer::visualization::Visualization::GetInstance().DrawFunctionAllocationChart(10);

    // 2. 大小分布直方图
    memory_tracer::visualization::Visualization::GetInstance().DrawSizeDistributionHistogram();

    // 3. 内存热点图表
    memory_tracer::visualization::Visualization::GetInstance().DrawMemoryHotspotsChart(10);

    // 4. 文件级别分配图
    memory_tracer::visualization::Visualization::GetInstance().DrawFileAllocationChart(10);

    // 5. 内存使用时间线
    memory_tracer::visualization::Visualization::GetInstance().DrawMemoryTimeline();

    // 导出 JSON 报告
    std::cout << "Exporting JSON report..." << std::endl;
    memory_tracer::storage::Storage::GetInstance().ExportToJson("memory_report.json");

    // 生成文本报告
    std::cout << "\n=== Detailed Report ===" << std::endl;
    std::cout << memory_tracer::visualization::Visualization::GetInstance().ExportReportToText() << std::endl;

    // 检查内存泄漏
    auto leaks = memory_tracer::storage::Storage::GetInstance().GetLeaks();
    std::cout << "\n=== Potential Memory Leaks ===" << std::endl;
    std::cout << "Found " << leaks.size() << " potential memory leaks." << std::endl;

    if (!leaks.empty()) {
        for (size_t i = 0; i < std::min(size_t(5), leaks.size()); ++i) {
            const auto& leak = leaks[i];
            std::cout << "  " << (i + 1) << ". " << leak.function << " @ " << leak.file
                      << ":" << leak.line << " (" << leak.size << " bytes)" << std::endl;
        }
    }

    // 清理
    MT_CAPTURE_SHUTDOWN();
    memory_tracer::visualization::Visualization::GetInstance().Shutdown();
    memory_tracer::stats::Stats::GetInstance().Shutdown();
    memory_tracer::storage::Storage::GetInstance().Shutdown();

    LOG_INFO("Memory Tracer test completed.");

    std::cout << "\n=== Test Completed ===" << std::endl;
    std::cout << "Check memory_tracer.log and memory_report.json for detailed information." << std::endl;

    return 0;
}
