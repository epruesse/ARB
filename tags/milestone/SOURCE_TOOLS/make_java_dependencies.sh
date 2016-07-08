#!/bin/bash
# ------------------------------------------------------------------------
#
#  very simple java dependency generator
#  XXX.class depends on all *.java containing the word XXX
#
#  Coded by Ralf Westram (coder@reallysoft.de) in September 2003
#  Copyright Department of Microbiology (Technical University Munich)
#
#  Visit our web site at: http://www.arb-home.de/
#
# ------------------------------------------------------------------------

SED=${ARBHOME}/SH/arb_sed

find_deps_for() {
    grep -w -H $1 *.java | $SED -e 's/^\([^:]*\)\.java:.*$/\1.class/ig' | sort | uniq
}

append_dep() {
    $SED -e "s/$/ : $1/ig"
}

make_dependencies() {
    for JAVA in *.java; do
        BASE=`basename $JAVA .java`
        DEPEND=`find_deps_for $BASE | append_dep $BASE.java`
        echo "$DEPEND"
    done
}

make_dependencies | sort


