CC = g++
CFLAGS = -I /home/leoyu/bf-sde-9.2.0/pkgsrc/bf-drivers/include -I /home/leoyu/bf-sde-9.2.0/install/include -Wno-missing-field-initializers -Werror -Wshadow -g -O2 -std=c++11


all: control

control: control.cpp
	$(CC) $(CFLAGS) -o control control.cpp -lpcap -lpthread -ldriver -lbfsys -lbfutils -lbf_switchd_lib -lm -ldl -L /home/leoyu/bf-sde-9.2.0/install/lib -Wl,-rpath -Wl,/home/leoyu/bf-sde-9.2.0/install/lib

clean:
	rm -f control