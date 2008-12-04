#!/bin/bash
# 
# Installs tools and libraries needed to run, compile or develop ARB on Ubuntu.
#
# Tested with:
#       * Ubuntu 8.10 (Intrepid Ibex)
#
# [ May as well work with other ubuntu flavors or debian.
#   Please report working tests and/or send needed changes to devel@arb-home.de
# ]

if [ -z "$1" ]; then
    echo ""
    echo "Usage: arb_UBUNTU_installTools.sh what"
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
    echo ""
else
    
    if [ "$1" == "arb_noOpenGL" ]; then
        apt-get -y install \
            gnuplot \
            gv \
            libmotif3 \
            xfig \
            
    elif [ "$1" == "arb_OpenGL" ]; then
        $0 arb_noOpenGL
        apt-get -y install \
            libpng12-0 \

    elif [ "$1" == "compile_common" ]; then
        apt-get -y install \
            g++ \
            libmotif-dev \
            libtiff4-dev \
            libx11-dev \
            libxaw7-dev \
            libxext-dev \
            libxp-dev \
            libxpm-dev \
            libxt-dev \
            lynx \
            rxp \
            sablotron \
            x11proto-print-dev \

    elif [ "$1" == "compile_noOpenGL" ]; then
        $0 arb_noOpenGL
        $0 compile_common
            
    elif [ "$1" == "compile_OpenGL" ]; then
        $0 arb_OpenGL
        $0 compile_common
        apt-get -y install \
            freeglut3-dev \
            libglew1.5-dev \
            libpng12-dev \

        DISTRIB=`cat /etc/lsb-release | grep DISTRIB_CODENAME | perl -npe 's/^.*=//'`
        SOURCE="deb http://techno.mikro.biologie.tu-muenchen.de/debian $DISTRIB non-free"

        apt-get install libglw-mesa-arb || ( \
            echo "-----------------------------------------" ;\
            echo "Install of arb-specific libraries failed!" ;\
            echo "Did you add the line" ;\
            echo "$SOURCE" ;\
            echo "to /etc/apt/sources.list, e.g. using" ;\
            echo "" ;\
            echo "sudo bash -c 'echo $SOURCE >> /etc/apt/sources.list'" ;\
            echo "sudo aptitude update" ;\
            echo "???" ;\
            )

    elif [ "$1" == "develop" ]; then
        $0 compile_OpenGL
        apt-get -y install \
            valgrind \
            ctags \

    else
        echo "Error: Unknown parameter '$1'"
    fi
fi
