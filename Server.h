#pragma once
#include <set>
#include <string>
#include <thread>
#include <netinet/in.h>

class Server {
private:
    int serverSocket = 0;
    int serverPort;
    int maxConnections;
    bool running = true;
    sockaddr_in serverAddress{};
    std::set<int> clientConnections;
    std::thread connectorThread;

    void handleClientConnection();

    void handleClientMessage(int clientSocket);

    void handleServerInput();

public:
    Server();

    Server(int port);

    Server(int port, int maxConnections);

    void start();

    void stop();

    void sendMessage(const std::string &message);

    std::string getServerAddress() const;

    int getServerPort() const;
};
