#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>

// Define buffer size and command size
#define BUFFER_SIZE 256
#define COMMAND_SIZE 10

// Handle client request
void handle_client(int client_Socket)
{
    char command[COMMAND_SIZE];
    // Receive command from client
    recv(client_Socket, command, COMMAND_SIZE, 0);

    // Handle put command
    if(strcmp(command, "put") == 0)
    {
        char filename[256];
        // Receive filename from client
        recv(client_Socket, filename, 256, 0);

        // Open file in binary write mode
        FILE *file = fopen(filename, "wb");
        if(file == NULL)
        {
            printf("Failed to create file\n");
            return;
        }

        // Receive file in chunks from client and write to file
        char buffer[BUFFER_SIZE];
        int bytes_received;
        while((bytes_received = recv(client_Socket, buffer, BUFFER_SIZE, 0)) > 0)
        {
            fwrite(buffer, 1, bytes_received, file);
        }

        fclose(file);
    }
    // Handle get command
    else if(strcmp(command, "get") == 0)
    {
        char filename[256];
        // Receive filename from client
        recv(client_Socket, filename, 256, 0);

        // Open file in binary read mode
        FILE *file = fopen(filename, "rb");
        if(file == NULL)
        {
            printf("File not found\n");
            return;
        }

        // Read file in chunks and send to client
        char buffer[BUFFER_SIZE];
        int bytes_read;
        while((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
        {
            send(client_Socket, buffer, bytes_read, 0);
        }

        fclose(file);

        int fileTermination = remove(filename);
        if(fileTermination == 0)
        {
            printf("File Successefuly Sent to Client\n");
        }
        else{
            printf("File has not been deleted from the server, Incomplete file transfer ! !\n");
        }

    }
    else
    {
        printf("Invalid command\n");
        return;
    }
}

int main()
{
    int server_Backlog = 4;
    // Create a new TCP socket using the IPv4 protocol
    int Network_Socket_Server = socket(AF_INET, SOCK_STREAM, 0);

    // Specify address for socket
    struct sockaddr_in server_Address;
    server_Address.sin_family = AF_INET; // IPv4 protocol
    server_Address.sin_port = htons(9001); // Port number
    server_Address.sin_addr.s_addr = INADDR_ANY; // IP address 0.0.0.0, utilizing any available IP address of local machine

    // Bind socket to address
    bind(Network_Socket_Server, (struct sockaddr*)&server_Address, sizeof(server_Address));

    // Listen for incoming connections
    listen(Network_Socket_Server, server_Backlog);

      printf("Welcome to the server standby for incoming connections ...\n");

    while(1)
    {
        // Accept incoming connection
        int client_Socket = accept(Network_Socket_Server, NULL, NULL);

        // Fork a new process to handle the client
        if(fork() == 0)
        {
            close(Network_Socket_Server);
            handle_client(client_Socket);
            close(client_Socket);
            exit(0);
        }
        else
        {
            close(client_Socket);
        }
    }

    return 0;
}
