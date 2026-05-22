#pragma once
#include <string>
#include <thread>
#include <openssl/types.h>

class Client {
private:
    int serverPort;
    std::string serverIP;
    int clientSocket = 0;
    bool running = false;
    SSL_CTX *sslContext = nullptr;
    SSL *sslConnection = nullptr;
    std::thread serverMessageThread;

    void closeConnection();

    void prepareTLSConnection();

    void createTLSConnection();

    void handleServerMessage();

    void handleClientInput();

public:
    Client(std::string ip, int port);

    void join();

    void disconnect();

    void sendMessage(const std::string &message) const;
};
