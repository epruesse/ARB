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
  export GCC=clang
  export GPP=clang++
  export MACH=DARWIN
;;
Linux)
;;
*)
 echo "Error: unhandled OSNAME '$OSNAME'"
 false
;;
esac

rm -f config.makefile

# prepare config.makefile
CFG=config.makefile
UNIT_TESTS=1# default is to run unit tests
case $MODE in
DEBUG)
 echo "DEBUG := 1" >> $CFG
 ;;
RELEASE)
 echo "DEBUG := 0" >> $CFG
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

echo "UNIT_TESTS := $UNIT_TESTS" >> $CFG
echo "OPENGL := 0" >> $CFG
echo "DEVELOPER := ANY" >> $CFG
echo "DEBUG_GRAPHICS := 0" >> $CFG
echo "PTPAN := 0" >> $CFG
echo "ARB_64 := 1" >> $CFG
echo "TRACESYM := 1" >> $CFG
echo "COVERAGE := 0" >> $CFG
# done with config.makefile

make build
make tarfile_quick

if [ $MODE = "DEBUG" ]; then
  DEBUG="-dbg"
fi
mv arb.tgz arb-r${SVN_REVISION}${DEBUG}.${TGTNAME}.tgz
mv arb-dev.tgz arb-r${SVN_REVISION}${DEBUG}-dev.${TGTNAME}.tgz

make ut
