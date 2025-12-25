#ifndef FLEX_HARDWARE_INFO_H
#define FLEX_HARDWARE_INFO_H

#include "flex_common.h"
#include <vector>
#include <map>

namespace FlexTools {

struct FlexCPUInfo {
    std::string model;
    std::string vendor;
    int cores;
    int threads;
    std::string architecture;
    double clockSpeed; // GHz
    std::vector<std::string> flags;
    int cacheSize; // KB
};

struct FlexMemoryInfo {
    long long total;    // KB
    long long free;     // KB
    long long available; // KB
    long long cached;   // KB
    long long buffers;  // KB
    long long swapTotal; // KB
    long long swapFree;  // KB
};

struct FlexDiskInfo {
    std::string device;
    std::string mountPoint;
    std::string filesystem;
    long long total;    // KB
    long long used;     // KB
    long long free;     // KB
    int usagePercent;   // %
    long long inodesTotal;
    long long inodesUsed;
    long long inodesFree;
    int inodeUsagePercent;
};

struct FlexNetworkInterface {
    std::string name;
    std::string macAddress;
    std::string ipAddress;
    std::string netmask;
    std::string broadcast;
    long long rxBytes;
    long long txBytes;
    long long rxPackets;
    long long txPackets;
    long long rxErrors;
    long long txErrors;
};

struct FlexGPUInfo {
    std::string vendor;
    std::string model;
    std::string driverVersion;
    long long memoryTotal; // MB
    long long memoryUsed;  // MB
};

class FlexHardwareInfo {
public:
    FlexHardwareInfo();
    
    // CPU 信息
    FlexCPUInfo getCPUInfo() const;
    
    // 内存信息
    FlexMemoryInfo getMemoryInfo() const;
    
    // 磁盘信息
    std::vector<FlexDiskInfo> getDiskInfo() const;
    
    // 网络接口信息
    std::vector<FlexNetworkInterface> getNetworkInfo() const;
    
    // GPU 信息 (如果可用)
    std::vector<FlexGPUInfo> getGPUInfo() const;
    
    // USB 设备信息
    std::vector<std::string> getUSBDevices() const;
    
    // PCI 设备信息
    std::vector<std::string> getPCIDevices() const;
    
    // 温度传感器信息
    std::map<std::string, double> getTemperatureInfo() const;
    
    // 风扇信息
    std::map<std::string, int> getFanInfo() const;
    
    // 电池信息
    std::map<std::string, std::string> getBatteryInfo() const;
    
    // 打印所有硬件信息
    void printAllInfo(FlexOutputFormat format = FlexOutputFormat::TEXT) const;
    
    // 导出为JSON
    std::string toJSON() const;
    
    // 导出为CSV
    std::string toCSV() const;
    
private:
    // 辅助方法
    std::vector<std::string> flexReadProcFile(const std::string& filename) const;
    std::string flexExecuteCommand(const std::string& cmd) const;
    std::vector<std::string> flexExecuteCommandLines(const std::string& cmd) const;
    
    // 特定信息解析
    void flexParseCPUInfo(FlexCPUInfo& cpu) const;
    void flexParseMemoryInfo(FlexMemoryInfo& mem) const;
    FlexDiskInfo flexParseDiskLine(const std::string& line) const;
    FlexNetworkInterface flexParseNetworkInterface(const std::string& name) const;
    
    // 缓存
    mutable FlexCPUInfo flexCPUInfoCache;
    mutable FlexMemoryInfo flexMemoryInfoCache;
    mutable std::vector<FlexDiskInfo> flexDiskInfoCache;
    mutable std::vector<FlexNetworkInterface> flexNetworkInfoCache;
    mutable bool flexCacheValid = false;
    
    void flexEnsureCache() const;
};

} // namespace FlexTools

#endif // FLEX_HARDWARE_INFO_H
