#!/bin/bash
#
# How to use:
#
# A. SETUP
# --------
#
# 1. create a new directory, e.g.
# mkdir my_arb_compile; cd my_arb_compile
# 
# 2. checkout arb, e.g. (the last argument is the name of the target directory) 
# svn co svn+ssh://USERNAME@svn.arb-home.de/svn/ARB/trunk myARB
# # or
# svn co http://vc.arb-home.de/readonly/trunk myARB
#
# 3. link the included compile script, e.g.
# ln -s myARB/util/arb_compile.sh compile_myARB.sh
#
# 4. call the script
# ./compile_myARB.sh
#
# 5. review the generated config
# vi compile_myARB.config
#
# 6. call the script again (this will generate config.makefile and fail)
# ./compile_myARB.sh
#
# 7. edit the generated config.makefile (default builds a openGL/64bit/linux-ARB)
# vi myARB/config.makefile
#
# 8. call the script again (this shall succeed)
# ./compile_myARB.sh
#
#
# B. UPGRADE
# -----------
#
# 1. simply call the script to upgrade ARB to newest revision
# ./compile_myARB.sh
#
#
# C. Multiple versions (in one 'my_arb_compile' directory)
# --------------------
#
# When you use a different name for the link target in step A.3. it
# is possible to work with multiple checkouts and multiple configs
# inside the same 'my_arb_compile' directory.
# The name used as checkout directory has to be changed for subsequent
# versions. Change 'myARB' to a different name in steps A.2., A.3. and A.7
#


default_config() {
    echo "# config to compile ARB via $BASENAME.sh"
    echo ""
    echo "# Subdirectory in which ARB is located:"
    echo "SUBDIR=myARB"
    echo ""
    echo "# Real CPU cores"
    echo "CORES=2"
    echo ""
    echo "# SVN revision to use (empty=newest)"
    echo "REVISION="
    echo ""
}

update() {
    set -x
    make clean && \
        svn revert -R . && \
        svn up --force $UPDATE_ARGS
}
build() {
    make -j$CORES tarfile
}

upgrade() {
    echo "Upgrading ARB checkout in '$ARBHOME'"
    echo ""
    echo "Starting build process"
    echo "(you may watch the output using"
    echo "     less +F $LOG"
    echo "in another terminal)"
    echo ""
    (update && build) >& $LOG
}

arbshell() {
    echo "ARBHOME now is '$ARBHOME'"
    $ARBHOME/bin/arb_ntree --help 2>&1 | grep version
    tcsh
    ARBHOME=$OLDARBHOME
    echo "ARBHOME now is again '$ARBHOME'"
}

# ---------------------------------------- 

if [ !$?ARBHOME ]; then
    ARBHOME=''
fi    
OLDARBHOME=$ARBHOME

BASEDIR=`pwd`
BASENAME=`basename $0 | perl -pne 's/.sh//'`

echo "BASENAME='$BASENAME'"

CONFIG=$BASENAME.config
LOG=$BASEDIR/$BASENAME.log

if [ -f $CONFIG ]; then
    source $CONFIG
else
    default_config > $CONFIG
    echo "$CONFIG has been generated"
    SUBDIR=
fi

if [ "$SUBDIR" = "" ]; then
    echo "Review config, then rerun this script"
else
    ARBHOME=$BASEDIR/$SUBDIR

    if [ ! -d $ARBHOME ]; then
        echo "ARBHOME='$ARBHOME' (no such directory)"
    else
        PATH=$ARBHOME/bin:$PATH
        if [ !$?LD_LIBRARY_PATH ]; then
            LD_LIBRARY_PATH=$ARBHOME/lib
        fi
        LD_LIBRARY_PATH=$ARBHOME/lib:$LD_LIBRARY_PATH

        export ARBHOME PATH LD_LIBRARY_PATH

        cd $ARBHOME

        if [ "$REVISION" = "" ]; then
            UPDATE_ARGS=
        else
            UPDATE_ARGS="-r $REVISION"
        fi

        (upgrade && echo "ARB upgrade succeeded" && arbshell) || ( echo "Error: Something failed (see $LOG)" && false)
    fi
fi

