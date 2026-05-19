#pragma once
#include <string>

class Client {
private:
    int clientSocket = 0;
    int serverPort;
    std::string serverIP;
    bool running = true;

public:
    Client(std::string ip, int port);

    void join();

    void disconnect();

    void sendMessage(const std::string &message) const;
};
