import subprocess
import argparse
import pandas as pd
import numpy as np

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-f', dest='file', action='store', help="Specify the fct file.")
    args = parser.parse_args()
    
    names = []
    stats = ['Number', 'Sum', 'Mean', '99%', '99.9%']
    
    fct_file = args.file
    dfs = pd.read_csv(fct_file, header=None, delimiter=r"\s+")
    dfs[4] = dfs[3] - dfs[2]
    # dfs = dfs[(dfs[3] < 2200000000)]
    dfs = dfs[(dfs[4] > 2100000000) & (dfs[4] < 2300000000)]

    # print(dfs[(dfs[4] > 2100000000) & (dfs[4] < 2400000000) & (dfs[3] > 2471000000)])

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
