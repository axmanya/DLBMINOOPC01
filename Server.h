#pragma once
#include <map>
#include <string>
#include <thread>
#include <netinet/in.h>
#include <openssl/types.h>

class Server {
private:
    int serverSocket = 0;
    sockaddr_in serverAddress{};
    int serverPort;
    int maxConnections;
    bool running = false;
    SSL_CTX *sslContext = nullptr;
    std::map<int, SSL *> clientConnections;
    std::thread connectorThread;

    void closeConnection();

    void loadTLSCertificate();

    void prepareTLSConnection();

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
