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
            PATCHDIR=$ARBHOME/patches.arb
            mkdir -p $PATCHDIR
            DATE=`date '+%Y%m%d_%H%M%S'`
            PATCHNAME=${NAME}_$DATE
            PATCH=$PATCHDIR/$PATCHNAME.patch
            FAKEPATCH=$PATCHDIR/fake.patch
            RECENT_PATCH=$ARBHOME/latest.patch

            cd $ARBHOME
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
                    ln --force $PATCH $RECENT_PATCH
                    ls -alh $PATCH $RECENT_PATCH
                fi
            fi
        fi
    fi
fi

