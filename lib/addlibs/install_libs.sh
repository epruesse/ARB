#!/bin/bash

ADDLIBDIR=$ARBHOME/lib/addlibs

install_lib() {
    # $1=libname $2=linknam

    LIB=$ADDLIBDIR/$1
    LINK=$ADDLIBDIR/$2

    if [ \! -r $LIB ]; then
        echo "Library $LIB missing. Please contact authors at devel@arb-home.de"
        false
    else
        if [ \! -r $LINK ]; then
            if [ -r $LIB ]; then
                echo "Symlinking $LINK -> $LIB"
                ln -s $1 $2
            fi
        else
            echo "Symlink $LINK already present"
            echo "(should link to $LIB or newer version):"
            ls -al $2
            echo ""
        fi
    fi
}

cd $ADDLIBDIR
install_lib libGLEW.so.1.3.5 libGLEW.so.1
install_lib libGLEW.so.1.3.5 libGLEW.so.1.3

