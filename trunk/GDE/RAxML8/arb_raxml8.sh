#!/bin/bash
set -e
set -x

cpu_has_feature() {
    case `uname` in
        Darwin)
            SHOW="sysctl machdep.cpu.features"
            ;;
        Linux)
            SHOW="grep flags /proc/cpuinfo"
            ;;
    esac
    $SHOW | grep -qi "$1"
}

can_run() {
    which "$1" && test -x `which "$1"`
}

dna_tree_thorough() {
    local MODEL=$1
    # try N ML searches
    $RAXML -p $SEED -s $SEQFILE -m $MODEL \
        -N $N \
        -n TREE_INFERENCE
    # run bootstraps
    $RAXML -p $SEED -b $SEED -s $SEQFILE -m $MODEL \
        -N autoMRE_IGN \
        -n BOOTSTRAP
    # draw bipartition information
    $RAXML -f b -m $MODEL \
        -t RAxML_bestTree.TREE_INFERENCE \
        -z RAxML_bootstrap.BOOTSTRAP \
        -n ML_TREE_WITH_SUPPORT
    # compute extended majority rule consensus
    $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.BOOTSTRAP -n BOOTSTRAP_CONSENSUS
    
    #import?
    # arb_read_tree...
}

dna_tree_quick() {
    # run fast bootstraps
    $RAXML -f a -m $MODEL -p $SEED -x $SEED -s $SEQFILE -n autoMRE_IGN -n FAST_BS
    # create consensus tree
    $RAXML -J MRE -m $MODEL -z RAxML_bootstrap.FAST_BS -n FAST_BS_MAJORITY
}   

calc_mem_size() {
    echo
}

aa_tree() {
    echo
    # -m PROTGAMMALG4X
}

report_error() {
    arb_message "$*"
}

help() {
    echo "$0: <seqtype> <protocol> <model> <seed>"
    echo "seqtype: DNA|AA"
    echo "protocol: large|small (tree size)"
    echo "model: GTRGAMMA|GTRcat"
    echo "seed: random-seed"
}

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


while [ -n "$1" ]; do 
  case "$1" in
      -p) # protocol
          PROT="$2"
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
      -nt) # seqtype dna
          SEQTYPE=N
          ;;
      -aa) # seqtype proto
          SEQTYPE=A
          ;;
      *)
          report_error unknown command
          ;;
  esac
  shift
done

case "${SEQTYPE}.${PROT}" in
    N.quick)
        dna_tree_quick
        ;;
    N.thorough)
        dna_tree_thorough
        ;;
esac
