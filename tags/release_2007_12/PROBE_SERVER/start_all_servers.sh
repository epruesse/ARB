#!/bin/bash

start_server() {
    ./bin/arb_probe_server_worker ps_serverdata/probe_db_$1_design.arb $1 ps_workerdir
}

echo 'Starting probe servers..'

# how many servers
EXPECT=6
start_server 15 &
start_server 16 &
start_server 17 &
start_server 18 &
start_server 19 &
start_server 20 &

show () {
    ps -auxw | grep bin/arb_probe_server_worker | grep -v grep
}
count_servers() {
    show | wc -l
}
while [ `count_servers` != $EXPECT ]; do
    sleep 1
done

echo $EXPECT servers are running.





