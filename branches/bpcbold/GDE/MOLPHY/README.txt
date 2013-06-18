downloaded from ftp://ftp.ism.ac.jp/pub/ISMLIB/MOLPHY/README
--------------------------------------------------------------------------------

MOLPHY: A Computer Program Package for Molecular Phylogenetics

Readme
       This is the MOLPHY (ProtML) distribution,  version 2.3.
        Copyright (c) 1992-1996, Jun Adachi & Masami Hasegawa.
                         All rights reserved.

        MOLPHY is a program package for MOLecular PHYlogenetics.

ProtML is a main program in MOLPHY for inferring evolutionary trees from
PROTein (amino acid) sequences by using the Maximum Likelihood method.

Programs (C language)
  ProtML: Maximum Likelihood Inference of Protein Phylogeny
  NucML:  Maximum Likelihood Inference of Nucleic Acid Phylogeny
  ProtST: Basic Statistics of Protein Sequences
  NucST:  Basic Statistics of Nucleic Acid Sequences
  NJdist: Neighbor Joining Phylogeny from Distance Matrix
Utilities (Perl)
  mollist:  get identifiers list        molrev:   reverse DNA sequences
  molcat:   concatenate sequences       molcut:   get partial sequences
  molmerge: merge sequences             nuc2ptn:  DNA -> Amino acid
  rminsdel: remove INS/DEL sites        molcodon: get specified codon sites
  molinfo:  get varied sites            mol2mol:  MOLPHY format beautifer
  inl2mol:  Interleaved -> MOLPHY       mol2inl:  MOLPHY -> Interleaved
  mol2phy:  MOLPHY -> Sequential        phy2mol:  Sequential -> MOLPHY
  must2mol: MUST -> MOLPHY              etc.

MOLPHY is a free software, and you can use and redistribute it.
The programs are written in a standard subset of C with UNIX-like OS.
The utilities are written in the "Perl" (Ver.4.036) with UNIX-like OS.
MOLPHY has been tested on SUN4's (cc & gcc with SUN-OS 4.1.3) and
HP9000/700 (cc, c89 & gcc with HP-UX 9.05).
However, MOLPHY has NOT been tested on VAX, IBM-PC, and Macintosh.

NETWORK DISTRIBUTION ONLY: The latest version of MOLPHY is always available
by anonymous ftp in ftp.ism.ac.jp: /pub/ISMLIB/MOLPHY/.
