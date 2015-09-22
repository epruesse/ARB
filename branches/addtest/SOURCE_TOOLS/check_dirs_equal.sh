#!/bin/bash

SED=${ARBHOME}/SH/arb_sed

check_existance_in() {
    local OTHER=$1
    local FILE=$2
    local RESULT=true

    while [ ! -z "$FILE" ]; do
        if [ ! -e $OTHER/$FILE ]; then
            echo "$OTHER/$FILE:0: Error: has not been generated"
            RESULT=false
        fi
        shift
        FILE=$2
    done
    $RESULT
}

check_equality() {
    local OLDDIR=$1
    local NEWDIR=$2
    local NAME=$3
    local RESULT=true

    while [ ! -z "$NAME" ]; do
        local OLDFILE=$OLDDIR/$NAME
        local NEWFILE=$NEWDIR/$NAME

        if [ ! -d $OLDFILE -a ! -d $NEWFILE ]; then
            local OPTIONS=
            OPTIONS="$OPTIONS --unified=3"
            OPTIONS="$OPTIONS --ignore-tab-expansion"
            OPTIONS="$OPTIONS --ignore-space-change"
            OPTIONS="$OPTIONS --ignore-blank-lines"
            OPTIONS="$OPTIONS --ignore-all-space"
            OPTIONS="$OPTIONS"

            local TMPFILE=/tmp/check_dirs_equal.$$

            diff $OPTIONS $OLDFILE $NEWFILE > $TMPFILE

            local ISDIFF=`wc -l <$TMPFILE`
            if [ "$ISDIFF" != "0" ]; then
                echo "-------------------- diff start"
                $ARBHOME/SOURCE_TOOLS/diff2grep.pl < $TMPFILE 
                echo "-------------------- diff end"
                RESULT=false
            fi

            rm $TMPFILE
        fi
        shift
        NAME=$3
    done
    $RESULT
}

get_files() {
    # returns relative paths of files matching WILDCARD 
    local DIR=$1
    local WILDCARD=$2
    ( \
        cd $DIR || exit; \
        find -P . -name "$WILDCARD" \
        | grep -v '\/\.svn\/' \
        | grep -v '\/patches\.arb\/' \
        | $SED 's/^\.\///' \
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

    local NEWFILES=`get_files $NEWDIR "$WILDCARD"`

    local RESULT=true
    
    check_missing $NEWDIR $OLDDIR "$WILDCARD" || RESULT=false
    check_equality $OLDDIR $NEWDIR $NEWFILES || RESULT=false

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

