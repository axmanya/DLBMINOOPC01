#include "Client.h"
#include "Server.h"

int main(int argc, char *argv[]) {
    std::string command = argv[1];
    if (command == "server") {
        Server server;
        server.start();
    } else if (command == "client") {
        Client client(argv[2], std::stoi(argv[3]));
        client.join();
    }
    return 0;
}
