C
C Max number of open logical units ( = file handles = io channels) 
C
c      INTEGER     lundim
c      PARAMETER  (lundim = 100)
C
C Number of indices of IO arrays, 
C
      INTEGER    ndx_max
      PARAMETER (ndx_max = 17)
C
C Fixed parameters, that set the limits within which MALLOC allocates.
C These values could be very large, they dont themselves cause memory 
C consumption. This is the only place you need to edit parameters
C before you recompile.
C 
C ** overflow checking only partially ok, needs work **
C
      INTEGER     tuplenmax       
      PARAMETER  (tuplenmax = 10)   ! oligo length maximum

      INTEGER     tupdimmax                  
      PARAMETER  (tupdimmax = 4 ** tuplenmax) ! max number of different oligos
C
C Parameters for GenBank alignment file
C
      INTEGER    old_seqsdim         
      PARAMETER (old_seqsdim = 5000) ! max number of old seqs

      INTEGER    old_basdim          
      PARAMETER (old_basdim = 10000) ! max AVERAGE # bases / seq

      INTEGER    old_bytdim
      PARAMETER (old_bytdim = old_seqsdim * old_basdim)

      INTEGER    old_tbldim
      PARAMETER (old_tbldim = old_basdim * old_seqsdim + tupdimmax)

      INTEGER    old_sidswid         
      PARAMETER (old_sidswid = 10)   ! max short-ID width

      INTEGER    old_lidswid         
      PARAMETER (old_lidswid = 80)   ! max length of full names
C
C Parameters for unaligned GenBank file
C
      INTEGER    new_seqsdim         
      PARAMETER (new_seqsdim = 5000) ! max number of new seqs

      INTEGER    new_basdim          
      PARAMETER (new_basdim = 10000) ! currently new sequence length

      INTEGER    new_bytdim
      PARAMETER (new_bytdim = new_seqsdim * new_basdim)

      INTEGER    new_sidswid         
      PARAMETER (new_sidswid = 10)   ! max short-ID width

      INTEGER    new_lidswid         
      PARAMETER (new_lidswid = 80)   ! max length of full names

      INTEGER    new_textwid         
      PARAMETER (new_textwid = 80)   ! max length of input file lines

