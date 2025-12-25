#ifndef FLEX_COMMON_H
#define FLEX_COMMON_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

// FlexTools 命名空间
namespace FlexTools {

// 输出颜色定义
#define FLEX_COLOR_RESET   "\033[0m"
#define FLEX_COLOR_RED     "\033[31m"
#define FLEX_COLOR_GREEN   "\033[32m"
#define FLEX_COLOR_YELLOW  "\033[33m"
#define FLEX_COLOR_BLUE    "\033[34m"
#define FLEX_COLOR_MAGENTA "\033[35m"
#define FLEX_COLOR_CYAN    "\033[36m"

// 信息级别
enum class FlexInfoLevel {
    BASIC,
    DETAILED,
    ALL
};

// 输出格式
enum class FlexOutputFormat {
    TEXT,
    JSON,
    CSV,
    XML
};

// 系统类型
enum class FlexSystemType {
    DEBIAN_BASED,
    RPM_BASED,
    UNKNOWN
};

// 日志级别
enum class FlexLogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

} // namespace FlexTools

#endif // FLEX_COMMON_H
