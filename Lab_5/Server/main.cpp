#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_PORT 54000

struct Client {
    SOCKET socket;
    std::string name;
};

std::vector<Client*> clients;
CRITICAL_SECTION clients_cs;

void broadcast(const std::string& message, SOCKET sender = INVALID_SOCKET) {
    EnterCriticalSection(&clients_cs);
    for (const auto& client : clients) {
        if (client->socket != sender) {
            send(client->socket, message.c_str(), message.size() + 1, 0);
        }
    }
    LeaveCriticalSection(&clients_cs);
}

DWORD WINAPI handle_client(LPVOID client_info) {
    Client* client = (Client*)client_info;
    char buf[4096];

    std::string connectMsg = client->name + " has connected.";
    std::cout << connectMsg << std::endl;
    broadcast(connectMsg);

    while (true) {
        ZeroMemory(buf, 4096);
        int bytesReceived = recv(client->socket, buf, 4096, 0);
        if (bytesReceived <= 0) {
            EnterCriticalSection(&clients_cs);
            closesocket(client->socket);
            clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
            LeaveCriticalSection(&clients_cs);

            std::string disconnectMsg = client->name + " has disconnected.";
            std::cout << disconnectMsg << std::endl;
            broadcast(disconnectMsg);

            delete client;
            break;
        }

        std::string msg = client->name + ": " + std::string(buf, 0, bytesReceived);
        std::cout << msg << std::endl;
        broadcast(msg, client->socket);
    }
    return 0;
}

int main() {
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    InitializeCriticalSection(&clients_cs);

    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverHint;
    serverHint.sin_family = AF_INET;
    serverHint.sin_port = htons(SERVER_PORT);
    serverHint.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(listening, (sockaddr*)&serverHint, sizeof(serverHint));
    listen(listening, SOMAXCONN);

    std::cout << "Server started on port " << SERVER_PORT << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listening, nullptr, nullptr);

        char nameBuf[64];
        recv(clientSocket, nameBuf, 64, 0);
        std::string clientName = nameBuf;

        Client* newClient = new Client{ clientSocket, clientName };
        EnterCriticalSection(&clients_cs);
        clients.push_back(newClient);
        LeaveCriticalSection(&clients_cs);

        CreateThread(nullptr, 0, handle_client, newClient, 0, nullptr);
    }

    closesocket(listening);
    DeleteCriticalSection(&clients_cs);
    WSACleanup();
    return 0;
}
