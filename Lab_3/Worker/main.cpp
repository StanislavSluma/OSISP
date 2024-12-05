#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#define BUFFER_SIZE 1000000

std::string ProcessTask(const std::string& task) {
    std::istringstream iss(task);
    std::vector<int> array;
    int num;

    while (iss >> num) {
        array.push_back(num);
    }

    std::sort(array.begin(), array.end());

    std::ostringstream oss;
    for (int n : array) {
        oss << n << " ";
    }

    return oss.str();
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "ru");
    if (argc < 2) {
        std::cerr << "������: �� ������� ����� ������" << std::endl;
        return 1;
    }

    std::string pipeName = "\\\\.\\pipe\\WorkerPipe" + std::string(argv[1]);

    HANDLE hPipe;
    DWORD bytesWritten, bytesRead;
    char buffer[BUFFER_SIZE];

    hPipe = CreateNamedPipeA(pipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, BUFFER_SIZE, BUFFER_SIZE, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "�� ������� ������� �����" << std::endl;
        return 1;
    }

    std::cout << "�������� ����������� ���������� �� ������ " << pipeName << "..." << std::endl;
    ConnectNamedPipe(hPipe, NULL);

    ReadFile(hPipe, buffer, BUFFER_SIZE, &bytesRead, NULL);
    std::string task(buffer);
    std::cout << "\n\n�������� �������: "; // << task << std::endl;

    std::string result = ProcessTask(task);

    WriteFile(hPipe, result.c_str(), result.size() + 1, &bytesWritten, NULL);

    CloseHandle(hPipe);

    return 0;
}
