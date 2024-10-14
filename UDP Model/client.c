#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void* receive_messages(void* arg);

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    int UDP_Socket;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Failed to create TCP client socket\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server\n");
        return 1;
    }

    // Create UDP socket for receiving messages
    UDP_Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDP_Socket < 0) {
        perror("Failed to create UDP socket\n");
        return 1;
    }

    struct sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons (8080);
    inet_pton(AF_INET, "127.0.0.1", &udp_addr.sin_addr);

    if (bind(UDP_Socket, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("Failed to bind UDP socket\n");
        return 1;
    }

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, &UDP_Socket);

    char buffer[BUFFER_SIZE];
    int client_id = -1; 
    while (1) {
        printf("\nEnter message: \n");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) > 0) {
            if (buffer[0] == '%') {
                // Handle command
                if (strcmp(buffer, "%put") == 0) {
                    char filename[256];
                    printf("Enter filename: \n");
                    fgets(filename, 256, stdin);
                    filename[strcspn(filename, "\n")] = 0; // remove newline character

                    // Send command and filename to server
                    send(client_socket, buffer, strlen(buffer), 0);
                    send(client_socket, filename, strlen(filename), 0);

                    // Open file in binary read mode
                    FILE* file = fopen(filename, "rb");
                    if (file == NULL) {
                        perror("File not found\n");
                        continue;
                    }

                    // Read file in chunks and send to server
                    char file_buffer[BUFFER_SIZE];
                    int bytes_read;
                    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
                        send(client_socket, file_buffer, bytes_read, 0);
                    }

                    fclose(file);

                    int fileTermination = remove(filename);
                    if (fileTermination == 0) {
                        printf("File Successfully Sent to server\n");
                    } else {
                        printf("File has not been deleted from the client, Incomplete file transfer ! !\n");
                    }
                } else if (strcmp(buffer, "%get") == 0) {
                    char filename[256];
                    printf("Enter filename: ");
                    fgets(filename, 256, stdin);
                    filename[strcspn(filename, "\n")] = 0; // remove newline character

                    // Send command and filename to server
                    send(client_socket, buffer, strlen(buffer), 0);
                    send(client_socket, filename, strlen(filename), 0);

                    // Open file in binary write mode
                    FILE* file = fopen(filename, "wb");
                    if (file == NULL) {
                        perror("Failed to write to file\n");
                        continue;
                    }

                    // Receive file in chunks from server and write to file
                    char file_buffer[BUFFER_SIZE];
                    int bytes_received;
                    while ((bytes_received = recv(client_socket, file_buffer, BUFFER_SIZE, 0)) > 0) {
                        fwrite(file_buffer, 1, bytes_received, file);
                    }

                    fclose(file);
                } else if (strcmp(buffer, "%whoami\n") == 0) {
                    if (client_id != -1) {
                        printf("Your client ID is: %d\n", client_id);
                    } else {
                        // Send command to server to get client ID
                        send(client_socket, buffer, strlen(buffer), 0);

                        // Receive client ID from server
                        char client_id_buffer[256];
                        int bytes_received = recv(client_socket, client_id_buffer, 256, 0);
                        if (bytes_received > 0) {
                            client_id = atoi(client_id_buffer);
                            printf("Your client ID is: %d\n", client_id);
                        } else {
                            printf("Failed to receive client ID from server\n");
                        }
                    }
                } else {
                    printf("Invalid command\n");
                }
            } else {
                send(client_socket, buffer, strlen(buffer), 0);
            }
        }
    }

    close(client_socket);
    close(UDP_Socket);
    return 0;
}

void* receive_messages(void* arg) {
    int udp_socket = *((int*)arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recvfrom(udp_socket, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer); 
        }
        sleep(1);
    }

    return NULL;
}
