#!/bin/bash
set -x

# set stamdard variables
export ARBHOME=`pwd`
export PATH=$ARBHOME/bin:$PATH
export LD_LIBRARY_PATH=$ARBHOME/lib

#set by OS variables
case `uname -s` in
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
# unsupported OS?
;;
esac


rm -f config.makefile

# prepare config.makefile
case $MODE in
DEBUG)
 echo "DEBUG := 1" >> config.makefile
 ;;
RELEASE)
 echo "DEBUG := 0" >> config.makefile
 ;;
esac

case `uname -s` in
Darwin)
 echo "DARWIN := 1" >> config.makefile
 echo "MACH := DARWIN" >> config.makefile
 echo "UNIT_TESTS := 0" >> config.makefile
 ;;
Linux)
 echo "LINUX := 1" >> config.makefile
 echo "MACH := LINUX" >> config.makefile
 echo "UNIT_TESTS := 1" >> config.makefile
 ;;
esac

echo "OPENGL := 0" >> config.makefile
echo "DEVELOPER := ANY" >> config.makefile
echo "DEBUG_GRAPHICS := 0" >> config.makefile
echo "PTPAN := 0" >> config.makefile
echo "ARB_64 := 1" >> config.makefile
echo "TRACESYM := 1" >> config.makefile
echo "COVERAGE := 0" >> config.makefile

make build
make tarfile_quick

if [ $MODE = "DEBUG" ]; then
  DEBUG="-dbg"
fi
mv arb.tgz arb-r${SVN_REVISION}${DEBUG}.${TGTNAME}.tgz
mv arb-dev.tgz arb-r${SVN_REVISION}${DEBUG}-dev.${TGTNAME}.tgz

make ut
