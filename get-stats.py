__author__ = 'sean.braley'
import os
import csv

base_policy = ("Lru", "Lfu", "Fifo")
special_policy = ("Lru", "Lfu", "Fifo")
cache_size = ("100", "500", "1000")

nodes_container = {}


def avg(l=[]):
    return sum(l)/float(len(l))


for bp in base_policy:
    for sp in special_policy:
        for size in cache_size:
            with open(os.path.join("results", "cs-trace-{0}-{1}-{2}-{3}.txt".format(bp, sp, size, "01")), "rb") as result:
                content = result.readlines()
                content.pop(0)
                nodes = set()
                for x in content:
                    nodes.add(x.split("\t")[1])
                for node in nodes:
                    hits_per_second = [int(x.split("\t")[3].strip()) for x in content if x.split("\t")[1] == node and x.split("\t")[2] == "CacheHits"]
                    misses_per_second = [int(x.split("\t")[3].strip()) for x in content if x.split("\t")[1] == node and x.split("\t")[2] == "CacheMisses"]
                    if node == "1":
                        #print "Node: {0}".format(node)
                        #print hits_per_second
                        #print avg(hits_per_second)
                        #print misses_per_second
                        #print avg(misses_per_second)
                        with open(os.path.join("results", "cs-trace-{0}-{1}-{2}-{3}.csv".format(bp, sp, size, "01")), "wb") as output:
                            answer_writer = csv.writer(output)
                            for i in range(len(hits_per_second)):
                                answer_writer.writerow([hits_per_second[i], misses_per_second[i]])
                                print "Hit:Miss ratio: {0:.2f}:{1:.2f}".format((hits_per_second[i]), (misses_per_second[i]))
                    nodes_container[node] = "d"