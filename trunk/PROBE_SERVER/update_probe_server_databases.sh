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

TREENAME=$DEST_DIR/current.tree
TREEVERSIONFILE=$DEST_DIR/current.tree.version
ZIPPEDTREENAME=${TREENAME}.gz

rm $TREENAME

# generate the tree version
touch $TREEVERSIONFILE
# TREEVERSION=`stat --format=%Y $TREEVERSIONFILE` # stat not generally available
TREEVERSION=`./getFiletime.pl $TREEVERSIONFILE`

create_group_design_db() {
    echo "------------------------------------------------------------"
    echo "Designing probes for length=$1"
    if [ -f $TREENAME.$1 ]; then
        rm $TREENAME.$1
    fi
    echo "./bin/arb_probe_group_design $DB $PT_SERVER.arb $OUT $1 $TREENAME.$1 $TREEVERSION"
    ./bin/arb_probe_group_design $DB $PT_SERVER.arb $OUT $1 $TREENAME.$1 $TREEVERSION
}

create_db() {
    create_group_db $1 && \
        create_group_design_db $1 && \
        mv $OUT$1_design.arb $DEST_DIR
}

create_dbs() {
    while [ \! -z "$1" ]; do
        create_db $1
        shift
    done
}

treenames() {
    BASENAME=$1
    shift
    TREENAMES=
    while [ \! -z "$1" ]; do
        TREENAMES="$TREENAMES $BASENAME.$1"
        shift
    done
    echo $TREENAMES
}

# --------------------------------------------------------------------------------

# which probelengths shall be generated
CREATE="15 16"
# CREATE=15 16 17 18 19 20

# create databases
create_dbs $CREATE

# merge trees
echo "------------------------------------------------------------"
SAVED_TREES=`treenames $TREENAME $CREATE`
./bin/pgd_tree_merge $SAVED_TREES $TREENAME

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
