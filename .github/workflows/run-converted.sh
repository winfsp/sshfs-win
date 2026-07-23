#!/bin/bash

# Check if filename is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <script-file>"
    exit 1
fi

filename=$(cygpath -u "$1")

# Check if file exists
if [ ! -f "$filename" ]; then
    echo "Error: File '$filename' not found"
    exit 1
fi

# Convert line endings using sed (in-place conversion)
dos2unix "$filename" 2>/dev/null || sed -i 's/\r$//' "$filename"

# Make file executable (if not already)
chmod +x "$filename"

# Execute the script
"$filename"
