#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_STRING "Server: Windows HTTP Server/1.0\r\n"
#define BUFFER_SIZE 1024

class WebServer {
private:
    SOCKET serverSocket;
    u_short port;
    std::mutex coutMutex;

    //Thread safe logging
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << message << std::endl;
    }

    //Initialize Winsock
    bool initializeWinsock() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            log("WSAStartup failed");
            return false;
        }
        return true;
    }

    //Handle client requests        
    void handleClient(SOCKET clientSocket) {
        char buffer[BUFFER_SIZE];
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string request(buffer);
            
            // Parse HTTP request
            std::istringstream requestStream(request);
            std::string method, path, version;
            requestStream >> method >> path >> version;

            // Handle request
            if (method == "GET") {
                serveFile(clientSocket, path);
            } else {
                sendError(clientSocket, 501, "Method Not Implemented");
            }
        }

        closesocket(clientSocket);
    }

    //Serve files
    void serveFile(SOCKET clientSocket, const std::string& path) {
        std::string filePath = "httpdocs" + path;
        if (filePath.back() == '/') {
            filePath += "index.html";
        }

        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            sendError(clientSocket, 404, "Not Found");
            return;
        }

        // Send HTTP headers
        std::string headers = "HTTP/1.1 200 OK\r\n";
        headers += SERVER_STRING;
        headers += "Content-Type: text/html\r\n";
        headers += "\r\n";
        send(clientSocket, headers.c_str(), headers.length(), 0);

        // Send file content
        std::vector<char> buffer(1024);
        while (file.read(buffer.data(), buffer.size())) {
            send(clientSocket, buffer.data(), file.gcount(), 0);
        }
        if (file.gcount() > 0) {
            send(clientSocket, buffer.data(), file.gcount(), 0);
        }
    }

    //Send error response
    void sendError(SOCKET clientSocket, int code, const std::string& message) {
        std::string response = "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n";
        response += SERVER_STRING;
        response += "Content-Type: text/html\r\n";
        response += "\r\n";
        response += "<html><body><h1>" + std::to_string(code) + " " + message + "</h1></body></html>";
        send(clientSocket, response.c_str(), response.length(), 0);
    }

public:
    WebServer(u_short port = 8080) : port(port), serverSocket(INVALID_SOCKET) {}

    //Start the server
    bool start() {
        if (!initializeWinsock()) {
            return false;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            log("Socket creation failed");
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            log("Bind failed");
            return false;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            log("Listen failed");
            return false;
        }

        log("Server started on port " + std::to_string(port));

        while (true) {
            SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                log("Accept failed");
                continue;
            }

            std::thread clientThread(&WebServer::handleClient, this, clientSocket);
            clientThread.detach();
        }

        return true;
    }

    //Destructor
    ~WebServer() {
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
        }
        WSACleanup();
    }
};

int main() {
    WebServer server(8080);
    if (!server.start()) {
        std::cerr << "Server failed to start" << std::endl;
        return 1;
    }
    return 0;
}
