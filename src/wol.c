#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wol.h"

#define WOL_PORT 9
#define MAGIC_PACKET_SIZE 102

int parse_mac(const char *mac_str, unsigned char *mac_bytes)
{
    int values[6];
    int i;

    // Parse MAC address in format AA:BB:CC:DD:EE:FF or AA-BB-CC-DD-EE-FF
    if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) == 6 ||
        sscanf(mac_str, "%x-%x-%x-%x-%x-%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) == 6)
    {
        for (i = 0; i < 6; i++)
        {
            mac_bytes[i] = (unsigned char)values[i];
        }
        return 0;
    }

    return -1;
}

int send_magic_packet(const char *mac_address)
{
    unsigned char mac_bytes[6];
    unsigned char magic_packet[MAGIC_PACKET_SIZE];
    int i, j;
    SOCKET udp_sock;
    struct sockaddr_in broadcast_addr;
    int broadcast_enable = 1;

    // Parse the MAC address
    if (parse_mac(mac_address, mac_bytes) != 0)
    {
        return -1;
    }

    // Build magic packet: 6 bytes of 0xFF followed by 16 repetitions of MAC
    for (i = 0; i < 6; i++)
    {
        magic_packet[i] = 0xFF;
    }

    for (i = 0; i < 16; i++)
    {
        for (j = 0; j < 6; j++)
        {
            magic_packet[6 + i * 6 + j] = mac_bytes[j];
        }
    }

    // Create UDP socket
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock == INVALID_SOCKET)
    {
        return -1;
    }

    // Enable broadcast
    if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST,
                   (char *)&broadcast_enable, sizeof(broadcast_enable)) < 0)
    {
        closesocket(udp_sock);
        return -1;
    }

    // Set up broadcast address
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    broadcast_addr.sin_port = htons(WOL_PORT);

    // Send magic packet
    if (sendto(udp_sock, (char *)magic_packet, MAGIC_PACKET_SIZE, 0,
               (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0)
    {
        closesocket(udp_sock);
        return -1;
    }

    closesocket(udp_sock);
    return 0;
}

void enable_wol()
{
    // Set execution policy first
    system("powershell -Command \"Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Bypass -Force\"");

    // Multi-vendor WOL enablement script
    char *script =
        "powershell -Command \""
        "$mfr = (Get-WmiObject win32_bios).Manufacturer; "
        "if ($mfr -like '*Dell*') { "
        "  if (-not (Get-Module -ListAvailable -Name DellBiosProvider)) { "
        "    Install-Module -Name DellBiosProvider -Force -ErrorAction SilentlyContinue; "
        "  } "
        "  Import-Module DellBiosProvider -ErrorAction SilentlyContinue; "
        "  if (Test-Path 'DellSmbios:\\PowerManagement\\WakeOnLan') { "
        "    Set-Item -Path 'DellSmbios:\\PowerManagement\\WakeOnLan' -Value 'LanOnly' -ErrorAction SilentlyContinue; "
        "  } "
        "} "
        "elseif ($mfr -like '*HP*' -or $mfr -like '*Hewlett*') { "
        "  $adapters = Get-NetAdapter | Where-Object { $_.Status -eq 'Up' }; "
        "  foreach ($adapter in $adapters) { "
        "    $props = Get-NetAdapterPowerManagement -Name $adapter.Name -ErrorAction SilentlyContinue; "
        "    if ($props) { "
        "      Enable-NetAdapterPowerManagement -Name $adapter.Name -WakeOnMagicPacket -ErrorAction SilentlyContinue; "
        "    } "
        "  } "
        "} "
        "elseif ($mfr -like '*Lenovo*') { "
        "  $adapters = Get-NetAdapter | Where-Object { $_.Status -eq 'Up' }; "
        "  foreach ($adapter in $adapters) { "
        "    Enable-NetAdapterPowerManagement -Name $adapter.Name -WakeOnMagicPacket -ErrorAction SilentlyContinue; "
        "  } "
        "} "
        "else { "
        "  $adapters = Get-NetAdapter | Where-Object { $_.Status -eq 'Up' }; "
        "  foreach ($adapter in $adapters) { "
        "    $power = Get-NetAdapterPowerManagement -Name $adapter.Name -ErrorAction SilentlyContinue; "
        "    if ($power) { "
        "      Enable-NetAdapterPowerManagement -Name $adapter.Name -WakeOnMagicPacket -ErrorAction SilentlyContinue; "
        "    } "
        "  } "
        "}\"";

    system(script);
}
