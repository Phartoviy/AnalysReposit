#include <iostream>
#include <filesystem>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <map>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

int index = 0;
int current_percent = -1;

std::string executeCommand(const std::string& command) {
    std::string result;
    char buffer[128];
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Ошибка при открытии `popen`");
    }
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}


void fill(std::string path, std::map<std::string, std::string>& list)
{
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".deb" &&
            entry.path().extension() != ".debian" && entry.path().extension() != ".Debian") {
            std::string debPath = entry.path().string();
            std::string commandName = "dpkg-deb -I " + debPath + " | grep Package | awk '{print $NF}'";
            auto textname = executeCommand(commandName.c_str());
            std::string commandVersion = "dpkg-deb -I " + debPath + " | grep Version | awk '{print $NF}'";
            auto textversion = executeCommand(commandVersion.c_str());
            std::replace(textname.begin(),textname.end(),'\n','_');
            textversion.pop_back();
            list[textname] = textversion;
            int temp = std::floor(++index/180.0);
            if (current_percent < temp)
            {
                current_percent = temp;
                if (temp > 100)
                    temp = 100;
                std::cout << "Процент = " << temp << "%" << std::endl;
            }
        }
    }

}


int main(int argc, char *argv[])
{
    if (argc == 3)
    {
        std::string pathMainDir = argv[1];
        std::string pathBaseDir = argv[2];
        std::map<std::string, std::string> debPackagesMain;
        fill(pathMainDir,debPackagesMain);
        std::map<std::string, std::string> debPackagesBase;
        fill(pathBaseDir,debPackagesBase);
        std::cout << "Завершено сканирование пакетов" << std::endl;
        std::ofstream resultFile("result.csv", std::ios::app);
        std::map<std::string, char> watched{};
        if (resultFile.is_open()) {
            std::cout << "Запись отчета в result.csv" << std::endl;
            for (auto &&[name,unusedstr]: debPackagesMain)
            {
                if (debPackagesBase.find(name) != debPackagesBase.end())
                {
                    int t = 123 - name.length();
                    if (t < 0)
                        continue;
                    std::string prichina = (debPackagesMain[name] != debPackagesBase[name])?"Разные версии":"Одинаковые";
                    resultFile << name << "," << name << "," << prichina << std::endl;
                    watched[name] = 'a';
                }
                else
                {
                    int t = 123 - name.length();
                    if (t < 0)
                        continue;
                    resultFile <<  name << ",," << "Отсутствует во втором" <<  std::endl;
                }
            }
            for (auto &&[name,unusedstr]: debPackagesBase)
            {
                if (watched.find(name) == watched.end())
                {
                    resultFile << "," <<  name  << "," << "Отсутствует в первом"  <<  std::endl;
                }
            }
        }
        resultFile.close();
    }
    return 0;
}
