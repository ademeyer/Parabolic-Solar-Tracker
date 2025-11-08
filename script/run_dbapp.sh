#!/bin/bash

# Script to run db_app and log output
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="$SCRIPT_DIR/log.txt"
APP_PATH="$SCRIPT_DIR/db_app"

echo "=== Starting db_app at $(date) ===" >> "$LOG_FILE"

# Run the application and append output to log
"$APP_PATH" >> "$LOG_FILE" 2>&1

EXIT_CODE=$?
echo "=== db_app exited with code $EXIT_CODE at $(date) ===" >> "$LOG_FILE"