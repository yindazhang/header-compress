import sys
import random
import math
import os
from optparse import OptionParser
from custom_rand import CustomRand

class Flow:
	def __init__(self, src, dst, size, t):
		self.src, self.dst, self.size, self.t = src, dst, size, t
	def __str__(self):
		return "%d %d %d %d\n"%(self.src, self.dst, self.size, self.t)

def translate_bandwidth(b):
	if b == None:
		return None
	if type(b)!=str:
		return None
	if b[-1] == 'G':
		return float(b[:-1])*1e9
	if b[-1] == 'M':
		return float(b[:-1])*1e6
	if b[-1] == 'K':
		return float(b[:-1])*1e3
	return float(b)

def poisson(lam):
	return -math.log(1-random.random())*lam

if __name__ == "__main__":
	random.seed(42)

	parser = OptionParser()
	parser.add_option("-n", "--nhost", dest = "nhost", help = "number of hosts", default = "215")
	parser.add_option("-l", "--load", dest = "load", help = "the percentage of the traffic load to the network capacity, by default 0.5", default = "0.5")
	parser.add_option("-b", "--bandwidth", dest = "bandwidth", help = "the bandwidth of host link (G/M/K), by default 25G", default = "25G")
	parser.add_option("-t", "--time", dest = "time", help = "the total run time (s), by default 0.5", default = "0.5")
	options,args = parser.parse_args()

	base_t = 2e9

	if not options.nhost:
		print("please use -n to enter number of hosts")
		sys.exit(0)
	
	nhost = int(options.nhost)
	load = float(options.load)
	bandwidth = translate_bandwidth(options.bandwidth)
	time = float(options.time)*1e9 # translates to ns
	if bandwidth == None:
		print("bandwidth format incorrect")
		sys.exit(0)

	num_of_byte = 10000000
	cdf = [[num_of_byte, 0], [num_of_byte+1, 1]]

	# create a custom random generator, which takes a cdf, and generate number according to the cdf
	customRand = CustomRand()
	if not customRand.setCdf(cdf):
		print("Error: Not valid cdf")
		sys.exit(0)

	avg = customRand.getAvg()
	avg_inter_arrival = 1e9/(bandwidth*load/8./avg)

	output = "../trace/" + "ML_" + options.nhost + "_" + options.load + "_" + \
			options.bandwidth + "_" + options.time + ".tr"

	if not os.path.exists("../trace/"):
		os.makedirs("../trace/")
	ofile = open(output, "w")

	n_flow_estimate = int(time / avg_inter_arrival * nhost)
	ofile.write("%d \n"%n_flow_estimate)

	host_pair = [i for i in range(nhost)]
	random.shuffle(host_pair)

	GROUP_NUMBER = 8

	t = base_t
	n_flow = 0

	while True:
		inter_t = int(poisson(avg_inter_arrival))
		if inter_t <= 0:
			inter_t = 1
		
		t += inter_t
		if t > base_t + time:
			break

		size = int(customRand.rand())
		if size <= 0:
			size = 1

		for i in range(nhost):
			if i % GROUP_NUMBER == GROUP_NUMBER - 1 or i == nhost - 1:
				dst = (i // GROUP_NUMBER) * GROUP_NUMBER
			else:
				dst = i + 1
			
			ofile.write("%d %d %d %d\n"%(host_pair[i], host_pair[dst], size, t))

		n_flow += nhost

	ofile.seek(0)
	ofile.write("%d"%n_flow)
	ofile.close()
	print(n_flow)