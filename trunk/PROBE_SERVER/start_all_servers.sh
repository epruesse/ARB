#!/bin/bash

start_server() {
    ./bin/arb_probe_server_worker ps_serverdata/probe_db_$1_design.arb $1 ps_workerdir
}

start_server 15 &
start_server 16 &
start_server 17 &
start_server 18 &
start_server 19 &
start_server 20 &

echo Servers started.



