import math
from optparse import OptionParser

loads = [0.8]
# loads = [0.4, 0.5, 0.6, 0.7, 0.8]

transport_version = [0]
# transport_version = [0, 1]

ip_version = [1]
# ip_version = [0, 1]

compress_version = [0, 1, 2, 3]
# compress_version = [0, 1, 2, 3]

labels = [16384]

thresholds = [100]
# thresholds = [50, 150, 200]

# dataset = "WebSearch"
# dataset = "ML"
datasets = ["Storage", "WebSearch", "Cache", "Hadoop", "RPC", "ML"]
durations = ["0.5", "0.5", "0.5", "0.5", "0.5", "0.5"]

def AddLoad(start, outFile):
    global hG
    arr = loads
    for index in range(len(datasets)):
        dataset = datasets[index]
        duration = durations[index]
        for load in loads:
            cmd = start
            cmd += "--time=" + duration + " "
            cmd += "--flow=" + dataset + "_215_" + str(load) + "_25G_" + duration
            cmd += '" > '
            print(cmd + outFile + "-" + str(load) + "-" + dataset + ".out &")
        print()
    print()

def AddTransport(start, outFile):
    for transport in transport_version:
        cmd = start
        cmd += "--transport_version=" + str(transport) + " "
        AddLoad(cmd, outFile + "-Transport" + str(transport))

def AddLabel(start, outFile):
    for label in labels:
        cmd = start
        cmd += "--label_size=" + str(label) + " "
        AddTransport(cmd, outFile + "-Label" + str(label))


def AddThres(start, outFile):
    for thres in thresholds:
        cmd = start
        cmd += "--threshold=" + str(thres) + " "
        AddLabel(cmd, outFile + "-Thres" + str(thres))


def AddIP(start, outFile):
    for ip in ip_version:
        cmd = start
        cmd += "--ip_version=" + str(ip) + " "
        AddThres(cmd, outFile + "-IP" + str(ip))


def AddMPLS(start, outFile):
    for compress in compress_version:
        cmd = start
        cmd += "--compress_version=" + str(compress) + " "
        AddIP(cmd, outFile + "MPLS" + str(compress))


if __name__ == "__main__":
    start = 'nohup ./ns3 run "scratch/header-compress '
    outFile = ""
    AddMPLS(start, outFile)

