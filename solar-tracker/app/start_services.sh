#!/bin/sh

# Start the Python script server in the background
python3 /app/spa/script-server.py &

# Start Nginx in the foreground
exec nginx -g 'daemon off;'
