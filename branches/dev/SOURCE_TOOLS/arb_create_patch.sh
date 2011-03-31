#!/bin/bash

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
                    echo "No patch generated (no diff)"
                else
                    DIFF=1
                    if [ -e $RECENT_PATCH ]; then
                        DIFF=`diff $PATCH $RECENT_PATCH | wc -l`
                    fi

                    if [ $DIFF = 0 ]; then
                        echo "No patch generated (same as last patch)"
                        rm $PATCH
                    else
                        ln --force $PATCH $RECENT_PATCH
                        ls -hog $PATCH $RECENT_PATCH
                    fi
                fi
            fi
        fi
    fi
fi

