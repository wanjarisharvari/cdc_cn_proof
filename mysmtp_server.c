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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
#define MAX_EMAIL_SIZE 65536  
#define MAILBOX_DIR "./mailbox/"

typedef struct {
    int client_sock;
    struct sockaddr_in client_addr;
} client_args;

void *handle_client(void *args);
void process_command(int client_sock, char *command);
void handle_data_command(int client_sock);
void store_email(const char *recipient, const char *sender, const char *email_body);
void list_emails(int client_sock, const char *email);
void get_email(int client_sock, const char *email, int id);
char *get_current_date();
void ensure_mailbox_dir_exists();
void sanitize_filename(char *filename);
int is_valid_email(const char *email);

char current_sender[BUFFER_SIZE] = "";
char current_recipient[BUFFER_SIZE] = "";
int data_mode = 0;


void ensure_mailbox_dir_exists() {
    struct stat st = {0};
    if (stat(MAILBOX_DIR, &st) == -1) {
        // Directory doesn't exist, create it
        if (mkdir(MAILBOX_DIR, 0700) == -1) {
            perror("Failed to create mailbox directory");
            exit(1);
        }
        printf("Created mailbox directory: %s\n", MAILBOX_DIR);
    }
}

void *handle_client(void *args) {
    client_args *client_data = (client_args *)args;
    int client_sock = client_data->client_sock;
    struct sockaddr_in client_addr = client_data->client_addr;
    
    free(args);
    
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    while ((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        char *cmd_start = buffer;
        char *cmd_end;
        
        while ((cmd_end = strstr(cmd_start, "\r\n")) != NULL) {
            *cmd_end = '\0';  
            
            if (strlen(cmd_start) > 0) {
                printf("Received: %s\n", cmd_start);
                process_command(client_sock, cmd_start);
                
                // Check if the client wants to quit
                if (strncmp(cmd_start, "QUIT", 4) == 0) {
                    close(client_sock);
                    printf("Client disconnected: %s\n", inet_ntoa(client_addr.sin_addr));
                    return NULL;
                }
            }
            
            cmd_start = cmd_end + 2;  
        }
    }
    
    if (bytes_read == 0) {
        printf("Client disconnected unexpectedly: %s\n", inet_ntoa(client_addr.sin_addr));
    } else if (bytes_read < 0) {
        perror("recv failed");
    }
    
    close(client_sock);
    return NULL;
}

void process_command(int client_sock, char *command) {
    if (data_mode) {
        handle_data_command(client_sock);
        return;
    }
    
    // Not in DATA mode
    if (strncmp(command, "HELO", 4) == 0) {
        char client_id[BUFFER_SIZE];

        if (sscanf(command, "HELO %s", client_id) == 1) {
            printf("HELO received from %s\n", client_id);
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "200 OK\r\n");
            send(client_sock, response, strlen(response), 0);
        } 
        
        else {
            send(client_sock, "400 ERR Invalid HELO command\r\n", 30, 0);
        }
    } 
    
    else if (strncmp(command, "MAIL FROM:", 10) == 0 || strncmp(command, "MAIL FROM: ", 11) == 0) {
        char sender[BUFFER_SIZE];
        // Handle both with and without space after colon
        if (sscanf(command, "MAIL FROM: %s", sender) == 1 || sscanf(command, "MAIL FROM:%s", sender) == 1) {
            
            if (is_valid_email(sender)) {
                printf("MAIL FROM: %s\n", sender);
                strcpy(current_sender, sender);
                send(client_sock, "200 OK\r\n", 8, 0);
            } 
            
            else {
                send(client_sock, "400 ERR Invalid email format\r\n", 29, 0);
            }
        } 
        
        else {
            send(client_sock, "400 ERR Invalid MAIL FROM command\r\n", 35, 0);
        }
    } 
    
    else if (strncmp(command, "RCPT TO:", 8) == 0 || strncmp(command, "RCPT TO: ", 9) == 0) {
        char recipient[BUFFER_SIZE];
        // Handle both with and without space after colon
        if (sscanf(command, "RCPT TO: %s", recipient) == 1 || sscanf(command, "RCPT TO:%s", recipient) == 1) {
            
            if (is_valid_email(recipient)) {
                printf("RCPT TO: %s\n", recipient);
                strcpy(current_recipient, recipient);
                send(client_sock, "200 OK\r\n", 8, 0);
            } 
            
            else {
                send(client_sock, "400 ERR Invalid email format\r\n", 29, 0);
            }
        } 
        
        else {
            send(client_sock, "400 ERR Invalid RCPT TO command\r\n", 33, 0);
        }
    } 
    
    else if (strncmp(command, "DATA", 4) == 0) {
        
        
        if (strlen(current_sender) == 0 || strlen(current_recipient) == 0) {
            send(client_sock, "400 ERR MAIL FROM and RCPT TO required before DATA\r\n", 53, 0);
            return;
        }
        
        printf("DATA command received\n");
        data_mode = 1;
        send(client_sock, "354 Start mail input; end with <CRLF>.<CRLF>\r\n", 47, 0);
        
        // Now wait for the email data in a separate function
        handle_data_command(client_sock);
    } 
    
    else if (strncmp(command, "LIST", 4) == 0) {
        char email[BUFFER_SIZE];
        if (sscanf(command, "LIST %s", email) == 1) {
            printf("LIST %s\n", email);
            list_emails(client_sock, email);
        } 
        
        else {
            send(client_sock, "400 ERR Invalid LIST command\r\n", 30, 0);
        }
    } 
    
    else if (strncmp(command, "GET_MAIL", 8) == 0) {
        char email[BUFFER_SIZE];
        int id;
        if (sscanf(command, "GET_MAIL %s %d", email, &id) == 2) {
            printf("GET_MAIL %s %d\n", email, id);
            get_email(client_sock, email, id);
        } 
        
        else {
            send(client_sock, "400 ERR Invalid GET_MAIL command\r\n", 34, 0);
        }
    } 
    
    else if (strncmp(command, "QUIT", 4) == 0) {
        printf("QUIT command received\n");
        send(client_sock, "200 Goodbye\r\n", 14, 0);
    } 
    
    else {
        printf("Unknown command: %s\n", command);
        send(client_sock, "400 ERR Invalid command\r\n", 25, 0);
    }
}

void handle_data_command(int client_sock) {
    char buffer[BUFFER_SIZE];
    char email_body[MAX_EMAIL_SIZE] = "";
    int email_body_len = 0;
    int bytes_read;
    
    while ((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        char *end_marker = strstr(buffer, "\r\n.\r\n");
        if (end_marker != NULL) {
            // Found end marker, extract the text up to it
            *end_marker = '\0';
            
            // Add the remaining data to email_body 
            int remaining_len = strlen(buffer);
            if (email_body_len + remaining_len < MAX_EMAIL_SIZE) {
                strcat(email_body, buffer);
                email_body_len += remaining_len;
            }
            
            data_mode = 0;
            
            store_email(current_recipient, current_sender, email_body);
            
            send(client_sock, "200 Message stored successfully\r\n", 33, 0);
            
            current_sender[0] = '\0';
            current_recipient[0] = '\0';
            
            printf("DATA received, message stored.\n");
            return;
        } 
        
        else {
            // No end marker yet, append to email body
            if (email_body_len + bytes_read < MAX_EMAIL_SIZE) {
                strcat(email_body, buffer);
                email_body_len += bytes_read;
            } 
            
            else {
                
                send(client_sock, "500 SERVER ERROR Message too large\r\n", 36, 0);
                data_mode = 0;
                return;
            }
        }
    }
    
    if (bytes_read <= 0) {
        data_mode = 0;
        perror("Error receiving data");
    }
}

// void store_email(const char *recipient, const char *sender, const char *email_body) {
//     char sanitized_recipient[BUFFER_SIZE];
//     strcpy(sanitized_recipient, recipient);
//     sanitize_filename(sanitized_recipient);
    
//     char filename[BUFFER_SIZE];
//     snprintf(filename, sizeof(filename), "%s%s.txt", MAILBOX_DIR, sanitized_recipient);
    
//     FILE *file = fopen(filename, "a");
//     if (file == NULL) {
//         perror("Unable to open mailbox file");
//         return;
//     }

//     fprintf(file, "From: %s\n", sender);
//     fprintf(file, "Date: %s\n", get_current_date());
//     fprintf(file, "%s\n", email_body);
//     fprintf(file, "---END OF MESSAGE---\n");  // Add a separator between emails
    
//     fclose(file);
//     printf("Email stored for %s from %s\n", recipient, sender);
// }




long get_next_email_id(const char *recipient) {
    char counter_filename[BUFFER_SIZE];
    snprintf(counter_filename, sizeof(counter_filename), "%s%s_counter.txt", MAILBOX_DIR, recipient);

    FILE *counter_file = fopen(counter_filename, "r+");
    long email_id = 1;

    if (counter_file == NULL) {
        counter_file = fopen(counter_filename, "w");
        if (counter_file == NULL) {
            perror("Unable to create counter file");
            return -1;
        }
        fprintf(counter_file, "%ld", email_id);
        fclose(counter_file);
        return email_id;
    }

    fscanf(counter_file, "%ld", &email_id);
    email_id++;  

    // Rewind file and update with new ID
    rewind(counter_file);
    fprintf(counter_file, "%ld", email_id);
    fclose(counter_file);

    return email_id;
}

void store_email(const char *recipient, const char *sender, const char *email_body) {
    char sanitized_recipient[BUFFER_SIZE];
    strcpy(sanitized_recipient, recipient);
    sanitize_filename(sanitized_recipient);
    
    char filename[BUFFER_SIZE];
    snprintf(filename, sizeof(filename), "%s%s.txt", MAILBOX_DIR, sanitized_recipient);
    
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Unable to open mailbox file");
        return;
    }

    long email_id = get_next_email_id(sanitized_recipient);
    if (email_id == -1) {
        fclose(file);
        return;
    }

    fprintf(file,"\n");
    fprintf(file, "Email-ID: %ld\n", email_id);
    fprintf(file, "From: %s\n", sender);
    fprintf(file, "Date: %s\n", get_current_date());
    fprintf(file, "%s\n", email_body);
    fprintf(file, "---END OF MESSAGE---\n");  

    fclose(file);
    printf("Email stored for %s from %s with ID %ld\n", recipient, sender, email_id);
}



// void list_emails(int client_sock, const char *email) {
//     char sanitized_email[BUFFER_SIZE];
//     strcpy(sanitized_email, email);
//     sanitize_filename(sanitized_email);
    
//     char filename[BUFFER_SIZE];
//     snprintf(filename, sizeof(filename), "%s%s.txt", MAILBOX_DIR, sanitized_email);
    
//     FILE *file = fopen(filename, "r");
//     if (file == NULL) {
//         send(client_sock, "401 NOT FOUND\r\n", 16, 0);
//         return;
//     }
    
//     // First, send a success status
//     send(client_sock, "200 OK\r\n", 9, 0);
    
//     char line[BUFFER_SIZE];
//     int id = 1;
//     int reading_header = 0;
//     char current_from[BUFFER_SIZE] = "";
//     char current_date[BUFFER_SIZE] = "";
    
//     while (fgets(line, sizeof(line), file)) {
//         // Remove trailing newline
//         line[strcspn(line, "\r\n")] = '\0';
        
//         if (strncmp(line, "From:", 5) == 0) {
//             strcpy(current_from, line + 6);  // Skip "From: "
//             reading_header = 1;
//         } else if (strncmp(line, "Date:", 5) == 0) {
//             strcpy(current_date, line + 6);  // Skip "Date: "
//         } else if (strcmp(line, "---END OF MESSAGE---") == 0) {
//             // End of an email, send the summary
//             char response[BUFFER_SIZE];
//             snprintf(response, sizeof(response), "%d: Email from %s (%s)\r\n", id++, current_from, current_date);
//             send(client_sock, response, strlen(response), 0);
//             reading_header = 0;
//             current_from[0] = '\0';
//             current_date[0] = '\0';
//         }
//     }
    
//     fclose(file);
//     printf("Emails retrieved; list sent.\n");
// }


void list_emails(int client_sock, const char *email) {
    char sanitized_email[BUFFER_SIZE];
    strcpy(sanitized_email, email);
    sanitize_filename(sanitized_email);
    
    char filename[BUFFER_SIZE];
    snprintf(filename, sizeof(filename), "%s%s.txt", MAILBOX_DIR, sanitized_email);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        send(client_sock, "401 NOT FOUND\r\n", 16, 0);
        return;
    }

    char response[MAX_EMAIL_SIZE]; 
    strcpy(response, "200 OK\r\n");  
    
    char line[BUFFER_SIZE];
    int id = 1;
    char current_from[BUFFER_SIZE] = "";
    char current_date[BUFFER_SIZE] = "";

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';  // Remove newline

        if (strncmp(line, "From:", 5) == 0) {
            strcpy(current_from, line + 6);
        } else if (strncmp(line, "Date:", 5) == 0) {
            strcpy(current_date, line + 6);
        } else if (strcmp(line, "---END OF MESSAGE---") == 0) {
            char email_entry[BUFFER_SIZE];
            snprintf(email_entry, sizeof(email_entry), "%d: Email from %s (%s)\r\n", id++, current_from, current_date);
            strcat(response, email_entry);  
            current_from[0] = '\0';
            current_date[0] = '\0';
        }
    }
    
    fclose(file);

    //strcat(response, "END OF LIST\r\n");  
    send(client_sock, response, strlen(response), 0);  

    printf("Emails retrieved; list sent.\n");
}




void get_email(int client_sock, const char *email, int id) {
    char sanitized_email[BUFFER_SIZE];
    strcpy(sanitized_email, email);
    sanitize_filename(sanitized_email);
    
    char filename[BUFFER_SIZE];
    snprintf(filename, sizeof(filename), "%s%s.txt", MAILBOX_DIR, sanitized_email);
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        send(client_sock, "401 NOT FOUND\r\n", 16, 0);
        return;
    }
    
    char line[BUFFER_SIZE];
    int email_id = -1;
    int reading_email = 0;
    char email_content[MAX_EMAIL_SIZE] = "200 OK\r\n";  

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';  
        
        if (strncmp(line, "Email-ID:", 9) == 0) {  
            email_id = atoi(line + 10);  // Extract the email ID
            
            if (email_id == id) {  
                reading_email = 1;  // Start reading this email
                strcat(email_content, line);
                //strcat(email_content, "\r\n");
            } else {
                reading_email = 0;
            }
        } 
        else if (strcmp(line, "---END OF MESSAGE---") == 0) {  
            if (reading_email) {  
                //strcat(email_content, "\r\n");  
                break;  
            }
        } 
        else if (reading_email) {
            strcat(email_content, line);
            strcat(email_content, "\r\n");
        }
    }
    
    fclose(file);

    if (reading_email) {  
        send(client_sock, email_content, strlen(email_content), 0);
        printf("Email with id %d sent.\n", id);
    } else {  
        send(client_sock, "401 NOT FOUND\r\n", 16, 0);
    }
}



char *get_current_date() {
    static char date_str[20];
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(date_str, sizeof(date_str), "%d-%m-%Y", tm_info);
    return date_str;
}

void sanitize_filename(char *filename) {
    char *p;
    for (p = filename; *p; p++) {
        if (*p == '@' || *p == '/' || *p == '\\' || *p == ':' || *p == '*' || 
            *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') {
            *p = '_';
        }
    }
}

int is_valid_email(const char *email) {
    if (strchr(email, '@') == NULL) {
        return 0;
    }
    return 1;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int PORT = atoi(argv[1]);
    int server_sock;
    struct sockaddr_in server_addr;

    ensure_mailbox_dir_exists();

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Set socket option to reuse address
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Setsockopt failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, 5) == -1) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_args *args = malloc(sizeof(client_args));
        socklen_t client_len = sizeof(args->client_addr);
        
        args->client_sock = accept(server_sock, (struct sockaddr *)&args->client_addr, &client_len);
        
        if (args->client_sock == -1) {
            perror("Accept failed");
            free(args);
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(args->client_addr.sin_addr));
        
        send(args->client_sock, "220 My_SMTP Server Ready\r\n", 27, 0);
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)args) != 0) {
            perror("Thread creation failed");
            close(args->client_sock);
            free(args);
            continue;
        }
        
        pthread_detach(thread_id);
    }

    close(server_sock);
    return 0;
}