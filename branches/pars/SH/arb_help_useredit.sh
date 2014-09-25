#!/bin/bash
# 
# helper script to pack helpfiles edited in userland into a tarball
# in order to send them back to ARB developers

TARBALL=~/arb_edited_helpfiles.tgz
HELPFILE=${1:-}
MODE=${2:-}

pack() {
    local WORKDIR=~/.arb_tmp/help.$$
    mkdir -p $WORKDIR
    local PREVDIR=`pwd`
    cd $WORKDIR

    local SUFFIX=ORG
    if [ "$MODE" = "end" ]; then
        SUFFIX=USEREDIT
    fi

    local PACKED_FILE=$WORKDIR/${HELPFILE%.hlp}_${SUFFIX}.hlp
    local FULLHELP=${ARBHOME}/lib/help/${HELPFILE}

    if [ -f $TARBALL ]; then
        tar -zxf $TARBALL
    fi

    if [ -f $PACKED_FILE ]; then
        if [ "$MODE" = "end" ]; then
            cp -p $FULLHELP $PACKED_FILE
        fi
    else
        cp -p $FULLHELP $PACKED_FILE
    fi

    tar -zcf $TARBALL .

    cd $PREVDIR
    rm -rf $WORKDIR
}

show() {
    if [ -f $TARBALL ]; then
        echo "------------------------------------------------------------"
        echo "Your helpfile modifications were stored in archive"
        echo "    $TARBALL"
        echo "Contents are:"
        echo ""
        tar -ztvf $TARBALL
        echo ""
        echo "Please send that file to devel@arb-home.de if you think"
        echo "your modifications should be available to the public."
        echo "(Remove the archive file afterwards)"
        echo "------------------------------------------------------------"
    fi
}

case "$MODE" in
    start|end)
    pack $HELPFILE $MODE
    if [ "$MODE" = "end" ]; then show; fi
    ;;

    *)
    echo "Usage: arb_help_useredit.sh helpfile start|end"
    echo "Packs current state of 'helpfile' into $TARBALL"
    echo "Script is called by ARB whenever you edit a helpfile"
    ;;
esac

