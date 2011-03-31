#!/bin/bash

check_existance_in() {
    local OTHER=$1
    local FILE=$2
    local RESULT=true

    while [ ! -z "$FILE" ]; do
        # local NAME=`basename $FILE`
        if [ ! -e $OTHER/$FILE ]; then
            echo "$OTHER/$FILE:0: Error: has not been generated"
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
        # local NAME=`basename $NEWFILE`
        local OLDFILE=$OLDDIR/$NEWFILE

        if [ ! -d $OLDFILE -a ! -d $NEWFILE ]; then
            local ISDIFF=`diff $NEWFILE $OLDFILE | wc -l`
            if [ "$ISDIFF" != "0" ]; then
                diff_as_grep $OLDFILE $NEWFILE
                RESULT=false
            # else
                # echo "equal $NEWFILE"
            fi
        fi
        shift
        NEWFILE=$2
    done
    $RESULT
}

get_files() {
    # returns relative paths of files matching WILDCARD 
    local DIR=$1
    local WILDCARD=$2
    # echo "DIR='$DIR'" 1>&2
    # echo "WILDCARD='$WILDCARD'" 1>&2
    ( \
        cd $DIR || exit; \
        find -P . -name "$WILDCARD" \
        | grep -v '\/\.svn\/' \
        | grep -v '\/patches\.arb\/' \
        | sed 's/^\.\///' \
        )
}

check_missing() {
    local D1=$1
    local D2=$2
    local WILDCARD="$3"

    local FILES1=`get_files $D1 "$WILDCARD"`
    local FILES2=`get_files $D2 "$WILDCARD"`

    local RESULT=true
    
    check_existance_in $D2 $FILES1 || RESULT=false
    check_existance_in $D1 $FILES2 || RESULT=false

    $RESULT
}

compare_dirs() {
    local NEWDIR=$1
    local OLDDIR=$2
    local WILDCARD="$3"
    local RECURSE=$4

    local NEWFILES=`get_files $NEWDIR "$WILDCARD"`

    local RESULT=true
    
    check_missing $NEWDIR $OLDDIR "$WILDCARD" || RESULT=false
    check_equality $OLDDIR $NEWFILES || RESULT=false

    $RESULT
}

# --------------------

GLOBAL_NEWDIR=$1
GLOBAL_OLDDIR=$2
GLOBAL_WILDCARD="$3"

if [ -z "$GLOBAL_WILDCARD" ]; then
    echo "Usage: check_dirs_equal.sh newdir olddir wildcard"
    echo "Recursively compares two directory-trees"
    false
else
    cd $GLOBAL_NEWDIR || exit
    GLOBAL_NEWDIR=`pwd`
    cd - >/dev/null
    cd $GLOBAL_OLDDIR || exit
    GLOBAL_OLDDIR=`pwd`
    cd - >/dev/null

    # echo "GLOBAL_NEWDIR='$GLOBAL_NEWDIR'"
    # echo "GLOBAL_OLDDIR='$GLOBAL_OLDDIR'"
    
    compare_dirs $GLOBAL_NEWDIR $GLOBAL_OLDDIR "$GLOBAL_WILDCARD" 
fi

