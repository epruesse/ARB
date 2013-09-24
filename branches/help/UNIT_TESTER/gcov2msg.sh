#!/bin/bash

CAKEDEPTH=3

enter() {
    local DIR=$1
    echo "cake[$CAKEDEPTH]: Entering directory \`$DIR'"
}
leave() {
    local DIR=$1
    echo "cake[$CAKEDEPTH]: Leaving directory \`$DIR'"
}

show_coverage() {
    local DIR=$1
    if [ -d $DIR/GENC ]; then
        # handle special compilation in PROBE_COM/GENC and NAMES_COM/GENC
        ./gcov2msg.pl --srcdirs=$DIR/C --builddir=$DIR $DIR/O 
    else 
        ./gcov2msg.pl $DIR 
    fi
}

show_coverage_otherdir() {
    local DIR=$1
    enter $DIR
    show_coverage $DIR
    local EXITCODE=$?
    leave $DIR
    return $EXITCODE
}

# ------------------------------------------------------------

main () {
    local FULLUNITDIR=$1
    local UNITLIBNAME=$2

    if [ -z "$UNITLIBNAME" ]; then
        echo "Usage: gcov2msg.sh fullunitdir unitlibname"
        false
    else
        # perform additional coverage analysis for some tests
        if [ "$UNITLIBNAME" = "arb_probe" ]; then
            show_coverage $FULLUNITDIR && \
            show_coverage_otherdir `dirname $FULLUNITDIR`/PROBE_COM
        else
            show_coverage $FULLUNITDIR
        fi
    fi
}

main $*
