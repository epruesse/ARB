#!/bin/bash

NAME=$1
OLDERTHANMINUTES=$2

if [ -z "$OLDERTHANMINUTES" ]; then
    echo "Usage: arb_cleanup_patches.sh name olderthanminutes"
    false
else
    if [ ! -d "$ARBHOME" ]; then
        echo "ARBHOME undefined"
        false
    else
        PATCHDIR=$ARBHOME/patches.arb
        if [ -d $PATCHDIR ]; then
            find $PATCHDIR -name "${NAME}_*.patch" -mmin +$OLDERTHANMINUTES -exec rm {} \;
        else
            echo "No patches here."
        fi
    fi
fi

