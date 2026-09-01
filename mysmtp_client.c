/*
Assignmetn 6
Sharvari
22CS30049
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>

#define BUFFER_SIZE 4096
#define MAX_EMAIL_SIZE 65536  

void send_command(int sock, const char *command) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s\r\n", command);
    
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("Error sending command");
        exit(1);
    }
}

void handle_data_mode(int sock) {
    char buffer[BUFFER_SIZE];
    char *email_body = malloc(MAX_EMAIL_SIZE);
    if (email_body == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    // Initialize email_body as empty
    email_body[0] = '\0';
    int total_len = 0;
    
    printf("Enter your message (end with a single dot '.' on a new line):\n");
    
    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            free(email_body);
            return;
        }
        
        // Check for end-of-message marker (a single period on a line by itself)
        if (strcmp(buffer, ".\n") == 0 || strcmp(buffer, ".\r\n") == 0) {
            strcat(email_body, "\r\n.\r\n");  // Add proper SMTP termination
            break;
        }
        
        // Handle dot-stuffing: If line starts with a period, add another period
        if (buffer[0] == '.') {
            memmove(buffer + 1, buffer, strlen(buffer) + 1);
            buffer[0] = '.';
        }
        
        // Check for buffer overflow
        if (total_len + strlen(buffer) >= MAX_EMAIL_SIZE - 5) {  // -5 for \r\n.\r\n
            fprintf(stderr, "Email size limit exceeded\n");
            free(email_body);
            return;
        }
        
        strcat(email_body, buffer);
        total_len += strlen(buffer);
    }
    
    // Send the email body to the server
    if (send(sock, email_body, strlen(email_body), 0) < 0) {
        perror("Error sending email data");
    }
    
    free(email_body);
    
    receive_response(sock);
}

void receive_response(int sock) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    // Use a timeout to prevent blocking indefinitely
    struct timeval tv;
    tv.tv_sec = 3;  
    tv.tv_usec = 0;
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
    }
    
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout occurred, no data received
            printf("Response timeout, server may still be processing...\n");
            return;
        } else {
            perror("recv failed");
            return;
        }
    } else if (bytes_received == 0) {
        printf("Connection closed by server\n");
        exit(1);
    }
    
    buffer[bytes_received] = '\0';
    
    printf("%s", buffer);
    
    // Clear timeout
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to My_SMTP server.\n");
    receive_response(sock);
    
    char command[BUFFER_SIZE];
    int quit = 0;
    
    while (!quit) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // Remove trailing newline
        command[strcspn(command, "\r\n")] = '\0';
        
        if (strlen(command) == 0) {
            continue;
        }
        
        // Check if we're entering DATA mode
        if (strncmp(command, "DATA", 4) == 0) {
            send_command(sock, command);
            receive_response(sock);
            handle_data_mode(sock);
            continue;
        }
        
        // Check if we're quitting
        if (strncmp(command, "QUIT", 4) == 0) {
            send_command(sock, command);
            receive_response(sock);
            quit = 1;
            continue;
        }
        
        send_command(sock, command);
        receive_response(sock);
    }

    close(sock);
    return 0;
}