#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status
sudo chmod -R 777 /mydata/
sudo apt update
sudo apt-get install cmake python3-pip
pip3 install pandas matplotlib
# Change to working directory
cd /mydata
# Clone or update header-compress repo
HEADER_DIR="header-compress"
if [ -d "$HEADER_DIR" ]; then
    echo "Directory $HEADER_DIR exists."
    echo "Starting git pull in $HEADER_DIR..."
    # Use subshell to avoid changing current directory permanently
    (cd "$HEADER_DIR" && git pull)
else
    echo "Directory $HEADER_DIR does not exist."
    echo "Starting git clone..."
    if ! git clone git@github.com:yindazhang/header-compress.git; then
        echo "SSH clone failed, trying HTTPS..."
        git clone https://github.com/yindazhang/header-compress.git
    fi
fi
# Check and install ns-allinone-3.37 if needed
NS_DIR="ns-allinone-3.37"
NS_TAR="ns-allinone-3.37.tar.bz2"
NS_URL="https://www.nsnam.org/releases/ns-allinone-3.37.tar.bz2"
if [ -d "$NS_DIR" ]; then
    echo "Directory $NS_DIR exists."
else
    echo "Directory $NS_DIR does not exist."
    echo "Downloading and installing ns-allinone-3.37..."
    if [ ! -f "$NS_TAR" ]; then
        wget "$NS_URL"
    else
        echo "$NS_TAR already downloaded."
    fi
    tar xjf "$NS_TAR"
fi
# Remove old src and scratch directories before copying new ones
rm -rf "$NS_DIR/ns-3.37/src/" "$NS_DIR/ns-3.37/scratch/"
# Copy src and scratch from header-compress to ns-3.37
cp -r "$HEADER_DIR/src/" "$NS_DIR/ns-3.37/"
cp -r "$HEADER_DIR/scratch/" "$NS_DIR/ns-3.37/"
# Build ns-3.37
cd "$NS_DIR/ns-3.37/"
mkdir -p logs
./ns3 configure --build-profile=optimized --out=build/optimized
./ns3 build
# Kill any running ns3.37-header-compress processes
sudo killall -9 ns3.37-header-compress || echo "No running ns3.37-header-compress processes found."

# nohup ./ns3 run "scratch/header-compress --ip_version=0" > 0.txt &

