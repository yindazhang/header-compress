#ifndef MY_CONFIG_H
#define MY_CONFIG_H

#define DEFAULT_PORT 80

int ip_version = 1; // 0 for ipv4, 1 for ipv6
int mpls_version = 0; // add mpls or not

uint32_t dynamic_thres = 0;
uint32_t threshold = 1000;

double start_time = 2;
double duration = 1;

std::string file_name = "";

#endif