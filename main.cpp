#include "Client.h"
#include "LogManager.h"
#include "Server.h"

int main(int argc, char *argv[]) {
    LogManager::getInstance()->setLogLevel(LogLevel::INF);
    LogManager::getInstance()->setLogToConsole(true);

    if (argc < 2) {
        LogManager::getInstance()->Error("Invalid command first argument either server or client");
        exit(-1);
    }

    std::string command = argv[1];
    if (command == "server") {
        LogManager::getInstance()->setLogToFile(true);
        LogManager::getInstance()->setLogFilePath("");
        LogManager::getInstance()->setLogFileName("log.txt");

        Server server;
        server.start();
    } else if (command == "client") {
        if (argc < 3) {
            LogManager::getInstance()->Error("Invalid command second argument ip");
            return -1;
        }

        if (argc < 4) {
            LogManager::getInstance()->Error("Invalid command third argument port");
            return -1;
        }

        Client client(argv[2], std::stoi(argv[3]));
        client.join();
    } else {
        LogManager::getInstance()->Error("Invalid command first argument either server or client");
        return -1;
    }
    return 0;
}
