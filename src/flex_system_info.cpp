#include "flex_system_info.h"
#include <sys/sysinfo.h>
#include <pwd.h>
#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <cstring>

namespace FlexTools {

FlexSystemInfo::FlexSystemInfo() {
    if (uname(&flexUnameData) != 0) {
        throw std::runtime_error("Failed to get system information");
    }
    flexCacheValid = false;
}

std::map<std::string, std::string> FlexSystemInfo::getBasicInfo() const {
    flexEnsureCache();
    return flexBasicInfoCache;
}

void FlexSystemInfo::flexEnsureCache() const {
    if (flexCacheValid) {
        return;
    }
    
    // 填充基本信息缓存
    flexBasicInfoCache["System Name"] = flexUnameData.sysname;
    flexBasicInfoCache["Node Name"] = flexUnameData.nodename;
    flexBasicInfoCache["Kernel Release"] = flexUnameData.release;
    flexBasicInfoCache["Kernel Version"] = flexUnameData.version;
    flexBasicInfoCache["Machine"] = flexUnameData.machine;
    flexBasicInfoCache["Operating System"] = getOSName();
    flexBasicInfoCache["Hostname"] = getHostname();
    flexBasicInfoCache["Uptime"] = getUptime();
    flexBasicInfoCache["Distribution"] = getDistribution();
    
    // 填充详细信息缓存
    flexDetailedInfoCache = flexBasicInfoCache;
    flexDetailedInfoCache["Domain Name"] = flexUnameData.__domainname;
    
    // 处理器信息
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        int processorCount = 0;
        std::string cpuModel;
        while (std::getline(cpuinfo, line)) {
            if (line.find("processor") != std::string::npos) {
                processorCount++;
            }
            if (line.find("model name") != std::string::npos && cpuModel.empty()) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    cpuModel = line.substr(colonPos + 2);
                }
            }
        }
        flexDetailedInfoCache["Processor Count"] = std::to_string(processorCount);
        if (!cpuModel.empty()) {
            flexDetailedInfoCache["CPU Model"] = cpuModel;
        }
        cpuinfo.close();
    }
    
    // 内存信息
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal") != std::string::npos) {
                flexDetailedInfoCache["Total Memory"] = line.substr(line.find(":") + 2);
                break;
            }
        }
        meminfo.close();
    }
    
    // 系统架构
    flexDetailedInfoCache["Architecture"] = flexUnameData.machine;
    
    flexCacheValid = true;
}

std::string FlexSystemInfo::getUptime() const {
    std::ifstream uptimeFile("/proc/uptime");
    if (!uptimeFile.is_open()) {
        return "Unknown";
    }
    
    double uptime_seconds;
    uptimeFile >> uptime_seconds;
    uptimeFile.close();
    
    int days = static_cast<int>(uptime_seconds) / 86400;
    int hours = (static_cast<int>(uptime_seconds) % 86400) / 3600;
    int minutes = (static_cast<int>(uptime_seconds) % 3600) / 60;
    int seconds = static_cast<int>(uptime_seconds) % 60;
    
    std::ostringstream oss;
    if (days > 0) oss << days << " days, ";
    if (hours > 0) oss << hours << " hours, ";
    oss << minutes << " minutes, " << seconds << " seconds";
    
    return oss.str();
}

std::vector<std::string> FlexSystemInfo::getLoadAverage() const {
    std::vector<std::string> loadAvg;
    std::ifstream loadavgFile("/proc/loadavg");
    
    if (loadavgFile.is_open()) {
        std::string line;
        if (std::getline(loadavgFile, line)) {
            std::istringstream iss(line);
            std::string load;
            for (int i = 0; i < 3; ++i) {
                iss >> load;
                loadAvg.push_back(load);
            }
        }
        loadavgFile.close();
    }
    
    return loadAvg;
}

std::vector<std::string> FlexSystemInfo::getLoggedUsers() const {
    std::vector<std::string> users;
    std::array<char, 128> buffer;
    std::string result;
    
    // 使用 who 命令获取登录用户
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("who", "r"), pclose);
    if (!pipe) {
        return users;
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result = buffer.data();
        size_t spacePos = result.find(' ');
        if (spacePos != std::string::npos) {
            std::string username = result.substr(0, spacePos);
            if (std::find(users.begin(), users.end(), username) == users.end()) {
                users.push_back(username);
            }
        }
    }
    
    return users;
}

std::string FlexSystemInfo::getOSName() const {
    // 尝试读取 /etc/os-release
    std::ifstream osRelease("/etc/os-release");
    if (osRelease.is_open()) {
        std::string line;
        while (std::getline(osRelease, line)) {
            if (line.find("PRETTY_NAME") != std::string::npos) {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos) {
                    std::string name = line.substr(eqPos + 1);
                    name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
                    return name;
                }
            }
        }
        osRelease.close();
    }
    
    // 尝试读取 /etc/lsb-release
    std::ifstream lsbRelease("/etc/lsb-release");
    if (lsbRelease.is_open()) {
        std::string line;
        while (std::getline(lsbRelease, line)) {
            if (line.find("DISTRIB_DESCRIPTION") != std::string::npos) {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos) {
                    std::string name = line.substr(eqPos + 1);
                    name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
                    return name;
                }
            }
        }
        lsbRelease.close();
    }
    
    return "Unknown";
}

std::string FlexSystemInfo::getDistribution() const {
    std::ifstream osRelease("/etc/os-release");
    if (osRelease.is_open()) {
        std::string line;
        while (std::getline(osRelease, line)) {
            if (line.find("ID=") == 0) {
                std::string id = line.substr(3);
                id.erase(std::remove(id.begin(), id.end(), '\"'), id.end());
                return id;
            }
        }
        osRelease.close();
    }
    return "unknown";
}

std::string FlexSystemInfo::getHostname() const {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Unknown";
}

void FlexSystemInfo::printInfo(FlexInfoLevel level, FlexOutputFormat format) const {
    auto info = (level == FlexInfoLevel::BASIC) ? getBasicInfo() : getDetailedInfo();
    
    if (format == FlexOutputFormat::TEXT) {
        std::cout << FLEX_COLOR_CYAN << "\n=== FlexTools System Information ===" << FLEX_COLOR_RESET << std::endl;
        for (const auto& [key, value] : info) {
            std::cout << FLEX_COLOR_YELLOW << std::setw(25) << std::left << key 
                      << FLEX_COLOR_RESET << ": " << value << std::endl;
        }
        
        // 显示负载均衡
        auto loadAvg = getLoadAverage();
        if (!loadAvg.empty()) {
            std::cout << FLEX_COLOR_YELLOW << std::setw(25) << std::left << "Load Average (1,5,15 min)"
                      << FLEX_COLOR_RESET << ": " << loadAvg[0] << ", " 
                      << loadAvg[1] << ", " << loadAvg[2] << std::endl;
        }
        
        // 显示登录用户
        auto users = getLoggedUsers();
        if (!users.empty()) {
            std::cout << FLEX_COLOR_YELLOW << std::setw(25) << std::left << "Logged Users"
                      << FLEX_COLOR_RESET << ": ";
            for (size_t i = 0; i < users.size(); ++i) {
                std::cout << users[i];
                if (i < users.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
    } else if (format == FlexOutputFormat::JSON) {
        std::cout << toJSON(level) << std::endl;
    }
}

std::string FlexSystemInfo::toJSON(FlexInfoLevel level) const {
    auto info = (level == FlexInfoLevel::BASIC) ? getBasicInfo() : getDetailedInfo();
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"flex_system_info\": {\n";
    
    bool first = true;
    for (const auto& [key, value] : info) {
        if (!first) oss << ",\n";
        first = false;
        
        // 替换键中的空格为下划线
        std::string jsonKey = key;
        std::replace(jsonKey.begin(), jsonKey.end(), ' ', '_');
        std::transform(jsonKey.begin(), jsonKey.end(), jsonKey.begin(), ::tolower);
        
        oss << "    \"" << jsonKey << "\": \"" << value << "\"";
    }
    
    // 添加负载均衡信息
    auto loadAvg = getLoadAverage();
    if (!loadAvg.empty()) {
        oss << ",\n";
        oss << "    \"load_average\": {\n";
        oss << "      \"1_min\": " << loadAvg[0] << ",\n";
        oss << "      \"5_min\": " << loadAvg[1] << ",\n";
        oss << "      \"15_min\": " << loadAvg[2] << "\n";
        oss << "    }";
    }
    
    oss << "\n  }\n";
    oss << "}";
    
    return oss.str();
}

std::string FlexSystemInfo::toCSV(FlexInfoLevel level) const {
    auto info = (level == FlexInfoLevel::BASIC) ? getBasicInfo() : getDetailedInfo();
    std::ostringstream oss;
    
    // 标题行
    oss << "Key,Value\n";
    
    for (const auto& [key, value] : info) {
        // 处理CSV特殊字符
        std::string escapedValue = value;
        if (value.find(',') != std::string::npos || 
            value.find('"') != std::string::npos ||
            value.find('\n') != std::string::npos) {
            // 转义双引号
            size_t pos = 0;
            while ((pos = escapedValue.find('"', pos)) != std::string::npos) {
                escapedValue.replace(pos, 1, "\"\"");
                pos += 2;
            }
            escapedValue = "\"" + escapedValue + "\"";
        }
        
        oss << key << "," << escapedValue << "\n";
    }
    
    return oss.str();
}

} // namespace FlexTools
