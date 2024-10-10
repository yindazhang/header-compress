import subprocess
import argparse
import pandas as pd
import numpy as np

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='')
    args = parser.parse_args()
    
    names = []
    stats = ['Number', 'Sum', 'Mean', '99%', '99.9%']
    
    fct_file = "../logs/Hadoop_216_0.8_25G_0.5s_IP1_MPLS0.fct"
    dfs = pd.read_csv(fct_file, header=None, delimiter=r"\s+")
    dfs[4] = dfs[3] - dfs[2]
    dfs = dfs[(dfs[4] > 2200000000) & (dfs[4] < 2300000000)]

    df_vec = [dfs, dfs[dfs[1] < 100000], \
            dfs[(dfs[1] < 1000000) & (dfs[1] >= 100000)],\
            dfs[dfs[1] >= 1000000]]

    for df in df_vec:
        tmpdf = df[2].sort_values()
        size = len(tmpdf)

        print("Flow number: " + str(size))
        print("FCT mean: " + str(tmpdf.mean()))
        print("FCT 99%: " + str(tmpdf.iloc[int(0.99 * size)]))
        print("FCT 99.9%: " + str(tmpdf.iloc[int(0.999 * size)]))

    print("Finish FCT")