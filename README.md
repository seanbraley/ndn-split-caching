# ndn-split-caching
ndnSIM code for split level caching, and tests to evaluate its performance compared to typical caching in standard and vertical handoff scenarios

## Purpose
This code is intended to address and test the idea of split level caching, that is to have two different caches with different policies, and a discrimination between the data that is coming to the node

## SETUP NOTE:
* You must replace /ns-3/src/ndnSIM/model/cs/ with the cs folder in this repo (there were changes made)
* You must also replace /ns-3/src/ndnSIM/utils/trie folder with the trie folder in this repo
* Follow ndnSIM guide here: http://ndnsim.net/2.0/getting-started.html for getting set up, create the scenario folder as they recommmend
* Copy the extensions and scenarios folders into the scenarios folder that gets created.


## Experiment Notes:
* In order to get statistics, you must move the trace files into a subfolder of the results folder
* Once there the get-stats script by running `python get-stats.py subfolder-name scenario-#`
* This script requires the following trace files:
    * app-delays-trace-mobile-#
    * app-delays-trace-stationary-#
    * cs-trace-ap-#
    * cs-trace-routers-#
* These trace files are created when a scenario is run