#ifndef MY_CONFIG_H
#define MY_CONFIG_H

#define DEFAULT_PORT 80

int ip_version = 0; // 0 for ipv4, 1 for ipv6
int mpls_version = 1; // add mpls or not
int vxlan_version = 0;

uint32_t threshold = 100;
uint32_t mtu = 1400;

double start_time = 2;
double duration = 0.5;

std::string file_name = "";

#endif