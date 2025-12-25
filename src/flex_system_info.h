#ifndef FLEX_SYSTEM_INFO_H
#define FLEX_SYSTEM_INFO_H

#include "flex_common.h"
#include <sys/utsname.h>
#include <unistd.h>

namespace FlexTools {

class FlexSystemInfo {
public:
    FlexSystemInfo();
    
    // 获取系统基本信息
    std::map<std::string, std::string> getBasicInfo() const;
    
    // 获取详细系统信息
    std::map<std::string, std::string> getDetailedInfo() const;
    
    // 获取内核信息
    std::map<std::string, std::string> getKernelInfo() const;
    
    // 获取系统运行时间
    std::string getUptime() const;
    
    // 获取系统负载
    std::vector<std::string> getLoadAverage() const;
    
    // 获取用户信息
    std::vector<std::string> getLoggedUsers() const;
    
    // 获取环境变量
    std::map<std::string, std::string> getEnvironmentVars() const;
    
    // 格式化输出
    void printInfo(FlexInfoLevel level = FlexInfoLevel::BASIC, 
                   FlexOutputFormat format = FlexOutputFormat::TEXT) const;
    
    // 导出为JSON
    std::string toJSON(FlexInfoLevel level = FlexInfoLevel::BASIC) const;
    
    // 导出为CSV
    std::string toCSV(FlexInfoLevel level = FlexInfoLevel::BASIC) const;
    
private:
    struct utsname flexUnameData;
    
    // 辅助方法
    std::string flexReadFile(const std::string& filename) const;
    std::vector<std::string> flexExecuteCommand(const std::string& cmd) const;
    
    // 特定信息获取
    std::string getOSName() const;
    std::string getOSVersion() const;
    std::string getHostname() const;
    std::string getDistribution() const;
    
    // 内部数据缓存
    mutable bool flexCacheValid = false;
    mutable std::map<std::string, std::string> flexBasicInfoCache;
    mutable std::map<std::string, std::string> flexDetailedInfoCache;
    
    void flexEnsureCache() const;
};

} // namespace FlexTools

#endif // FLEX_SYSTEM_INFO_Hf
