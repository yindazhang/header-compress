import subprocess
import argparse
import pandas as pd
import numpy as np

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='')
    args = parser.parse_args()
    
    names = []
    stats = ['Number', 'Sum', 'Mean', '99%', '99.9%']
    
    fct_file = "../logs/Hadoop_216_0.8_25G_1.0s_IP0_MPLS0.fct"
    df = pd.read_csv(fct_file, header=None, delimiter=r"\s+")
    
    tmpdf = df[2].sort_values()
    size = len(tmpdf)

    print("Flow number: " + str(size))
    print("FCT mean: " + str(tmpdf.mean()))
    print("FCT 99%: " + str(tmpdf.iloc[int(0.99 * size)]))
    print("FCT 99.9%: " + str(tmpdf.iloc[int(0.999 * size)]))

    print("Finish FCT")