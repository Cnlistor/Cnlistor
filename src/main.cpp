#include "flex_system_info.h"
#include "flex_hardware_info.h"
#include "flex_package_info.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <getopt.h>
#include <memory>

using namespace FlexTools;

void printFlexToolsUsage(const char* programName) {
    std::cout << "FlexTools - Linux System Information and Log Collection Tool" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "\nUsage: " << programName << " [OPTIONS]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -s, --system        Display system information" << std::endl;
    std::cout << "  -h, --hardware      Display hardware information" << std::endl;
    std::cout << "  -p, --packages      Display installed packages" << std::endl;
    std::cout << "  -l, --logs          Collect and display system logs" << std::endl;
    std::cout << "  -a, --all           Display all information" << std::endl;
    std::cout << "  -o, --output FILE   Export output to file" << std::endl;
    std::cout << "  -f, --format FORMAT Output format (text, json, csv)" << std::endl;
    std::cout << "  -v, --verbose       Verbose output" << std::endl;
    std::cout << "  -q, --quiet         Quiet mode (minimal output)" << std::endl;
    std::cout << "  --version           Display version information" << std::endl;
    std::cout << "  --help              Display this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << programName << " --system --hardware" << std::endl;
    std::cout << "  " << programName << " --packages --format json" << std::endl;
    std::cout << "  " << programName << " --logs --output system_logs.txt" << std::endl;
    std::cout << "  " << programName << " --all --format csv --output report.csv" << std::endl;
}

void printFlexToolsVersion() {
    std::cout << "FlexTools v1.0.0" << std::endl;
    std::cout << "Copyright (c) 2024 FlexTools Project" << std::endl;
    std::cout << "A flexible system information and log collection tool for Linux" << std::endl;
}

void printFlexToolsBanner() {
    std::cout << FLEX_COLOR_CYAN << R"(
███████╗██╗     ███████╗██╗  ██╗████████╗ ██████╗  ██████╗ ██╗     ███████╗
██╔════╝██║     ██╔════╝╚██╗██╔╝╚══██╔══╝██╔═══██╗██╔═══██╗██║     ██╔════╝
█████╗  ██║     █████╗   ╚███╔╝    ██║   ██║   ██║██║   ██║██║     ███████╗
██╔══╝  ██║     ██╔══╝   ██╔██╗    ██║   ██║   ██║██║   ██║██║     ╚════██║
██║     ███████╗███████╗██╔╝ ██╗   ██║   ╚██████╔╝╚██████╔╝███████╗███████║
╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝   ╚═╝    ╚═════╝  ╚═════╝ ╚══════╝╚══════╝
)" << FLEX_COLOR_RESET << std::endl;
}

int main(int argc, char* argv[]) {
    bool showSystem = false;
    bool showHardware = false;
    bool showPackages = false;
    bool showLogs = false;
    bool showAll = false;
    bool verbose = false;
    bool quiet = false;
    bool showVersion = false;
    std::string outputFile;
    FlexOutputFormat format = FlexOutputFormat::TEXT;
    
    // 命令行参数解析
    static struct option long_options[] = {
        {"system", no_argument, 0, 's'},
        {"hardware", no_argument, 0, 'h'},
        {"packages", no_argument, 0, 'p'},
        {"logs", no_argument, 0, 'l'},
        {"all", no_argument, 0, 'a'},
        {"output", required_argument, 0, 'o'},
        {"format", required_argument, 0, 'f'},
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"version", no_argument, 0, 0},
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "shpla:o:f:vq", 
                              long_options, &option_index)) != -1) {
        switch (opt) {
            case 's':
                showSystem = true;
                break;
            case 'h':
                showHardware = true;
                break;
            case 'p':
                showPackages = true;
                break;
            case 'l':
                showLogs = true;
                break;
            case 'a':
                showAll = true;
                break;
            case 'o':
                outputFile = optarg;
                break;
            case 'f':
                if (std::string(optarg) == "json") {
                    format = FlexOutputFormat::JSON;
                } else if (std::string(optarg) == "csv") {
                    format = FlexOutputFormat::CSV;
                } else if (std::string(optarg) == "xml") {
                    format = FlexOutputFormat::XML;
                } else {
                    format = FlexOutputFormat::TEXT;
                }
                break;
            case 'v':
                verbose = true;
                break;
            case 'q':
                quiet = true;
                break;
            case 0:
                if (long_options[option_index].name == std::string("help")) {
                    printFlexToolsUsage(argv[0]);
                    return 0;
                } else if (long_options[option_index].name == std::string("version")) {
                    printFlexToolsVersion();
                    return 0;
                }
                break;
            default:
                printFlexToolsUsage(argv[0]);
                return 1;
        }
    }
    
    // 如果没有指定任何选项，显示帮助
    if (!showSystem && !showHardware && !showPackages && !showLogs && !showAll && !showVersion) {
        if (!quiet) {
            printFlexToolsBanner();
        }
        printFlexToolsUsage(argv[0]);
        return 0;
    }
    
    // 如果指定了--all，显示所有信息
    if (showAll) {
        showSystem = true;
        showHardware = true;
        showPackages = true;
        showLogs = true;
    }
    
    try {
        // 重定向输出到文件
        std::unique_ptr<std::ofstream> outFile;
        if (!outputFile.empty()) {
            outFile = std::make_unique<std::ofstream>(outputFile);
            if (!outFile->is_open()) {
                std::cerr << FLEX_COLOR_RED << "Error: Cannot open output file: " 
                          << outputFile << FLEX_COLOR_RESET << std::endl;
                return 1;
            }
            // 保存原cout缓冲区
            auto oldCoutBuffer = std::cout.rdbuf();
            // 重定向cout到文件
            std::cout.rdbuf(outFile->rdbuf());
            
            // 恢复cout在退出时
            struct RestoreCout {
                std::streambuf* old;
                ~RestoreCout() { std::cout.rdbuf(old); }
            } restore{oldCoutBuffer};
        }
        
        if (!quiet && outputFile.empty()) {
            printFlexToolsBanner();
        }
        
        // 显示系统信息
        if (showSystem) {
            FlexSystemInfo sysInfo;
            FlexInfoLevel level = verbose ? FlexInfoLevel::DETAILED : FlexInfoLevel::BASIC;
            if (format == FlexOutputFormat::TEXT && !quiet) {
                sysInfo.printInfo(level, format);
            } else {
                if (format == FlexOutputFormat::JSON) {
                    std::cout << sysInfo.toJSON(level) << std::endl;
                } else if (format == FlexOutputFormat::CSV) {
                    std::cout << sysInfo.toCSV(level) << std::endl;
                }
            }
        }
        
        // 显示硬件信息
        if (showHardware) {
            FlexHardwareInfo hwInfo;
            if (format == FlexOutputFormat::TEXT && !quiet) {
                hwInfo.printAllInfo(format);
            } else {
                if (format == FlexOutputFormat::JSON) {
                    std::cout << hwInfo.toJSON() << std::endl;
                } else if (format == FlexOutputFormat::CSV) {
                    std::cout << hwInfo.toCSV() << std::endl;
                }
            }
        }
        
        // 显示包信息
        if (showPackages) {
            FlexPackageInfo pkgInfo;
            auto packages = pkgInfo.getAllPackages();
            if (format == FlexOutputFormat::TEXT && !quiet) {
                pkgInfo.printPackages(packages, format);
            } else if (format == FlexOutputFormat::JSON) {
                // 需要实现toJSON方法
                std::cout << "{\"packages\": []}" << std::endl; // 占位符
            }
        }
        
        if (!outputFile.empty() && !quiet) {
            std::cout.rdbuf(restore.old);
            std::cout << FLEX_COLOR_GREEN << "FlexTools: Output written to: " 
                      << outputFile << FLEX_COLOR_RESET << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << FLEX_COLOR_RED << "FlexTools Error: " << e.what() 
                  << FLEX_COLOR_RESET << std::endl;
        return 1;
    } catch (...) {
        std::cerr << FLEX_COLOR_RED << "FlexTools Error: Unknown exception occurred" 
                  << FLEX_COLOR_RESET << std::endl;
        return 1;
    }
    
    return 0;
}
