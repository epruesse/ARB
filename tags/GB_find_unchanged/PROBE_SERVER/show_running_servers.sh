#!/bin/bash

show () {
    ps -auxw | grep bin/arb_probe_server_worker | grep -v grep
}
count_servers() {
    show | wc -l
}

SERVERS=`count_servers`
SERVERS=$SERVERS

echo "Found $SERVERS running servers:"
show

