#!/bin/bash 

# --------------------------------------------------------------------------------
# configuration:

# working directories
DEST_DIR=./ps_serverdata
CLIENTNAME=arb_probe_library.jar

# --------------------------------------------------------------------------------

mkdir -p $DEST_DIR

CLIENTSOURCE=../PROBE_WEB/CLIENT/$CLIENTNAME
CLIENTDEST=$DEST_DIR/$CLIENTNAME

if [ -f $CLIENTSOURCE ]; then
    echo Preparing client for download..
    cp -p $CLIENTSOURCE $CLIENTDEST
else
    echo "Could not update client version (file not found '$CLIENTSOURCE')"
    exit 1
fi

ls -al $DEST_DIR
echo ""
echo "Client has been updated!"
