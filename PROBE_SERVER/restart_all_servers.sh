#!/bin/bash

./kill_all_servers.sh
rm ps_workerdir/*
./start_all_servers.sh

