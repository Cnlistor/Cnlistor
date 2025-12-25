#include "flex_hardware_info.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <array>
#include <memory>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <dirent.h>
#include <unistd.h>

namespace FlexTools {

FlexHardwareInfo::FlexHardwareInfo() {
    flexCacheValid = false;
}

FlexCPUInfo FlexHardwareInfo::getCPUInfo() const {
    flexEnsureCache();
    return flexCPUInfoCache;
}

FlexMemoryInfo FlexHardwareInfo::getMemoryInfo() const {
    flexEnsureCache();
    return flexMemoryInfoCache;
}

std::vector<FlexDiskInfo> FlexHardwareInfo::getDiskInfo() const {
    flexEnsureCache();
    return flexDiskInfoCache;
}

std::vector<FlexNetworkInterface> FlexHardwareInfo::getNetworkInfo() const {
    flexEnsureCache();
    return flexNetworkInfoCache;
}

void FlexHardwareInfo::flexEnsureCache() const {
    if (flexCacheValid) {
        return;
    }
    
    // 解析CPU信息
    flexParseCPUInfo(flexCPUInfoCache);
    
    // 解析内存信息
    flexParseMemoryInfo(flexMemoryInfoCache);
    
    // 解析磁盘信息
    flexDiskInfoCache.clear();
    std::array<char, 1024> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("df -k 2>/dev/null", "r"), pclose);
    
    if (pipe) {
        bool firstLine = true;
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            std::string line = buffer.data();
            if (firstLine) {
                firstLine = false;
                continue;
            }
            
            FlexDiskInfo disk = flexParseDiskLine(line);
            if (!disk.device.empty() && disk.device != "tmpfs" && 
                disk.device != "devtmpfs" && disk.device != "udev") {
                flexDiskInfoCache.push_back(disk);
            }
        }
    }
    
    // 解析网络信息
    flexNetworkInfoCache.clear();
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == 0) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;
            
            std::string ifName = ifa->ifa_name;
            // 检查是否已存在该接口
            bool exists = false;
            for (const auto& ni : flexNetworkInfoCache) {
                if (ni.name == ifName) {
                    exists = true;
                    break;
                }
            }
            
            if (!exists) {
                FlexNetworkInterface ni = flexParseNetworkInterface(ifName);
                if (!ni.name.empty()) {
                    flexNetworkInfoCache.push_back(ni);
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    
    flexCacheValid = true;
}

void FlexHardwareInfo::flexParseCPUInfo(FlexCPUInfo& cpu) const {
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return;
    }
    
    std::string line;
    int processorCount = 0;
    int physicalCores = 0;
    bool hyperthreading = false;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("processor") != std::string::npos) {
            processorCount++;
        }
        
        if (line.find("model name") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                cpu.model = line.substr(colonPos + 2);
            }
        } else if (line.find("vendor_id") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                cpu.vendor = line.substr(colonPos + 2);
            }
        } else if (line.find("cpu cores") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                physicalCores = std::stoi(line.substr(colonPos + 2));
                cpu.cores = physicalCores;
            }
        } else if (line.find("siblings") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                cpu.threads = std::stoi(line.substr(colonPos + 2));
                hyperthreading = (cpu.threads > physicalCores);
            }
        } else if (line.find("flags") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string flagsStr = line.substr(colonPos + 2);
                std::istringstream iss(flagsStr);
                std::string flag;
                while (iss >> flag) {
                    cpu.flags.push_back(flag);
                }
            }
        } else if (line.find("cache size") != std::string::npos) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string cacheStr = line.substr(colonPos + 2);
                size_t spacePos = cacheStr.find(' ');
                if (spacePos != std::string::npos) {
                    cpu.cacheSize = std::stoi(cacheStr.substr(0, spacePos));
                }
            }
        }
    }
    cpuinfo.close();
    
    // 获取架构信息
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("uname -m", "r"), pclose);
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        cpu.architecture = result;
        cpu.architecture.erase(std::remove(cpu.architecture.begin(), 
                                         cpu.architecture.end(), '\n'), 
                             cpu.architecture.end());
    }
    
    // 获取时钟速度
    std::ifstream cpuMHz("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (cpuMHz.is_open()) {
        double mhz;
        cpuMHz >> mhz;
        cpuMHz.close();
        cpu.clockSpeed = mhz / 1000.0; // 转换为GHz
    }
}

void FlexHardwareInfo::flexParseMemoryInfo(FlexMemoryInfo& mem) const {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        long long value;
        std::string unit;
        
        iss >> key >> value >> unit;
        
        if (key == "MemTotal:") {
            mem.total = value;
        } else if (key == "MemFree:") {
            mem.free = value;
        } else if (key == "MemAvailable:") {
            mem.available = value;
        } else if (key == "Cached:") {
            mem.cached = value;
        } else if (key == "Buffers:") {
            mem.buffers = value;
        } else if (key == "SwapTotal:") {
            mem.swapTotal = value;
        } else if (key == "SwapFree:") {
            mem.swapFree = value;
        }
    }
    meminfo.close();
}

FlexDiskInfo FlexHardwareInfo::flexParseDiskLine(const std::string& line) const {
    FlexDiskInfo disk;
    std::istringstream iss(line);
    
    iss >> disk.device;
    
    // 跳过非磁盘设备
    if (disk.device.empty() || disk.device.find("://") != std::string::npos) {
        return disk;
    }
    
    long long blocks;
    iss >> blocks;
    disk.total = blocks;
    
    iss >> blocks;
    disk.used = blocks;
    
    iss >> blocks;
    disk.free = blocks;
    
    std::string percentStr;
    iss >> percentStr;
    if (!percentStr.empty() && percentStr.back() == '%') {
        disk.usagePercent = std::stoi(percentStr.substr(0, percentStr.size() - 1));
    }
    
    // 获取挂载点
    std::getline(iss, disk.mountPoint);
    disk.mountPoint.erase(0, disk.mountPoint.find_first_not_of(" \t"));
    
    // 获取文件系统类型
    std::string cmd = "df -T " + disk.device + " 2>/dev/null | tail -1 | awk '{print $2}'";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (pipe) {
        if (fgets(buffer.data(), sizeof(buffer), pipe.get()) != nullptr) {
            disk.filesystem = buffer.data();
            disk.filesystem.erase(std::remove(disk.filesystem.begin(), 
                                            disk.filesystem.end(), '\n'), 
                                disk.filesystem.end());
        }
    }
    
    // 获取inode信息
    if (!disk.mountPoint.empty()) {
        struct statvfs vfs;
        if (statvfs(disk.mountPoint.c_str(), &vfs) == 0) {
            disk.inodesTotal = vfs.f_files;
            disk.inodesFree = vfs.f_ffree;
            disk.inodesUsed = disk.inodesTotal - disk.inodesFree;
            if (disk.inodesTotal > 0) {
                disk.inodeUsagePercent = static_cast<int>((disk.inodesUsed * 100.0) / disk.inodesTotal);
            }
        }
    }
    
    return disk;
}

FlexNetworkInterface FlexHardwareInfo::flexParseNetworkInterface(const std::string& name) const {
    FlexNetworkInterface ni;
    ni.name = name;
    
    // 获取MAC地址
    std::string cmd = "cat /sys/class/net/" + name + "/address 2>/dev/null";
    std::array<char, 128> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (pipe) {
        if (fgets(buffer.data(), sizeof(buffer), pipe.get()) != nullptr) {
            ni.macAddress = buffer.data();
            ni.macAddress.erase(std::remove(ni.macAddress.begin(), 
                                          ni.macAddress.end(), '\n'), 
                              ni.macAddress.end());
        }
    }
    
    // 获取网络统计信息
    std::string statsPath = "/sys/class/net/" + name + "/statistics/";
    auto readNetworkStat = [&](const std::string& statName) -> long long {
        std::ifstream file(statsPath + statName);
        long long value = 0;
        if (file.is_open()) {
            file >> value;
            file.close();
        }
        return value;
    };
    
    ni.rxBytes = readNetworkStat("rx_bytes");
    ni.txBytes = readNetworkStat("tx_bytes");
    ni.rxPackets = readNetworkStat("rx_packets");
    ni.txPackets = readNetworkStat("tx_packets");
    ni.rxErrors = readNetworkStat("rx_errors");
    ni.txErrors = readNetworkStat("tx_errors");
    
    return ni;
}

void FlexHardwareInfo::printAllInfo(FlexOutputFormat format) const {
    if (format == FlexOutputFormat::TEXT) {
        std::cout << FLEX_COLOR_CYAN << "\n=== FlexTools Hardware Information ===" << FLEX_COLOR_RESET << std::endl;
        
        // CPU 信息
        FlexCPUInfo cpu = getCPUInfo();
        std::cout << FLEX_COLOR_GREEN << "\n[CPU Information]" << FLEX_COLOR_RESET << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Model: " << FLEX_COLOR_RESET << cpu.model << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Vendor: " << FLEX_COLOR_RESET << cpu.vendor << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Architecture: " << FLEX_COLOR_RESET << cpu.architecture << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Cores/Threads: " << FLEX_COLOR_RESET 
                  << cpu.cores << " cores, " << cpu.threads << " threads" << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Clock Speed: " << FLEX_COLOR_RESET 
                  << std::fixed << std::setprecision(2) << cpu.clockSpeed << " GHz" << std::endl;
        if (cpu.cacheSize > 0) {
            std::cout << FLEX_COLOR_YELLOW << "  Cache Size: " << FLEX_COLOR_RESET 
                      << cpu.cacheSize << " KB" << std::endl;
        }
        
        // 内存信息
        FlexMemoryInfo mem = getMemoryInfo();
        std::cout << FLEX_COLOR_GREEN << "\n[Memory Information]" << FLEX_COLOR_RESET << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Total RAM: " << FLEX_COLOR_RESET 
                  << std::fixed << std::setprecision(2) 
                  << (mem.total / 1024.0 / 1024.0) << " GB" << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Available RAM: " << FLEX_COLOR_RESET 
                  << std::fixed << std::setprecision(2) 
                  << (mem.available / 1024.0 / 1024.0) << " GB" << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Cached: " << FLEX_COLOR_RESET 
                  << std::fixed << std::setprecision(2) 
                  << (mem.cached / 1024.0 / 1024.0) << " GB" << std::endl;
        std::cout << FLEX_COLOR_YELLOW << "  Buffers: " << FLEX_COLOR_RESET 
                  << std::fixed << std::setprecision(2) 
                  << (mem.buffers / 1024.0 / 1024.0) << " GB" << std::endl;
        
        if (mem.swapTotal > 0) {
            std::cout << FLEX_COLOR_YELLOW << "  Total Swap: " << FLEX_COLOR_RESET 
                      << std::fixed << std::setprecision(2) 
                      << (mem.swapTotal / 1024.0 / 1024.0) << " GB" << std::endl;
            int swapUsage = (mem.swapTotal - mem.swapFree) * 100 / mem.swapTotal;
            std::cout << FLEX_COLOR_YELLOW << "  Swap Usage: " << FLEX_COLOR_RESET 
                      << swapUsage << "%" << std::endl;
        }
        
        // 磁盘信息
        auto disks = getDiskInfo();
        std::cout << FLEX_COLOR_GREEN << "\n[Disk Information]" << FLEX_COLOR_RESET << std::endl;
        for (const auto& disk : disks) {
            // 只显示主要文件系统
            if (disk.mountPoint == "/" || disk.mountPoint.find("/home") != std::string::npos ||
                disk.mountPoint.find("/boot") != std::string::npos) {
                std::cout << FLEX_COLOR_YELLOW << "  Device: " << FLEX_COLOR_RESET << disk.device << std::endl;
                std::cout << FLEX_COLOR_YELLOW << "  Mount Point: " << FLEX_COLOR_RESET << disk.mountPoint << std::endl;
                std::cout << FLEX_COLOR_YELLOW << "  Filesystem: " << FLEX_COLOR_RESET << disk.filesystem << std::endl;
                std::cout << FLEX_COLOR_YELLOW << "  Total: " << FLEX_COLOR_RESET 
                          << std::fixed << std::setprecision(2) 
                          << (disk.total / 1024.0 / 1024.0) << " GB" << std::endl;
                std::cout << FLEX_COLOR_YELLOW << "  Used: " << FLEX_COLOR_RESET 
                          << std::fixed << std::setprecision(2) 
                          << (disk.used / 1024.0 / 1024.0) << " GB (" 
                          << disk.usagePercent << "%)" << std::endl;
                if (disk.inodesTotal > 0) {
                    std::cout << FLEX_COLOR_YELLOW << "  Inodes: " << FLEX_COLOR_RESET 
                              << disk.inodesUsed << "/" << disk.inodesTotal 
                              << " (" << disk.inodeUsagePercent << "%)" << std::endl;
                }
                std::cout << std::endl;
            }
        }
        
        // 网络信息
        auto networks = getNetworkInfo();
        std::cout << FLEX_COLOR_GREEN << "\n[Network Information]" << FLEX_COLOR_RESET << std::endl;
        for (const auto& net : networks) {
            if (!net.macAddress.empty() && net.macAddress != "00:00:00:00:00:00") {
                std::cout << FLEX_COLOR_YELLOW << "  Interface: " << FLEX_COLOR_RESET << net.name << std::endl;
                std::cout << FLEX_COLOR_YELLOW << "  MAC Address: " << FLEX_COLOR_RESET << net.macAddress << std::endl;
                if (net.rxBytes > 0 || net.txBytes > 0) {
                    std::cout << FLEX_COLOR_YELLOW << "  RX/TX Bytes: " << FLEX_COLOR_RESET 
                              << (net.rxBytes / 1024.0 / 1024.0) << " MB / " 
                              << (net.txBytes / 1024.0 / 1024.0) << " MB" << std::endl;
                }
                std::cout << std::endl;
            }
        }
    } else if (format == FlexOutputFormat::JSON) {
        std::cout << toJSON() << std::endl;
    } else if (format == FlexOutputFormat::CSV) {
        std::cout << toCSV() << std::endl;
    }
}

std::string FlexHardwareInfo::toJSON() const {
    std::ostringstream oss;
    
    FlexCPUInfo cpu = getCPUInfo();
    FlexMemoryInfo mem = getMemoryInfo();
    auto disks = getDiskInfo();
    auto networks = getNetworkInfo();
    
    oss << "{\n";
    oss << "  \"flex_hardware_info\": {\n";
    
    // CPU信息
    oss << "    \"cpu\": {\n";
    oss << "      \"model\": \"" << cpu.model << "\",\n";
    oss << "      \"vendor\": \"" << cpu.vendor << "\",\n";
    oss << "      \"architecture\": \"" << cpu.architecture << "\",\n";
    oss << "      \"cores\": " << cpu.cores << ",\n";
    oss << "      \"threads\": " << cpu.threads << ",\n";
    oss << "      \"clock_speed_ghz\": " << std::fixed << std::setprecision(2) << cpu.clockSpeed << ",\n";
    oss << "      \"cache_size_kb\": " << cpu.cacheSize << "\n";
    oss << "    },\n";
    
    // 内存信息
    oss << "    \"memory\": {\n";
    oss << "      \"total_mb\": " << (mem.total / 1024.0) << ",\n";
    oss << "      \"available_mb\": " << (mem.available / 1024.0) << ",\n";
    oss << "      \"cached_mb\": " << (mem.cached / 1024.0) << ",\n";
    oss << "      \"buffers_mb\": " << (mem.buffers / 1024.0) << ",\n";
    oss << "      \"swap_total_mb\": " << (mem.swapTotal / 1024.0) << ",\n";
    oss << "      \"swap_free_mb\": " << (mem.swapFree / 1024.0) << "\n";
    oss << "    },\n";
    
    // 磁盘信息
    oss << "    \"disks\": [\n";
    bool firstDisk = true;
    for (const auto& disk : disks) {
        if (!firstDisk) oss << ",\n";
        firstDisk = false;
        
        oss << "      {\n";
        oss << "        \"device\": \"" << disk.device << "\",\n";
        oss << "        \"mount_point\": \"" << disk.mountPoint << "\",\n";
        oss << "        \"filesystem\": \"" << disk.filesystem << "\",\n";
        oss << "        \"total_gb\": " << std::fixed << std::setprecision(2) << (disk.total / 1024.0 / 1024.0) << ",\n";
        oss << "        \"used_gb\": " << std::fixed << std::setprecision(2) << (disk.used / 1024.0 / 1024.0) << ",\n";
        oss << "        \"usage_percent\": " << disk.usagePercent << "\n";
        oss << "      }";
    }
    oss << "\n    ],\n";
    
    // 网络信息
    oss << "    \"network_interfaces\": [\n";
    bool firstNet = true;
    for (const auto& net : networks) {
        if (!firstNet) oss << ",\n";
        firstNet = false;
        
        oss << "      {\n";
        oss << "        \"name\": \"" << net.name << "\",\n";
        oss << "        \"mac_address\": \"" << net.macAddress << "\",\n";
        oss << "        \"rx_bytes\": " << net.rxBytes << ",\n";
        oss << "        \"tx_bytes\": " << net.txBytes << "\n";
        oss << "      }";
    }
    oss << "\n    ]\n";
    
    oss << "  }\n";
    oss << "}";
    
    return oss.str();
}

std::string FlexHardwareInfo::toCSV() const {
    std::ostringstream oss;
    
    FlexCPUInfo cpu = getCPUInfo();
    FlexMemoryInfo mem = getMemoryInfo();
    auto disks = getDiskInfo();
    auto networks = getNetworkInfo();
    
    oss << "Category,Key,Value\n";
    
    // CPU信息
    oss << "CPU,Model,\"" << cpu.model << "\"\n";
    oss << "CPU,Vendor,\"" << cpu.vendor << "\"\n";
    oss << "CPU,Architecture,\"" << cpu.architecture << "\"\n";
    oss << "CPU,Cores," << cpu.cores << "\n";
    oss << "CPU,Threads," << cpu.threads << "\n";
    oss << "CPU,Clock Speed (GHz)," << std::fixed << std::setprecision(2) << cpu.clockSpeed << "\n";
    oss << "CPU,Cache Size (KB)," << cpu.cacheSize << "\n";
    
    // 内存信息
    oss << "Memory,Total (MB)," << (mem.total / 1024.0) << "\n";
    oss << "Memory,Available (MB)," << (mem.available / 1024.0) << "\n";
    oss << "Memory,Cached (MB)," << (mem.cached / 1024.0) << "\n";
    oss << "Memory,Buffers (MB)," << (mem.buffers / 1024.0) << "\n";
    oss << "Memory,Swap Total (MB)," << (mem.swapTotal / 1024.0) << "\n";
    oss << "Memory,Swap Free (MB)," << (mem.swapFree / 1024.0) << "\n";
    
    // 磁盘信息
    for (const auto& disk : disks) {
        oss << "Disk,Device,\"" << disk.device << "\"\n";
        oss << "Disk,Mount Point,\"" << disk.mountPoint << "\"\n";
        oss << "Disk,Filesystem,\"" << disk.filesystem << "\"\n";
        oss << "Disk,Total (GB)," << std::fixed << std::setprecision(2) << (disk.total / 1024.0 / 1024.0) << "\n";
        oss << "Disk,Used (GB)," << std::fixed << std::setprecision(2) << (disk.used / 1024.0 / 1024.0) << "\n";
        oss << "Disk,Usage %," << disk.usagePercent << "\n";
    }
    
    // 网络信息
    for (const auto& net : networks) {
        oss << "Network,Interface,\"" << net.name << "\"\n";
        oss << "Network,MAC Address,\"" << net.macAddress << "\"\n";
        oss << "Network,RX Bytes," << net.rxBytes << "\n";
        oss << "Network,TX Bytes," << net.txBytes << "\n";
    }
    
    return oss.str();
}

} // namespace FlexTools
