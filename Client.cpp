#include "Client.h"

#include <iostream>
#include <ostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/ssl.h>

/**
 * Closes the client connection cleanly and ensures TLS / SSL socket is also cleaned up
 */
void Client::closeConnection() {
    // clean SSL / TLS connection to release resources
    if (sslConnection != nullptr) {
        SSL_shutdown(sslConnection);
        SSL_free(sslConnection);
        sslConnection = nullptr;
    }

    // clean SSL / TLS context to release resources
    if (sslContext != nullptr) {
        SSL_CTX_free(sslContext);
        sslContext = nullptr;
    }

    close(clientSocket);
}

/**
 * This method prepares the OpenSSL context
 * and ensures OpenSSL can build up a secure TLS connection
 */
void Client::prepareTLSConnection() {
    std::cout << "Prepare TLS Connection" << std::endl;

    // loading os ssl error strings and algorithm array for OpenSSL
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // prepare the SSL Context for clientside TLS method returns status 1 if successful
    // https://docs.openssl.org/3.1/man3/SSL_CTX_new
    sslContext = SSL_CTX_new(TLS_client_method());
    if (sslContext == nullptr) {
        std::cerr << "Failed to create SSL context: " + std::to_string(errno) << std::endl;
        exit(-1);
    }
}

/**
 * Creates a secure SSL / TLS connection using OpenSSL and created context
 * verifies that a secure connection can be established
 */
void Client::createTLSConnection() {
    std::cout << "Create TLS Connection" << std::endl;

    // create a ssl context structure which can be used for the connection
    sslConnection = SSL_new(sslContext);
    if (sslConnection == nullptr) {
        closeConnection();
        std::cerr << "Failed to create SSL connection: " + std::to_string(errno) << std::endl;
        exit(-1);
    }

    // bind the ssl structure to the socket and wrap the socket in TLS
    // https://docs.openssl.org/3.1/man3/SSL_set_fd
    SSL_set_fd(sslConnection, clientSocket);

    // perform the initial handshake with the server and accept the secure connection, returns 1 if successful
    // https://docs.openssl.org/3.1/man3/SSL_connect/
    int SSLStatus = SSL_connect(sslConnection);
    if (SSLStatus <= 0) {
        closeConnection();
        std::cerr << "Failed to connect to server: " + std::to_string(errno) << std::endl;
        exit(-1);
    }
}

/**
 * Method to handle incoming server messages received securely using SSL / TLS,  will run as long as the server and client is running
 * */
void Client::handleServerMessage() {
    char buffer[1024] = {};

    while (running) {
        // ensure the buffer is cleared before reading otherwise old message pieces are still there
        memset(buffer, 0, sizeof(buffer));

        // recv with SSL / TLS Socket from server, returns 1 if status is successful
        // https://docs.openssl.org/3.1/man3/SSL_read
        int readStatus = SSL_read(sslConnection, buffer, sizeof(buffer) - 1);
        if (readStatus <= 0) {
            if (readStatus != 0)
                std::cerr << "Server disconnected or SSL read failed: " + std::to_string(errno) << std::endl;
            disconnect();
            break;
        }

        std::cout << buffer << std::endl;
    }
}

/**
 * Main Thread to handle the client input commands, allows sending messages to server
 * or to send commands to the client for management, will run as long as the server and client is running
 */
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

    // prepare OpenSSL connection for SSL / TLS
    prepareTLSConnection();

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

    createTLSConnection();
    std::cout << "TLS Connection Established" << std::endl;
    running = true;

    // push message receiving to background thread to release main thread for sending messages
    serverMessageThread = std::thread(&Client::handleServerMessage, this);
    handleClientInput();
}

void Client::disconnect() {
    std::cout << "Disconnecting from Server..." << std::endl;
    running = false;
    closeConnection();

    // ensure background threads are joining again to allow shutdown
    if (serverMessageThread.joinable() && serverMessageThread.get_id() != std::this_thread::get_id()) {
        serverMessageThread.join();
    }

    exit(0);
}

void Client::sendMessage(const std::string &message) const {

    // send message using SSL / TLS to the server, returns sending status, if 1 it was successful
    // https://docs.openssl.org/3.1/man3/SSL_write
    int sendStatus = SSL_write(sslConnection, message.c_str(), static_cast<int>(message.length()));
    if (sendStatus == 0) {
        std::cerr << "Server disconnected or SSL write failed: " + std::to_string(errno) << std::endl;
    } else if (sendStatus <= -1)
        std::cerr << "Could not send message to server" + std::string(std::strerror(errno)) << std::endl;
}
