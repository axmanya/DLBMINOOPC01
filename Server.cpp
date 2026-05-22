#include "Server.h"

#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/ssl.h>

#include "LogManager.h"

/**
 * Method to close the server connection including the SSL Context and OpenSSL resources
 */
void Server::closeConnection() {
    // clear memory from SSL Context
    if (sslContext != nullptr) {
        SSL_CTX_free(sslContext);
        sslContext = nullptr;
    }

    // clean SSL ciphers and digests to prevent side effects on OpenSSL after shutdown
    // https://docs.openssl.org/3.1/man3/OpenSSL_add_all_algorithms
    EVP_cleanup();
    close(serverSocket);
}

/**
* Load TLS Certificate using the OpenSSL library and use it for secure communication
* also verifies that the private key and certificate match before the connection is used
*
* @note <b>To generate a certificate use openssl library command:</b> <code>openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes</code>
*/
void Server::loadTLSCertificate() {
    LogManager::getInstance()->Information("Loading TLS certificate");

    // load generated public key / certificate into SSLContext returns status 1 if successful
    // Steps to load and verify the certificate: https://docs.openssl.org/3.1/man3/SSL_CTX_use_certificate
    if (SSL_CTX_use_certificate_file(sslContext, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        LogManager::getInstance()->Error("Failed to load certificate: " + std::to_string(errno));
        SSL_CTX_free(sslContext);
        exit(-1);
    }

    // load generated private key into SSLContext returns status 1 if successful
    if (SSL_CTX_use_PrivateKey_file(sslContext, "server.key", SSL_FILETYPE_PEM) <= 0) {
        LogManager::getInstance()->Error("Failed to load private key: " + std::to_string(errno));
        SSL_CTX_free(sslContext);
        exit(-1);
    }

    // verify that private key and certificate match returns status 1 if successful
    if (!SSL_CTX_check_private_key(sslContext)) {
        LogManager::getInstance()->Error("Private key does not match the certificate: " + std::to_string(errno));
        SSL_CTX_free(sslContext);
        exit(-1);
    }
}

/**
 * This method prepares the OpenSSL context, is setting options and context arguments
 * and ensures OpenSSL can build up a secure TLS connection
 */
void Server::prepareTLSConnection() {
    LogManager::getInstance()->Information("Initialize secure TLS Socket");

    // loading os ssl error strings and algorithm array for OpenSSL
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // prepare the SSL Context for serverside TLS method returns status 1 if successful
    // https://docs.openssl.org/3.1/man3/SSL_CTX_new
    sslContext = SSL_CTX_new(TLS_server_method());
    if (sslContext == nullptr) {
        LogManager::getInstance()->Error(
            "Failed to create SSL context: " + std::to_string(errno));
        exit(-1);
    }

    // this method is setting some options for the ssl context
    // SSL_OP_SINGLE_DH_USE = forces new ephemeral Diffie-Hellman (DH) key for every handshake, ensuring Forward Secrecy
    // https://docs.openssl.org/1.0.2/man3/SSL_CTX_set_options
    SSL_CTX_set_options(sslContext, SSL_OP_SINGLE_DH_USE);
}

/**
 * Method to handle client connections / accept them securely using SSL/TLS, will run as long as the server is running
 * Start separate handling of client messages to release the thread again for connection handling
 */
void Server::handleClientConnection() {
     LogManager::getInstance()->Information("Waiting for client connection");

    while (running) {
        // accept incoming connection, returns an integer value representing the status code of the connection
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        // if socket status is valid so equals to 1
        if (clientSocket != -1) {
            LogManager::getInstance()->Information("Create Secure Client Connection");

            // create a ssl context structure which can be used for the connection
            SSL *clientSSL = SSL_new(sslContext);
            if (clientSSL == nullptr) {
                LogManager::getInstance()->Error("Failed to create SSL context: " + std::to_string(errno));
                close(clientSocket);
                continue;
            }

            // bind the ssl structure to the socket and wrap the socket in TLS
            // https://docs.openssl.org/3.1/man3/SSL_set_fd
            SSL_set_fd(clientSSL, clientSocket);

            // perform the initial handshake with the client and accept the secure connection, returns 1 if successful
            // https://docs.openssl.org/3.1/man3/SSL_accept
            int sslStatus = SSL_accept(clientSSL);
            if (sslStatus <= 0) {
                if (sslStatus != 0)
                    LogManager::getInstance()->Error("Failed to accept client connection: " + std::to_string(errno));
                SSL_shutdown(clientSSL);
                SSL_free(clientSSL);
                close(clientSocket);
                continue;
            }

            // register the SSL Context of the client in the server connection pool
            clientConnections[clientSocket] = clientSSL;

            LogManager::getInstance()->Information("Client " + std::to_string(clientSocket) + " connected");
            sendMessage("[Server]Welcome Client " + std::to_string(clientSocket) + "!");

            // push reading of client messages to background thread
            // detach the thread to allow the server to continue accepting new connections
            std::thread clientThread(&Server::handleClientMessage, this, clientSocket);
            clientThread.detach();
        }
    }
}

/**
 * Method to handle incoming client messages received securely using SSL / TLS,  will run as long as the server is running
 *
 * @param clientSocket defines the client socket, which is used to read messages from
 */
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

        // read the SSL structure which will be used to handle the communication
        SSL *clientSSL = connection->second;

        // recv with SSL / TLS Socket from client, returns 1 if status is successful
        // https://docs.openssl.org/3.1/man3/SSL_read
        int readStatus = SSL_read(clientSSL, clientBuffer, sizeof(clientBuffer) - 1);

        // check if the socket has been closed beforehand and ensure the program closes the connection cleanly
        if (readStatus <= 0) {
            if (readStatus == 0)
                LogManager::getInstance()->Information("Client " + std::to_string(clientSocket) + " disconnected");
            else
                LogManager::getInstance()->Error(
                    "Could not receive message from client " + std::to_string(clientSocket));

            SSL_shutdown(clientSSL);
            SSL_free(clientSSL);
            close(clientSocket);
            clientConnections.erase(clientSocket);
            sendMessage("[Server]Client " + std::to_string(clientSocket) + " disconnected");
            break;
        }

        LogManager::getInstance()->Information("Client" + std::to_string(clientSocket), clientBuffer);

        // forward the message to all clients
        sendMessage("[Client" + std::to_string(clientSocket) + "]" + clientBuffer);
    }
}

/**
 * Main Thread to handle the server input commands, allows sending messages to all clients
 * or to send commands to the server for management, will run as long as the server is running
 */
void Server::handleServerInput() {
    while (running) {
        // define a message buffer and read the server input as entire line
        std::string message;
        std::getline(std::cin, message);

        // check for command
        if (message.starts_with("/")) {
            if (message == "/stop")
                stop();
            else if (message == "/list") {
                LogManager::getInstance()->Information("Connections: " + std::to_string(clientConnections.size()));
                std::string connections;
                for (auto client_connection: clientConnections)
                    connections += "Client" + std::to_string(client_connection.first) + "\t";
                LogManager::getInstance()->Information(connections);
            } else
                LogManager::getInstance()->Error("Unknown command");
        } else {
            LogManager::getInstance()->Information(message);
            sendMessage("[Server]" + message);
        }
    }
}

/**
 * Constructor with default values port 9000 and max connections of 10 clients
 */
Server::Server() : serverPort(9000), maxConnections(10) {
}

/**
 * Constructor with a default value for max connections of 10 clients with variable port
 *
 * @param port defines the target port for the server
 */
Server::Server(int port) : serverPort(port), maxConnections(10) {
}

/**
 * Constructor with no default values all are handed in as variables
 *
 * @param port defines the target port for the server
 * @param maxConnections defines the maximum number of clients that can connect to the server
 */
Server::Server(int port, int maxConnections) : serverPort(port), maxConnections(maxConnections) {
}

/**
 * Method to start the server, will initialize the server socket and ensure a secure SSL / TLS socket is created
 * The server will bind to the given port and listen for incoming connections.
 *
 * It will also handle client connections and communication in a separate Thread to release the main thread
 * for command listening
 */
void Server::start() {
    LogManager::getInstance()->setLogPrefix("Server");
    LogManager::getInstance()->Information("Starting server on port " + std::to_string(serverPort));

    // prepare OpenSSL connection for SSL / TLS
    prepareTLSConnection();
    loadTLSCertificate();

    // create a server socket with base parameters:
    // AF_INET = ipv4
    // SOCK_STREAM = TCP / IP
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        LogManager::getInstance()->Error("Failed to create server socket: " + std::string(std::strerror(errno)));
        exit(-1);
    }

    // define a server address structure for ipv4 with the given port that accepts any incoming address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to the port and network, returns status 1 if successful
    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
        closeConnection();
        LogManager::getInstance()->Error("Failed to bind server socket: " + std::string(std::strerror(errno)));
        exit(-1);
    }

    // listen for max incoming connections, returns status 1 if successful
    if (listen(serverSocket, maxConnections) < 0) {
        closeConnection();
        LogManager::getInstance()->Error("Failed to listen on server socket: " + std::string(std::strerror(errno)));
        exit(-1);
    }

    LogManager::getInstance()->Information("Server is started and listening on port " + std::to_string(serverPort));
    running = true;

    // Start a background job to handle client connections separately using the reference of the current object
    connectorThread = std::thread(&Server::handleClientConnection, this);
    handleServerInput();
}

/**
 * Stop the server and gracefully close all client connections
 */
void Server::stop() {
    LogManager::getInstance()->Information("Stopping server");
    sendMessage("[Server]Stopping server...");
    running = false;

    closeConnection();

    // loop the client connection pool and close each SSL / TLS socket
    for (auto &[clientSocket, clientSsl]: clientConnections) {
        if (clientSsl != nullptr) {
            SSL_shutdown(clientSsl);
            SSL_free(clientSsl);
        }
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

/**
 * Send a message from the server to all clients
 * @param message defines the message to send
 */
void Server::sendMessage(const std::string &message) {
    if (!running) return;

    LogManager::getInstance()->Debug(message);

    // send message to all connections in server pool for this loop map
    for (const auto &[clientSocket, clientSSL]: clientConnections) {
        // send message using SSL / TLS to the client, returns sending status, if 1 it was successful
        // https://docs.openssl.org/3.1/man3/SSL_write
        int sendStatus = SSL_write(clientSSL, message.c_str(), static_cast<int>(message.length()));
        if (sendStatus <= 0)
            LogManager::getInstance()->Error(
                "Could not send message to client " + std::to_string(clientSocket) + ": " + std::string(
                    std::strerror(errno)));
    }
}

/**
 * @return extracts the IP address of the server address structure
 */
std::string Server::getServerAddress() const {
    return inet_ntoa(serverAddress.sin_addr);
}

/**
 * @return extracts the port of the server and returns it
 */
int Server::getServerPort() const {
    return serverPort;
}
