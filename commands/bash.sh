sudo killall -9 ns3.42-header-compress

nohup ./ns3 run "scratch/header-compress --ip_version=0" > 0.txt &
nohup ./ns3 run "scratch/header-compress --ip_version=1" > 1.txt &