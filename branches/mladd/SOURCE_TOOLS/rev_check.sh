#!/bin/bash

SED=${ARBHOME}/SH/arb_sed

current() {
    $ARBHOME/SOURCE_TOOLS/build_info.pl && \
        grep ARB_SVN_REVISION $ARBHOME/TEMPLATES/svn_revision.h \
        | $SED 's/#define ARB_SVN_REVISION //' \
        | $SED 's/"//g'
}

SUCC=last.success

set_succeeded() {
    echo `current` > $SUCC
}
reset() {
    if [ -f $SUCC ]; then rm $SUCC
    fi
}

last_succeeded() {
    if [ -f $SUCC ]; then cat $SUCC
    else echo "NoneSucceededYet"
    fi
}


if [ -z "$1" ]; then
    echo "Usage: rev_check.sh [ last_succeeded | current | set_succeeded | reset ]"
else
    if [ "$1" = "last_succeeded" ]; then last_succeeded
    else
        if [ "$1" = "current" ]; then current
        else
            if [ "$1" = "set_succeeded" ]; then set_succeeded
            else
                if [ "$1" = "reset" ]; then reset
                else
                    echo "Unknown argument '$1'"
                    false
                fi
            fi
        fi
    fi
fi



