#ifndef MY_CONFIG_H
#define MY_CONFIG_H

#include <string>

#define DEFAULT_PORT 80
#define NUM_SOCKET 5

int ip_version = 0; // 0 for ipv4, 1 for ipv6
int mpls_version = 1; // add mpls or not
int vxlan_version = 0;

uint32_t label_size = 16384;
uint32_t threshold = 100;

double start_time = 2;
double duration = 0.5;

std::string file_name = "";

#endif
