#!/bin/bash

./kill_all_servers.sh

# cleanup: remove result files older than 3 hours
echo Cleaning ps_workerdir..
find ./ps_workerdir/ -name "*.res" -mmin +180 -exec rm {} \;

./start_all_servers.sh

