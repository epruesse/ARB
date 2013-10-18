#!/bin/bash

if [ -z "$5" ] ; then # need at least 2 'inputtext's
    echo "Usage: genhelptree.sh outputfile title header [inputtext]+"
    echo "       Generates help pre-sources 'outputfile_*' from several 'inputtext's"
    echo "       Creates an index page."
    exit 1
fi

INDEXFILE=$1
MAINTITLE=$2
HEADER=$3
shift;shift;shift
SED=${ARBHOME}/SH/arb_sed

test -f $INDEXFILE && chmod u+w $INDEXFILE

BASE=`echo $INDEXFILE | $SED -e 's/^.*\///' | $SED -e 's/.help//'`
PREFIX=`echo $INDEXFILE | $SED -e 's/\/[^\/]*$//'`
echo "BASE='$BASE' PREFIX='$PREFIX'"

write_index() {
    echo "$BASE [docindex]"
    echo ""
    echo "Documents provided with $BASE:"
    echo ""
#     cat $HEADER

    while [ \! -z "$1" ]; do
        SUBBASE=`echo $1 | $SED -e 's/^.*\///'`
        ESCAPED_SUBBASE=`echo $SUBBASE | $SED -e 's/\./_/'`
        echo "    - LINK{agde_${BASE}_sub_${ESCAPED_SUBBASE}.hlp}"
        shift
    done
}

write_index $* > $INDEXFILE
test -f $INDEXFILE && chmod a-w $INDEXFILE

while [ \! -z "$1" ]; do
    SUBBASE=`echo $1 | $SED -e 's/^.*\///'`
    ESCAPED_SUBBASE=`echo $SUBBASE | $SED -e 's/\./_/'`
    ./genhelp.sh $PREFIX/${BASE}_sub_$ESCAPED_SUBBASE.help "$SUBBASE" $HEADER $1
    shift
done

# exit 1
