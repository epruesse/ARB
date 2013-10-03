#!/bin/bash

if [ -z "$4" ] ; then
    echo "Usage: genhelp.sh outputfile title header inputtext"
    echo "       Generates help pre-source 'outputfile' from 'inputtext'"
    exit 1
fi

OUTPUTFILE=$1
TITLE=$2
HEADER=$3
INPUTTEXT=$4
SED=${ARBHOME}/SH/arb_sed

if [ \! -f $HEADER ]; then
    echo "Header '$HEADER' not found"
    exit 1
fi
if [ \! -f $INPUTTEXT ]; then
    echo "Input '$INPUTTEXT' not found"
    exit 1
fi

write_help() {
    echo "$TITLE"
    echo ""
    echo "# Do not edit!!! Generated from ../$INPUTTEXT"
    echo ""
    cat $HEADER
    $SED -e 's/^/    /' < $INPUTTEXT
}

test -f $OUTPUTFILE && chmod u+w $OUTPUTFILE
write_help > $OUTPUTFILE
test -f $OUTPUTFILE && chmod a-w $OUTPUTFILE
