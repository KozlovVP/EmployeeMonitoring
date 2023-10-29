#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <future>
#include <vector>
#include <fstream>
#include <map>
#include <string>
#include <ws2tcpip.h>
#include <thread> // Добавлен заголовочный файл для работы с потоками

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 6387)

std::map<std::string, SOCKET> clients;
std::map<int, std::string> name_id;\

// Асинхронная функция для отправки сообщения по сокету
void sendMessage()
{
    // getting client id 
    int clientId;
    std::cout << "Enter client id: ";
    std::cin >> clientId;
    std::string command;
    std::cout << "Enter 1 if you want to see last activity" << std::endl;
    std::cout << "Enter 2 if you want to get a screenshot" << std::endl;
    std::cout << "Your command (enter 1 or 2): " << std::endl;
    std::cin >> command;

    // Отправка сообщения по сокету
    std::string message = command;
    int bytesSent = send(clients[name_id[clientId-1]], message.c_str(), message.size(), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Error while sending query" << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Query sent successfully" << std::endl;
    }

    if (command == "1") {
        // Обработка сообщения от клиента
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clients[name_id[clientId - 1]], buffer, sizeof(buffer), 0);
        std::string time(buffer);
        std::cout << "Last activity time: " << time << std::endl;
    }

    if (command == "2") {
        // Получение скриншота от клиента
        std::ofstream file("received.png", std::ios::binary);

        char buffer[1024];
        int bytesRead;
        while ((bytesRead = recv(clients[name_id[clientId - 1]], buffer, sizeof(buffer), 0)) > 0) {
            file.write(buffer, bytesRead);
        }

        file.close();

        // Открытие скриншота в новом окне
        const wchar_t* filePath = L"received.png";
        ShellExecute(NULL, L"open", filePath, NULL, NULL, SW_SHOWNORMAL);
    }
}

int main() {
    // Инициализация библиотеки Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error while initializing Winsock" << std::endl;
        return 1;
    }

    // Создание сокета
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error while creating server socket" << std::endl;
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr));
    serverAddress.sin_port = htons(8080); // Порт сервера

    // Привязка сокета к адресу сервера
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Error while binding server socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Прослушивание входящих соединений
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error while listening to socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is launched. Waiting for connections" << std::endl;

    std::cout << "Clients:" << std::endl;
    int user_with_number = 0;
    while (true) {
        // Принятие входящего соединения
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept error" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        // Обработка сообщения от клиента
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        std::string user_name(buffer);

        // Добавление клиента в массив
        clients[user_name] = clientSocket;
        name_id[user_with_number] = user_name;
        std::cout << clients.size() << ") " << user_name << std::endl;

        // Запуск асинхронной функции для отправки сообщения клиенту в отдельном потоке
        std::thread sendMessageThread(sendMessage);
        sendMessageThread.detach();

        user_with_number += 1;
    }

    // Закрытие сокета сервера
    closesocket(serverSocket);

    // Освобождение ресурсов Winsock
    WSACleanup();

    return 0;
}
