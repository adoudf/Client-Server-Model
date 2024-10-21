# Client-Server-Model
# TCP/UDP Server-Client Application

## Description
This project implements a simple TCP/UDP server-client application that allows multiple clients to connect to a server. Clients can send messages, upload files, download files, and retrieve their client ID. The server broadcasts messages received from clients to all connected clients using UDP.

## Features
- **TCP Connection**: Establishes a reliable connection between the server and clients.
- **File Transfer**: Supports uploading and downloading files.
- **Client Identification**: Clients can retrieve their unique client ID.
- **Message Broadcasting**: Messages sent by clients are broadcasted to all other clients.

## Installation Instructions
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd <repository-directory>
   
## Compile 
-gcc -o server server.c -lpthread
-gcc -o client client.c -lpthread

