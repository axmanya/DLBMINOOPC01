#include "Client.h"

#include <iostream>
#include <ostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "LogManager.h"

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
    LogManager::getInstance()->Information("Initialize secure TLS Socket");

    // loading os ssl error strings and algorithm array for OpenSSL
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // prepare the SSL Context for clientside TLS method returns status 1 if successful
    // https://docs.openssl.org/3.1/man3/SSL_CTX_new
    sslContext = SSL_CTX_new(TLS_client_method());
    if (sslContext == nullptr) {
        LogManager::getInstance()->Error("Failed to create SSL context: " + std::to_string(errno));
        exit(-1);
    }
}

/**
 * Creates a secure SSL / TLS connection using OpenSSL and created context
 * verifies that a secure connection can be established
 */
void Client::createTLSConnection() {
    LogManager::getInstance()->Information("Create secure connection using TLS");

    // create a ssl context structure which can be used for the connection
    sslConnection = SSL_new(sslContext);
    if (sslConnection == nullptr) {
        closeConnection();
        LogManager::getInstance()->Error("Failed to create SSL connection:" + std::to_string(errno));
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
        LogManager::getInstance()->Error("Failed to connect to server: " + std::to_string(errno));
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
                LogManager::getInstance()->Error(
                    "Server disconnected or SSL read failed. SSL error: " + std::to_string(errno));
            disconnect();
            break;
        }

        LogManager::getInstance()->Information("", buffer);
    }
}

/**
 * Main Thread to handle the client input commands, allows sending messages to server
 * or to send commands to the client for management, will run as long as the server and client is running
 */
void Client::handleClientInput() {
    while (running) {
        // define a message buffer and read the server input as entire line
        std::string message;
        std::getline(std::cin, message);

        // check for command
        if (message.starts_with("/")) {
            if (message == "/stop")
                disconnect();
            else
                LogManager::getInstance()->Error("Unknown command");
        } else {
            sendMessage(message);
        }
    }
}

/**
 * Constructor for connection requires ip and port to establish a connection to the server
 * @param ip given port where server is listening
 * @param port given port where server is listening
 */
Client::Client(std::string ip, int port) : serverIP(std::move(ip)), serverPort(port) {
}

/**
 * Creates a client socket using SSL / TLS for secure connection
 * connects to the server on a specified IP and port
 */
void Client::join() {
    LogManager::getInstance()->setLogPrefix( "Client");
    LogManager::getInstance()->Information("Starting Client");

    // prepare OpenSSL connection for SSL / TLS
    prepareTLSConnection();

    // create a server socket with base parameters:
    // AF_INET = ipv4
    // SOCK_STREAM = TCP / IP
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        LogManager::getInstance()->Error("Failed to create client socket");
        exit(-1);
    }

    // define a server address structure for ipv4 with the given port that accepts any incoming address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0) {
        closeConnection();
        LogManager::getInstance()->Error("Failed to bind client socket");
        exit(-1);
    }

    LogManager::getInstance()->Information("Connecting to server " + serverIP + ":" + std::to_string(serverPort));

    // connect to the server socket, returns 1 if successful
    int serverSocket = connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress));
    if (serverSocket == -1) {
        closeConnection();
        LogManager::getInstance()->Error("Failed to accept client connection: " + std::to_string(errno));
        exit(-1);
    }

    createTLSConnection();
    LogManager::getInstance()->Information("SSL connection established");
    running = true;

    // push message receiving to background thread to release main thread for sending messages
    serverMessageThread = std::thread(&Client::handleServerMessage, this);
    handleClientInput();
}

/**
 * Stop the client server connection and clean up SSL / TLS connections
 */
void Client::disconnect() {
    LogManager::getInstance()->Information("Disconnecting from server");
    running = false;
    closeConnection();

    // ensure background threads are joining again to allow shutdown
    if (serverMessageThread.joinable() && serverMessageThread.get_id() != std::this_thread::get_id()) {
        serverMessageThread.join();
    }

    exit(0);
}

/**
 * Send a message from the client to the connected server socket using SSL / TLS
 * @param message defines the message to send
 */
void Client::sendMessage(const std::string &message) const {
    LogManager::getInstance()->Debug("Sending message to server: " + message);

    // send message using SSL / TLS to the server, returns sending status, if 1 it was successful
    // https://docs.openssl.org/3.1/man3/SSL_write
    int sendStatus = SSL_write(sslConnection, message.c_str(), static_cast<int>(message.length()));
    if (sendStatus == 0) {
        LogManager::getInstance()->Information("Socket was closed");
    } else if (sendStatus <= -1)
        LogManager::getInstance()->Error("Could not send message to server" + std::string(std::strerror(errno)));
}
