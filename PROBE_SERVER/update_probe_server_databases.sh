#!/bin/bash

SOURCE_DIR=./ps_input_db
TEMP_DIR=./ps_tmpdata
DEST_DIR=./ps_serverdata
WORKER_DIR=./ps_workerdir

DB_BASENAME=probe_db_

mkdir -p $SOURCE_DIR
mkdir -p $DEST_DIR
mkdir -p $TEMP_DIR
mkdir -p $WORKER_DIR

usage() {
    echo ""
    echo "Usage: update_probe_server <db> <tree>"
    echo "where.."
    echo "  <db>   = name of database (has to be in $SOURCE_DIR)"
    echo "  <tree> = name of tree to provide"
    echo ""
}

if [ -z "$2" ]; then
    usage
    echo "Error: Missing arguments"
    echo ""
    if [ -z "$1" ]; then
        echo "Content of $SOURCE_DIR:"
        echo "-----------------------"
        ls -al $SOURCE_DIR
        echo ""
    fi
    exit 1
fi

DB=$SOURCE_DIR/$1
TREE=$2

if [ \! -f "$DB" ]; then
    usage
    echo "Error: File '$DB' not found"
    exit 1
fi

PTS=../lib/pts
PT_SERVER=probe_server
PT_SERVER_DB=$PTS/$PT_SERVER.arb

UPDATE=0

if [ \! -f $PT_SERVER_DB ]; then
    UPDATE=1 # pt server db does not exists
else
    if [ $DB -nt $PT_SERVER_DB ]; then
        UPDATE=1 # source db is newer
    fi
fi

if [ $UPDATE = 1 ]; then
    echo "Terminate running PT-Server.."
    arb_pt_server -kill -D$PT_SERVER_DB

    echo "Updating PT-Server.."
    cp $DB $PT_SERVER_DB
    rm $PT_SERVER_DB.pt
fi

OUT=$TEMP_DIR/$DB_BASENAME
rm $OUT*
rm $DEST_DIR/$DB_BASENAME*

create_group_db() {
    echo "------------------------------------------------------------"
    echo "Generating probe_groups for length=$1"
    echo "./bin/arb_probe_group $DB $TREE $PT_SERVER.arb $OUT $1"
    ./bin/arb_probe_group $DB $TREE $PT_SERVER.arb $OUT $1
}

TREEFLAG=-c # 1st: create (see below)
TREENAME=$DEST_DIR/current.tree
TREEVERSIONFILE=$DEST_DIR/current.tree.version
ZIPPEDTREENAME=${TREENAME}.gz

# generate the tree version
touch $TREEVERSIONFILE
# TREEVERSION=`stat --format=%Y $TREEVERSIONFILE` # stat not generally available
TREEVERSION=`./getFiletime.pl $TREEVERSIONFILE`

create_group_design_db() {
    echo "------------------------------------------------------------"
    echo "Designing probes for length=$1"
    echo "./bin/arb_probe_group_design $DB $PT_SERVER.arb $OUT $1 $TREEFLAG $TREENAME $TREEVERSION"
    ./bin/arb_probe_group_design $DB $PT_SERVER.arb $OUT $1 $TREEFLAG $TREENAME $TREEVERSION
    TREEFLAG=-x # from 2nd -> expand
}

create_db() {
    create_group_db $1 && \
        create_group_design_db $1 && \
        mv $OUT$1_design.arb $DEST_DIR
}

if [ -f $TREENAME ]; then
    rm $TREENAME
fi

create_db 15
# create_db 16
# create_db 17
# create_db 18
# create_db 19
# create_db 20

# prepare zipped tree
if [ -f $TREENAME ]; then
    echo Zipping $TREENAME ..
    if [ -f $ZIPPEDTREENAME ]; then
        rm $ZIPPEDTREENAME
    fi
    gzip -c $TREENAME > $ZIPPEDTREENAME
    touch -r $TREEVERSIONFILE $ZIPPEDTREENAME
    rm $TREENAME $TREEVERSIONFILE
else
    echo "Error: $TREENAME was not generated"
    exit 1
fi

CLIENTBASENAME=arb_probe_library.jar
CLIENTSOURCE=../PROBE_WEB/CLIENT/$CLIENTBASENAME
CLIENTZIP=$DEST_DIR/$CLIENTBASENAME.gz

if [ -f $CLIENTSOURCE ]; then
    echo Packing client for download..
    gzip -c $CLIENTSOURCE > $CLIENTZIP
else
    echo "Could not update client version (file not found '$CLIENTSOURCE')"
    exit 1
fi

ls -al $DEST_DIR
