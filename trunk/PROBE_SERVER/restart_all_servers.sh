#!/bin/bash

./kill_all_servers.sh
rm -f ps_workerdir/*
./start_all_servers.sh

