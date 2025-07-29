#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status
# Change to working directory
cd /mydata
VERSION="3.37"
HEADER_DIR="header-compress"
NS_DIR="ns-allinone-$VERSION"
# Remove old commands directory and copy new commands from header-compress
rm -rf "$NS_DIR/ns-$VERSION/commands/"
cp -r "$HEADER_DIR/commands/" "$NS_DIR/ns-$VERSION/"
# Change to ns directory
cd "$NS_DIR/ns-$VERSION/commands/"
# Define CDFs, loads, and durations for each CDF
CDFS=("VL2" "WebSearch" "Cache" "Hadoop" "RPC")
LOADS=(0.4 0.5 0.6 0.7 0.8)
declare -A DURATION=( ["VL2"]=1.0 ["WebSearch"]=0.5 ["Cache"]=0.5 ["Hadoop"]=0.5 ["RPC"]=0.2 )

# Run ML_gen.py for each load
for load in "${LOADS[@]}"; do
    python3 ML_gen.py -l "$load" -t 1.0 &
done
wait
echo "Finished ML"

# Run traffic_gen.py for each CDF and load with corresponding duration
for cdf in "${CDFS[@]}"; do
    duration=${DURATION[$cdf]}
    for load in "${LOADS[@]}"; do
        python3 traffic_gen.py -l "$load" -c "$cdf" -t "$duration" &
    done
    wait  # Wait for all background jobs of this CDF to finish before continuing
    echo "Finished $cdf"
done
