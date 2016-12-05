#!/bin/bash
set -e

BASES_PER_THREAD=300

# set up environment
if [ -z $LD_LIBRARY_PATH ]; then
    LD_LIBRARY_PATH="$ARBHOME/lib"
else
    LD_LIBRARY_PATH="$ARBHOME/lib:$LD_LIBRARY_PATH"
fi
export LD_LIBRARY_PATH

# always wait on exit
trap on_exit EXIT
on_exit() {
    wait_and_exit
}

# return true if argument is file in path executable by user
can_run() {
    which "$1" &>/dev/null && test -x `which "$1" &>/dev/null`
}

# called at exit of script (by trap) and on error
# e.g. if ctrl-c is pressed
wait_and_exit() {
    # kill any backgrounded processes
    if [ -n "`jobs`" ]; then
        kill `jobs -p`
    fi
    read -p "Press key to close window"
    exit
}

# show error in ARB and exit
report_error() {
    SELF=`basename "$0"`
    echo "ARB ERROR: $*"
    arb_message "$SELF failed with: $*"
    wait_and_exit
}

# create named temporary directory
prepare_tmp_dir() {
    ARB_TMP="$HOME/.arb_tmp"
    NAME="$1"
    DATE=`date +%Y-%m-%d--%H-%M-%S`
    for N in "" ".2" ".3" ".4"; do
        DIR="$ARB_TMP/$NAME--$DATE$N"
        if [ -e "$DIR" ]; then
            continue;
        else
            mkdir -p "$DIR"
            echo "$DIR"
            return
        fi
    done
    report_error "Unable to create temporary directory"
}

dump_features() {
    case `uname` in
        Darwin)
            sysctl machdep.cpu.features
            ;;
        Linux)
            grep -m1 flags /proc/cpuinfo 2>/dev/null
            ;;
    esac
}
cpu_has_feature() {
    dump_features | grep -qi "$1" &>/dev/null
}

cpu_get_cores() {
    # honor Torque/PBS num processes (or make sure we follow, if enforced)
    if [ ! -z $PBS_NP ]; then
        echo $PBS_NP
        return
    fi
    # extract physical CPUs from host
    case `uname` in
        Darwin)
            sysctl -n hw.ncpu
            ;;
        Linux)
            grep -c "^processor" /proc/cpuinfo
            ;;
    esac
}

# this is the "thorough" protocol.
# 1. do $REPEATS searches for best ML tree
# 2. run $BOOTSTRAP BS searches
# 3. combine into ML tree with support values
# 4. calculate consensus tree
# 5. import trees into ARB
dna_tree_thorough() {
    # try N ML searches
    $RAXML -p "$SEED" -s "$SEQFILE" -m $MODEL \
        -N "$REPEATS" \
        -n TREE_INFERENCE &

    # wait for raxml to complete if we have not enough
    # cores to run bootstrap search at the same time
    if [ $(($THREADS * 2)) -gt $CORES ]; then
        echo "Not enough cores found ($CORES) to run ML search and BS in"
        echo "parallel with $THREADS threads. Waiting..."
        wait
    fi

    # run bootstraps
    $RAXML -p "$SEED" -b "$SEED" -s "$SEQFILE" -m $MODEL \
        -N "$BOOTSTRAPS" \
        -n BOOTSTRAP &
    wait

    # draw bipartition information
    $RAXML -f b -m $MODEL \
        -t RAxML_bestTree.TREE_INFERENCE \
        -z RAxML_bootstrap.BOOTSTRAP \
        -n ML_TREE_WITH_SUPPORT
    # import
    arb_read_tree tree_${TREENAME} RAxML_bipartitions.ML_TREE_WITH_SUPPORT

    if [ -n "$MRE" ]; then
        # compute extended majority rule consensus
        $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.BOOTSTRAP -n BOOTSTRAP_CONSENSUS

        # import
        arb_read_tree tree_${TREENAME}_mre RAxML_MajorityRuleExtendedConsensusTree.BOOTSTRAP_CONSENSUS
    fi
}

# this is the fast protocol
# 1. run fast bootstraps
# 2. calculate consensus
# 3. import into ARB
dna_tree_quick() {
    # run fast bootstraps
    $RAXML -f a -m $MODEL -p "$SEED" -x "$SEED" -s "$SEQFILE" -N "$BOOTSTRAPS" -n FAST_BS
    # import
    arb_read_tree tree_${TREENAME} RAxML_bipartitions.FAST_BS

    # create consensus tree
    if [ -n "$MRE" ]; then
        $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.FAST_BS -n FAST_BS_MAJORITY
        # import
        arb_read_tree tree_${TREENAME}_mre RAxML_MajorityRuleExtendedConsensusTree.FAST_BS_MAJORITY
    fi
}   

calc_mem_size() {
    # FIXME
    echo
}

aa_tree() {
    # FIXME
    echo
    # -m PROTGAMMALG4X
}

###### main #####

MRE=Y
TREENAME=raxml

while [ -n "$1" ]; do 
  case "$1" in
      -p) # protocol
          PROTOCOL="$2"
          shift
          ;;
      -m) # subst model
          MODEL="$2"
          shift
          ;;
      -s) # random seed
          SEED="$2"
          shift
          ;;
      -b) # bootstraps
          BOOTSTRAPS="$2"
          shift
          ;;
      -r) # number of tree searches
          REPEATS="$2"
          shift
          ;;
      -n) # tree name
          TREENAME="$2"
          shift
          ;;
      -nomre) # don't create mre tree
          MRE=
          ;;
      -nt) # seqtype dna
          SEQTYPE=N
          ;;
      -aa) # seqtype proto
          SEQTYPE=A
          ;;
      -f) # input file
          FILE="$2"
          shift
          ;;
      -t) # threads
          THREADS="$2"
          shift
          ;;
      *)
          report_error argument not understood: "$1"
          ;;
  esac
  shift
done

# use time as random SEED if empty
if [ -z "$SEED" ]; then
    # seconds since 1970
    SEED=`date +%s`
fi

# prepare data in tempdir
DIR="`prepare_tmp_dir raxml`"
SEQFILE="$DIR/dna.phy"

arb_convert_aln --arb-notify -GenBank "$FILE" -phylip "$SEQFILE" 2>&1 |\
  grep -v "^WARNING(14): LOCUS" || true # remove spurious warning
rm $FILE

cd "$DIR"

# select RAxML binary
if cpu_has_feature avx && can_run raxmlHPC8-AVX.PTHREADS; then
    RAXML=raxmlHPC8-AVX.PTHREADS
elif cpu_has_feature sse3 && can_run raxmlHPC8-SSE3.PTHREADS; then
    RAXML=raxmlHPC8-SSE3.PTHREADS
elif can_run raxmlHPC8-PTHREADS; then
    RAXML=raxmlHPC8-PTHREADS
else
    report_error Could not find working RAxML binary. 
fi

# get some numbers
CORES=`cpu_get_cores`
read NSEQS BP < <(head -n 1 $SEQFILE)

# warn if model is not recommended for given number of sequences
BAD_PRACTICE="This is not considered good practice.\nPlease refer to the RAxML manual for details."
if [ "$MODEL" == "GTRGAMMA" -a $NSEQS -gt 10000 ]; then
    arb_message "Using the GTRGAMMA model on more than 10,000 sequences.\n$BAD_PRACTICE"
fi
if [ "$MODEL" == "GTRCAT" -a $NSEQS -lt 150 ]; then
    arb_message "Using the GTRCAT model on less than 150 sequences.\n$BAD_PRACTICE"
fi

CORES=$(( $CORES + 1 - 1 ))
# calculate number of threads (if not passed)
if [ -z "$THREADS" ]; then
    THREADS=$(( $BP / $BASES_PER_THREAD + 2))
    # +1 is for master thread,
    # another +1 for the first $BASES_PER_THREAD (bash truncates)

    if [ $CORES -lt 1 ]; then
        report_error "failed to detect number of cores.\nPlease specify 'CPU thread override'."
    fi

    if [ $THREADS -gt $CORES ]; then
        THREADS=$CORES
    fi
else
    if [ $CORES -lt 1 ]; then
        CORES=$THREADS
    fi
fi
RAXML="$RAXML -T $THREADS"


case "${SEQTYPE}.${PROTOCOL}" in
    N.quick)
        dna_tree_quick
        ;;
    N.thorough)
        dna_tree_thorough
        ;;
esac

# FIXME: cleanup temp dir

