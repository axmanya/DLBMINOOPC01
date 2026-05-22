#include "Server.h"

#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

void Server::handleClientConnection() {
    while (running) {
        // accept incoming connection, returns an integer value representing the status code of the connection
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        // if socket status is valid so equals to 1
        if (clientSocket != -1) {
            clientConnections.insert(clientSocket);
            sendMessage("Welcome Client " + std::to_string(clientSocket) + "!");
            std::cout << "Client connected: " << clientSocket << std::endl;

            // push reading of client messages to background thread
            // detach the thread to allow the server to continue accepting new connections
            std::thread clientThread(&Server::handleClientMessage, this, clientSocket);
            clientThread.detach();
        }
    }
}

void Server::handleClientMessage(int clientSocket) {
    char clientBuffer[1024] = {};

    while (running) {
        // ensure the buffer is empty otherwise pieces from previous messages still exist
        memset(clientBuffer, 0, sizeof(clientBuffer));

        // find the client connection using the given socket as key
        auto connection = clientConnections.find(clientSocket);
        if (connection == clientConnections.end()) {
            break;
        }

        // check if the socket has been closed beforehand and ensure the program closes the connection cleanly
        ssize_t readStatus = recv(clientSocket, clientBuffer, sizeof(clientBuffer), 0);
        if (readStatus <= 0) {
            if (readStatus == 0)
                std::cout << "Client disconnected: " << clientSocket << std::endl;
            else
                std::cerr << "Could not read from client " + std::to_string(clientSocket) + ": " + std::string(
                    std::strerror(errno));

            close(clientSocket);
            clientConnections.erase(clientSocket);
            sendMessage("Client " + std::to_string(clientSocket) + " disconnected");
            break;
        }
        sendMessage(clientBuffer);
    }
}

void Server::handleServerInput() {
    while (running) {
        // define a message buffer and read the server input as entire line
        std::string message;
        std::getline(std::cin, message);
        if (message.starts_with("/")) {
            if (message == "/stop")
                stop();
            else
                std::cout << "Unknown command: " << message << std::endl;
        } else {
            sendMessage(message);
        }
    }
}

Server::Server() : serverPort(9000), maxConnections(10) {
}

Server::Server(int port) : serverPort(port), maxConnections(10) {
}

Server::Server(int port, int maxConnections) : serverPort(port), maxConnections(maxConnections) {
}

void Server::start() {
    std::cout << "Server starting..." << std::endl;

    // create a server socket with base parameters:
    // AF_INET = ipv4
    // SOCK_STREAM = TCP / IP
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create server socket: " + std::string(std::strerror(errno));
        exit(-1);
    }

    // define a server address structure for ipv4 with the given port that accepts any incoming address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to the port and network, returns status 1 if successful
    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
        close(serverSocket);
        std::cerr << "Failed to bind server socket: " + std::string(std::strerror(errno));
        exit(-1);
    }

    // listen for max incoming connections, returns status 1 if successful
    if (listen(serverSocket, maxConnections) < 0) {
        close(serverSocket);
        std::cerr << "Failed to listen on server socket: " + std::string(std::strerror(errno));
        exit(-1);
    }

    std::cout << "Server started on " << getServerAddress() << ":" << getServerPort() << std::endl;

    // Start a background job to handle client connections separately using the reference of the current object
    connectorThread = std::thread(&Server::handleClientConnection, this);
    handleServerInput();
}

void Server::stop() {
    sendMessage("Stopping server...");
    running = false;
    close(serverSocket);

    // loop the client connection pool and close
    for (int clientSocket: clientConnections) {
        close(clientSocket);
    }

    // clear memory from client connections
    clientConnections.clear();

    // ensure background threads are joining again to allow shutdown
    if (connectorThread.joinable()) {
        connectorThread.join();
    }

    exit(0);
}

void Server::sendMessage(const std::string &message) {
    if (!running) return;

    std::cout << message << std::endl;
    for (int clientSocket: clientConnections) {
        ssize_t sendStatus = send(clientSocket, message.c_str(), message.length(), 0);
        if (sendStatus <= 0)
            std::cerr << "Could not send message to client " + std::to_string(clientSocket) + ": " + std::string(
                std::strerror(errno));
    }
}

std::string Server::getServerAddress() const {
    return inet_ntoa(serverAddress.sin_addr);
}

int Server::getServerPort() const {
    return serverPort;
}
