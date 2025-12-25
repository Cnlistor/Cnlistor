#ifndef FLEX_PACKAGE_INFO_H
#define FLEX_PACKAGE_INFO_H

#include "flex_common.h"
#include <vector>
#include <map>

namespace FlexTools {

struct FlexPackage {
    std::string name;
    std::string version;
    std::string architecture;
    std::string description;
    std::string status; // installed, removed, etc.
    std::string installDate;
    size_t size; // in bytes
    std::string maintainer;
    std::string section;
    int priority;
    std::vector<std::string> dependencies;
    std::vector<std::string> provides;
    std::vector<std::string> conflicts;
};

class FlexPackageInfo {
public:
    FlexPackageInfo();
    
    // 检测系统类型
    FlexSystemType flexDetectSystemType() const;
    
    // 获取所有已安装的包
    std::vector<FlexPackage> getAllPackages() const;
    
    // 搜索包
    std::vector<FlexPackage> searchPackages(const std::string& keyword) const;
    
    // 获取特定包的信息
    FlexPackage getPackageInfo(const std::string& packageName) const;
    
    // 获取包依赖
    std::vector<std::string> getPackageDependencies(const std::string& packageName) const;
    
    // 获取包文件列表
    std::vector<std::string> getPackageFiles(const std::string& packageName) const;
    
    // 统计包信息
    std::map<std::string, int> getPackageStatistics() const;
    
    // 检查包更新
    std::vector<FlexPackage> checkForUpdates() const;
    
    // 打印包信息
    void printPackages(const std::vector<FlexPackage>& packages, 
                      FlexOutputFormat format = FlexOutputFormat::TEXT) const;
    
    // 导出为JSON
    std::string toJSON(const std::vector<FlexPackage>& packages) const;
    
    // 导出为CSV
    std::string toCSV(const std::vector<FlexPackage>& packages) const;
    
private:
    FlexSystemType flexSystemType;
    
    // Debian 系统方法
    std::vector<FlexPackage> flexGetDebianPackages() const;
    FlexPackage flexGetDebianPackageInfo(const std::string& packageName) const;
    FlexPackage flexParseDPKGLine(const std::string& line) const;
    
    // RPM 系统方法
    std::vector<FlexPackage> flexGetRPMPackages() const;
    FlexPackage flexGetRPMPackageInfo(const std::string& packageName) const;
    FlexPackage flexParseRPMLine(const std::string& line) const;
    
    // 辅助方法
    std::vector<std::string> flexExecuteCommand(const std::string& cmd) const;
    std::string flexExecuteCommandSingle(const std::string& cmd) const;
    std::string flexTrim(const std::string& str) const;
    std::vector<std::string> flexSplit(const std::string& str, char delimiter) const;
    
    // 缓存
    mutable std::vector<FlexPackage> flexPackageCache;
    mutable bool flexCacheValid = false;
    
    void flexEnsureCache() const;
};

} // namespace FlexTools

#endif // FLEX_PACKAGE_INFO_H
