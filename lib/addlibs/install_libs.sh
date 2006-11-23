#!/bin/bash

install_lib() {
    # $1=libname $2=linknam $3=destdir

    if [ \! -r $1 ]; then
        echo "Library $1 missing. Please contact authors at devel@arb-home.de"
        false
    else
        DESTLIB=$3/$1
        DESTLINK=$3/$2
        if [ \! -r $DESTLIB ]; then
            echo "Required library $DESTLIB not present."
            echo "Shall I install it? (y/n) [y]"
            read VAR
            case "$VAR" in
              n)
                echo "Skipped installation of $DESTLIB. ARB may not work properly"
              ;;
              *)
                echo "Installing $1 into $3"
                install $1 $3
              ;;
            esac
        else
            echo "$DESTLIB already installed"
        fi

        if [ \! -r $DESTLINK ]; then
            if [ -r $DESTLIB ]; then
                echo "Symlinking $DESTLINK -> $DESTLIB"
                (cd $3; ln -s $1 $2 )
            fi
        else
            echo "Symlink $DESTLINK already present (should link to $DESTLIB or newer version):"
            ls -al $DESTLINK
        fi
    fi
}

DEST=/usr/lib

install_lib libGLEW.so.1.2.4 libGLEW.so.1 $DEST

