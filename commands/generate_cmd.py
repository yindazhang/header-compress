from optparse import OptionParser
import math

loads = [0.8]
#loads = [0.4, 0.5, 0.6, 0.7, 0.8]

ip_version = [0, 1]

mpls_version = [0, 1]
#mpls_version = [0, 1]

dynamics = [0]

# thresholds = [200, 500, 1000]
thresholds = [1000]

dataset = "Hadoop"

def AddLoad(start, outFile):
    global hG
    arr = loads
    for load in loads:
        cmd = start
        cmd += "--time=0.5 "
        cmd += "--flow=" + dataset + "_216_" + str(load) + "_25G_0.5"
        cmd += "\" > "
        print(cmd + outFile + "-" + str(load) + "-" + dataset + ".out &")
    print()

def AddDynamic(start, outFile):
    for dy in dynamics:
        cmd = start
        cmd += "--dynamic=" + str(dy) + " "
        AddLoad(cmd, outFile + "-Dynamic" + str(dy))

def AddThres(start, outFile):
    for thres in thresholds:
        cmd = start
        cmd += "--threshold=" + str(thres) + " "
        AddDynamic(cmd, outFile + "-Thres" + str(thres))

def AddIP(start, outFile):
    for ip in ip_version:
        cmd = start
        cmd += "--ip_version=" + str(ip) + " "
        AddThres(cmd, outFile + "-IP" + str(ip))

def AddMPLS(start, outFile):
    for mpls in mpls_version:
        cmd = start
        cmd += "--mpls_version=" + str(mpls) + " "
        AddIP(cmd, outFile + "MPLS" + str(mpls))

if __name__=="__main__":
    start = "nohup ./ns3 run \"scratch/header-compress "
    outFile = ""
    AddMPLS(start, outFile)               