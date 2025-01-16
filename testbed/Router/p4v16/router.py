import argparse
import json
import sys, os, time

sys.path.append(os.path.dirname(os.path.realpath(__file__))+"/libs")
from mgr import *
from scapy.all import Ether, sendp


def config():
    print("PID: " + str(os.getpid())) 
    m = Manager(p4name="main")

    data = {}
    with open('libs/servers.json', 'r') as f:
        servers_json=json.loads(f.read())
    server_ports = []
    for val in servers_json.values():
        server_ports.append(val['port_id'])

    # 1/0 <-> 2/0
    # multicast_ports = [140, 132, 141, 133, 142, 134, 143, 135]
    # push_ports = [142, 134]

    print("--- Set up ports (physically occupied for successful enabling) ---")
    # m.enab_ports_sym_bw(multicast_ports, "25G")
    m.enab_ports_sym_bw([140, 141, 142, 143], "10G")
    # m.enab_ports_sym_bw_b(redyellow_ports, "100G")

    print("--- Configure DP states ---")
    
    for server in servers_json.values():
        print(server)

        m.add_rule_exact_read_action_arg(
                    tbl_name="ti_forward4_user",
                    action_name='ai_forward_user',
                    match_arg_name="hdr.ipv4.dstAddr",
                    match_arg=server['ipv4_addr'],
                    action_arg_name="egress_port",
                    action_arg=server['out3_port_id'])
        
        m.add_rule_exact_read4_action_arg(
                    tbl_name="ti_forward6_user",
                    action_name='ai_forward_user',
                    match_arg_name0="hdr.ipv6.dstAddr1",
                    match_arg0=server['ipv6_addr1'],
                    match_arg_name1="hdr.ipv6.dstAddr2",
                    match_arg1=server['ipv6_addr2'],
                    match_arg_name2="hdr.ipv6.dstAddr3",
                    match_arg2=server['ipv6_addr3'],
                    match_arg_name3="hdr.ipv6.dstAddr4",
                    match_arg3=server['ipv6_addr4'],
                    action_arg_name="egress_port",
                    action_arg=server['out3_port_id'])
    
    m.disconnect()


def debug(log_dir):
    print("PID: " + str(os.getpid()))
    m = Manager(p4name="main")

    send_in = m.read_reg_element_for_pipe("reg_send_in_", 0, pipeid=1)
    send_out = m.read_reg_element_for_pipe("reg_send_out_", 0, pipeid=1)
    print(send_out - send_in)

    '''
    while True:
        index = m.read_reg_element_for_pipe("reg_send_in_", 0, pipeid=1)
        print(index)
        time.sleep(1)
        break
    '''

    m.disconnect()


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    subcmds = parser.add_subparsers(dest="cmd")

    cmd_config = subcmds.add_parser("config")

    cmd_debug = subcmds.add_parser("debug")
    cmd_debug.add_argument("-d", "--dir", type=str, required=False, default="logs", help="Name of the directory to hold the debugging stats")

    cmd_config = subcmds.add_parser("control")

    args = parser.parse_args(['config'] if len(sys.argv)==1 else None)

    if args.cmd == "config":
        config()
    elif args.cmd == "debug":
        debug(args.dir)
    else:
        print("ERR")