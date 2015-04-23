# ndn-split-caching
ndnSIM code for split level caching, and tests to evaluate its performance compared to typical caching in standard and vertical handoff scenarios

## Purpose
This code is intended to adress and test the idea of split level caching, that is to have two different caches with different policies, and a discrimination between the data that is coming to the node

## Project Stages
- [ ] Implement Basic ndnSIM scenario with caches
- [ ] Compare performance of Lru vs Lfu vs. FiFo

## SETUP NOTE:
* You must replace /ns-3/src/ndnSIM/model/cs/ with the cs folder in this repo (there were changes made)
* You must also replace /ns-3/src/ndnSIM/utils/trie folder with the trie folder in this repo

Test add to readme!  RS
