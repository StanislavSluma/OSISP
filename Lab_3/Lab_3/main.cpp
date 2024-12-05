#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <random>
#include <vector>
#include <chrono>
#include <fstream>

#define BUFFER_SIZE 1000000

static std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

double GetElapsedTime() {
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    return elapsed.count();
}

void SendTaskToWorker(const std::string& pipeName, const std::vector<int>& array, std::vector<int>& sortedArray) {
    HANDLE hPipe;
    DWORD bytesWritten, bytesRead;
    char buffer[BUFFER_SIZE];

    if (!WaitNamedPipeA(pipeName.c_str(), NMPWAIT_WAIT_FOREVER)) {
        std::cerr << "Не удалось дождаться канала: " << pipeName << std::endl;
        return;
    }

    hPipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe != INVALID_HANDLE_VALUE) {
        std::string task;
        for (int num : array) {
            task += std::to_string(num) + " ";
        }

        WriteFile(hPipe, task.c_str(), task.size() + 1, &bytesWritten, NULL);

        ReadFile(hPipe, buffer, BUFFER_SIZE, &bytesRead, NULL);
        std::string result(buffer);

        std::istringstream iss(result);
        int num;
        while (iss >> num) {
            sortedArray.push_back(num);
        }

        CloseHandle(hPipe);
    }
    else {
        std::cerr << "Не удалось подключиться к каналу: " << pipeName << std::endl;
    }
}


std::vector<int> MergeArrays(const std::vector<int>& left, const std::vector<int>& right) {
    std::vector<int> result;
    size_t i = 0, j = 0;

    while (i < left.size() && j < right.size()) {
        if (left[i] <= right[j]) {
            result.push_back(left[i]);
            i++;
        }
        else {
            result.push_back(right[j]);
            j++;
        }
    }

    while (i < left.size()) {
        result.push_back(left[i]);
        i++;
    }
    while (j < right.size()) {
        result.push_back(right[j]);
        j++;
    }

    return result;
}

LPWSTR ConvertStringToLPWSTR(const std::string& str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (len == 0) {
        return NULL;
    }
    LPWSTR wideStr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wideStr, len);
    return wideStr;
}

std::vector<int> GenerateRandomArray(int size, int minValue, int maxValue) {
    std::vector<int> array(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(minValue, maxValue);

    for (int& num : array) {
        num = dis(gen);
    }

    return array;
}

int main() {
    setlocale(LC_ALL, "ru");
    int workerCount = 4;
    std::cout << "Введите кол - во процессов: ";
    std::cin >> workerCount;

    if (workerCount < 2) {
        workerCount = 2;
    }

    std::vector<std::string> pipeNames(workerCount);
    for (size_t i = 0; i < workerCount; ++i) {
        pipeNames[i] = "\\\\.\\pipe\\WorkerPipe" + std::to_string(i + 1);
    }

    int arraySize = 65536 * 4;
    std::vector<int> bigArray = GenerateRandomArray(arraySize, -100, 100);
    std::vector<int> bigArray_copy(bigArray);

    std::vector<std::vector<int>> parts(workerCount);

    int partSize = bigArray.size() / workerCount;

    for (int i = 0; i < workerCount; ++i) {
        int start = i * partSize;
        int end = (i == workerCount - 1) ? bigArray.size() : (i + 1) * partSize;
        parts[i] = std::vector<int>(bigArray.begin() + start, bigArray.begin() + end);
    }

    STARTUPINFO* si = new STARTUPINFO[workerCount];
    PROCESS_INFORMATION* pi = new PROCESS_INFORMATION[workerCount];

    for (int i = 0; i < workerCount; ++i) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi[i], sizeof(pi[i]));


        std::string workerCmd = "Worker.exe " + std::to_string(i + 1);
        LPWSTR cmd = ConvertStringToLPWSTR(workerCmd);

        if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si[i], &pi[i])) {
            std::cerr << "Не удалось запустить рабочий процесс " << i + 1 << std::endl;
            std::cerr << GetLastError() << std::endl;
            return 1;
        }
    }


    Sleep(1024);

    startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<int>> sortedParts(workerCount);
    for (int i = 0; i < workerCount; ++i) {
        SendTaskToWorker(pipeNames[i], parts[i], sortedParts[i]);
    }

    std::vector<int> sortedArray = sortedParts[0];
    for (int i = 1; i < workerCount; ++i) {
        sortedArray = MergeArrays(sortedArray, sortedParts[i]);
    }

    std::cout << "\n\n" << GetElapsedTime();

    std::cout << "\nОбычная сортировка:";

    startTime = std::chrono::high_resolution_clock::now();
    std::sort(bigArray_copy.begin(), bigArray_copy.end());

    std::cout << "\n" << GetElapsedTime();


    /*std::cout << "\n\nОтсортированный массив: ";
    for (int num : sortedArray) {
        std::cout << num << " ";
    }
    std::cout << std::endl;*/

    for (int i = 0; i < workerCount; ++i) {
        WaitForSingleObject(pi[i].hProcess, INFINITE);
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }
    return 0;
}
