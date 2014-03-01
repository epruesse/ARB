#!/bin/bash
set -x
set -o errexit

FAKE=${1:-}

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
    TARSUF=""
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

if [ -z "${TGTNAME:-}" ]; then
    echo "Error: unknown TGTNAME - build refused"
    false
fi
    
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
    if [ "$FAKE" == "fake_build" ]; then
        echo "Faking build"
        echo "Faked arb.tgz"     > arb.tgz
        echo "Faked arb-dev.tgz" > arb-dev.tgz
    else
        make build
        make tarfile_quick
    fi

    # jenkins archieves all files matching "**/arb*.tgz"
    # jenkins publishes     files matching "**/arb*.tgz", but not "**/arb*dev*.tgz,**/arb*bin*.tgz"

    RELEASE_SOURCE=0
    if [ -n "${SVN_TAG:-}" ]; then
        # tagged build
        RELEASE_SOURCE=1
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
            # this section is executed only once
            if [ $RELEASE_SOURCE == 1 ]; then # only done for tagged builds
                # pack source (svn version of slave and master have to match!)
                if [ "$FAKE" == "fake_build" ]; then
                    echo "Faked ${VERSION_ID}-source.tgz" > ${VERSION_ID}-source.tgz
                else
                    make save
                    # archived and published on ftp:
                    cp --dereference arbsrc.tgz ${VERSION_ID}-source.tgz
                    rm arbsrc*.tgz
                fi
            fi
            # move extra files into folder 'toftp' - contents are copied to ftp flatened
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

    make ut

    echo "-------------------- compiled-in version info:"
    (bin/arb_ntree --help || true)
    echo "-------------------- existing tarballs:"
    ls -al arb*.tgz
    echo "--------------------"
else
    echo "Skipping this build."
    # @@@ maybe need to fake unit-test-result here
fi
