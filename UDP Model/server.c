#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>


void* handle_client(void* arg);


#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Client Structure
typedef struct {
    int socket;
    int client_id;
    struct sockaddr_in udp_addr;
} Client;
Client clients[MAX_CLIENTS];
int client_count = 0;
int client_ID_Counter = 1;

pthread_mutex_t client_lock;

// UDP socket for broadcasting messages
int UDP_Socket;

void* handle_client(void* arg) 
{
    int client_index = *(int*)arg;
    int client_socket = clients[client_index].socket;
    int client_id = clients[client_index].client_id;
    char message_Buffer[BUFFER_SIZE];

    while (1) 
    {
        memset(message_Buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, message_Buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            break; 

        }

        message_Buffer[bytes_received] = '\0'; // Null-terminate the received message

        // If the message starts with %, it's a command
        if (message_Buffer[0] == '%')
        {
            printf("Command from Client %d: %s\n", client_id, message_Buffer);

            if (strcmp(message_Buffer, "%put") == 0) 
            {
                char filename[256];
                // Receive filename from client
                if (recv(client_socket, filename, 256, 0) <= 0) 
                {
                    perror("Failed to receive filename\n");
                    return NULL;
                }

                // Open file in binary write mode
                FILE* file = fopen(filename, "wb");
                if (file == NULL) {
                    perror("Failed to create file\n");
                    return NULL;
                }

                // Receive file in chunks from client and write to file
                char file_buffer[BUFFER_SIZE];
                int bytes_received;
                while ((bytes_received = recv(client_socket, file_buffer, BUFFER_SIZE, 0)) > 0) {
                    if (fwrite(file_buffer, 1, bytes_received, file) < bytes_received) 
                    {
                        perror("Failed to write to file\n");
                        fclose(file);
                        return NULL;
                    }
                }

                fclose(file);
            } else if (strcmp(message_Buffer, "%get") == 0) 
            9{
                char filename[256];
                // Receive filename from client
                if (recv(client_socket, filename, 256, 0) <= 0) 
                {
                    perror("Failed to receive filename\n");
                    return NULL;
                }

                // Open file in binary read mode
                FILE* file = fopen(filename, "rb");
                if (file == NULL) {
                    perror("File not found");
                    return NULL;
                }

                // Read file in chunks and send to client
                char file_buffer[BUFFER_SIZE];
                int bytes_read;
                while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) 
                {
                    send(client_socket, file_buffer, bytes_read, 0);
                }

                fclose(file);

                int fileTermination = remove(filename);
                if (fileTermination == 0) {
                    printf("File Successfully Sent to Client\n");
                } else {
                    printf("File has not been deleted from the server, Incomplete file transfer ! !\n");
                }
            } else if (strcmp(message_Buffer, "%whoami\n") == 0) 
            {
                // Send client ID to client
                char client_id_buffer[256];
                sprintf(client_id_buffer, "%d", client_id);
                send(client_socket, client_id_buffer, strlen(client_id_buffer), 0);
            } else {
                printf("Invalid command\n");
            }
        } else {
            // Prepend the client ID to the message
            char message_with_id[BUFFER_SIZE];
            snprintf(message_with_id, BUFFER_SIZE, "Client %d: %.900s ", client_id, message_Buffer);

            printf("%s\n", message_with_id);

            // Relay the message to all connected clients using UDP
            pthread_mutex_lock(&client_lock);
            for (int i = 0; i < client_count; i++) {
                if (clients[i].socket != client_socket) {
                    sendto(udp_socket, message_with_id, strlen(message_with_id), 0, (struct sockaddr*)&clients[i].udp_addr, sizeof(clients[i].udp_addr));
                }
            }
            pthread_mutex_unlock(&client_lock);
        }
    }

    
    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == client_socket) {
            clients[i] = clients[client_count - 1]; 
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create TCP server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create TCP server socket\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9001);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified port and address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind server socket\n");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Failed to listen for incoming connections\n");
        return 1;
    }

    printf("Server listening on port 9001...\n");
    pthread_mutex_init(&client_lock, NULL);

    // Create UDP socket for broadcasting messages
    UDP_Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDP_Socket < 0) {
        perror("Failed to create UDP socket\n");
        return 1;
    }

    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Failed to accept client connection\n");
            continue;
        }

        pthread_mutex_lock(&client_lock);
        if (client_count < MAX_CLIENTS) {
            // Assign a unique ID to the new client
            clients[client_count].socket = client_socket;
            clients[client_count].client_id = client_ID_Counter++;

            // Get client's UDP address
            socklen_t udp_addr_len = sizeof(clients[client_count].udp_addr);
            getpeername(client_socket, (struct sockaddr*)&clients[client_count].udp_addr, &udp_addr_len);

            int client_index = client_count;
            client_count++;
            pthread_mutex_unlock(&client_lock);

            pthread_t client_thread;
            pthread_create(&client_thread, NULL, handle_client, &client_index);
            pthread_detach(client_thread);
        } else {
            pthread_mutex_unlock(&client_lock);
            close(client_socket);
            printf("Too many clients connected. Connection rejected.\n");
        }
    }

    close(server_socket);
    close(UDP_Socket);
    pthread_mutex_destroy(&client_lock);
    return 0;
}
