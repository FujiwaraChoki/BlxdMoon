#ifndef WOL_H
#define WOL_H

#include <winsock2.h>

// Parse MAC address string "AA:BB:CC:DD:EE:FF" to 6 bytes
// Returns 0 on success, -1 on failure
int parse_mac(const char *mac_str, unsigned char *mac_bytes);

// Send WOL magic packet to wake machine with given MAC address
// Returns 0 on success, -1 on failure
int send_magic_packet(const char *mac_address);

// Enable WOL on the local machine (multi-vendor support)
void enable_wol();

#endif
