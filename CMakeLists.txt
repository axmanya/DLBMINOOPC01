cmake_minimum_required(VERSION 4.2)
project(DLBMINOOPC01_Aufgabe1)

set(CMAKE_CXX_STANDARD 23)

# find OPENSSL package and throw error if not found, to ensure build process succeeds
find_package(OpenSSL REQUIRED)

add_executable(DLBMINOOPC01_Aufgabe1 main.cpp
        Server.h
        Server.cpp
        Client.h
        Client.cpp
)

# link OpenSSL libaries to the project, this is required to use them
target_link_libraries(DLBMINOOPC01_Aufgabe1 PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Copy server key and certificate to output directory to prevent server crash during startup
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/server.key  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/server.crt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
