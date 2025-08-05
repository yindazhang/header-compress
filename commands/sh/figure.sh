#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status
# Change to working directory
cd "../figures"
python3 -m cdf.figure

