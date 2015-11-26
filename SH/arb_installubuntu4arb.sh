#!/bin/bash
# 
# Installs tools and libraries needed to run, compile or develop ARB on Ubuntu.
#
# Tested with:
#       * Ubuntu 12.04 (Precise Pangolin)
#
# [ May as well work with other ubuntu flavors or debian.
#   Please report working tests and/or send needed changes to devel@arb-home.de
# ]

listwords() {
    if [ ! -z "$1" ]; then
        echo "    $1"
        shift 
        listwords $*
    fi
}

if [ -z "$1" ]; then
    echo ""
    echo "Usage: arb_installubuntu4arb.sh what"
    echo ""
    echo "what                installs"
    echo ""
    echo "arb_noOpenGL        things needed to run the Non-openGL-version of ARB"
    echo "arb_OpenGL          things needed to run the openGL-version of ARB"
    echo ""
    echo "compile_noOpenGL    things needed to compile and run the Non-openGL-version of ARB"
    echo "compile_OpenGL      things needed to compile and run the openGL-version of ARB"
    echo ""
    echo "develop             install 'compile_OpenGL' plus some optional development tools"
    echo "devdox              install 'develop' plus things needed for doxygen (big)" 
    echo ""
else
    ARB_PACKAGES=/tmp/$USER.arb_installubuntu4arb
    if [ "$1" == "echo" ]; then
        if [ "$2" == "arb_noOpenGL" ]; then
            echo \
                gnuplot \
                gv \
                libmotif4 \
                libxerces-c28 \
                transfig \
                xfig \
                xterm \

#                treetool 

        elif [ "$2" == "arb_OpenGL" ]; then
            $0 echo arb_noOpenGL
            echo \
                libpng12-0 \

        elif [ "$2" == "compile_common" ]; then
            echo \
                g++ \
                libglib2.0-dev \
                libmotif-dev \
                libreadline-dev \
                libtiff4-dev \
                libx11-dev \
                libxaw7-dev \
                libxext-dev \
                libxerces-c2-dev \
                libxml2-utils \
                libxpm-dev \
                libxt-dev \
                lynx \
                x11proto-print-dev \
                xsltproc \
                xutils-dev \

        elif [ "$2" == "compile_noOpenGL" ]; then
            $0 echo arb_noOpenGL
            $0 echo compile_common
            
        elif [ "$2" == "compile_OpenGL" ]; then
            $0 echo arb_OpenGL
            $0 echo compile_common
            echo \
                freeglut3-dev \
                libglew1.5-dev \
                libpng12-dev \
                
            echo \
                libglw-mesa-arb \
                >> $ARB_PACKAGES

        elif [ "$2" == "develop" ]; then
            $0 echo compile_OpenGL
            echo \
                valgrind \
                pixmap \
                ctags \

        elif [ "$2" == "devdox" ]; then
            $0 echo develop
            echo \
                doxygen \
                texlive-latex-base \
                texlive-base-bin \

        else
            echo error_unknown_target_$2
        fi
    else
        echo "Ubuntu for ARB installer"
        test -f $ARB_PACKAGES && rm $ARB_PACKAGES
        PACKAGES=`$0 echo $1`
        ALLPACKAGES=`echo $PACKAGES;test -f $ARB_PACKAGES && cat $ARB_PACKAGES`
        echo "Packages needed for '$1': `echo $ALLPACKAGES | wc -w`"
        # listwords $PACKAGES

        echo '-------------------- [apt start]'
        apt-get -y install $PACKAGES
        echo '-------------------- [apt end]'

        if [ -f $ARB_PACKAGES ]; then
            echo "Packages provided by ARB developers: `cat $ARB_PACKAGES | wc -w`"
            listwords `cat $ARB_PACKAGES`
            DISTRIB=`cat /etc/lsb-release | grep DISTRIB_CODENAME | perl -npe 's/^.*=//'`
            SOURCE="deb http://dev.arb-home.de/debian $DISTRIB non-free"
            echo '-------------------- [apt start]'
            apt-get install \
                `cat $ARB_PACKAGES` \
                || ( \
                echo "-----------------------------------------" ;\
                echo "Install of arb-specific libraries failed!" ;\
                echo "Did you add the line" ;\
                echo "$SOURCE" ;\
                echo "to /etc/apt/sources.list, e.g. using" ;\
                echo "" ;\
                echo "sudo apt-add-repository '$SOURCE'" ;\
                echo "sudo apt-get update" ;\
                echo "???" ;\
                )
            echo '-------------------- [apt end]'
        fi
        test -f $ARB_PACKAGES && rm $ARB_PACKAGES
    fi
fi
