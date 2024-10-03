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

int main()
{
    // Create a new TCP socket using the IPv4 protocol
    int Network_Socket_Client = socket(AF_INET, SOCK_STREAM, 0);

    // Specify address for socket
    struct sockaddr_in server_Address;
    server_Address.sin_family = AF_INET; // IPv4 protocol
    server_Address.sin_port = htons(9001); // Port number
    server_Address.sin_addr.s_addr = INADDR_ANY; // IP address 0.0.0.0, utilizing any available IP address of local machine

    // Connect to the server
    int connection_Status = connect(Network_Socket_Client, (struct sockaddr*)&server_Address, sizeof(server_Address));
    if(connection_Status == -1)
    {
        printf("Connection failed\n");
        return 1;
    }
while(1)
{

    // Prompt user to enter command (put/get)
    char command[COMMAND_SIZE];
    printf("Enter command <put/get>: ");
    fgets(command, COMMAND_SIZE, stdin);
    command[strcspn(command, "\n")] = 0; // remove newline character

    // Handle put command
    if(strcmp(command, "put") == 0)
    {
        char filename[256];
        printf("Enter the filename: ");
        fgets(filename, 256, stdin);
        filename[strcspn(filename, "\n")] = 0; // remove newline character

        // Send command and filename to server
        send(Network_Socket_Client, command, COMMAND_SIZE, 0);
        send(Network_Socket_Client, filename, 256, 0);

        // Open file in binary read mode
        FILE *file = fopen(filename, "rb");
        if(file == NULL)
        {
            printf("File not found\n");
            return 1;
        }

        // Read file in chunks and send to server
        char buffer[BUFFER_SIZE];
        int bytes_read;
        while((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
        {
            send(Network_Socket_Client, buffer, bytes_read, 0);
        }

        fclose(file);

        int fileTermination = remove(filename);
        if(fileTermination == 0)
        {
            printf("File Successefuly Sent to server\n");
        }
        else{
            printf("File has not been deleted from the client, Incomplete file transfer ! !\n");
        }

    }
    // Handle get command
    else if(strcmp(command, "get") == 0)
    {
        char filename[256];
        printf("Enter filename: ");
        fgets(filename, 256, stdin);
        filename[strcspn(filename, "\n")] = 0; // remove newline character

        // Send command and filename to server
        send(Network_Socket_Client, command, COMMAND_SIZE, 0);
        send(Network_Socket_Client, filename, 256, 0);

        // Open file in binary write mode
        FILE *file = fopen(filename, "wb");
        if(file == NULL)
        {
            printf("Failed to write to file\n");
            return 1;
        }

        // Receive file in chunks from server and write to file
        char buffer[BUFFER_SIZE];
        int bytes_received;
        while((bytes_received = recv(Network_Socket_Client, buffer, BUFFER_SIZE, 0)) > 0)
        {
            fwrite(buffer, 1, bytes_received, file);
        }

        fclose(file);
    }
    else
    {
        printf("Invalid command\n");
        return 1;
    }

    // Close socket
    close(Network_Socket_Client);
}

    return 0;
}
