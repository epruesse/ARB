#!/bin/bash
set -x
set -o errexit

# set standard variables expected by ARB build
export ARBHOME=`pwd`
export PATH=$ARBHOME/bin:$PATH
export LD_LIBRARY_PATH=$ARBHOME/lib

# OS dependant settings
OSNAME=`uname -s`
case $OSNAME in
  Darwin)
    export PREFIX=/opt/local
    export PATH=$PATH:$PREFIX/sbin:$PREFIX/bin
    export CC=clang
    export CXX=clang++
    export MACH=DARWIN
    ;;
  Linux)
    ;;
  *)
    echo "Error: unhandled OSNAME '$OSNAME'"
    false
    ;;
esac

# prepare config.makefile
CFG=config.makefile
rm -f $CFG

TARSUF=""
UNIT_TESTS=1

case $MODE in
  DEBUG)
    DEBUG=1
    TARSUF="-dbg"
    ;;
  NDEBUG)
    DEBUG=0
    TARSUF="-ndbg"
    ;;
  RELEASE)
    DEBUG=0
    TARSUF="-user"
    UNIT_TESTS=0
    ;;
  *)
    echo "Error: unknown MODE '$MODE' passed to jenkins_build.sh"
    false
    ;;
esac

case $OSNAME in
  Darwin)
    echo "DARWIN := 1" >> $CFG
    echo "MACH := DARWIN" >> $CFG
    UNIT_TESTS=0
    ;;
  Linux)
    echo "LINUX := 1" >> $CFG
    echo "MACH := LINUX" >> $CFG
    ;;
  *)
    echo "Error: unhandled OSNAME '$OSNAME'"
    false
    ;;
esac

echo "DEBUG := $DEBUG" >> $CFG
echo "UNIT_TESTS := $UNIT_TESTS" >> $CFG

echo "OPENGL := 0" >> $CFG
echo "DEVELOPER := ANY" >> $CFG
echo "DEBUG_GRAPHICS := 0" >> $CFG
echo "PTPAN := 0" >> $CFG
echo "ARB_64 := 1" >> $CFG
echo "TRACESYM := 1" >> $CFG
echo "COVERAGE := 0" >> $CFG
# done with config.makefile

# skip build?
BUILD=1
if [ "$MODE" == "NDEBUG" -a $UNIT_TESTS == 0 ]; then
    echo "Modes NDEBUG and RELEASE are identical for $OSNAME"
    BUILD=0
fi

# build, tar and test
if [ $BUILD == 1 ]; then
    make build
    make tarfile_quick

    if [ "$MODE" == "RELEASE" ]; then
        # published on ftp:
        mv arb.tgz arb-r${SVN_REVISION}${TARSUF}.${TGTNAME}.tgz
    else
        # not published on ftp (needed by SINA):
        mv arb.tgz arb-r${SVN_REVISION}${TARSUF}-bin.${TGTNAME}.tgz
    fi
    # not published on ftp (needed by SINA):
    mv arb-dev.tgz arb-r${SVN_REVISION}${TARSUF}-dev.${TGTNAME}.tgz

    make ut
else
    echo "Skipping this build."
    # @@@ maybe need to fake unit-test-result here
fi
