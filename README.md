# Memory Tracer - C++ 内存追踪工具

一个功能强大的 C++ 程序内存申请追踪工具，可以详细记录和可视化内存分配情况。

## 功能特性

- **内存捕获**：捕获所有内存申请操作（malloc/new/delete/free）
- **详细信息记录**：时间戳、分配大小、内存地址、调用栈、线程ID
- **统计分析**：按函数/文件进行内存申请统计和汇总
- **可视化展示**：以 ASCII 图表方式直观展示内存分配情况
- **多线程支持**：支持多线程环境的内存追踪
- **实时监控**：支持实时内存使用监控

## 模块架构

```
modules/
├── logger/          # 日志模块 - 提供基础日志服务
├── capture/         # 捕获模块 - 自动捕获内存申请操作
├── storage/         # 存储模块 - 存储和管理内存申请信息
├── stats/           # 统计模块 - 统计和分析内存申请数据
└── visualization/   # 可视化模块 - 图表展示统计信息
```

### 1. logger 模块
提供基础日志服务，支持多线程，记录线程号和时间戳。

### 2. capture 模块
通过 hook 系统的 malloc/free 函数，自动捕获用户程序中的所有内存申请动作，记录调用栈信息。

### 3. storage 模块
存储和管理内存申请信息，提供高效的查询接口（按函数、文件、大小、时间范围查询）。

### 4. stats 模块
统计和分析内存申请数据，按函数/对象汇总统计信息，生成详细报告。

### 5. visualization 模块
将统计信息以 ASCII 图表方式直观展示（柱状图、直方图、时间线图等）。

## 快速开始

### 1. 安装依赖
```bash
# 安装 conan (如果尚未安装)
pip install conan

# 检测并创建 Conan profile
conan profile detect

# 安装项目依赖
./scripts/setup.sh
# 或手动执行
conan install . --build=missing
```

### 2. 构建
```bash
bazel build //...
```

### 3. 运行测试程序
```bash
bazel run //examples/test_program:test_program
```

### 4. 查看输出
测试程序运行后会生成：
- `memory_tracer.log` - 日志文件
- `memory_report.json` - JSON 格式的详细报告
- 终端输出各种可视化图表

## 使用示例

### 在你的代码中集成

```cpp
#include "capture/capture.h"
#include "logger/logger.h"
#include "storage/storage.h"
#include "stats/stats.h"
#include "visualization/visualization.h"

int main() {
    // 1. 初始化所有模块
    memory_tracer::logger::Logger::GetInstance().SetLogFile("my_app_mem.log");
    memory_tracer::storage::Storage::GetInstance().Initialize("./data");
    memory_tracer::stats::Stats::GetInstance().Initialize();
    memory_tracer::visualization::Visualization::GetInstance().Initialize();

    // 2. 初始化捕获模块（这会 hook malloc/free）
    MT_CAPTURE_INIT();
    MT_CAPTURE_START();

    // 3. 你的程序代码
    // ... 你的代码 ...
    int* ptr = new int(42);
    delete ptr;

    // 4. 停止捕获并生成报告
    MT_CAPTURE_STOP();

    // 5. 处理捕获的数据
    const auto& allocations = memory_tracer::capture::Capture::GetInstance().GetAllocations();
    memory_tracer::storage::Storage::GetInstance().AddAllocations(allocations);
    memory_tracer::stats::Stats::GetInstance().AddAllocations(allocations);

    // 6. 显示可视化图表
    memory_tracer::visualization::Visualization::GetInstance().DrawFunctionAllocationChart(10);
    memory_tracer::visualization::Visualization::GetInstance().DrawSizeDistributionHistogram();

    // 7. 导出报告
    memory_tracer::storage::Storage::GetInstance().ExportToJson("my_report.json");

    // 8. 清理
    MT_CAPTURE_SHUTDOWN();
    memory_tracer::visualization::Visualization::GetInstance().Shutdown();
    memory_tracer::stats::Stats::GetInstance().Shutdown();
    memory_tracer::storage::Storage::GetInstance().Shutdown();

    return 0;
}
```

### 使用便捷宏

```cpp
#define MT_CAPTURE_INIT()    memory_tracer::capture::Capture::GetInstance().Initialize()
#define MT_CAPTURE_START()    memory_tracer::capture::Capture::GetInstance().StartCapture()
#define MT_CAPTURE_STOP()     memory_tracer::capture::Capture::GetInstance().StopCapture()
#define MT_CAPTURE_SHUTDOWN() memory_tracer::capture::Capture::GetInstance().Shutdown()
```

## 构建产物

每个模块会生成独立的共享库 (.so) 和头文件：
- `liblogger.so` - 日志模块
- `libcapture.so` - 捕获模块
- `libstorage.so` - 存储模块
- `libstats.so` - 统计模块
- `libvisualization.so` - 可视化模块

## 技术栈

- **构建工具**: Bazel
- **包管理**: Conan
- **日志**: spdlog
- **调用栈捕获**: backward-cpp
- **数据存储**: nlohmann/json
- **C++ 标准**: C++17

## 项目结构

```
CppPipeline/
├── WORKSPACE              # Bazel 工作空间配置
├── BUILD                  # 顶层 Bazel 构建文件
├── conanfile.txt          # Conan 依赖配置
├── conanfile.py           # Conan 配方文件
├── .bazelrc               # Bazel 配置
├── scripts/
│   └── setup.sh           # 初始化脚本
├── modules/
│   ├── logger/
│   ├── capture/
│   ├── storage/
│   ├── stats/
│   └── visualization/
└── examples/
    └── test_program/      # 示例程序
```

## 许可证

详见 LICENSE 文件
