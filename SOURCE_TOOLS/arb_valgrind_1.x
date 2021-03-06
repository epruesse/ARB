#!/bin/bash 

if [ -z $1 ] ; then
    echo ''
    echo 'Usage: arb_valgrind_1.x [-c <callers>] [-f <filter>] [-l [-r]] <arb_program> <arguments>'
    echo ''
    echo '    runs valgrind (versions 1.x) on <arb_program> piping results through a filter'
    echo '    so that the output can be used as emacs error messages'
    echo ''
    echo '    <callers>      show <callers> stackframes (default: none)'
    echo '                   [in fact they are always shown, but not marked as errors]'
    echo '    <filter>       regexpr to filter the reason (default: all)'
    echo '    -l [-r]        turn on leak-checking (-r for reachable blocks)'
    echo ''
    echo ''
    echo 'Usage: arb_valgrind update'
    echo ''
    echo '    Updates the source file list which is needed to create correct file refs.'
    echo '    Called automatically by normal use if list is missing.'
    echo '    Call if files are not highlighted as errors (i.e if you have new files).'
    echo ''
    echo 'Environment:'
    echo ''
    echo '      $ARBHOME     a directory which has to contain a subdirectory SOURCE_TOOLS.'
    echo '                   SOURCE_TOOLS has to contain valgrind2grep and has to be writeable for the user'
    echo ''
    echo '      $ARB_VALGRIND_SOURCE_ROOT       down from here the script scans for sources'
    echo '                                      (defaults to $ARBHOME if empty)'
    echo ''
    echo 'Note: I use this from inside emacs as follows:'
    echo '          M-x compile'
    echo '      with:'
    echo '          (cd $ARBHOME;make nt) && arb_valgrind -l arb_ntree ~/ARB/demo.arb'
    echo ''

else
    if [ -z $ARB_VALGRIND_SOURCE_ROOT ] ; then
        ARB_VALGRIND_SOURCE_ROOT=$ARBHOME
    fi

    DIR=$ARBHOME/SOURCE_TOOLS
    LIST=$DIR/valgrind2grep.lst

    UPDATE=0
    RUN=0
    CALLERS=0
    FILTER='.*'
    LEAK_CHECK=''

    if [ ! -f $LIST ] ; then
        UPDATE=1
    fi
    if [ $1 = "update" ] ; then
        UPDATE=1
    else
        RUN=1
        SCAN_ARGS=1

        while [ $SCAN_ARGS = 1 ] ; do
            SCAN_ARGS=0
            if [ $1 = '-c' ] ; then
                CALLERS=$2
                shift 2
                SCAN_ARGS=1
            fi
            if [ $1 = '-f' ] ; then
                FILTER=$2
                shift 2
                SCAN_ARGS=1
            fi
            if [ $1 = '-l' ] ; then
                LEAK_CHECK='--leak-check=yes --leak-resolution=high'
                shift 1
                SCAN_ARGS=1
            fi
            if [ $1 = '-r' ] ; then
                LEAK_CHECK="$LEAK_CHECK --show-reachable=yes"
                shift 1
                SCAN_ARGS=1
            fi
        done
    fi

    if [ $UPDATE = 1 ] ; then
        echo "Creating list of source files starting in $ARB_VALGRIND_SOURCE_ROOT ..."
        find $ARB_VALGRIND_SOURCE_ROOT -name "*.[ch]" -o -name "*.[ch]xx" -o -name "*.[ch]pp" -o -name "*.cc" -o -name "*.hh" > $LIST
        echo 'done.'
    fi
    if [ $RUN = 1 ] ; then
        echo "Running valgrind on '$*' ..."
        echo "CALLERS='$CALLERS'"
        echo "FILTER ='$FILTER'"
        valgrind -v --error-limit=no --num-callers=10 $SHOW_REACHABLE $LEAK_CHECK $* 2>&1 >/tmp/arb_valgrind_$USER.stdout | $DIR/valgrind2grep $CALLERS "$FILTER"
        echo 'valgrind done.'
    fi
fi
