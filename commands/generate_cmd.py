from optparse import OptionParser
import math

loads = [0.8]
#loads = [0.4, 0.5, 0.6, 0.7, 0.8]

ip_version = [0, 1]

mpls_version = [0]
#mpls_version = [0, 1]

dataset = "Hadoop"

def AddLoad(start, outFile):
    global hG
    arr = loads
    for load in loads:
        cmd = start
        cmd += "--time=1.0 "
        cmd += "--flow=" + dataset + "_216_" + str(load) + "_25G_1.0"
        cmd += "\" > "
        print(cmd + outFile + "-" + str(load) + "-" + dataset + ".out &")
    print()

def AddIP(start, outFile):
    for ip in ip_version:
        cmd = start
        cmd += "--ip_version=" + str(ip) + " "
        AddLoad(cmd, outFile + "-IP" + str(ip))

def AddMPLS(start, outFile):
    for mpls in mpls_version:
        cmd = start
        cmd += "--mpls_version=" + str(mpls) + " "
        AddIP(cmd, outFile + "MPLS" + str(mpls))

if __name__=="__main__":
    start = "nohup ./ns3 run \"scratch/header-compress "
    outFile = ""
    AddMPLS(start, outFile)               