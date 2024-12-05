#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

void ExportRegistryKey(HKEY hKey, const std::string& subKey, const std::string& searchValue, std::ofstream& outFile) {
    DWORD index = 0;
    CHAR valueName[256];
    BYTE valueData[1024];
    DWORD valueNameSize, valueDataSize, valueType;

    outFile << "[" << subKey << "]\n";

    while (true) {
        valueNameSize = sizeof(valueName);
        valueDataSize = sizeof(valueData);

        LONG result = RegEnumValueA(hKey, index, valueName, &valueNameSize, NULL, &valueType, valueData, &valueDataSize);
        if (result == ERROR_NO_MORE_ITEMS) break;
        if (result == ERROR_SUCCESS) {
            if (searchValue.empty() || searchValue == valueName) {
                outFile << "\"" << valueName << "\"=";
                switch (valueType) {
                case REG_SZ:
                    outFile << "\"" << (char*)valueData << "\"\n";
                    break;
                case REG_DWORD:
                    outFile << "dword:" << std::hex << *(DWORD*)valueData << "\n";
                    break;
                default:
                    outFile << "unsupported_type\n";
                    break;
                }
            }
        }
        index++;
    }

    index = 0;
    CHAR subKeyName[256];
    DWORD subKeyNameSize;
    HKEY hSubKey;

    while (true) {
        subKeyNameSize = sizeof(subKeyName);
        LONG result = RegEnumKeyExA(hKey, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL);
        if (result == ERROR_NO_MORE_ITEMS) break;
        if (result == ERROR_SUCCESS) {
            std::string fullSubKeyPath = subKey + "\\" + subKeyName;
            if (RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                ExportRegistryKey(hSubKey, fullSubKeyPath, searchValue, outFile);
                RegCloseKey(hSubKey);
            }
        }
        index++;
    }
}
// key: SOFTWARE\Arkane\Dishonored
// value: Res
int main() {
    setlocale(LC_ALL, "ru");
    std::string rootKey, searchValue, outputFile = "registry_output.reg";

    std::cout << "¬ведите путь к ключу реестра (например, SOFTWARE\\Microsoft): ";
    std::getline(std::cin, rootKey);

    std::cout << "¬ведите им€ значени€ дл€ поиска (оставьте пустым, чтобы искать все значени€): ";
    std::getline(std::cin, searchValue);

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, rootKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        std::ofstream outFile(outputFile);
        if (outFile.is_open()) {
            ExportRegistryKey(hKey, rootKey, searchValue, outFile);
            outFile.close();
            std::cout << "Ёкспорт завершен, файл сохранен: " << outputFile << std::endl;
        }
        else {
            std::cerr << "ќшибка: не удалось открыть файл дл€ записи." << std::endl;
        }
        RegCloseKey(hKey);
    }
    else {
        std::cerr << "ќшибка: не удалось открыть ключ реестра." << std::endl;
    }
    return 0;
}
