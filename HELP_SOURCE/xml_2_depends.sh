#!/bin/bash -u

XSLTPROC=$1 ; shift
STYLESHEET=$1 ; shift
XSLTPROCARGS="$@"

while read LINE; do
    for XML in $LINE; do
        $XSLTPROC --stringparam xml $XML $XSLTPROCARGS $STYLESHEET $XML
    done
done
