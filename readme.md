# Prerequisites

The server and clients will only run on unix systems (linux or mac os).
Additionally, the OpenSSL Library needs to be installed as explained in the next section.

On Mac OS X the OpenSSL can be installed using homebrew:

```Shell 
brew install openssl
```

For Linux the OpenSSL can be installed using apt or yum:

```bash
sudo apt update
sudo apt install libssl-dev
```

# Starting Project

1. run **1_generate_certificate.sh** to ensure a current new certificate is generated
2. run **2_build.sh** to build the project using the CMakeLists.txt
3. run **3_start_server.sh** to start the server
4. run **4_start_client.sh** to start the client

This will start the server and client and will connect to the server for the demonstration.
If more than one client is needed, start the 4_start_client.sh script multiple times.

# Base Knowledge
## Networking in C++

Networking in C++ follows a server client principle using the socket.h library on unix systems (linux or mac os) and
winsock.h on Windows systems.

### Communication Pattern

Client server communication follows the following pattern:

| Client  |        | Server |
|---------|--------|--------|
| socket  |        | socket |
| bind    |        | bind   |
|         |        | listen |
| connect | -----> | accept |
| send    | -----> | recv   |
| recv    | <----- | send   |

The sever listens for connections and serves them he receives messages and send messages back to the client.
At the end of the communication, the socket will be closed again and the cycle start again.

### SSL Communication

SSL can be used to secure the communication between the client and the server. For this the OpenSSL is often used and
the development package needs to be installed.

On Mac OS X the OpenSSL can be installed using homebrew:

```Shell 
brew install openssl
```

For Linux the OpenSSL can be installed using apt or yum:

```bash
sudo apt update
sudo apt install libssl-dev
```

Update your CMAKE file to include the OpenSSL:

```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(project_chat_server PRIVATE OpenSSL::SSL OpenSSL::Crypto)
```

# References

- Küveler, G. & Schwoch, D. (2017). C/C++ für Studium und Beruf: Eine Einführung mit vielen Beispielen, Aufgaben und
  Lösungen. Springer. https://link.springer.com/book/10.1007/978-3-658-18581-7
- Küveler, G. & Hoch, T. (2019). C/C++ anwenden:Technisch-wissenschaftliche Übungsaufgaben mit Lösungen.
  Springer. https://link.springer.com/book/10.1007/978-3-658-22165-2
- Sonigara, N. GeeksForGeeks. https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/
- Aticleworld (2020). ssl server client programming using openssl in
  c. https://aticleworld.com/ssl-server-client-using-openssl-in-c/
- OpenSSL Project. OpenSSL Manual Pages. https://docs.openssl.org/