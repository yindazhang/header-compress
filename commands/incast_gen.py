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
	parser.add_option("-c", "--cdf", dest = "cdf_file", help = "the file of the traffic size cdf", default = "Hadoop")
	parser.add_option("-n", "--nhost", dest = "nhost", help = "number of hosts", default = "215")
	parser.add_option("-l", "--load", dest = "load", help = "the percentage of the traffic load to the network capacity, by default 0.5", default = "0.4")
	parser.add_option("-b", "--bandwidth", dest = "bandwidth", help = "the bandwidth of host link (G/M/K), by default 25G", default = "25G")
	parser.add_option("-o", "--oversubscription", dest = "oversubscription", help = "the oversubscription ratio, by default 4", default = "4")
	parser.add_option("-i", "--incast", dest = "incast", help = "the incast degree, by default 50", default = "16")
	parser.add_option("-t", "--time", dest = "time", help = "the total run time (s), by default 0.5", default = "0.5")
	options,args = parser.parse_args()

	base_t = 2e9

	if not options.nhost:
		print("please use -n to enter number of hosts")
		sys.exit(0)
	
	fileName = "traffic_cdf/" + options.cdf_file + ".txt"

	nhost = int(options.nhost)
	load = float(options.load)
	bandwidth = translate_bandwidth(options.bandwidth)
	ratio = float(options.oversubscription)
	time = float(options.time)*1e9 # translates to ns
	incast = int(options.incast)
	if bandwidth == None:
		print("bandwidth format incorrect")
		sys.exit(0)

	file = open(fileName,"r")
	lines = file.readlines()
	# read the cdf, save in cdf as [[x_i, cdf_i] ...]
	cdf = []
	for line in lines:
		# print(line.strip().split('  '))
		x,z = map(float, line.strip().split())
		cdf.append([x,z])

	# create a custom random generator, which takes a cdf, and generate number according to the cdf
	customRand = CustomRand()
	if not customRand.setCdf(cdf):
		print("Error: Not valid cdf")
		sys.exit(0)

	avg = customRand.getAvg()
	avg_inter_arrival = 1e9/(bandwidth*load/8./avg)/nhost

	output = "../trace/" + options.cdf_file + options.incast + "_" + options.nhost + "_" + options.load + "_" + \
			options.bandwidth + "_" + options.time + ".tr"

	if not os.path.exists("../trace/"):
		os.makedirs("../trace/")
	ofile = open(output, "w")

	t = base_t
	n_flow = 0
	vec = []

	while True:
		inter_t = int(poisson(avg_inter_arrival))
		if inter_t <= 0:
			inter_t = 1
		
		t += inter_t
		if t > base_t + time:
			break

		src = random.randint(1, nhost-1)
		dst = random.randint(1, nhost-1)
		while dst == src:
			dst = random.randint(1, nhost-1)

		size = int(customRand.rand())
		n_flow += 1
		vec.append((t, src, dst, size))
	
	avg_inter_arrival = 1e9/(bandwidth*ratio*0.95/8./avg)
	host_pair = [i for i in range(36, 180)]
	random.shuffle(host_pair)
	if nhost < len(host_pair):
		print("Error: nhost too small for incast generation")
		sys.exit(0)

	print(len(vec))

	idx = 0
	t = base_t
	while t < base_t + time:		
		inter_t = int(poisson(avg_inter_arrival))
		if inter_t <= 0:
			inter_t = 1

		t += inter_t
		if t > base_t + time:
			break

		src = host_pair[idx % incast]
		idx += 1

		s = int(customRand.rand())
		n_flow += 1
		vec.append((t, src, 0, s))
		
	vec = sorted(vec, key=lambda x: x[0])
	for i in range(len(vec)):
		ofile.write("%d %d %d %d\n"%(vec[i][1], vec[i][2], vec[i][3], vec[i][0]))

	ofile.close()
	print(n_flow)
