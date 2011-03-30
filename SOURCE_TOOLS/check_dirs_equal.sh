#!/bin/bash

check_existance_in() {
    local OTHER=$1
    local FILE=$2
    local RESULT=true

    while [ ! -z "$FILE" ]; do
        local NAME=`basename $FILE`
        if [ ! -e $OTHER/$NAME ]; then
            echo "$OTHER/$NAME:0: Error: has not been generated"
            RESULT=false
        fi
        shift
        FILE=$2
    done
    $RESULT
}

diff_as_grep() {
    local OLDFILE=$1
    local NEWFILE=$2

    local OPTIONS=
    OPTIONS="$OPTIONS --unified=3"
    OPTIONS="$OPTIONS --ignore-tab-expansion"
    OPTIONS="$OPTIONS --ignore-space-change"
    OPTIONS="$OPTIONS --ignore-blank-lines"
    OPTIONS="$OPTIONS"

    echo "-------------------- diff start"
    diff $OPTIONS $OLDFILE $NEWFILE | $ARBHOME/SOURCE_TOOLS/diff2grep.pl
    echo "-------------------- diff end"
}

check_equality() {
    local OLDDIR=$1
    local NEWFILE=$2
    local RESULT=true

    while [ ! -z "$NEWFILE" ]; do
        local NAME=`basename $NEWFILE`
        local OLDFILE=$OLDDIR/$NAME
        local ISDIFF=`diff $NEWFILE $OLDFILE | wc -l`
        if [ "$ISDIFF" != "0" ]; then
            diff_as_grep $OLDFILE $NEWFILE
            RESULT=false
        else
            echo "equal $NEWFILE"
        fi
        shift
        NEWFILE=$2
    done
    $RESULT
}

check_missing() {
    local D1=$1
    local D2=$2
    local TYPE=$3

    local FILES1=`dir $D1/*.$TYPE`
    local FILES2=`dir $D2/*.$TYPE`

    check_existance_in $D2 $FILES1 && \
    check_existance_in $D1 $FILES2
}

compare_dirs() {
    local NEWDIR=$1
    local OLDDIR=$2
    local TYPE=$3

    local NEWFILES=`dir $NEWDIR/*.$TYPE`

    check_missing $NEWDIR $OLDDIR $TYPE && \
    check_missing $OLDDIR $NEWDIR $TYPE && \
    check_equality $OLDDIR $NEWFILES
}

# --------------------

NEWDIR=$1
OLDDIR=$2
TYPE=$3

if [ -z "$TYPE" ]; then
    echo "Usage: check_dirs_equal.sh newdir olddir extension"
    false
else
    compare_dirs $NEWDIR $OLDDIR $TYPE
fi
