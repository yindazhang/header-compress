sudo chmod -R 777 /mydata/

# ssh-keygen -t ed25519 -C "1293516092@qq.com"

cd /mydata
git clone https://github.com/yindazhang/header-compress.git
# git clone git@github.com:yindazhang/header-compress.git

wget https://www.nsnam.org/releases/ns-allinone-3.37.tar.bz2
tar xjf ns-allinone-3.37.tar.bz2

rm -rf ns-allinone-3.37/ns-3.37/src/ ns-allinone-3.37/ns-3.37/scratch/
cp -r header-compress/src/ ns-allinone-3.37/ns-3.37/
cp -r header-compress/scratch/ ns-allinone-3.37/ns-3.37/
cp -r header-compress/commands/ ns-allinone-3.37/ns-3.37/

cd ns-allinone-3.37/ns-3.37/
mkdir logs
./ns3 configure --build-profile=optimized --out=build/optimized
./ns3 build

sudo killall -9 ns3.37-header-compress

# cd commands/
# python3 generate_cmd.py
# python3 traffic_gen.py -l 0.8

# nohup ./ns3 run "scratch/header-compress --ip_version=0" > 0.txt &
