#!/bin/bash

show () {
    ps -auxw | grep bin/arb_probe_server_worker | grep -v grep
}
count_servers() {
    show | wc -l
}

SERVERS=`count_servers`
SERVERS=$SERVERS

if [ $SERVERS != 6 ]; then
    echo "Only '$SERVERS' servers are running - restarting them."
    ./restart_all_servers.sh
fi

