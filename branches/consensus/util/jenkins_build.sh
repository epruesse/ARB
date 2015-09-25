#!/bin/bash
set -x
set -o errexit

ARG=${1:-}

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

# fallback language (avoid perl spam)
if [ -z "${LANG:-}" ]; then
    echo "Note: LANG was unset (using fallback 'C')"
    export LANG=C
else
    echo "Note: LANG is '$LANG'"
fi

# prepare config.makefile
CFG=config.makefile
rm -f $CFG

TARSUF=""
UNIT_TESTS=1
DEBUG=0
SANITIZE=1
DEVELOPER="JENKINS" # see ../UNIT_TESTER/README.txt@DEVEL_JENKINS

case $MODE in
  DEBUG)
    DEBUG=1
    TARSUF="-dbg"
    ;;
  NDEBUG)
    TARSUF="-ndbg"
    ;;
  RELEASE)
    DEVELOPER="RELEASE"
    UNIT_TESTS=0
    SANITIZE=0 # never sanitize release!
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
    # OSX make causes random failures if called with '-j 2'
    # (e.g. target 'binlink' gets triggered multiple times, causing build failure when it's executed concurrently)
    JMAKE=make
    ;;
  Linux)
    echo "LINUX := 1" >> $CFG
    echo "MACH := LINUX" >> $CFG
    JMAKE="/usr/bin/time --verbose make -j `util/usecores.pl`"
    ;;
  *)
    echo "Error: unhandled OSNAME '$OSNAME'"
    false
    ;;
esac

if [ -z "${TGTNAME:-}" ]; then
    echo "Error: unknown TGTNAME - build refused"
    false
fi
    
echo "DEBUG := $DEBUG" >> $CFG
echo "UNIT_TESTS := $UNIT_TESTS" >> $CFG
echo "OPENGL := 0" >> $CFG
echo "DEVELOPER := $DEVELOPER" >> $CFG
echo "DEBUG_GRAPHICS := 0" >> $CFG
echo "PTPAN := 0" >> $CFG
echo "ARB_64 := 1" >> $CFG
echo "TRACESYM := 1" >> $CFG
echo "SANITIZE := $SANITIZE" >> $CFG
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
    # JMAKE="make"
    if [ "$ARG" == "fake_build" ]; then
        echo "Faking build"
        echo "Faked arb.tgz"     > arb.tgz
        echo "Faked arb-dev.tgz" > arb-dev.tgz
    else
        if [ "$ARG" == "from_tarball" ]; then
            echo "Test clean before make (tarball build)"
            ${JMAKE} clean
        fi
        ${JMAKE} build
        ${JMAKE} tarfile_quick
    fi

    # jenkins archieves all files matching "**/arb*.tgz"
    # jenkins publishes     files matching "**/arb*.tgz", but not "**/arb*dev*.tgz,**/arb*bin*.tgz"

    if [ -n "${SVN_TAG:-}" ]; then
        # tagged build
        VERSION_ID=${SVN_TAG}${TARSUF}
        # remove arb-prefixes (added below)
        VERSION_ID="${VERSION_ID##arb[-_]}"
    else
        # normal build
        VERSION_ID=r${SVN_REVISION}${TARSUF}
    fi

    VERSION_ID=arb-${VERSION_ID}
    VERSION_ID_TARGET=${VERSION_ID}.${TGTNAME}

    if [ "$MODE" == "RELEASE" ]; then
        if [ "${TGTNAME}" == "ubuntu1004-amd64" ]; then
            # perform things needed only once (pack source, copy README + install script):
            # 1. pack source (svn version of slave and master must match!)
            if [ "$ARG" == "fake_build" ]; then
                echo "Faked ${VERSION_ID}-source.tgz" > ${VERSION_ID}-source.tgz
            else
                if [ "$ARG" == "from_tarball" ]; then
                    echo "Note: build from tarball - do not attempt to create a tarball"
                else
                    # check resource usage:
                    ${JMAKE} check_res

                    # save tarball:
                    ${JMAKE} save
                    # archived and published on ftp:
                    cp --dereference arbsrc.tgz ${VERSION_ID}-source.tgz
                    rm arbsrc*.tgz
                fi
            fi
            # 2. move extra files into folder 'toftp' - content is copied to release directory
            mkdir toftp
            cp -p arb_README.txt toftp
            cp -p arb_install.sh toftp
            ls -al toftp
        fi

        # archived and published on ftp:
        mv arb.tgz ${VERSION_ID_TARGET}.tgz
    else
        # only archived (needed by SINA):
        mv arb.tgz ${VERSION_ID_TARGET}-bin.tgz
    fi
    # only archived (needed by SINA):
    mv arb-dev.tgz ${VERSION_ID_TARGET}-dev.tgz

    ${JMAKE} ut

    echo "-------------------- compiled-in version info:"
    (bin/arb_ntree --help || true)
    echo "-------------------- existing tarballs:"
    ls -al arb*.tgz
    echo "--------------------"
else
    echo "Skipping this build."
    # @@@ maybe need to fake unit-test-result here
fi
