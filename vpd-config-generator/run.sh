#!/bin/bash
# VPD Configuration Generator Launcher

echo "=========================================="
echo "VPD Configuration Generator"
echo "=========================================="
echo ""

# Check if we're in the right directory
if [ ! -f "vpd_config_generator_web.py" ]; then
    echo "Error: Please run this script from the vpd-config-generator directory"
    exit 1
fi

# Check Python version
python3 --version > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Python 3 is not installed"
    exit 1
fi

# Check if port 8080 is in use and kill it
if lsof -Pi :8080 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo "Port 8080 is in use. Stopping existing server..."
    lsof -ti:8080 | xargs kill -9 2>/dev/null
    sleep 1
    echo "Port cleared."
    echo ""
fi

echo "Starting web-based VPD Configuration Generator..."
echo ""
echo "The tool will open in your browser at http://localhost:8080"
echo "Press Ctrl+C to stop the server"
echo ""
echo "=========================================="
echo ""

# Run the web version
python3 vpd_config_generator_web.py

# Made with Bob
