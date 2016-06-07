#!/bin/bash -x

NAME=$1
if [ -z "$NAME" ]; then
    echo "Usage: arb_create_patch.sh name"
    false
else
    if [ ! -d "$ARBHOME" ]; then
        echo "No patch created (ARBHOME undefined)"
    else
        if [ ! -d $ARBHOME/.svn ]; then
            echo "No patch created (no SVN checkout in ARBHOME)"
        else
            cd $ARBHOME
            PATCHDIR=./patches.arb
            mkdir -p $PATCHDIR
            DATE=`date '+%Y%m%d_%H%M%S'`
            PATCHNAME=${NAME}_$DATE
            PATCH=$PATCHDIR/$PATCHNAME.patch
            FAKEPATCH=$PATCHDIR/fake.patch
            RECENT_PATCH=./latest.patch
            INTERDIFF_PATCH=./interdiff.patch
            IGNORE_WHITE_PATCH=./latest_iwhite.patch
            WHITE_PATCH=./white.patch

            svn diff > $PATCH
            
            if [ -e $PATCH ]; then
                if [ ! -s $PATCH ]; then
                    rm $PATCH
                    if [ ! -e $FAKEPATCH ]; then
                        echo "Index: No changes" > $FAKEPATCH
                        echo "===================================================================" >> $FAKEPATCH
                    fi
                    ln --force $FAKEPATCH $RECENT_PATCH
                    touch $FAKEPATCH
                    rm -f $INTERDIFF_PATCH $IGNORE_WHITE_PATCH $WHITE_PATCH
                    echo "No patch generated (no diff)"
                else
                    DIFF=1
                    INTER=0
                    if [ -e $RECENT_PATCH ]; then
                        DIFF=`diff $PATCH $RECENT_PATCH | wc -l`
                        INTER=DIFF
                    fi

                    if [ $DIFF = 0 ]; then
                        echo "No patch generated (same as last patch)"
                        rm $PATCH
                        rm -f $INTERDIFF_PATCH
                    else
                        (   svn diff -x -b > $IGNORE_WHITE_PATCH ; \
                            interdiff $PATCH $IGNORE_WHITE_PATCH > $WHITE_PATCH ) &

                        if [ $INTER != 0 ]; then
                            interdiff -w $RECENT_PATCH $PATCH > $INTERDIFF_PATCH
                        else
                            rm -f $INTERDIFF_PATCH
                        fi
                        ln --force $PATCH $RECENT_PATCH
                        ls -hog $PATCH $RECENT_PATCH
                    fi
                fi
            fi
        fi
    fi
fi

