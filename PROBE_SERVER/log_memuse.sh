#!/bin/bash

LOG=memuse.log

log() {
    ps -auxw | grep arb | grep -v grep >> $1
}

echo "log starts at `date`" > $LOG
tail -f $LOG &

while [ 1 = 1 ]; do
    log $LOG
    sleep 4
done

