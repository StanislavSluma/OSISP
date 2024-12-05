#include <windows.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <fstream>

struct ThreadParams {
    char* data;
    size_t length;
    int threadID;
    HANDLE fileHandle;
    OVERLAPPED overlapped;
};

static std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

double GetElapsedTime() {
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    return elapsed.count();
}

void* MapFileToMemory(const char* filename, size_t& fileSize) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка при открытии файла." << std::endl;
        return nullptr;
    }

    fileSize = GetFileSize(hFile, NULL);
    HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hMapFile) {
        std::cerr << "Ошибка при создании отображения файла. " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return nullptr;
    }

    char* data = (char*)MapViewOfFile(hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (!data) {
        std::cerr << "Ошибка при отображении файла в память." << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        return nullptr;
    }

    CloseHandle(hMapFile);
    CloseHandle(hFile);
    return data;
}

void SimpleReadFile(const char* filename, std::vector<char>& buffer) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка при открытии файла." << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    buffer.resize(fileSize);

    DWORD bytesRead;
    ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);

    CloseHandle(hFile);
}

void SimpleReadFileMultiThreaded(const char* filename, std::vector<char>& buffer, int numThreads) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка при открытии файла для чтения." << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    buffer.resize(fileSize);

    std::vector<HANDLE> threads(numThreads);
    std::vector<ThreadParams> params(numThreads);
    std::vector<OVERLAPPED> overlappedStructs(numThreads);

    size_t chunkSize = fileSize / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        params[i].data = buffer.data() + i * chunkSize;
        params[i].length = (i == numThreads - 1) ? (fileSize - i * chunkSize) : chunkSize;
        params[i].threadID = i;
        params[i].fileHandle = hFile;

        overlappedStructs[i] = {};
        overlappedStructs[i].Offset = i * chunkSize;
        overlappedStructs[i].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        params[i].overlapped = overlappedStructs[i];

        threads[i] = CreateThread(NULL, 0, [](LPVOID params) -> DWORD {
            ThreadParams* threadParams = (ThreadParams*)params;

            HANDLE hFile = threadParams->fileHandle;

            OVERLAPPED* overlapped = &threadParams->overlapped;
            DWORD bytesRead = 0;
            BOOL result = ReadFile(hFile, threadParams->data, threadParams->length, &bytesRead, overlapped);

            if (!result && GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(overlapped->hEvent, INFINITE);
                GetOverlappedResult(hFile, overlapped, &bytesRead, FALSE);
            }

            CloseHandle(overlapped->hEvent);
            return 0;
            }, &params[i], 0, NULL);
    }

    WaitForMultipleObjects(numThreads, threads.data(), TRUE, INFINITE);

    for (HANDLE thread : threads) {
        CloseHandle(thread);
    }

    CloseHandle(hFile);
}

void SimpleWriteFile(const char* filename, std::vector<char>& buffer) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка при открытии файла." << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);

    DWORD bytesWrite;
    WriteFile(hFile, buffer.data(), fileSize, &bytesWrite, NULL);
    CloseHandle(hFile);
}

void SimpleWriteFileMultiThreaded(const char* filename, std::vector<char>& buffer, int numThreads) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка при открытии файла для чтения." << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);

    std::vector<HANDLE> threads(numThreads);
    std::vector<ThreadParams> params(numThreads);
    std::vector<OVERLAPPED> overlappedStructs(numThreads);

    size_t chunkSize = fileSize / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        params[i].data = buffer.data() + i * chunkSize;
        params[i].length = (i == numThreads - 1) ? (fileSize - i * chunkSize) : chunkSize;
        params[i].threadID = i;
        params[i].fileHandle = hFile;

        overlappedStructs[i] = {};
        overlappedStructs[i].Offset = i * chunkSize;
        overlappedStructs[i].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        params[i].overlapped = overlappedStructs[i];

        threads[i] = CreateThread(NULL, 0, [](LPVOID params) -> DWORD {
            ThreadParams* threadParams = (ThreadParams*)params;

            HANDLE hFile = threadParams->fileHandle;

            OVERLAPPED* overlapped = &threadParams->overlapped;
            DWORD bytesRead = 0;
            BOOL result = WriteFile(hFile, threadParams->data, threadParams->length, &bytesRead, overlapped);

            if (!result && GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(overlapped->hEvent, INFINITE);
                GetOverlappedResult(hFile, overlapped, &bytesRead, FALSE);
            }

            CloseHandle(overlapped->hEvent);
            return 0;
            }, &params[i], 0, NULL);
    }

    WaitForMultipleObjects(numThreads, threads.data(), TRUE, INFINITE);

    for (HANDLE thread : threads) {
        CloseHandle(thread);
    }

    CloseHandle(hFile);
}

DWORD WINAPI ThreadFunction(LPVOID params) {
    ThreadParams* threadParams = (ThreadParams*)params;
    char* data = threadParams->data;
    size_t length = threadParams->length;

    size_t numElements = length / sizeof(char);
    
    char key = '0';
    for (size_t i = 0; i < numElements; ++i) {
        *(data + i) ^= key;
    }

    std::cout << "Поток " << threadParams->threadID << ": Шифрование завершено" << std::endl;
    return 0;
}

void ProcessFileMultiThreaded(char* data, size_t length, int numThreads) {
    std::vector<HANDLE> threads(numThreads);
    std::vector<ThreadParams> params(numThreads);

    size_t chunkSize = length / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        params[i].data = data + i * chunkSize;
        params[i].length = (i == numThreads - 1) ? (length - i * chunkSize) : chunkSize;
        params[i].threadID = i;

        threads[i] = CreateThread(NULL, 0, ThreadFunction, &params[i], 0, NULL);
    }

    WaitForMultipleObjects(numThreads, threads.data(), TRUE, INFINITE);

    for (HANDLE thread : threads) {
        CloseHandle(thread);
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    const char* filename = "big_data.txt";
    size_t fileSize;
    size_t threads_amount = 1;
    int work;
    std::cin >> work;
    std::cout << "Количество потоков: ";
    std::cin >> threads_amount;
    threads_amount = threads_amount > 0 ? threads_amount : 1;
    if (work & 1) {
        //Отображение
        std::cout << "Отображение файла в память и многопоточная обработка:" << std::endl;
        startTime = std::chrono::high_resolution_clock::now();
        char* mappedData = (char*)MapFileToMemory(filename, fileSize);
        if (mappedData) {
            ProcessFileMultiThreaded(mappedData, fileSize, threads_amount);
            UnmapViewOfFile(mappedData);
        }
        else {
            std::cerr << "Ошибка чтения";
            return 0;
        }
        double time = GetElapsedTime();
        std::cout << "Время выполнения: " << time << " секунд" << std::endl;
    }

    if (work & 2) {
        //Не отображение
        std::cout << "\nОбычная работа с файлом и многопоточная обработка:" << std::endl;
        std::vector<char> fileData;
        startTime = std::chrono::high_resolution_clock::now();
        //SimpleReadFile(filename, fileData);
        SimpleReadFileMultiThreaded(filename, fileData, threads_amount);
        ProcessFileMultiThreaded(fileData.data(), fileData.size(), threads_amount);
        SimpleWriteFileMultiThreaded(filename, fileData, threads_amount);
        //SimpleWriteFile(filename, fileData);
        double time = GetElapsedTime();
        std::cout << "Время выполнения: " << time << " секунд" << std::endl;
    }
    return 0;
}