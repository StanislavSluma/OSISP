#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 54000

DWORD WINAPI receiveMessages(LPVOID serverSocket) {
    SOCKET sock = *(SOCKET*)serverSocket;
    char buf[4096];

    while (true) {
        ZeroMemory(buf, 4096);
        int bytesReceived = recv(sock, buf, 4096, 0);
        if (bytesReceived > 0) {
            std::string msg(buf, 0, bytesReceived);
            std::cout << msg << std::endl;
        }
    }
    return 0;
}

int main() {
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverHint;
    serverHint.sin_family = AF_INET;
    serverHint.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverHint.sin_addr);

    if (connect(serverSocket, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed." << std::endl;
        return -1;
    }

    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    send(serverSocket, name.c_str(), name.size() + 1, 0);

    CreateThread(nullptr, 0, receiveMessages, &serverSocket, 0, nullptr);

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input == "/quit") {
            break;
        }
        send(serverSocket, input.c_str(), input.size() + 1, 0);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
