#!/bin/bash

show () {
    ps -auxw | grep bin/arb_probe_server_worker | grep -v grep
}
count_servers() {
    show | wc -l
}

SERVERS=`count_servers`
echo "Found $SERVERS running servers:"
show
if [ $SERVERS != 0 ]; then
    echo "Killing servers.."
    pkill -f arb_probe_server_worker

    while [ `count_servers` != 0 ]; do
        echo "[waiting for servers to terminate]"
        sleep 1
    done
fi
echo "No running servers!"



