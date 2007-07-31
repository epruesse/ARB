C
C Fixed parameters, change here only to recompile
C
      INTEGER     min_tuplen        ! Min oligo length
      PARAMETER  (min_tuplen = 1)   

      INTEGER     max_tuplen        ! Max oligo length
      PARAMETER  (max_tuplen = 10)  

      INTEGER     oli_dim           ! Max # different oligos      
      PARAMETER  (oli_dim = 4 ** max_tuplen) 
C
C Alignment 
C
      INTEGER    old_seqsdim           ! max number of seqs
      PARAMETER (old_seqsdim = 50000)  ! arbitrary, but dont exceed

      INTEGER    old_basdim            ! max AVERAGE # bases / seq
      PARAMETER (old_basdim = 10000)   ! arbitrary, but dont exceed

      INTEGER    old_bytdim
      PARAMETER (old_bytdim = old_seqsdim * old_basdim)

      INTEGER    old_tbldim
      PARAMETER (old_tbldim = old_basdim * old_seqsdim + oli_dim)

      INTEGER    old_sidswid         ! max short-ID width
      PARAMETER (old_sidswid = 10)   ! fixed Genbank

      INTEGER    old_lidswid         ! max length of full names
      PARAMETER (old_lidswid = 80)   ! arbitrary, but dont exceed
C
C Added alignment 
C
      INTEGER    add_seqsdim           ! max number of seqs
      PARAMETER (add_seqsdim = 50000)  ! arbitrary, but dont exceed

      INTEGER    add_basdim            ! max AVERAGE # bases / seq
      PARAMETER (add_basdim = 10000)   ! arbitrary, but dont exceed

      INTEGER    add_bytdim
      PARAMETER (add_bytdim = add_seqsdim * add_basdim)

      INTEGER    add_tbldim
      PARAMETER (add_tbldim = add_basdim * add_seqsdim + oli_dim)

      INTEGER    add_sidswid         ! max short-ID width
      PARAMETER (add_sidswid = 10)   ! fixed Genbank

      INTEGER    add_lidswid         ! max length of full names
      PARAMETER (add_lidswid = 80)   ! arbitrary, but dont exceed
C
C New sequence; some is redundant currently, but may be needed later
C
      INTEGER    new_seqsdim           ! max number of seqs
      PARAMETER (new_seqsdim = 5000)   ! arbitrary, but dont exceed

      INTEGER    new_basdim            ! max AVERAGE # bases / seq 
      PARAMETER (new_basdim = 100000)  ! arbitrary, but dont exceed

      INTEGER    new_bytdim
      PARAMETER (new_bytdim = new_seqsdim * new_basdim)

      INTEGER    new_sidswid         ! max short-ID width
      PARAMETER (new_sidswid = 10)   ! fixed Genbank

      INTEGER    new_lidswid         ! max length of full names
      PARAMETER (new_lidswid = 80)   ! arbitrary, but dont exceed
     
