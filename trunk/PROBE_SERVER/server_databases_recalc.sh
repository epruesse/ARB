#!/bin/bash 

# --------------------------------------------------------------------------------
# configuration:

# working directories
SOURCE_DIR=./ps_input_db
TEMP_DIR=./ps_tmpdata
DEST_DIR=./ps_serverdata
WORKER_DIR=./ps_workerdir

# names output files
DB_BASENAME=probe_db_
TREE_BASENAME=current.tree
CLIENTNAME=arb_probe_library.jar

# which pt server to build and use
PTS=../lib/pts
PT_SERVER=probe_server

# which probelengths shall be generated
# CREATE="15 16"
CREATE="15 16 17 18 19 20"

# does NOT work for probes longer than 20
# CREATE="20 30 40"

# --------------------------------------------------------------------------------

mkdir -p $SOURCE_DIR
mkdir -p $DEST_DIR
mkdir -p $TEMP_DIR
mkdir -p $WORKER_DIR

usage() {
    echo ""
    echo "Usage: server_databases_recalc <tree>"
    echo "where.."
    echo "  <tree> = name of tree to provide ('DEFAULT' to use last selected tree)"
    echo ""
}

if [ -z "$1" ]; then
    usage
    echo "Error: Missing arguments"
    echo ""
    exit 1
fi

DB=../lib/pts/probe_server.arb # use last database
TREE=$1

if [ \! -f "$DB" ]; then
    usage
    echo "Error: File '$DB' not found"
    exit 1
fi

OUT=$TEMP_DIR/$DB_BASENAME
rm $DEST_DIR/$DB_BASENAME*

# ls -al $PTS/ $SOURCE_DIR/

create_group_db() {
    echo "------------------------------------------------------------"
    echo "Generating probe_groups for length=$1"
    echo "./bin/arb_probe_group -noprobe $DB $TREE $PT_SERVER.arb $OUT $1"
    ./bin/arb_probe_group -noprobe $DB $TREE $PT_SERVER.arb $OUT $1
}

TREENAME=$DEST_DIR/$TREE_BASENAME
TREEVERSIONFILE=$DEST_DIR/$TREE_BASENAME.version
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

abort_work() {
    echo "Aborting.."
    exit 1
}

create_dbs() {
    while [ \! -z "$1" ]; do
        create_db $1 || abort_work
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

# create databases
create_dbs $CREATE

# merge trees
echo "------------------------------------------------------------"
SAVED_TREES=`treenames $TREENAME $CREATE`
echo ./bin/pgd_tree_merge $SAVED_TREES $TREENAME
./bin/pgd_tree_merge $SAVED_TREES $TREENAME || rm $TREENAME
rm $SAVED_TREES

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
echo "Fine - ready to start the server!"
