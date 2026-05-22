#include "Client.h"

#include <iostream>
#include <ostream>
#include <utility>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


void Client::handleServerMessage() {
    char buffer[1024] = {};

    while (running) {
        // ensure the buffer is cleared before reading otherwise old message pieces are still there
        memset(buffer, 0, sizeof(buffer));

        ssize_t readStatus = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (readStatus <= 0) {
            if (readStatus != 0)
                std::cerr << "Could not read from server: " + std::to_string(errno) << std::endl;
            disconnect();
            break;
        }
        std::cout << buffer << std::endl;
    }
}

void Client::handleClientInput() {
    while (running) {
        std::string message;
        std::getline(std::cin, message);

        if (message.starts_with("/")) {
            if (message == "/exit")
                disconnect();
            else
                std::cout << "Unknown command: " << message << std::endl;
        } else {
            sendMessage(message);
        }
    }
}

Client::Client(std::string ip, int port) : serverIP(std::move(ip)), serverPort(port) {
}

void Client::join() {
    std::cout << "Try to join " << serverIP << ":" << serverPort << std::endl;

    // create a server socket with base parameters:
    // AF_INET = ipv4
    // SOCK_STREAM = TCP / IP
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Failed to create client socket" << std::strerror(errno) << std::endl;
        exit(-1);
    }

    // define a server address structure for ipv4 with the given port that accepts any incoming address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0) {
        close(clientSocket);
        std::cerr << "Failed to bind client socket" << std::strerror(errno) << std::endl;
        exit(-1);
    }

    // connect to the server socket, returns 1 if successful
    int serverSocket = connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress));
    if (serverSocket == -1) {
        close(clientSocket);
        std::cerr << "Failed to accept client connection: " + std::to_string(errno) << std::endl;
        exit(-1);
    }

    std::cout << "Connected to " << serverIP << ":" << serverPort << std::endl;

    // push message receiving to background thread to release main thread for sending messages
    serverMessageThread = std::thread(&Client::handleServerMessage, this);
    handleClientInput();
}

void Client::disconnect() {
    std::cout << "Disconnected from server" << std::endl;
    running = false;
    close(clientSocket);

    // ensure background threads are joining again to allow shutdown
    if (serverMessageThread.joinable() && serverMessageThread.get_id() != std::this_thread::get_id()) {
        serverMessageThread.join();
    }

    exit(0);
}

void Client::sendMessage(const std::string &message) const {
    ssize_t sendStatus = send(clientSocket, message.c_str(), message.length(), 0);
    if (sendStatus <= 0)
        std::cerr << "Could not send message to server: " + std::to_string(errno) << std::endl;
}