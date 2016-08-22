#!/bin/bash

show_globals() {
    if [ \! -z $1 ]; then
        DIR=`dirname $1`
        echo $DIR:0: list of globals in $1
        nm -C --defined-only $1 | grep '^[0-9a-f]* [^brdtTWV]'
        shift
        show_globals $*
    fi
}

show_globals `find $ARBHOME -name "*.a"`

