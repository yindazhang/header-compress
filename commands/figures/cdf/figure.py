import os
import sys

import matplotlib.pyplot as plt

import pandas as pd

from configure import init_plt

files = [
    "VL2",
    "WebSearch",
    "Cache",
    "Hadoop",
    "RPC",
]
line = [
    "-",
    "--",
    "-.",
    ":",
    "-",
]
color = [
    "g",
    "k",
    "b",
    "r",
    "brown",
]

FIGSIZE = (8, 4)
LINEWIDTH = 2


def flow_cdf():
    plt.figure(figsize=FIGSIZE)
    for file in files:
        df = pd.read_csv("../traffic_cdf/" + file + ".txt", sep="\\s+", header=None)
        plt.plot(
            df[0],
            df[1] * 100,
            linestyle=line[files.index(file)],
            color=color[files.index(file)],
            linewidth=LINEWIDTH,
        )

    plt.xscale("log")
    plt.xlabel("Flow Size (Bytes)", fontweight="bold")

    plt.ylim(0, 100)
    plt.ylabel("Cumulative % of Flows", fontweight="bold")

    plt.savefig("flow-cdf.pdf", bbox_inches="tight")
    plt.close()


def byte_cdf():
    plt.figure(figsize=FIGSIZE)

    for file in files:
        sizes = []
        total = 0
        preRow = None
        df = pd.read_csv("../traffic_cdf/" + file + ".txt", sep="\\s+", header=None)
        for row in df.itertuples():
            if preRow is not None:
                size = (row[1] + preRow[1]) * (row[2] - preRow[2]) / 2.0
                sizes.append([row[1], total + size])
                total += size
            preRow = row
        df = pd.DataFrame(sizes, columns=[0, 1])
        df[1] /= total

        plt.plot(
            df[0],
            df[1] * 100,
            linestyle=line[files.index(file)],
            color=color[files.index(file)],
            linewidth=LINEWIDTH,
        )

    plt.xscale("log")
    plt.xlabel("Flow Size (Bytes)", fontweight="bold")

    plt.ylim(0, 100)
    plt.ylabel("Cumulative % of Bytes", fontweight="bold")

    plt.savefig("byte-cdf.pdf", bbox_inches="tight")
    plt.close()


if __name__ == "__main__":
    init_plt()

    flow_cdf()
    byte_cdf()

