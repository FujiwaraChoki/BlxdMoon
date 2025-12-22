#ifndef SHELL_H
#define SHELL_H

// Start an interactive reverse shell using the given socket
// Blocks until the shell exits
void StartInteractiveShell(int socket_fd);

#endif
