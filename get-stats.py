__author__ = 'sean.braley'
import os
import csv
import sys

#########################################################
## Change these to match the folder and scenario you want
#########################################################

folder = "size-100-75"
scenario = "04"

if len(sys.argv) == 3:
    folder = sys.argv[1]
    scenario = sys.argv[2]

base_policy = ("Lru", "Lfu", "Fifo")
special_policy = ("Lru", "Lfu", "Fifo")
cache_size = ("100", "500", "1000")

nodes_container = {}


def avg(l=[]):
    return sum(l)/float(len(l))


for bp in base_policy:
    for sp in special_policy:
        for size in cache_size:
            pass

ap_avg = []
router_avg = []

for tp in ('ap', 'routers'):
            # with open(os.path.join("results", "cs-trace-{0}-{1}-{2}-{3}.txt".format(bp, sp, size, "01")), "rb") as result:
    #with open(os.path.join("results", "basic-only-lru", "cs-trace-{0}.txt".format(tp)), "rb") as result:
    with open(os.path.join("results", folder, "cs-trace-{0}-{1}.txt".format(tp, scenario)), "rb") as result:
        content = result.readlines()
        content.pop(0)
        nodes = set()
        for x in content:
            nodes.add(x.split("\t")[1])

        # csv Output
        # with open(os.path.join("results", folder, "cs-trace-{0}-{1}.csv".format(tp, scenario)), "a") as output:

        for node in nodes:
            hits_per_second = [int(x.split("\t")[3].strip()) for x in content if x.split("\t")[1] == node and x.split("\t")[2] == "CacheHits"]
            misses_per_second = [int(x.split("\t")[3].strip()) for x in content if x.split("\t")[1] == node and x.split("\t")[2] == "CacheMisses"]

            cache_hit_ratio = (sum(hits_per_second)/float((sum(hits_per_second)+sum(misses_per_second)))) if \
                (sum(hits_per_second)+sum(misses_per_second)) != 0 else 0

            print "Node: {0} ({1}): Cache hit ratio: {2:.2f}".format(node, tp, cache_hit_ratio)

            if tp == "ap":
                ap_avg.append(cache_hit_ratio)
            else:
                router_avg.append(cache_hit_ratio)
                #print hits_per_second
                #print avg(hits_per_second)
                #print misses_per_second
                #print avg(misses_per_second)

                # answer_writer = csv.writer(output)

                # answer_writer.writerow(["Node: {0}".format(node), tp] + hits_per_second)

print("Average AP Hit Ratio: {0:.2f}".format(avg(ap_avg)))
print("Average Router Hit Ratio: {0:.2f}".format(avg(router_avg)))

sta_avg = []
sta_avg_throughput = []
sta_avg_delivery = []

mob_avg = []
mob_avg_throughput = []
mob_avg_delivery = []

for tp in ('mobile', 'stationary'):

    with open(os.path.join("results", folder, "app-delays-trace-{0}-{1}.txt".format(tp, scenario)), "rb") as result:
        content = result.readlines()
        content.pop(0)
        nodes = set()
        for x in content:
            nodes.add(x.split("\t")[1])

        for node in nodes:
                delays = [float(x.split("\t")[5].strip()) for x in content if x.split("\t")[1] == node and x.split("\t")[4] == "FullDelay"]

                avg_delay = avg(delays)

                throughput = len(delays)/600.0

                delivery_ratio = len(delays)/(600.0*10)

                print "Node: {0} ({1}): Average Delay: {2:.2f}; Throughput: {3:.2f}; Delivery Ratio: {4:.2f}".\
                    format(node, tp, avg_delay, throughput, delivery_ratio)

                if tp == "stationary":
                    sta_avg.append(avg_delay)
                    sta_avg_throughput.append(throughput)
                    sta_avg_delivery.append(delivery_ratio)
                else:
                    mob_avg.append(avg_delay)
                    mob_avg_throughput.append(throughput)
                    mob_avg_delivery.append(delivery_ratio)

print("Average Stationary Consumer Delay: {0:.2f}; Throughput: {1:.2f}; Delivery Ratio: {2:.2f}".format(avg(sta_avg), avg(sta_avg_throughput), avg(sta_avg_delivery)))
print("Average Mobile Consumer Delay: {0:.2f}; Throughput: {1:.2f}; Delivery Ratio: {2:.2f}".format(avg(mob_avg), avg(mob_avg_throughput), avg(mob_avg_delivery)))