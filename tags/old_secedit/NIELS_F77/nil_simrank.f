      PROGRAM SIMRNK
C     
C Niels Larsen (niels@vitro.cme.msu.edu)
C 
C Version 1.0, 19 october, 1992. 
C Version 1.1, 20 november, 1992. 
C      Chimera check option added. 
C Version 1.2, 27 april, 1993. 
C      Better binary IO, # oligos added to output.
C Version 1.3, 26 may, 1993.   
C      Better and simpler chimera check option.
C Version 2.0, 21 june, 1993.  
C      Histogram added; read multiple sequences; error message if 
C      out of memory; percent score changed to sab; stepsize, tuplen,
C      minimum fragment length, output width and length of sequences 
C      included now command line options; major clean up.
C Version 2.1, 11 august, 1993.
C      -simple option added.   
C Version 2.2, 20 october, 1993. 
C      -u and -n options added, for subset and superset selection
C Version 2.3, 23 october, 1993.
C      separated similarity_rank and check_chimera in subroutines
C Version 2.4, 26 october, 1993. 
C      fixed bugs, made -p work, made selection and addition cleaner,
C      changed binary file format, exclude db seq if short-ID matches
C      that of search probe, some cleanup. 
C Version 2.5, 10 november, 1993.
C      Added histogram smoothening and changed header a bit.
C Version 2.6, 3 february, 1997. 
C      Ported to Linux using the Absoft f77 v3.4a. Two small
C      bugs found, great compiler. 
C
C Compile this with (Absoft compiler on Linux):
C
C f77 -g -f -s -W -N9 -lU77 simrank.f -o simrank
C
C -----------------------------------------------------------------------------
C
C This line gives seqmentation fault *sometimes* - WHY ? 
C
c         WRITE (cl_outlun,'(1x,<cl_outwid>a1)') ('=',ipos=1,cl_outwid)
C
C    DESCRIBE  when stable
C
C ---------------------------------------------------------------------------
C
      IMPLICIT none

      CHARACTER*6 program_ID / 'SIMRNK' /, pidstr
      CHARACTER*3 version_ID / '2.6'    /, vidstr
      CHARACTER*15 rev_date  / 'February 3 1997' / 
      CHARACTER*10 server_ID / 'RDPMAILSRV' / 

      INCLUDE 'sim_cmdlin.inc'
      INCLUDE 'sim_params.inc'
C
C ---------------------------------------------------------------------------
C
C Variables for concatenated Genbank entries (the database)
C
      BYTE       old_seqs (old_bytdim)   ! cat'ed sequences
      POINTER   (old_seqsptr,old_seqs) 

      INTEGER    old_llst (old_bytdim)   ! linked list
      POINTER   (old_llstptr,old_llst)

      CHARACTER*(old_sidswid)  old_sids (old_seqsdim)  ! short-ID's 
      POINTER   (old_sidsptr,old_sids) 

      CHARACTER*(old_lidswid)  old_lids (old_seqsdim)  ! full names 
      POINTER   (old_lidsptr,old_lids) 

      INTEGER    old_begs (old_seqsdim)  ! begs of seqs in old_seqs 
      POINTER   (old_begsptr,old_begs)

      INTEGER    old_ends (old_seqsdim)  ! ends of seqs in old_seqs
      POINTER   (old_endsptr,old_ends)

      INTEGER    old_onums (old_seqsdim)  ! # different tuplen-mers per seq
      POINTER   (old_onumsptr,old_onums)

      INTEGER    old_obegs (0:oli_dim)  ! first occurence of each oligo
      POINTER   (old_obegsptr,old_obegs)

      INTEGER*2  old_tbl (old_seqsdim * old_basdim + 4 ** max_tuplen)
      POINTER   (old_tblptr,old_tbl)

      INTEGER    old_tbegs (0:oli_dim)   
      POINTER   (old_tbegsptr,old_tbegs)

      LOGICAL*1  old_incl (old_seqsdim)
      POINTER   (old_inclptr,old_incl)

      INTEGER    old_seqsmax, bin_seqsmax, nam_seqsmax
      INTEGER    old_tblmax, bin_tblmax
      INTEGER    old_tblpos
      INTEGER    old_byts
C
C ---------------------------------------------------------------------------
C
C Variables for user added concatenated Genbank entries
C
      BYTE       add_seqs (add_bytdim)   ! cat'ed sequences
      POINTER   (add_seqsptr,add_seqs) 

      INTEGER    add_llst (add_bytdim)   ! linked list
      POINTER   (add_llstptr,add_llst)

      CHARACTER*(add_sidswid)  add_sids (add_seqsdim)  ! short-ID's 
      POINTER   (add_sidsptr,add_sids) 

      CHARACTER*(add_lidswid)  add_lids (add_seqsdim)  ! full names 
      POINTER   (add_lidsptr,add_lids) 

      INTEGER    add_begs (add_seqsdim)  ! begs of seqs in add_seqs 
      POINTER   (add_begsptr,add_begs)

      INTEGER    add_ends (add_seqsdim)  ! ends of seqs in add_seqs
      POINTER   (add_endsptr,add_ends)

      INTEGER    add_onums (add_seqsdim)  ! # different tuplen-mers per seq
      POINTER   (add_onumsptr,add_onums)

      INTEGER    add_obegs (0:oli_dim)  ! first occurence of each oligo
      POINTER   (add_obegsptr,add_obegs)

      INTEGER*2  add_tbl (add_seqsdim * add_basdim + 4 ** max_tuplen)
      POINTER   (add_tblptr,add_tbl)

      INTEGER    add_tbegs (0:oli_dim)   
      POINTER   (add_tbegsptr,add_tbegs)

      INTEGER    add_seqsmax, add_tblmax
      INTEGER    add_tblpos
      INTEGER    add_byts
C
C ---------------------------------------------------------------------------
C 
C Arrays that hold union of selections, additions and rdp alignment
C
      CHARACTER*(old_sidswid) scr_sids (old_seqsdim)
      POINTER   (scr_sidsptr, scr_sids)

      CHARACTER*(old_lidswid) scr_lids (old_seqsdim)  ! full names 
      POINTER   (scr_lidsptr,scr_lids) 

      INTEGER    scr_onums (old_seqsdim)  ! # different tuplen-mers per seq
      POINTER   (scr_onumsptr,scr_onums)

      INTEGER*2  scr_tbl (old_seqsdim * old_basdim + 4 ** max_tuplen)
      POINTER   (scr_tblptr,scr_tbl)

      INTEGER    scr_tblmax
      INTEGER    scr_tblpos

      INTEGER    tot_seqsmax
C
C ---------------------------------------------------------------------------
c$$$C  
c$$$C Scratch arrays
c$$$C
c$$$      CHARACTER*(old_sidswid) scr_sids (old_seqsdim)
c$$$      POINTER   (scr_sidsptr, scr_sids)

      INTEGER    scr_ndcs (old_seqsdim)
      POINTER   (scr_ndcsptr,scr_ndcs)
C
C ---------------------------------------------------------------------------
C
C Parameters and variables for new Genbank entry
C
      BYTE       new_seqs (new_bytdim)    ! new sequence
      POINTER   (new_seqsptr,new_seqs)

      INTEGER    new_cods (new_bytdim)   ! integer codes
      POINTER   (new_codsptr,new_cods)

      CHARACTER*(new_sidswid)  new_sids (new_seqsdim)  ! short-ID's 
      POINTER   (new_sidsptr,new_sids) 

      CHARACTER*(new_lidswid)  new_lids (new_seqsdim)  ! full names 
      POINTER   (new_lidsptr,new_lids) 

      INTEGER    new_begs (new_seqsdim)  ! begs of seqs in new_seqs 
      POINTER   (new_begsptr,new_begs)

      INTEGER    new_ends (new_seqsdim)  ! ends of seqs in new_seqs
      POINTER   (new_endsptr,new_ends)

      INTEGER    new_seqsmax, new_byts
C
C ---------------------------------------------------------------------------
C
C Variables for binary file
C
      INTEGER    bin_tuplen
      INTEGER    bin_oligos 
      INTEGER    bin_seqsdim
      INTEGER    bin_basdim
      INTEGER    bin_sidswid
      INTEGER    bin_lidswid
C
C ---------------------------------------------------------------------------
C
C Match results and outputs
C  
C ---------------------------------------------------------------------------
C
      INCLUDE 'sim_res_eqv.inc'

      INTEGER str_beg, str_end
      INTEGER iseq,jseq,indx,ialf,ipos,ioli
      INTEGER oli_number
      LOGICAL noerrs
      CHARACTER*80 line
      INTEGER linend, begpos, endpos
      LOGICAL ch_is_alpha

      INTEGER IARGC, totarg

      CHARACTER*1000 usestr
      CHARACTER*1000 wrapstr
      CHARACTER*1  LF / 10 /

      LOGICAL*1 bool
C
C IUB_base is true for valid IUB base symbols, false otherwise
C  
      DATA IUB_base  
C               A  B  C  D  E  F  G  H  I  J  K  L  M -
     -  / 65*0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1,
C               N  O  P  Q  R  S  T  U  V  W  X  Y  Z -
     -          1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0,  6*0,
C               a  b  c  d  e  f  g  h  i  j  k  l  m -
     -          1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1,
C               n  o  p  q  r  s  t  u  v  w  x  y  z -
     -          1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0,  5*0 /
C
C WC_base is true if an AaCcUuTtGg, false otherwise
C  
      DATA WC_base  
C               A  B  C  D  E  F  G  H  I  J  K  L  M -
     -  / 65*0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
C               N  O  P  Q  R  S  T  U  V  W  X  Y  Z -
     -          0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,  6*0,
C               a  b  c  d  e  f  g  h  i  j  k  l  m -
     -          1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
C               n  o  p  q  r  s  t  u  v  w  x  y  z -
     -          0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,  5*0 /

c      INTEGER i

c$$$      INTEGER ieee_err,ieee_handler,SIGPFE_ABORT
c$$$      INTEGER inx,over,under,div,inv,accrued
c$$$      INTEGER IEEE_FLAGS
c$$$      CHARACTER*10 ieee_id
c$$$C
c$$$C -----------------------------------------------------------------------------
c$$$C
c$$$C For C pre-processor; it is for signalling floating points errors
c$$$C
c$$$# define inexact    0
c$$$# define division   1
c$$$# define underflow  2
c$$$# define overflow   3
c$$$# define invalid    4
c$$$C
c$$$C Establish signal handlers for division by zero, underflow, overflow
c$$$C and invalid operand. Machines nowadays dont just stop like old VAXes.
c$$$C
c$$$      ieee_err = ieee_handler('set','division' ,SIGPFE_ABORT)
c$$$      ieee_err = ieee_handler('set','underflow',SIGPFE_ABORT)
c$$$      ieee_err = ieee_handler('set','overflow', SIGPFE_ABORT)
c$$$      ieee_err = ieee_handler('set','invalid',  SIGPFE_ABORT)
C
C------------------------------------------------------------------------------
C
C Parse the command line, and stop program with message if something wrong
C
      totarg = iargc()

      cl_outwid = 78

      IF (totarg.EQ.0) THEN

         WRITE (6,'(/a)') 
     -         ' SIMILARITY_RANK and CHECK_CHIMERA, version ' // 
     -          version_ID // ', ' // rev_date
         WRITE (6,'(/a)') 
     -         ' Copyright Niels Larsen, 1997.' 

         usestr = 
     -     'Without the -chimera option, the program returns a ranked '       //
     -     'listing of the most similar sequences to each of the flatfile '   //
     -     'entries in ''new_entries''. The sequences need not be aligned. '  // 
     -     'With the -chimera option, the best possible breakpoint is '       // 
     -     'searched for (by checking if certain parts of the sequence '      //
     -     'are most similar to different sets of relatives), and an '        //
     -     'additional histogram is returned together with ranked listings '  //
     -     'for the fragments. Refer to the README file for explanation of '  //
     -     'how the program works, and how to interpret the output.'

         CALL text_wrap (usestr(1:str_end(usestr)),1,cl_outwid,' ',wrapstr)
         WRITE (6,'(/a)') wrapstr (1:str_end(wrapstr))

         usestr = 
     -     ' Usage: simrnk -b bin_file          mandatory     '             //LF// 
     -     '               -i new_entries       mandatory     '             //LF// 
     -     '               -a rdp_GB_entries    mandatory if no bin_file  ' //LF// 
     -     '             [ -u usr_GB_entries ]  D = none      '             //LF//
     -     '             [ -n name_list      ]  all selected if omitted   ' //LF//
     -     '             [ -o out_file       ]  D = screen    '             //LF// 
     -     '             [ -e error_file     ]  D = screen    '             //LF//
     -     '             [ -simple           ]  D = off       '             //LF//
     -     '             [ -chimera          ]  D = off       '             //LF//
     -     '             [ -html             ]  D = off       '             //LF//
     -     '             [ -urlbase          ]  D = ""        '             //LF//
     -     '             [ -d list_string    ]  D = none      '             //LF//
     -     '             [ -t tuple          ]  D = 7         '             //LF// 
     -     '             [ -p part_length    ]  D = 50        '             //LF//
     -     '             [ -w list_width     ]  D = 78        '             //LF// 
     -     '             [ -l list_length    ]  D = 20        '             //LF//
     -     '             [ -s step_length    ]  D = 10        '             //LF//
     -     '             [ -f frag_length    ]  D = 10        '

         WRITE (6,'(/a/)') usestr(1:str_end(usestr))
         STOP

      ELSE

         CALL parse_cmd_line 

      END IF
C
C ------------------------------------------------------------------------------
C Input ASCII concatenated Genbank entries
C ------------------------------------------------------------------------------
C
      oli_number = 4 ** cl_tuplen 
C
C If no existing binary file,
C
      IF (.NOT.cl_binflag) THEN

         OPEN (UNIT = cl_alilun,
     -         FILE = cl_alifnm, 
     -       STATUS = 'OLD')
C
C Find number of sequences and bases (other than AaUuTtGgCc are ignored)
C
         CALL feel_gbk (cl_alilun,WC_base,50,old_seqsmax,old_byts)
C         print*,'after feel_gbk, old_seqsmax, old_byts = ',old_seqsmax,old_byts

         IF (old_seqsmax.LT.1) THEN
            CALL write_message (cl_errlun,cl_errfnm,
     -            'Error: No valid entries in ' // 
     -         cl_alifnm(str_beg(cl_alifnm):str_end(cl_alifnm)))
            STOP
         END IF

         noerrs = old_seqsmax .LE. old_seqsdim    .AND.
     -               old_byts .LE. old_seqsdim * old_basdim

         IF (.NOT.noerrs) THEN

            CALL write_message (cl_errlun,cl_errfnm, 
     -         'Error: Needed bounds for ' // 
     -         cl_alifnm(str_beg(cl_alifnm):str_end(cl_alifnm)) // 
     -         ' overflow dynamic memory limits')

            WRITE (cl_errlun,'(2(a,i)/2(a,i))')
     -         ' **  old_seqsmax = ',old_seqsmax, 
     -         '     old_seqsdim = ',old_seqsdim,
     -         ' **     old_byts = ',old_byts,
     -         '      old_bytdim = ',old_bytdim

            CALL write_message (cl_errlun,cl_errfnm, 
     -         'Increase dynamic maxima in sim_params.inc and recompile')

            STOP

         END IF
C
C Allocate memory for sequences (long 1d array), short-ID's, full names, 
C tables of beginnings and ends of each sequence in old_seq
C
         CALL get_memory (old_seqsptr, old_byts, 'old_seqs', 'main')
         CALL get_memory (old_sidsptr, old_seqsmax * old_sidswid, 'old_sids', 'main')
         CALL get_memory (old_lidsptr, old_seqsmax * old_lidswid, 'old_lids', 'main')
         CALL get_memory (old_begsptr, old_seqsmax * 4, 'old_begs', 'main')
         CALL get_memory (old_endsptr, old_seqsmax * 4, 'old_ends', 'main')
C
C Read sequences into old_seqs, store beginnings and ends of sequences, and
C their short-ID's and full names
C
         REWIND (UNIT = cl_alilun)
C
C Read Genbank alignment, while throwing away all non-WC bases, again: 
C
C  old_seqs :  Holds all sequences as one long composite sequence
C  old_begs :  Pointers to where in old_seqs each sequence begins
C  old_ends :  Pointers to where in old_seqs each sequence ends
C  old_sids :  Short-ID's for each sequence
C  old_lids :  Full names for each sequence
C
         CALL get_gbk ( cl_alilun, old_seqsmax, WC_base, cl_partlen, old_byts, 
     -                  old_seqs, old_begs, old_ends, old_sids, old_lids )
C         print*,'after get_gbk, old_seqsmax, old_byts = ',old_seqsmax, old_byts

         CLOSE (UNIT = cl_alilun)
C
C Translate  A = 0, U = 1, G = 2, C = 3 
C       
         CALL To_Code4 ( old_seqs, 1, old_byts )
C
C Create linked list
C
         CALL get_memory ( old_llstptr, old_byts * 4, 'old_llst', 'main' )
         CALL get_memory ( old_obegsptr, oli_number * 4, 'old_obegs', 'main' )

         CALL make_lnk_lst ( 4, cl_tuplen, oli_number, old_seqsmax, old_byts,
     -                       1, old_seqsmax, old_seqs, old_begs, old_ends,
     -                       old_obegs, old_llst )
C
C No need for 4-coded sequence anymore
C
         CALL FREE (old_seqsptr)
C
C From old_llst and old_obegs construct three tables: old_tbl, 
C old_tbegs and old_onums:
C
C old_tbl:  Contains runs terminated by 0 of the sequence numbers, in
C           which a given oligo occurs; old_tbegs points to the start 
C           of those runs, example
C
C Old_tbegs:  |                       |                   |   |      etc
C Old_tbl    001 004 013 015 246 000 001 007 013 056 000 000 344 000 etc
C
C old_onums: Contains for each sequence the number of different oligos
C
C IMPROVE memory estimate for old_tbl
C
         CALL get_memory (old_tblptr, (old_byts+oli_number) * 2, 'old_tbl', 'main')
         CALL get_memory (old_tbegsptr, oli_number * 4, 'old_tbegs', 'main')
         CALL get_memory (old_onumsptr, old_seqsmax * 4, 'old_onums', 'main')

         CALL make_tables ( oli_number, 
     -                      old_seqsmax, old_byts, old_byts+oli_number, 
     -                      old_ends, old_obegs, old_llst,
     -                      old_tblmax, old_tbl, old_tbegs, old_onums)

c$$$         DO ioli = 0, oli_number-1
c$$$            IF (old_obegs(ioli).GT.0) THEN
c$$$               WRITE (6,'(i5,1x,$)') old_obegs(ioli)
c$$$               i = old_llst(old_obegs(ioli))
c$$$               DO WHILE (i.GT.0)
c$$$                  WRITE (6,'(i5,1x,$)') i
c$$$                  i = old_llst(i)
c$$$               END DO
c$$$               WRITE (6,'()')
c$$$            END IF
c$$$         END DO
c$$$         STOP


         CALL FREE (old_obegsptr)
         CALL FREE (old_llstptr)
         CALL FREE (old_begsptr)
         CALL FREE (old_endsptr)
C
C The arrays old_tbl, old_tbegs, old_onums, old_sids, old_lids are used below.
C
C -----------------------------------------------------------------------------
C Binary output of table, names, ends and begs, table begs and onums
C -----------------------------------------------------------------------------
C
C Write in binary format the data in -a file, pre-processed as far as 
C possible,
C
         OPEN (UNIT = cl_binlun,
     -         FILE = cl_binfnm,
     -       STATUS = 'NEW',
     -         FORM = 'UNFORMATTED')
C     -       BUFFER = 1000000)            ! Absoft specific
C     -      FILEOPT = 'BUFFER=1000000')  ! SUN specific 
C     -    BLOCKSIZE = 1000000)           ! VAX specific
C
C Write program- and version-ID into file, so simrnk will only read its own files
C
         WRITE (cl_binlun) program_ID
         WRITE (cl_binlun) version_ID
C
C Parameters for comparison when binary file is read next time
C
         WRITE (cl_binlun) cl_tuplen,oli_number,old_seqsdim,old_basdim,
     -                     old_sidswid,old_lidswid,
     -                     old_seqsmax,old_tblmax
C
C Save arrays using binary write
C
         CALL sa_bin_io ('WRITE',cl_binlun,1,old_seqsmax,old_sids)  ! string array
         CALL sa_bin_io ('WRITE',cl_binlun,1,old_seqsmax,old_lids)  ! string array
         CALL i4_bin_io ('WRITE',cl_binlun,1,old_seqsmax,old_onums)
         CALL i2_bin_io ('WRITE',cl_binlun,1,old_tblmax,old_tbl)
         CALL i4_bin_io ('WRITE',cl_binlun,0,oli_number-1,old_tbegs)

         CLOSE (UNIT = cl_binlun)

      ELSE
C
C ---------------------------------------------------------------------
C Binary input of table, names, ends and begs, table begs and onums
C ---------------------------------------------------------------------
C
         OPEN (UNIT = cl_binlun,
     -         FILE = cl_binfnm,
     -       STATUS = 'OLD',
     -         FORM = 'UNFORMATTED')
C     -       BUFFER = 1000000)            ! Absoft specific
C     -      FILEOPT = 'BUFFER=1000000')  ! SUN specific 
C     -    BLOCKSIZE = 1000000)           ! VAX specific
C 
C Read program ID, stop with error message if wrong
C
         READ (cl_binlun) pidstr

         IF (pidstr.NE.program_ID) THEN
            CALL write_message (cl_errlun,cl_errfnm,
     -          'Error: Binary file program-ID mismatch:' // LF //
     -          'File ID is ' // pidstr // ' but should be ' // 
     -           program_ID // LF //
     -          'Solution: Delete ' // 
     -           cl_binfnm(str_beg(cl_binfnm):str_end(cl_binfnm)) // 
     -          ' and restart the program')
            STOP
         END IF
C 
C Read version ID, stop with error message if wrong
C
         READ (cl_binlun) vidstr

         IF (vidstr.NE.version_ID) THEN
            CALL write_message (cl_errlun,cl_errfnm,
     -          'Error: Binary file version-ID mismatch:' // LF //
     -          'File ID is ' // vidstr // ' but should be ' // 
     -           version_ID // LF //
     -          'Solution: Delete ' // 
     -           cl_binfnm(str_beg(cl_binfnm):str_end(cl_binfnm)) // 
     -          ' and restart the program')
            STOP
         END IF
C
C Parameters for comparison with settings in this program version; No need 
C to check bin_seqsmax, bin_tblmax and bin_byts against dimensions, they
C couldnt have been written if they exceed, since all dimensions are
C written to file too
C
         READ (cl_binlun) bin_tuplen,bin_oligos,bin_seqsdim,bin_basdim,
     -                    bin_sidswid,bin_lidswid,
     -                    bin_seqsmax,bin_tblmax

         cl_tuplen = bin_tuplen  ! temporary fix
         oli_number = bin_oligos !     - 

         old_seqsmax = bin_seqsmax
         old_tblmax = bin_tblmax

         IF (.NOT. (
     -            bin_tuplen.EQ.cl_tuplen     .AND.  
     -            bin_oligos.EQ.oli_number    .AND.
     -            bin_seqsdim.EQ.old_seqsdim  .AND.
     -            bin_basdim.EQ.old_basdim    .AND.
     -            bin_sidswid.EQ.old_sidswid  .AND.
     -            bin_lidswid.EQ.old_lidswid 
     -             ) ) THEN

            CALL write_message (cl_errlun, cl_errfnm, 
     -          'Error: Program parameters mismatch in ' // 
     -           cl_binfnm(str_beg(cl_binfnm):str_end(cl_binfnm)) )

            WRITE (cl_errlun,'(2(a,i6),5(/2(a,i6)))') 
     -          ' **  bin_tuplen  = ',bin_tuplen,  '     cl_tuplen = ',cl_tuplen,
     -          ' **  bin_oligos  = ',bin_oligos,  '    oli_number = ',oli_number,
     -          ' ** bin_seqsdim  = ',bin_seqsdim, '   old_seqsdim = ',old_seqsdim,
     -          ' **  bin_basdim  = ',bin_basdim,  '    old_basdim = ',old_basdim,
     -          ' ** bin_sidswid  = ',bin_sidswid, '   old_sidswid = ',old_sidswid,
     -          ' ** bin_lidswid  = ',bin_lidswid, '   old_lidswid = ',old_lidswid

            CALL write_message ( cl_errlun, cl_errfnm,
     -          'You may recompile the program, or use the easier way: ' //LF//
     -          'regenerate the binary file by deleting it' )
            STOP

         END IF
C
C Get memory and load arrays with binary read
C
         CALL get_memory (old_sidsptr, old_seqsmax * old_sidswid, 'old_sids', 'main')
         CALL get_memory (old_lidsptr, old_seqsmax * old_lidswid, 'old_lids', 'main')
         CALL get_memory (old_onumsptr, old_seqsmax * 4, 'old_onums', 'main')
         CALL get_memory (old_tblptr, old_tblmax * 2, 'old_tbl', 'main')
         CALL get_memory (old_tbegsptr, oli_number * 4, 'old_tbegs', 'main')

         CALL sa_bin_io ('READ',cl_binlun,1,old_seqsmax,old_sids)  ! string array
         CALL sa_bin_io ('READ',cl_binlun,1,old_seqsmax,old_lids)  ! string array
         CALL i4_bin_io ('READ',cl_binlun,1,old_seqsmax,old_onums)
         CALL i2_bin_io ('READ',cl_binlun,1,old_tblmax,old_tbl)
         CALL i4_bin_io ('READ',cl_binlun,0,oli_number-1,old_tbegs)

         CLOSE (UNIT = cl_binlun)

      END IF
C 
C ------------------------------------------------------------------------------
C -u on command line. Reads added GenBank sequences and redo the data tables
C so the additional sequences are accomodated.
C ------------------------------------------------------------------------------
C
      add_seqsmax = 0   ! used later with old_incl

      IF ( cl_addflag ) THEN

         OPEN (UNIT = cl_addlun,
     -         FILE = cl_addfnm, 
     -       STATUS = 'OLD')
C
C Find number of sequences and bases (other than AaUuTtGgCc are ignored);
C cl_partlen does not exclude added sequences.
C
         CALL feel_gbk ( cl_addlun, WC_base, cl_tuplen, add_seqsmax, add_byts )
C         print*,'after feel_gbk, add_seqsmax, add_byts = ',add_seqsmax,add_byts

         IF ( add_seqsmax.LT.1 ) THEN
            CALL write_message ( cl_errlun, cl_errfnm,
     -            'Warning: No valid entries in ' // 
     -            cl_addfnm(str_beg(cl_addfnm):str_end(cl_addfnm)) // LF //
     -            'please examine it' )
            STOP
         END IF

         noerrs = add_seqsmax .LE. add_seqsdim    .AND.
     -               add_byts .LE. add_seqsdim * add_basdim

         IF (.NOT.noerrs) THEN

            CALL write_message ( cl_errlun, cl_errfnm, 
     -           'Error: Needed bounds for ' // 
     -           cl_addfnm(str_beg(cl_addfnm):str_end(cl_addfnm)) // 
     -           ' overflow dynamic memory limits')

            WRITE ( cl_errlun, '(2(a,i)/2(a,i))' )
     -           ' **  add_seqsmax = ',add_seqsmax, 
     -           '     add_seqsdim = ',add_seqsdim,
     -           ' **     add_byts = ',add_byts,
     -           '      add_bytdim = ',add_bytdim
            
            CALL write_message ( cl_errlun, cl_errfnm, 
     -           'Increase dynamic maxima in sim_params.inc and recompile')

            STOP

         END IF
C
C Allocate memory for sequences (long 1d array), short-ID's, full names, 
C tables of beginnings and ends of each sequence in add_seq
C
         CALL get_memory (add_seqsptr ,add_byts, 'add_seqs', 'main')
         CALL get_memory (add_sidsptr, add_seqsmax * add_sidswid, 'add_sids', 'main')
         CALL get_memory (add_lidsptr, add_seqsmax * add_lidswid, 'add_lids', 'main')
         CALL get_memory (add_begsptr, add_seqsmax * 4, 'add_begs', 'main')
         CALL get_memory (add_endsptr, add_seqsmax * 4, 'add_ends', 'main')
C     
C Read sequences into add_seqs, store beginnings and ends of sequences, and
C their short-ID's and full names
C
         REWIND (UNIT = cl_addlun)
C
C Read Genbank alignment, while throwing away all non-WC bases, again: 
C
C  add_seqs :  Holds all sequences as one long composite sequence
C  add_begs :  Pointers to where in add_seqs each sequence begins
C  add_ends :  Pointers to where in add_seqs each sequence ends
C  add_sids :  Short-ID's for each sequence
C  add_lids :  Full names for each sequence
C
         CALL get_gbk ( cl_addlun, add_seqsmax, WC_base, cl_tuplen, add_byts, 
     -                  add_seqs, add_begs, add_ends, add_sids, add_lids )
C         print*,'after feel_gbk, add_seqsmax, add_byts = ',add_seqsmax,add_byts

         CLOSE (UNIT = cl_addlun)
C
C Translate  A = 0, U = 1, G = 2, C = 3 
C       
         CALL To_Code4 ( add_seqs, 1, add_byts )
C
C Create linked list
C
         CALL get_memory ( add_llstptr, add_byts * 4, 'add_llst', 'main' )
         CALL get_memory ( add_obegsptr, oli_number * 4, 'add_obegs', 'main' )

         CALL make_lnk_lst ( 4, cl_tuplen, oli_number, add_seqsmax, add_byts,
     -                       1, add_seqsmax, add_seqs, add_begs, add_ends,
     -                       add_obegs, add_llst )
C
C No need for (4-coded) sequence anymore
C
         CALL FREE (add_seqsptr)
C
C From add_llst and add_obegs construct three tables: add_tbl, 
C add_tbegs and add_onums:
C
C add_tbl:  Contains runs terminated by 0 of the sequence numbers, in
C           which a given oligo occurs; add_tbegs points to the start 
C           of those runs, example
C
C Add_tbegs:  |                       |                   |   |      etc
C Add_tbl    001 004 013 015 246 000 001 007 013 056 000 000 344 000 etc
C
C add_onums: Contains for each sequence the number of different oligos
C
C The memory estimate for add_tbl can probably be improved, but below it is
C set high enough to accomodate the case, where for each sequence all oligos
C are different.
C
         CALL get_memory ( add_tblptr, (add_byts+oli_number) * 2, 'add_tbl', 'main' )
         CALL get_memory ( add_tbegsptr, oli_number * 4, 'add_tbegs', 'main' )
         CALL get_memory ( add_onumsptr, add_seqsmax * 4, 'add_onums', 'main' )

         CALL make_tables ( oli_number, 
     -                      add_seqsmax, add_byts, add_byts+oli_number, 
     -                      add_ends, add_obegs, add_llst,
     -                      add_tblmax, add_tbl, add_tbegs, add_onums )

         CALL FREE (add_obegsptr)
         CALL FREE (add_llstptr)
         CALL FREE (add_begsptr)
         CALL FREE (add_endsptr)
C
C Now recreate old_sids, old_lids, old_tbegs, old_onums and old_tbl by
C merging add_sids, add_lids, add_onums and add_tbl, and by updating
C old_tbegs. This takes a lot of memory, but what to do. 
C
C First make new versions of old_sids, old_lids and old_onums with 
C enough room to hold add_sids, add_lids and add_onums
C
         tot_seqsmax = old_seqsmax + add_seqsmax

         CALL get_memory ( scr_sidsptr, tot_seqsmax * old_sidswid, 'scr_sids', 'main' )
         CALL get_memory ( scr_lidsptr, tot_seqsmax * old_lidswid, 'scr_lids', 'main' )
         CALL get_memory ( scr_onumsptr, tot_seqsmax * 4, 'scr_onumsptr', 'main' )

         jseq = 0

         DO iseq = 1, old_seqsmax
            jseq = jseq + 1
            scr_sids(jseq) = old_sids(iseq)
            scr_lids(jseq) = old_lids(iseq)
            scr_onums(jseq) = old_onums(iseq)
         END DO

         DO iseq = 1, add_seqsmax
            jseq = jseq + 1
            scr_sids(jseq) = add_sids(iseq)
            scr_lids(jseq) = add_lids(iseq)
            scr_onums(jseq) = add_onums(iseq)
         END DO

         CALL FREE ( old_sidsptr ) 
         CALL FREE ( old_lidsptr )
         CALL FREE ( old_onumsptr )
C
C Reallocate for larger version; I wish one could incrementally allocate
C
         CALL get_memory ( old_sidsptr, tot_seqsmax * old_sidswid, 'old_sids', 'main' )
         CALL get_memory ( old_lidsptr, tot_seqsmax * old_lidswid, 'old_lids', 'main' )
         CALL get_memory ( old_onumsptr, tot_seqsmax * 4, 'old_onumsptr', 'main' )

         DO iseq = 1, tot_seqsmax
            old_sids(iseq) = scr_sids(iseq)
            old_lids(iseq) = scr_lids(iseq)
            old_onums(iseq) = scr_onums(iseq)
         END DO

         CALL FREE ( scr_sidsptr ) 
         CALL FREE ( scr_lidsptr )
         CALL FREE ( scr_onumsptr )
C
C Do the same with old_tbl, and update old_tbegs,
C
         scr_tblmax = old_tblmax + add_tblmax - oli_number 

         CALL get_memory ( scr_tblptr, scr_tblmax * 2, 'scr_tbl', 'main' )

         scr_tblpos = 1

         DO ioli = 0, oli_number - 1

            old_tblpos = old_tbegs (ioli)
            old_tbegs (ioli) = scr_tblpos

            DO WHILE ( old_tbl(old_tblpos).NE.0 )
               scr_tbl (scr_tblpos) = old_tbl (old_tblpos)
               scr_tblpos = scr_tblpos + 1
               old_tblpos = old_tblpos + 1 
            END DO

            add_tblpos = add_tbegs (ioli)

            DO WHILE ( add_tbl(add_tblpos).NE.0 )
               scr_tbl(scr_tblpos) = add_tbl(add_tblpos) + old_seqsmax
               scr_tblpos = scr_tblpos + 1 
               add_tblpos = add_tblpos + 1 
            END DO 

            scr_tbl (scr_tblpos) = 0 
            scr_tblpos = scr_tblpos + 1

         END DO

         CALL FREE ( add_tblptr ) 
         CALL FREE ( add_tbegsptr )

         CALL FREE ( old_tblptr )
C
C Reallocate and copy back into old_tbl
C
         old_seqsmax = tot_seqsmax
         old_tblmax = scr_tblpos - 1 
         
         CALL get_memory ( old_tblptr, old_tblmax * 2, 'old_tbl', 'main' )
         
         DO old_tblpos = 1, old_tblmax
            old_tbl (old_tblpos) = scr_tbl (old_tblpos)
         END DO
         
         CALL FREE ( scr_tblptr )

      ELSE

         add_seqsmax = 0 

      END IF
C
C ------------------------------------------------------------------------------
C -n on command line. Input optional namelist which flags subset of database
C ------------------------------------------------------------------------------
C
C -n on the command line imports and extracts short-IDs from a namelist 
C formatted as the list of names returned by the RDP email server (both 
C alphabetic and phylogenetically ordered lists work). Below a copy of the 
C total name list is sorted in descending order, and each name in
C the imported list is searched for in the full list. The search is done 
C by a binary search procedure, so the input order doesnt matter, and 
C it is fast in time. If a short-ID from the imported list is absent from 
C the database, a warning message is printed, but program continues. The 
C array old_incl contains flags of which sequences to include in the 
C searches, all other arrays are temporary.
C 
      CALL get_memory ( old_inclptr, old_seqsmax, 'old_incl', 'main' )

      IF ( cl_namflag ) THEN
C
C If user sequences added (add_seqsmax > 0), then always include them even 
C though they are not in name list; otherwise only include those in name
C list,
C
         IF (add_seqsmax.GT.0) THEN
            bool = .FALSE.
            CALL l1_array_init ( 1, old_seqsmax, old_incl, 1,
     -           old_seqsmax-add_seqsmax, bool )
            bool = .TRUE.
            CALL l1_array_init ( 1, old_seqsmax, old_incl, 
     -           old_seqsmax-add_seqsmax+1, old_seqsmax, bool )
         ELSE
            bool = .FALSE.
            CALL l1_array_init ( 1, old_seqsmax, old_incl, 1, 
     -           old_seqsmax, bool )
         END IF
C
C Make and sort copy of short-ID list from main alignment
C
         CALL get_memory ( scr_sidsptr, old_seqsmax * old_sidswid, 'scr_sids', 'main' )
         CALL get_memory ( scr_ndcsptr, old_seqsmax * 4, 'scr_ndcs', 'main' )

         DO iseq = 1, old_seqsmax
            scr_sids (iseq) = old_sids (iseq)
            scr_ndcs (iseq) = iseq
         END DO

         CALL str_shellsort ( scr_sids, scr_ndcs, old_seqsmax )
C
C Extract short-ID's from namelist file using str_field. Use the 
C field if it contains at least one alphabetic character. 
C
         OPEN (UNIT = cl_namlun,
     -         FILE = cl_namfnm,
     -       STATUS = 'OLD')

         nam_seqsmax = 0    ! total number of sequences in namelist

         DO WHILE (.TRUE.)

            READ ( cl_namlun, '(q,a)', END=99 ) linend,line

            CALL str_field (line, 1, linend, 1, ' ', begpos, endpos)

            IF ( begpos.GT.0 .AND. endpos.GE.begpos ) THEN
               ialf = 0
               DO ipos = begpos, endpos
                  IF (ch_is_alpha(line(ipos:ipos))) ialf = ialf + 1 
               END DO
               IF ( ialf.GT.0 ) THEN
                  CALL str_lst_lookup ( 'DESCENDING', old_seqsmax, 
     -                          scr_sids, line(begpos:endpos), indx )
                  IF ( indx.GT.0 ) THEN
                     old_incl(scr_ndcs(indx)) = .TRUE.
                     nam_seqsmax = nam_seqsmax + 1 
                  ELSE   
                     CALL write_message ( cl_errlun, cl_errfnm, 
     -                  'Warning: Short-ID "' // line(begpos:endpos) // 
     -                  '" not found in database' )
                  END IF
               END IF
            END IF

         END DO

 99      CLOSE ( UNIT = cl_namlun )

         CALL FREE ( scr_sidsptr)
         CALL FREE ( scr_ndcsptr)

      ELSE 
         bool = .TRUE.
         CALL l1_array_init ( 1, old_seqsmax, old_incl, 1, old_seqsmax, bool )
      END IF
C
C ------------------------------------------------------------------------------
C Exclude sequences with fewer unique oligos than cl_partlen, D = 50, but
C controlled by -p on the command line. Dont exclude any of the added ones
C though. If the namelist contains a sequence with too few oligos it will 
C excluded. 
C ------------------------------------------------------------------------------
C
      DO iseq = 1, old_seqsmax - add_seqsmax
         IF ( old_onums(iseq).LT.cl_partlen ) old_incl(iseq) = .FALSE.
      END DO
C 
C ------------------------------------------------------------------------------
C Input sequence(s) to be analyzed
C ------------------------------------------------------------------------------
C
      OPEN (UNIT = cl_newlun,
     -      FILE = cl_newfnm,
     -    STATUS = 'OLD')

      CALL feel_gbk ( cl_newlun, WC_base, cl_tuplen, new_seqsmax, new_byts )
C         print*,'after feel_gbk, new_seqsmax, new_byts = ',new_seqsmax,new_byts

      IF (new_seqsmax.LT.1) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -        'Error: No valid entries in input file ' //
     -       cl_newfnm(str_beg(cl_newfnm):str_end(cl_newfnm)))
         STOP
      END IF

      noerrs = new_seqsmax .LE. new_seqsdim    .AND.
     -            new_byts .LE. new_seqsdim * new_basdim

      IF (.NOT.noerrs) THEN

         CALL write_message (cl_errlun,cl_errfnm, 
     -      'Error: Needed bounds for ' // 
     -      cl_alifnm(str_beg(cl_alifnm):str_end(cl_alifnm)) // 
     -      ' overflow dynamic memory limits')

         WRITE (cl_errlun,'(2(a,i)/2(a,i))')
     -      ' **  new_seqsmax = ',new_seqsmax, 
     -      '     new_seqsdim = ',new_seqsdim,
     -      ' **     new_byts = ',new_byts,
     -      '      new_bytdim = ',new_bytdim

         CALL write_message (cl_errlun,cl_errfnm, 
     -      'Increase dynamic maxima in sim_params.inc and recompile')

         STOP

      END IF

      CALL get_memory (new_seqsptr, new_byts, 'new_seqs', 'main')
      CALL get_memory (new_sidsptr, new_seqsmax * new_sidswid, 'new_sids', 'main')
      CALL get_memory (new_lidsptr, new_seqsmax * new_lidswid, 'new_lids', 'main')
      CALL get_memory (new_begsptr, new_seqsmax * 4, 'new_begs', 'main')
      CALL get_memory (new_endsptr, new_seqsmax * 4, 'new_ends', 'main')

      REWIND (UNIT = cl_newlun)

      CALL get_gbk ( cl_newlun, new_seqsmax, WC_base, cl_tuplen, new_byts, 
     -               new_seqs, new_begs, new_ends, new_sids, new_lids )
C         print*,'after feel_gbk, new_seqsmax, new_byts = ',new_seqsmax,new_byts

      CLOSE (UNIT = cl_newlun)
C
C Convert to integer codes 0,1,2,3 
C
      CALL To_Code4 ( new_seqs, 1, new_byts )
C
C Translate sequence to unique integers based on tuplen and alphabet size
C
      CALL get_memory ( new_codsptr, (new_byts-cl_tuplen+1) * 4,
     -                 'new_cods', 'main' )

      CALL make_iol_seq ( 4, cl_tuplen, new_byts, new_seqs, 1, new_byts, new_cods )

      CALL FREE (new_seqsptr)
C
C -------------------------------------------------------------------------
C Comparison and output section. These program parts are encapsulated 
C because they will become two separate programs soon.
C -----------------------------------------------------------------------------
C
      IF (cl_outlun.NE.6)
     -   OPEN (UNIT = cl_outlun,
     -         FILE = cl_outfnm,
     -       STATUS = 'NEW')

      IF (cl_chimflag) THEN
         CALL check_chimera ( oli_number, old_basdim, old_seqsmax, 
     -                        old_sids, old_lids, old_onums, 
     -                        old_tblmax, old_tbegs, old_tbl, old_incl,
     -                        new_seqsmax, new_byts, new_sids, new_lids,
     -                        new_begs, new_ends, new_cods )
      ELSE
         CALL similarity_rank ( oli_number, old_basdim, old_seqsmax, 
     -                          old_sids, old_lids, old_onums, 
     -                          old_tblmax, old_tbegs, old_tbl, old_incl,
     -                          new_seqsmax, new_byts, new_sids, new_lids,
     -                          new_begs, new_ends, new_cods )
      END IF

      IF (cl_outlun.NE.6) CLOSE (UNIT = cl_outlun)
      IF (cl_errlun.NE.6) CLOSE (UNIT = cl_errlun)
C
C Free memory
C
      CALL FREE (old_sidsptr)
      CALL FREE (old_lidsptr)
      CALL FREE (old_onumsptr)
      CALL FREE (old_tblptr)
      CALL FREE (old_tbegsptr)

      CALL FREE (old_inclptr)

      CALL FREE (new_sidsptr)
      CALL FREE (new_lidsptr)
      CALL FREE (new_codsptr)
      CALL FREE (new_begsptr)
      CALL FREE (new_endsptr)

c$$$C
c$$$C --------------------------------------------------------------------------
c$$$C Floating point errors; print out floating point exception flags, if set
c$$$C --------------------------------------------------------------------------
c$$$C
c$$$      accrued = IEEE_FLAGS ('get','exception','',ieee_id)
c$$$
c$$$      inx   = AND (RSHIFT(accrued,inexact),1)
c$$$      div   = AND (RSHIFT(accrued,division),1)
c$$$      under = AND (RSHIFT(accrued,underflow),1)
c$$$      over  = AND (RSHIFT(accrued,overflow),1)
c$$$      inv   = AND (RSHIFT(accrued,invalid),1)
c$$$
c$$$C      IF (inx.NE.0) CALL write_message (cl_errlun,cl_errfnm,
c$$$C     -                     'Error: Floating point error INEXACT') 
c$$$      IF (under.NE.0) CALL write_message (cl_errlun,cl_errfnm,
c$$$     -                     'Error: Floating point error UNDERFLOW') 
c$$$      IF (over.NE.0) CALL write_message (cl_errlun,cl_errfnm,
c$$$     -                     'Error: Floating point error OVERFLOW') 
c$$$      IF (div.NE.0) CALL write_message (cl_errlun,cl_errfnm,
c$$$     -                     'Error: Floating point error DIVIDE BY ZERO') 
c$$$      IF (inv.NE.0) CALL write_message (cl_errlun,cl_errfnm,
c$$$     -                     'Error: Floating point error INVALID OPERAND') 

      END

      SUBROUTINE check_chimera ( oli_number, old_basdim, old_seqsmax, 
     -                           old_sids, old_lids, old_onums, 
     -                           old_tblmax, old_tbegs, old_tbl, old_incl,
     -                           new_seqsmax, new_byts, new_sids, new_lids,
     -                           new_begs, new_ends, new_cods )
      IMPLICIT none
      
      INCLUDE 'sim_cmdlin.inc'

      INTEGER       oli_number
      INTEGER       old_basdim
      INTEGER       old_seqsmax
      CHARACTER*(*) old_sids (old_seqsmax)
      CHARACTER*(*) old_lids (old_seqsmax)
      INTEGER       old_onums (old_seqsmax)
      INTEGER       old_tblmax
      INTEGER       old_tbegs (0:oli_number-1)
      INTEGER*2     old_tbl (old_tblmax)
      LOGICAL*1     old_incl (old_seqsmax)
      INTEGER       new_seqsmax
      INTEGER       new_byts
      CHARACTER*(*) new_sids (new_seqsmax)
      CHARACTER*(*) new_lids (new_seqsmax)
      INTEGER       new_begs (new_seqsmax)
      INTEGER       new_ends (new_seqsmax)
      INTEGER       new_cods (new_byts)
C
C Local variables
C
      CHARACTER*13 program_ID / 'CHECK_CHIMERA' /
      CHARACTER*3 version_ID / '2.6'    /
      CHARACTER*12 author_ID / 'Niels Larsen' /
      CHARACTER*23 email_ID  / 'niels@vitro.cme.msu.edu' /
      CHARACTER*16 rev_date  / 'February 3, 1997' / 
      CHARACTER*10 server_ID / 'RDPMAILSRV' / 

      REAL timarr(2), time, dtime
      CHARACTER*100 tim_str
      CHARACTER*24 datstr

      CHARACTER*999999 eqsline
      POINTER (eqslineptr,eqsline)

      INTEGER    oli_adds (old_seqsmax)
      POINTER   (oli_addsptr,oli_adds)

      INTEGER    oli_adds1 (old_seqsmax)
      POINTER   (oli_adds1ptr,oli_adds1)

      INTEGER    oli_adds2 (old_seqsmax)
      POINTER   (oli_adds2ptr,oli_adds2)

      INTEGER*4  chi_gain (old_basdim)
      POINTER   (chi_gainptr,chi_gain)

      INTEGER*4  chi_ndcs (old_basdim)
      POINTER   (chi_ndcsptr,chi_ndcs)

      INTEGER   chi_tblmax, chi_ndxmax
      INTEGER   chi_brkndx, chi_brkpos

      LOGICAL*1 tmp_incl (old_seqsmax)
      POINTER  (tmp_inclptr,tmp_incl)

      LOGICAL chi_flag

      INTEGER iseq, jseq, listlen
      
      INTEGER i4_array_max
      INTEGER new_onum1, new_onum2, new_onum

      CHARACTER*1000 usestr ! , wrapstr
      CHARACTER*10 scrstr1, scrstr2, scrstr3, scrstr4
      CHARACTER*1 LF / 10 /

      INTEGER tmp_inclmax
      INTEGER str_beg, str_end, str_end_ch
      INTEGER i, j
C
C -------------------------------------------------------------------------------
C
C Temporary include list used to exclude database short-ID's that match 
C that of sequence being analyzed,
C
      CALL get_memory ( tmp_inclptr, old_seqsmax, 'tmp_incl', 'check_chimera' )
C
C Lines and temporary arrays for oligo counts,
C
      CALL get_memory (eqslineptr, cl_outwid, 'eqsline', 'check_chimera') 
      CALL str_init (eqsline, 1, cl_outwid, '=')

      CALL get_memory ( oli_adds1ptr, old_seqsmax * 4, 'oli_adds1', 'check_chimera' ) 
      CALL get_memory ( oli_adds2ptr, old_seqsmax * 4, 'oli_adds2', 'check_chimera' ) 
      CALL get_memory ( oli_addsptr , old_seqsmax * 4, 'oli_adds' , 'check_chimera' ) 
C
C Loop through each sequence in new_seqs, 
C
      DO iseq = 1, new_seqsmax
C
C If sequence shorter than twice minimum fragment length, ignore it and
C continue with next entry
C
         IF ( new_ends(iseq) - new_begs(iseq) + 1 .LT. 2 * cl_fraglen ) THEN
            CALL write_message ( cl_errlun, cl_errfnm, 'Error: Sequence ' // 
     -                           new_sids(iseq)(str_beg(new_sids(iseq)):
     -                                          str_end(new_sids(iseq))) //
     -             ' shorter than 2 x minimum fragment length,' // LF // 
     -             ' entry ignored.')
            GO TO 100
         END IF

         time = dtime (timarr)
C
C Remove from include list database short-ID's that match new_sids(iseq)
C
         tmp_inclmax = 0

         DO jseq = 1, old_seqsmax
            IF ( old_sids(jseq).EQ.new_sids(iseq) ) THEN
               tmp_incl (jseq) = .FALSE.
            ELSE
               tmp_incl (jseq) = old_incl (jseq)
               IF ( tmp_incl(jseq) ) tmp_inclmax = tmp_inclmax + 1
            END IF
         END DO
C
C Move breakpoint and save gained values in chi_gain, and true sequence
C indices in chi_ndcs
C
         chi_ndxmax = ( new_ends(iseq) - new_begs(iseq) + 1 ) / cl_steplen

         CALL get_memory ( chi_gainptr, chi_ndxmax * 4, 'chi_gain', 'check_chimera' )
         CALL get_memory ( chi_ndcsptr, chi_ndxmax * 4, 'chi_ndcs', 'check_chimera' )

         CALL chimera_gain ( 
     -               oli_number, old_seqsmax, old_tblmax, 
     -               old_tbl, old_tbegs, tmp_incl,
     -               new_byts, new_cods, new_begs(iseq), new_ends(iseq),
     -               chi_ndxmax, chi_gain, chi_ndcs, chi_tblmax )
C
C Smoothen histogram by averaging over window size 5, 
C
         CALL smoothen ( chi_ndxmax, chi_gain, 1, chi_tblmax, 5, chi_gain ) 

         chi_flag = i4_array_max (chi_gain,1,chi_tblmax,chi_brkndx).GT.0
C
C Insert better detection of breakpoint here
C
C 
         chi_brkpos = chi_ndcs (chi_brkndx)

         IF (chi_flag) THEN

            CALL add_oligos ( oli_number, 
     -               old_seqsmax, old_tblmax, old_tbegs, old_tbl, tmp_incl,
     -               new_byts, new_begs(iseq),
     -               new_begs(iseq)+chi_brkpos-1, new_cods, 
     -               new_onum1, oli_adds1 )

            CALL add_oligos ( oli_number, 
     -               old_seqsmax, old_tblmax, old_tbegs, old_tbl, tmp_incl,
     -               new_byts, new_begs(iseq)+chi_brkpos,
     -               new_ends(iseq)-cl_tuplen+1, new_cods, 
     -               new_onum2, oli_adds2 )

         END IF

         CALL add_oligos ( oli_number, 
     -            old_seqsmax, old_tblmax, old_tbegs, old_tbl, tmp_incl,
     -            new_byts, new_begs(iseq),
     -            new_ends(iseq)-cl_tuplen+1, new_cods, 
     -            new_onum, oli_adds )

         time = dtime(timarr)
         CALL dtime_write (timarr,tim_str,i,j)

         IF ( cl_htmlflag ) THEN

            IF (iseq .EQ. 1) THEN
C               WRITE ( cl_outlun, '(a)') 
C     - 'Content-Type: text/html' //LF//LF
            ELSE 
               WRITE ( cl_outlun, '(a)') '<p><p>' //LF
            END IF

             WRITE ( cl_outlun, '(a)') 
     - '<head>' //LF//
     - '<title> Chimera check againt RDP </title>' //LF//
     - '</head>' //LF//LF//
     - '<body>'

            WRITE (cl_outlun,'(a)') 
     - '<h4>' // ' ' // program_ID // ' version ' // version_ID // ' </h4>' //LF//
     - '<p>' 

            WRITE (cl_outlun,'(a)') 
     - '<a href="../pepl/niels_cv.html">' // author_ID // '</a>, ' // 
     -  email_ID //LF// 
     - '<p>'

            WRITE (cl_outlun, '(a)') 
     - '<table cellspacing=0 cellpadding=0>'

            IF (new_sids(iseq).NE.server_ID) WRITE (cl_outlun,'(a)')  
     - '<tr align=left> <td> Short-ID <td> : <td nowrap> ' // 
     -    new_sids(iseq) (str_beg(new_sids(iseq)):str_end(new_sids(iseq))) 

            IF (str_end(new_lids(iseq)).GT.0) WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> Full name <td> : <td nowrap> ' //
     -         new_lids(iseq) (str_beg(new_lids(iseq)):
     -         MIN (cl_outwid-13,str_end(new_lids(iseq)))) 

            IF (cl_lstflag) THEN

               WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> RDP data <td> : <td nowrap> ' // 
     -  cl_lststr(str_beg(cl_lststr):str_end(cl_lststr))
            ELSE
               WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> RDP data <td> : <td nowrap> ' // 
     -      cl_binfnm(str_end_ch(cl_binfnm,'/')+1:str_end(cl_binfnm))
            END IF

            WRITE (scrstr1,'(i10)') tmp_inclmax
            WRITE (scrstr2,'(i10)') cl_tuplen
            WRITE (scrstr3,'(i10)') old_seqsmax - tmp_inclmax
            WRITE (scrstr4,'(i10)') cl_partlen

            WRITE (cl_outlun,'(a)')  
     - '<tr align=left> <td> Comments <td> : <td nowrap> A minimum of ' // 
     -          scrstr4(str_beg(scrstr4):str_end(scrstr4)) //
     -               ' unique oligos required'  //LF// 
     - '<tr align=left> <td> <td> : <td nowrap> A total of ' // 
     -          scrstr3(str_beg(scrstr3):str_end(scrstr3)) //
     -               ' sequences were excluded'    //LF//
     - '<tr align=left> <td> <td> : <td nowrap> ' // 
     -          scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     -               ' sequences were included in the search' //LF// 
     - '<tr align=left> <td> <td> : <td nowrap> The screening was based on ' // 
     -              scrstr2(str_beg(scrstr2):str_end(scrstr2))       // 
     -               '-base oligomers'

            CALL fdate(datstr)
            WRITE (cl_outlun,'(a)')
     - '<tr align=left> <td> Date <td nowrap> : <td> ' // datstr

            WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> CPU time <td nowrap> : <td> ' // tim_str (i:j)

            WRITE (cl_outlun,'(a)') 
     - '</table>' //LF//
     - '<p>'

         ELSE

            WRITE (cl_outlun,'(/a)') 
     - ' ' // program_ID // ' version ' // version_ID 

            WRITE (cl_outlun,'(/a)') 
     - ' ' // author_ID // ', ' // email_ID

            WRITE (cl_outlun,'(/a)') 
     - ' Short-ID  : ' // 
     -             new_sids(iseq) (str_beg(new_sids(iseq)):str_end(new_sids(iseq)))

            IF (str_end(new_lids(iseq)).GT.0) WRITE (cl_outlun,'(a)')   
     - ' Full name : ' //
     -             new_lids(iseq) (str_beg(new_lids(iseq)):
     -             MIN (cl_outwid-13,str_end(new_lids(iseq))))

            IF (cl_lstflag) THEN
               WRITE (cl_outlun,'(a)') 
     - ' RDP data  : ' // cl_lststr(str_beg(cl_lststr):str_end(cl_lststr))
            ELSE
               WRITE (cl_outlun,'(a)') 
     - ' RDP data  : ' // cl_binfnm(str_end_ch(cl_binfnm,'/')+1:str_end(cl_binfnm))
            END IF

            WRITE (scrstr1,'(i10)') tmp_inclmax
            WRITE (scrstr2,'(i10)') cl_tuplen
            WRITE (scrstr3,'(i10)') old_seqsmax - tmp_inclmax
            WRITE (scrstr4,'(i10)') cl_partlen
            
            WRITE (cl_outlun,'(a)')   
     - ' Comments  : A minimum of ' // scrstr4(str_beg(scrstr4):str_end(scrstr4)) //
     -               ' unique oligos required'  // LF // 
     - '           : A total of ' // scrstr3(str_beg(scrstr3):str_end(scrstr3)) //
     -               ' sequences were excluded'                      // LF //
     - '           : ' // scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     -             ' sequences were included in the search'          // LF // 
     - '           : The screening was based on '                    // 
     -              scrstr2(str_beg(scrstr2):str_end(scrstr2))       // 
     -             '-base oligomers'

            CALL fdate(datstr)
            WRITE (cl_outlun,'(a)')
     - ' Date      : ' // datstr

            WRITE (cl_outlun,'(a)')
     - ' CPU time  : ' // tim_str (i:j)

            WRITE (cl_outlun,'(/,1x,a)') eqsline (1:cl_outwid)

         END IF

         IF (chi_flag) THEN

            IF (cl_htmlflag) THEN

               WRITE (cl_outlun,'(a)') 
     -  '<hr>' //LF//
     -  '<p>' //LF//
     -  '<font color=red>Caution</font>: this service does not decide ' //LF//
     -  'if your sequence is chimeric, it merely supports your own ' //LF//
     -  'decision. To understand the output below and what the program ' //LF//
     -  'is doing with your data, please consult the ' //LF//
     -  '<A HREF=../docs/chimera_doc.html>method description</A>.<p> '

            ELSE 

               usestr = 
     -  ' Caution: this service does not decide if your sequence is ' //LF//
     -  ' chimeric, it merely supports your own decision. To understand ' //LF//
     -  ' the output below and what the program is doing with your data, ' //LF//
     -  ' please consult the method description.' 

C               CALL text_wrap (usestr(1:str_end(usestr)),1,cl_outwid,' ',wrapstr)
C               WRITE (cl_outlun,'(/a//)') wrapstr (1:str_end(wrapstr))
               WRITE (cl_outlun,'(/a//)') usestr (1:str_end(usestr))

            END IF 
C
C Output histogram,
C
            CALL output_histo ( chi_ndxmax, chi_tblmax, chi_gain, chi_ndcs,
     -                          "brown", "#646400" )

            CALL FREE (chi_gainptr)
            CALL FREE (chi_ndcsptr)
C
C Output ranked list for fragment one
C
            WRITE (scrstr1,'(i10)') 1 
            WRITE (scrstr2,'(i10)') chi_brkpos
            
            IF (cl_htmlflag) THEN

               WRITE (cl_outlun,'(a)') 
     - '<p>' //
     - '<strong> FRAGMENT 1, approximate sequence positions ' // 
     -            scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     -  ' to ' // scrstr2(str_beg(scrstr2):str_end(scrstr2)) // 
     -  ': </strong>'
      
            ELSE
               WRITE (cl_outlun,'(//a/)') ' FRAGMENT 1, approximate sequence positions ' // 
     -            scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     -  ' to ' // scrstr2(str_beg(scrstr2):str_end(scrstr2)) // ':'      
            END IF

            listlen = MIN ( tmp_inclmax, cl_listlen/2 )

            CALL output_list ( cl_outlun, listlen, cl_outwid, old_seqsmax,
     -                         old_sids, old_lids, oli_adds1, old_onums, 
     -                         new_onum1, new_sids(iseq), new_lids(iseq),
     -                         "brown" )
C
C Output ranked list for fragment two
C
            WRITE (scrstr1,'(i10)') chi_brkpos + 1  
            WRITE (scrstr2,'(i10)') new_ends (iseq) - new_begs (iseq) - cl_tuplen + 2

            IF (cl_htmlflag) THEN
               WRITE (cl_outlun,'(a)') 
     - '<p>' //
     - '<strong> FRAGMENT 2, approximate sequence positions ' // 
     -           scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     - ' to ' // scrstr2(str_beg(scrstr2):str_end(scrstr2)) // 
     - ': </strong>'
            ELSE
               WRITE (cl_outlun,'(//a/)') ' FRAGMENT 2, approximate sequence positions ' // 
     -            scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     -        ' to ' // scrstr2(str_beg(scrstr2):str_end(scrstr2)) // ':'
            END IF

            listlen = MIN ( tmp_inclmax, cl_listlen/2 )

            CALL output_list ( cl_outlun, listlen, cl_outwid, old_seqsmax,
     -                         old_sids, old_lids, oli_adds2, old_onums,
     -                         new_onum2, new_sids(iseq), new_lids(iseq),
     -                         "#646400" )
C
C Output ranked list for full length sequence
C
            WRITE (scrstr1,'(i10)') 1
            WRITE (scrstr2,'(i10)') new_ends (iseq) - new_begs (iseq) - cl_tuplen + 2

            IF (cl_htmlflag) THEN
               WRITE (cl_outlun,'(a)') 
     - '<p>' // 
     - '<strong> FULL LENGTH SEQUENCE, from position ' // 
     -           scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     - ' to ' // scrstr2(str_beg(scrstr2):str_end(scrstr2)) // ': </strong>'
            ELSE
               WRITE (cl_outlun,'(//a/)') 
     - ' FULL LENGTH SEQUENCE, from position ' // 
     -           scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     - ' to ' // scrstr2(str_beg(scrstr2):str_end(scrstr2)) // ':'
            END IF

            listlen = MIN ( tmp_inclmax, cl_listlen )

            CALL output_list ( cl_outlun, listlen, cl_outwid, old_seqsmax,
     -                         old_sids, old_lids, oli_adds, old_onums,
     -                         new_onum, new_sids(iseq), new_lids(iseq),
     -                         "black" )

            WRITE (cl_outlun,'()')

         ELSE

            IF (cl_htmlflag) THEN
               WRITE (6,'(a)') 
     -  '<hr>' //LF//
     -  '<p>' //LF//
     - 'There was ' //
     - 'no way of breaking your sequence in two chimeric halves, so their '//LF//
     - 'combined match with the database was better than that of '  //LF//
     - 'the full length sequence. Please read the ' //LF//
     - '<A HREF=../docs/chimera_doc.html>method description</A>, ' //LF//
     - 'so you know what this fact can mean. <p><hr>'

            ELSE

               usestr = 
     - ' There was no way of breaking your sequence in two chimeric ' //LF//
     - ' halves, so their combined match with the database was better ' //LF//
     - ' than that of the full length sequence. Please read the method ' //LF//
     - ' description, so you know what this fact can mean. ' 

C               CALL text_wrap (usestr(1:str_end(usestr)),1,cl_outwid,' ',wrapstr)
C               WRITE (cl_outlun,'(/a)') wrapstr (1:str_end(wrapstr)) 
               WRITE (cl_outlun,'(/a)') usestr (1:str_end(usestr)) 
               WRITE (cl_outlun,'(1x,/a)') eqsline (1:cl_outwid)
               
            END IF

         END IF

 100  END DO

      IF (cl_htmlflag) THEN
         WRITE (cl_outlun,'(a)') 
     - '</body>' //LF//
     - '</html>'
      END IF

      CALL FREE (eqslineptr)
      CALL FREE (tmp_inclptr)
      CALL FREE (oli_adds1ptr)
      CALL FREE (oli_adds2ptr)
      CALL FREE (oli_addsptr)

      RETURN
      END 

      SUBROUTINE smoothen ( dim, x, smhbeg, smhend, winlen, x_smh ) 
C
C IMPROVE
C
      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'

      INTEGER dim
      INTEGER x (dim)
      INTEGER smhbeg
      INTEGER smhend
      INTEGER winlen
      INTEGER x_smh (dim)
C
C Local variables
C     
      INTEGER  x_scr(dim)
      POINTER (x_scrptr, x_scr)

      CHARACTER*1 LF / 10 /

      INTEGER valsum
      INTEGER ipos, nghbrs, errors
C
C ----------------------------------------------------------------------------
C
C Return with message(s) if arguments, 
C
      errors = 0

      IF ( MOD (winlen,2) .EQ. 0 ) THEN
         CALL write_message (cl_errlun, cl_errfnm, 
     -      'Error: Window length is even in subroutine SMOOTH;' // LF // 
     -      'no averaging done')
         errors = errors + 1 
      END IF 

      IF ( smhend-smhbeg+1 .LT. winlen ) THEN 
         CALL write_message (cl_errlun, cl_errfnm, 
     -      'Error: Smoothening interval is too short for selected' // LF // 
     -      'window length; no averaging done')
         errors = errors + 1 
      END IF

      IF ( winlen .LT. 3 ) THEN 
         CALL write_message (cl_errlun, cl_errfnm, 
     -      'Error: Window length must be 3 or more; no averaging done')
         errors = errors + 1
      END IF

      IF ( errors .GT. 0 ) THEN
         DO ipos = smhbeg, smhend
            x_smh (ipos) = x (ipos)
         END DO
         RETURN
      END IF
C
C Do the averaging,
C
      CALL get_memory ( x_scrptr, dim * 4, 'x_scr', 'smooth' )

      nghbrs = winlen / 2

      DO ipos = smhbeg, smhbeg + nghbrs - 1
         x_scr (ipos) = x (ipos)
      END DO

      valsum = 0
      DO ipos = smhbeg, smhbeg + winlen - 2
         valsum = valsum + x (ipos)
      END DO

      DO ipos = smhbeg + nghbrs, smhend - nghbrs
         valsum = valsum + x (ipos+nghbrs)
         x_scr (ipos) = valsum / winlen
         valsum = valsum - x (ipos-nghbrs)
      END DO

      DO ipos = smhend - nghbrs + 1, smhend
         x_scr (ipos) = x (ipos)
      END DO

      DO ipos = smhbeg, smhend
         x_smh (ipos) = x_scr (ipos)
      END DO

      CALL FREE (x_scrptr)

      RETURN
      END

      SUBROUTINE similarity_rank ( oli_number, old_basdim, old_seqsmax, 
     -                             old_sids, old_lids, old_onums, 
     -                             old_tblmax, old_tbegs, old_tbl, old_incl,
     -                             new_seqsmax, new_byts, new_sids, new_lids,
     -                             new_begs, new_ends, new_cods )
      IMPLICIT none
      
      INCLUDE 'sim_cmdlin.inc'

      INTEGER       oli_number
      INTEGER       old_basdim
      INTEGER       old_seqsmax
      CHARACTER*(*) old_sids (old_seqsmax)
      CHARACTER*(*) old_lids (old_seqsmax)
      INTEGER       old_onums (old_seqsmax)
      INTEGER       old_tblmax
      INTEGER       old_tbegs (0:oli_number-1)
      INTEGER*2     old_tbl (old_tblmax)
      LOGICAL*1     old_incl (old_seqsmax)
      INTEGER       new_seqsmax
      INTEGER       new_byts
      CHARACTER*(*) new_sids (new_seqsmax)
      CHARACTER*(*) new_lids (new_seqsmax)
      INTEGER       new_begs (new_seqsmax)
      INTEGER       new_ends (new_seqsmax)
      INTEGER       new_cods (new_byts)
C
C Local variables
C
      CHARACTER*12 author_ID / 'Niels Larsen' /
      CHARACTER*23 email_ID  / 'niels@vitro.cme.msu.edu' /
      CHARACTER*3 version_ID / '2.6'    /
      CHARACTER*16 rev_date  / 'February 3, 1997' / 
      CHARACTER*10 server_ID / 'RDPMAILSRV' / 

      REAL*4 timarr(2), time, dtime
      CHARACTER*100 tim_str
      CHARACTER*24 datstr
      CHARACTER*1  LF / 10 /

      CHARACTER*10 scrstr1, scrstr2, scrstr3, scrstr4

      CHARACTER*999999 eqsline
      POINTER (eqslineptr,eqsline)

      INTEGER  oli_adds(old_seqsmax)
      POINTER (oli_addsptr,oli_adds)

      INTEGER iseq, listlen, new_onum

      INTEGER old_inclmax
      INTEGER str_beg, str_end, str_end_ch
      INTEGER i, j
C
C ------------------------------------------------------------------------------
C
C Find number of database sequences selected
C
      old_inclmax = 0
      DO iseq = 1, old_seqsmax
         IF (old_incl(iseq)) old_inclmax = old_inclmax + 1 
      END DO

      CALL get_memory (eqslineptr, cl_outwid, 'eqsline', 'similarity_rank') 
      CALL str_init (eqsline, 1, cl_outwid, '=')

      listlen = MIN ( old_inclmax, cl_listlen )
C
C Loop through each sequence in new_seqs, 
C
      DO iseq = 1, new_seqsmax

         CALL get_memory ( oli_addsptr, old_seqsmax * 4, 'oli_adds', 'similarity_rank' ) 

         time = dtime(timarr)
         CALL add_oligos ( oli_number,
     -            old_seqsmax, old_tblmax, old_tbegs, old_tbl, old_incl,
     -            new_byts, new_begs(iseq), 
     -            new_ends(iseq)-cl_tuplen+1, new_cods,
     -            new_onum, oli_adds )
         time = dtime(timarr)
         CALL dtime_write (timarr,tim_str,i,j)

         IF ( cl_simpflag ) THEN
            CALL output_simple ( cl_outlun, listlen, old_seqsmax, oli_adds, 
     -                           old_onums, old_sids, new_sids(iseq), 
     -                           new_onum )
         ELSE IF ( cl_htmlflag ) THEN

            WRITE ( cl_outlun, '(a)') '<p><p>' //LF

            WRITE ( cl_outlun, '(a)') 
     - '<head>' //LF//
     - '<title> Most similar RDP sequences </title>' //LF//
     - '</head>' //LF//LF//
     - '<body>'
            
            WRITE (cl_outlun,'(a)') 
     - '<h4>' // ' SIMILARITY_RANK version ' // version_ID // ' </h4>' //LF//
     - '<p>' 

            WRITE (cl_outlun,'(a)') 
     - '<a href="../pepl/niels_cv.html">' // author_ID // '</a>, ' // 
     -  email_ID //LF// 
     - '<p>'

            WRITE (cl_outlun, '(a)') 
     - '<table cellspacing=0 cellpadding=0>'

            IF (new_sids(iseq).NE.server_ID) WRITE (cl_outlun,'(a)')  
     - '<tr align=left> <td> Short-ID <td> : <td nowrap> ' //
     - '<font color=red> ' // 
     -    new_sids(iseq) (str_beg(new_sids(iseq)):str_end(new_sids(iseq))) //
     - ' </font>'

            IF (str_end(new_lids(iseq)).GT.0) WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> Full name <td> : <td nowrap> ' //
     - ' <font color=red> ' //
     -         new_lids(iseq) (str_beg(new_lids(iseq)):
     -         MIN (cl_outwid-13,str_end(new_lids(iseq)))) //
     - ' </font>'

            IF (cl_lstflag) THEN
               WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> RDP data <td> : <td nowrap> ' // 
     -  cl_lststr(str_beg(cl_lststr):str_end(cl_lststr))
            ELSE
               WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> RDP data <td> : <td nowrap ' // 
     -      cl_binfnm(str_end_ch(cl_binfnm,'/')+1:str_end(cl_binfnm))
            END IF

            WRITE (scrstr1,'(i10)') old_inclmax
            WRITE (scrstr2,'(i10)') cl_tuplen
            WRITE (scrstr3,'(i10)') old_seqsmax - old_inclmax
            WRITE (scrstr4,'(i10)') cl_partlen

            WRITE (cl_outlun,'(a)')  
     - '<tr align=left> <td> Comments <td> : <td nowrap> A minimum of ' // 
     -          scrstr4(str_beg(scrstr4):str_end(scrstr4)) //
     -               ' unique oligos required'  //LF// 
     - '<tr align=left> <td> <td> : <td nowrap> A total of ' // 
     -          scrstr3(str_beg(scrstr3):str_end(scrstr3)) //
     -               ' sequences were excluded'    //LF//
     - '<tr align=left> <td> <td> : <td nowrap> ' // 
     -          scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     -               ' sequences were included in the search' //LF// 
     - '<tr align=left> <td> <td> : <td nowrap> The screening was based on ' // 
     -              scrstr2(str_beg(scrstr2):str_end(scrstr2))       // 
     -               '-base oligomers'

            CALL fdate(datstr)
            WRITE (cl_outlun,'(a)')
     - '<tr align=left> <td> Date <td> : <td> ' // datstr

            WRITE (cl_outlun,'(a)') 
     - '<tr align=left> <td> CPU time <td> : <td> ' // tim_str (i:j)

            WRITE (cl_outlun,'(a)') 
     - '</table>' //LF//
     - '<p>'
C     
C Output as a table that lists in decreasing order the percentage of shared 
C oligos (relative to the sequence with fewest oligos); the associated number
C oligos, short-ID's and full names (if present) are also listed. Note that 
C a short good match will rank higher than a long not so good match, for both
C a partial new sequence and one in the database.
C
            CALL output_list ( cl_outlun, listlen, cl_outwid, old_seqsmax,
     -                         old_sids, old_lids, oli_adds, old_onums, 
     -                         new_onum, new_sids(iseq), new_lids(iseq),
     -                         "red" )

            WRITE (cl_outlun,'(a)')
     - '</body>' //LF//
     - '</html>'

      ELSE

                     WRITE (cl_outlun,'(/a)') 
     - ' SIMILARITY_RANK version ' // version_ID

                     WRITE (cl_outlun,'(/a/)') 
     - ' ' // author_ID // ', ' // email_ID

c$$$  	             IF (new_sids(iseq).NE.server_ID) WRITE (cl_outlun,'(a)')   
c$$$     - ' Seq. file : ' // 
c$$$     -               cl_newfnm( str_end_ch(cl_newfnm,'/')+1 : str_end(cl_newfnm) )

  	             IF (new_sids(iseq).NE.server_ID) WRITE (cl_outlun,'(a)')  
     - ' Short-ID  : ' // 
     -               new_sids(iseq) (str_beg(new_sids(iseq)):str_end(new_sids(iseq)))

                     IF (str_end(new_lids(iseq)).GT.0) WRITE (cl_outlun,'(a)') 
     - ' Full name : ' //
     -               new_lids(iseq) (str_beg(new_lids(iseq)):
     -               MIN (cl_outwid-13,str_end(new_lids(iseq))))

                     IF (cl_lstflag) THEN
                        WRITE (cl_outlun,'(a)') 
     - ' RDP data : ' // cl_lststr(str_beg(cl_lststr):str_end(cl_lststr))
                     ELSE
                        WRITE (cl_outlun,'(a)') 
     - ' RDP data : ' // cl_binfnm(str_end_ch(cl_binfnm,'/')+1:str_end(cl_binfnm))
                     END IF

                     WRITE (scrstr1,'(i10)') old_inclmax
                     WRITE (scrstr2,'(i10)') cl_tuplen
                     WRITE (scrstr3,'(i10)') old_seqsmax - old_inclmax
                     WRITE (scrstr4,'(i10)') cl_partlen

                     WRITE (cl_outlun,'(a)')  
     - ' Comments  : A minimum of ' // scrstr4(str_beg(scrstr4):str_end(scrstr4)) //
     -               ' unique oligos required'  // LF // 
     - '           : A total of ' // scrstr3(str_beg(scrstr3):str_end(scrstr3)) //
     -               ' sequences were excluded'                      // LF //
     - '           : ' // scrstr1(str_beg(scrstr1):str_end(scrstr1)) //
     -             ' sequences were included in the search'          // LF // 
     - '           : The screening was based on '                    // 
     -              scrstr2(str_beg(scrstr2):str_end(scrstr2))       // 
     -             '-base oligomers'

                     CALL fdate(datstr)
                     WRITE (cl_outlun,'(a)')
     - ' Date      : ' // datstr

                     WRITE (cl_outlun,'(a)') 
     - ' CPU time  : ' // tim_str (i:j)

      WRITE (cl_outlun,'(/,1x,a,/)') eqsline (1:cl_outwid)
C     
C Output as a table that lists in decreasing order the percentage of shared 
C oligos (relative to the sequence with fewest oligos); the associated number
C oligos, short-ID's and full names (if present) are also listed. Note that 
C a short good match will rank higher than a long not so good match, for both
C a partial new sequence and one in the database.
C
            listlen = MIN ( old_inclmax, cl_listlen )

            CALL output_list ( cl_outlun, listlen, cl_outwid, old_seqsmax,
     -                         old_sids, old_lids, oli_adds, old_onums, 
     -                         new_onum, new_sids(iseq), new_lids(iseq),
     -                         "red" )

            WRITE (cl_outlun,'()')

         END IF

         CALL FREE ( oli_addsptr )

      END DO

      CALL FREE ( eqslineptr )

      RETURN
      END 

      SUBROUTINE output_simple ( lun, listlen, old_seqsmax, oli_adds,
     -                           old_onums, old_sids, new_sid, new_onum )

      IMPLICIT none

      INTEGER       lun
      INTEGER       old_seqsmax
      INTEGER       oli_adds (old_seqsmax)
      INTEGER       old_onums (old_seqsmax)
      CHARACTER*(*) old_sids (old_seqsmax)
      CHARACTER*(*) new_sid
      INTEGER       new_onum
      INTEGER       listlen
C
C Local variables
C
      REAL       sab_vals (old_seqsmax)
      POINTER   (sab_valsptr,sab_vals)

      INTEGER    seq_ndcs (old_seqsmax)
      POINTER   (seq_ndcsptr,seq_ndcs)

      INTEGER   iseq

      CHARACTER*1 HT /9/
C
C -----------------------------------------------------------------------------
C
C Convert numbers of oligos (shared between the new sequence and each 
C database sequence) to Sab values. These values are simply the oligo
C counts divided by the number of distinct oligos in the new sequence
C or the database sequence, whichever is lowest. That means that a good
C short match will rank higher than a long less good match, for both a 
C partial new sequence and one in the database.
C
      CALL get_memory (sab_valsptr,old_seqsmax*4,'sab_vals','output_simple')

      DO iseq = 1, old_seqsmax
         IF (old_onums(iseq).GE.new_onum) THEN
            sab_vals(iseq) = REAL (oli_adds(iseq)) / new_onum
         ELSE
            sab_vals(iseq) = REAL (oli_adds(iseq)) / old_onums(iseq)
         END IF
      END DO
C
C Sort the Sab values in decreasing order (seq_ndcs are pointers to 
C the unsorted indices),
C      
      CALL get_memory (seq_ndcsptr,old_seqsmax*4,'seq_ndcs','output_simple') 

      DO iseq = 1, old_seqsmax
         seq_ndcs(iseq) = iseq
      END DO

      CALL r4_shellsort (sab_vals,seq_ndcs,old_seqsmax)
C
C Write top of list as 3 colums: new short-ID, old short-ID, S_ab
C
      WRITE (lun,'($, a)') new_sid
      DO iseq = 1, listlen
         WRITE (lun,'($,a,f5.3)') 
     -         HT // old_sids(seq_ndcs(iseq)) // '-*-', sab_vals(iseq)
      END DO
      WRITE (lun, '()')

      RETURN
      End 

      SUBROUTINE output_histo ( chi_ndxmax, chi_tblend, chi_gain, chi_ndcs,
     -                          beg_col, end_col )
C
C clean up and comment
C
      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'

C      INTEGER cl_outlun
C      INTEGER cl_outwid
      INTEGER chi_ndxmax
      INTEGER chi_gain (chi_ndxmax)
      INTEGER chi_ndcs (chi_ndxmax)
      CHARACTER*(*) beg_col
      CHARACTER*(*) end_col
C
C Local variables
C
      CHARACTER*500 star_str, plus_str, num_str, axis_str, comm_str, wrap_str
      CHARACTER*10  scrstr1

      INTEGER breakpnt
      INTEGER i4_array_max
      INTEGER hst_max, hst_wid
      INTEGER ndx, chi_tblend
      INTEGER beg_ndx, end_ndx
      INTEGER i, j

      REAL    x, y
      INTEGER unit
      REAL    scale_fact, oli_per_ch
      INTEGER str_beg, str_end
C
C ------------------------------------------------------------------------------
C      
      hst_max = i4_array_max ( chi_gain, 1, chi_tblend, breakpnt )
C
C The following statement sets the maximum histogram bar to at least 
C eight percent of the sequence length - arbitrary. The purpose is to
C not stretch low to full width of the output, while keeping the units
C dynamic
C
      hst_max = MAX ( hst_max, chi_ndcs (chi_tblend) * 6 / 100 )

      hst_wid = cl_outwid - 8 
      scale_fact = REAL (hst_wid) / hst_max
C
C Find unit, the value of which the histogram axis units are multipla,
C      
      x = 1.0
      unit = 0
      DO WHILE (unit.EQ.0)
         y = 1.0
         DO WHILE (unit.EQ.0 .AND. y.LE.9.0) 
            IF ( REAL( hst_max / ( x * y ) ) .LE. 4.0 ) unit = INT ( x * y )
            y = y + 1.0
         END DO
         x = x * 10.0
      END DO
C
C Oligos per character in histogram bars,
C
      oli_per_ch = REAL (hst_max) / hst_wid
C
C Compose number-string and axis string with vertical bars at units,
C
      CALL str_init ( num_str, 1, cl_outwid, ' ' )

      i = 1
      j = unit
      x = oli_per_ch

      DO WHILE (i.LE.hst_wid)
         IF (INT(x).GE.j) THEN
            axis_str (i:i) = '|'
            WRITE (num_str(i-4:i),'(I5)') j
            j = j + unit
         ELSE
            axis_str (i:i) = '-'
         END IF
         x = x + oli_per_ch
         i = i + 1 
      END DO
C
C Init histogram bars
C 
      IF (cl_htmlflag) THEN
         CALL str_init ( star_str, 1, cl_outwid, '>' )
         CALL str_init ( plus_str, 1, cl_outwid, '-' )
      ELSE
         CALL str_init ( star_str, 1, cl_outwid, '*' )
         CALL str_init ( plus_str, 1, cl_outwid, '+' )
      END IF
C
C Trim output to first non-zero values from both ends,
C
      beg_ndx = 1
      DO WHILE (chi_gain(beg_ndx).EQ.0)
         beg_ndx = beg_ndx + 1 
      END DO

      end_ndx = chi_tblend
      DO WHILE (chi_gain(end_ndx).EQ.0) 
         end_ndx = end_ndx - 1 
      END DO
C
C Use pre-html
C
      IF (cl_htmlflag) WRITE (cl_outlun,'(a)') '<pre> <font size=-2>'
C
C Upper axis values and axis,
C
      WRITE (cl_outlun,'(8x,a)') num_str (1:hst_wid)  
      WRITE (cl_outlun,'(6x,a)') ',-' // axis_str (1:hst_wid) 
C
C Insert notification about missing leading bars, if any, 
C      
      IF (beg_ndx.GT.1) THEN
         WRITE (scrstr1,'(i10)') beg_ndx - 1
         comm_str = '     | (' // scrstr1(str_beg(scrstr1):str_end(scrstr1)) // 
     -              ' position(s) of value zero omitted)'
         CALL text_wrap ( comm_str(1:str_end(comm_str)), 1, cl_outwid, ' ', wrap_str )
         WRITE (cl_outlun,'(a)') wrap_str(1:str_end(wrap_str))
      END IF
C
C Plot histogram bars, 
C
      IF (cl_htmlflag) THEN

         DO ndx = beg_ndx, end_ndx
            IF (chi_gain(ndx).LT.0) THEN
               WRITE (cl_outlun,'(a,i6)') 'Error... ', chi_gain(ndx)
            ELSE IF (ndx.EQ.breakpnt) THEN
               WRITE (cl_outlun,'(1x,i4,a)') chi_ndcs (ndx), 
     -              ' | ' // star_str( 1 : INT (chi_gain(ndx) * scale_fact) )
            ELSE IF (ndx.LT.breakpnt) THEN
               WRITE (cl_outlun,'(1x,i4,a)') chi_ndcs (ndx), 
     -              ' | <font color=' // beg_col // '>' // 
     -              plus_str( 1 : INT (chi_gain(ndx) * scale_fact) ) //
     -              '</font>'
            ELSE IF (ndx.GT.breakpnt) THEN
               WRITE (cl_outlun,'(1x,i4,a)') chi_ndcs (ndx), 
     -              ' | <font color=' // end_col // '>' // 
     -              plus_str( 1 : INT (chi_gain(ndx) * scale_fact) ) //
     -              '</font>'
            END IF
         END DO

      ELSE 
         DO ndx = beg_ndx, end_ndx
            IF (chi_gain(ndx).LT.0) THEN
               WRITE (cl_outlun,'(a,i6)') 'Error... ', chi_gain(ndx)
            ELSE IF (ndx.EQ.breakpnt) THEN
               WRITE (cl_outlun,'(1x,i4,a)') chi_ndcs (ndx), 
     -              ' | ' // star_str( 1 : INT (chi_gain(ndx) * scale_fact) )
            ELSE 
               WRITE (cl_outlun,'(1x,i4,a)') chi_ndcs (ndx), 
     -              ' | ' // plus_str( 1 : INT (chi_gain(ndx) * scale_fact) )
            END IF
         END DO
      END IF         
C
C Insert notification about missing leading bars, if any, 
C      
      IF (end_ndx.LT.chi_tblend) THEN
         WRITE (scrstr1,'(i10)') chi_tblend - end_ndx
         comm_str = '     | (' // scrstr1(str_beg(scrstr1):str_end(scrstr1)) // 
     -              ' position(s) of value zero omitted)'
         CALL text_wrap ( comm_str(1:str_end(comm_str)), 1, cl_outwid, ' ', wrap_str )
         WRITE (cl_outlun,'(a)') wrap_str(1:str_end(wrap_str))
      END IF
C
C Lower axis and values,
C
      WRITE (cl_outlun,'(6x,a)') '`-' // axis_str (1:hst_wid) 
      WRITE (cl_outlun,'(8x,a)') num_str (1:hst_wid) 
C
C End of pre-html
C
      IF (cl_htmlflag) WRITE (cl_outlun,'(a)') '</font> </pre>'

      RETURN
      END

      SUBROUTINE make_tables ( oli_number, 
     -                         seqsmax, basmax, tblmax, 
     -                         ends, obegs, llst,
     -                         tblpos, tbl, tbegs, onums )

      IMPLICIT none
C
C Set up tbl, tbegs and onums. Tbl has runs of numbers which stand for the
C sequences where a given oligo occurs. The starting point of each run 
C is stored in tbegs, that uses the oligos as indices. Onums becomes the 
C number of oligos in each sequence.
C
C IMPROVE - DO ioli = 0,old_oligos  is probably dumb
C
      INTEGER   oli_number                ! in 
      INTEGER   seqsmax                   ! in
      INTEGER   basmax                    ! in
      INTEGER   tblmax                    ! in
      INTEGER   ends (seqsmax)            ! in
      INTEGER   obegs (0:oli_number-1)    ! in
      INTEGER   llst (basmax)             ! in
      INTEGER   tblpos                    ! out
      INTEGER*2 tbl (tblmax)              ! out
      INTEGER   tbegs (0:oli_number-1)    ! out
      INTEGER   onums (seqsmax)           ! out
C
C Local variables
C
      INTEGER iseq, jseq, ioli
      INTEGER llst_pos
C
C -------------------------------------------------------------------------------
C
      DO iseq = 1, seqsmax
         onums(iseq) = 0
      END DO

      tblpos = 0
C
C For each possible oligo, follow linked list,
C      
      DO ioli = 0, oli_number-1
         
         tblpos = tblpos + 1 
         tbegs(ioli) = tblpos
         
         iseq = 1
         jseq = 0

         llst_pos = obegs(ioli)

         DO WHILE ( llst_pos.GT.0 ) 
C
C Find the sequence number which llst_pos belongs to
C            
            DO WHILE ( ends(iseq).LT.llst_pos )
               iseq = iseq + 1 
            END DO
            
            IF ( iseq.NE.jseq ) THEN
               tbl(tblpos) = iseq
               jseq = iseq
               tblpos = tblpos + 1
               onums(iseq) = onums(iseq) + 1
            END IF

            llst_pos = llst(llst_pos)
            
         END DO
         
         tbl (tblpos) = 0 
         
      END DO

      RETURN
      END 

      SUBROUTINE add_oligos ( oli_number, 
     -               old_seqsmax, old_tblmax, old_tbegs, old_tbl, old_incl,
     -               new_basmax, new_beg, new_end, new_cods,
     -               new_onum, oli_adds )
C
C Improve this. 
C I deliberately dont use assumed size arrays anymore
C
      IMPLICIT none

      INTEGER   oli_number
      INTEGER   old_seqsmax
      INTEGER   old_tblmax
      INTEGER   old_tbegs ( 0 : oli_number - 1 )  
      INTEGER*2 old_tbl (old_tblmax)
      LOGICAL*1 old_incl ( old_seqsmax )
      INTEGER   new_basmax
      INTEGER   new_beg
      INTEGER   new_end
      INTEGER   new_cods ( new_basmax )
      INTEGER   new_onum
      INTEGER   oli_adds ( old_seqsmax )
C
C Local variables
C
      LOGICAL*1 oli_dupl ( 0 : oli_number - 1 )
      POINTER   (oli_duplptr,oli_dupl)
      
      INTEGER   ipos, ioli
      INTEGER   tblpos, tblseq

      LOGICAL*1 bool
C
C -----------------------------------------------------------------------------
C
      CALL get_memory ( oli_duplptr, oli_number, 'oli_dupl', 'add_oligos' )

      bool = .FALSE.
      CALL l1_array_init ( 0, oli_number-1, oli_dupl, 0, oli_number-1, bool )
      CALL i4_array_init ( 1, old_seqsmax, oli_adds, 1, old_seqsmax, 0 )

      new_onum = 0
      
      DO ipos = new_beg, new_end

         ioli = new_cods (ipos)
         tblpos = old_tbegs (ioli)
         tblseq = old_tbl(tblpos)

         IF ( .NOT.oli_dupl(ioli) .AND. tblseq.GT.0) THEN

            DO WHILE ( tblseq.GT.0 )
               IF (old_incl(tblseq)) oli_adds ( tblseq ) = oli_adds ( tblseq ) + 1
               tblpos = tblpos + 1 
               tblseq = old_tbl(tblpos)
            END DO

            new_onum = new_onum + 1 

         END IF

         oli_dupl (ioli) = .TRUE.

      END DO
      
      CALL FREE (oli_duplptr)

      RETURN
      END
      SUBROUTINE by_array_init (dimbeg,dimend,array,inibeg,iniend,value)

      IMPLICIT none

      INTEGER   dimbeg, dimend
      BYTE      array (dimbeg:dimend)
      INTEGER   inibeg, iniend
      BYTE      value
C
C Local variables
C
      INTEGER   i
C
C -------------------------------------------------------------------------
C
      DO i = inibeg, iniend
         array (i) = value
      END DO

      RETURN
      END

      LOGICAL FUNCTION ch_is_alpha (ch)
C
C True if ch is an english alphabetic character, false otherwise
C
      IMPLICIT none

      CHARACTER*1 ch
      BYTE        bytch
C
C ------------------------------------------------------------------------------
C
      bytch = ICHAR (ch)

      ch_is_alpha = bytch.GE.65 .AND. bytch.LE.90   .OR.
     -              bytch.GE.97 .AND. bytch.LE.122

      RETURN
      END
      LOGICAL FUNCTION ch_is_digit (ch)
C
C True if ch is a digit, false otherwise
C
      IMPLICIT none

      CHARACTER*1 ch
      BYTE        bytch
C
C ------------------------------------------------------------------------------
C
      bytch = ICHAR (ch)

      ch_is_digit = bytch.GE.48 .AND. bytch.LE.57

      RETURN
      END
      LOGICAL FUNCTION ch_is_lower (ch)
C
C True if ch is an lower case english alphabetic character, false 
C otherwise
C
      IMPLICIT none

      CHARACTER*1 ch
      BYTE        bytch
C
C ------------------------------------------------------------------------------
C
      bytch = ICHAR (ch)

      ch_is_lower = bytch.GE.97 .AND. bytch.LE.122

      RETURN
      END
      LOGICAL FUNCTION ch_is_upper (ch)
C
C True if ch is an upper case english alphabetic character, false 
C otherwise
C
      IMPLICIT none

      CHARACTER*1 ch
      BYTE        bytch
C
C ------------------------------------------------------------------------------
C
      bytch = ICHAR (ch)

      ch_is_upper = bytch.GE.65 .AND. bytch.LE.90

      RETURN
      END

      SUBROUTINE chimera_gain ( 
     -               oli_number, old_seqsmax, old_tblmax, 
     -               old_tbl, old_tbegs, old_incl,
     -               new_seqsmax, new_cods, new_beg, new_end,
     -               chi_ndxmax, chi_gain, chi_ndcs, chi_tblmax)
C
C This function moves a hypothetical break point along the sequence in 
C order to compare possible chimeric fragments with the database. For 
C each position, it keeps (in chi_gain) the following number:
C    (oligos shared between fragment 1 and most similar database seq)
C  + (oligos shared between fragment 2 and most similar database seq)
C  - (oligos shared between fragment 1+2 and most similar database seq)
C In case of a chimeric sequence, that number should increase towards
C the true breakpoint. 
C Chi_ndcs contains the sequence numbering, the program only takes 
C samples for each cl_steplen'th position.
C
      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'
      INCLUDE 'sim_params.inc'

      INTEGER   oli_number                ! number of different oligos
      INTEGER   old_seqsmax               ! # seqs in alignment
      INTEGER   old_tblmax                ! max table index
      INTEGER*2 old_tbl (old_tblmax)      ! lists sequence with given oligo
      INTEGER   old_tbegs (0:oli_number)  ! holds old_tbl entry points
      LOGICAL*1 old_incl (old_seqsmax) 
      INTEGER   new_seqsmax
      INTEGER   new_cods (new_seqsmax)    ! unique numbers assigned to oligos
      INTEGER   new_beg                   ! start of sequence
      INTEGER   new_end                   ! end of sequence
      INTEGER   chi_ndxmax                ! 
      INTEGER   chi_gain (chi_ndxmax)     !
      INTEGER   chi_ndcs (chi_ndxmax)     ! 
      INTEGER   chi_tblmax                ! maximum chi_gain value max
C
C Local variables
C
      INTEGER*4  tot_olis (old_seqsmax)
      POINTER   (tot_olisptr,tot_olis)

      INTEGER*4  chi_olis1 (old_seqsmax)
      POINTER   (chi_olis1ptr,chi_olis1)

      INTEGER*4  chi_olis2 (old_seqsmax)
      POINTER   (chi_olis2ptr,chi_olis2)

      INTEGER    ipos
      INTEGER    tblpos, tblseq
      INTEGER    topseq,topseq1,topseq2
      INTEGER    breakpnt
      INTEGER    max_oli, i4_array_max

C      INTEGER    chi_max1, chi_max2
C
C ------------------------------------------------------------------------------
C
C Load tot_olis with the number of shared oligos between the full length
C sequence and each sequence in the database,
C
      CALL get_memory ( tot_olisptr, old_seqsmax * 4, 'tot_olis', 'chimera_gain' )
      CALL i4_array_init ( 1, old_seqsmax, tot_olis, 1, old_seqsmax, 0 )

      DO ipos = new_beg, new_end - cl_tuplen + 1
         tblpos = old_tbegs ( new_cods(ipos) )
         tblseq = old_tbl ( tblpos )
         DO WHILE ( tblseq.GT.0 )
            IF (old_incl(tblseq)) tot_olis ( tblseq ) = tot_olis ( tblseq ) + 1
            tblpos = tblpos + 1
            tblseq = old_tbl ( tblpos )
         END DO
      END DO
C
C Find the maximum numner of shared oligos; topseq becomes the index
C of the most similar sequence
C
      max_oli = i4_array_max ( tot_olis, 1, old_seqsmax, topseq )
      CALL FREE ( tot_olisptr )
C
C Initialize chi_olis1 with the number of shared oligos within the first 
C cl_fraglen positions from the 5 end of the sequence, 
C
      CALL get_memory ( chi_olis1ptr, old_seqsmax * 4, 'chi_olis1', 'chimera_gain' )
      CALL i4_array_init ( 1, old_seqsmax, chi_olis1, 1, old_seqsmax, 0 )

      DO ipos = new_beg, new_beg + cl_fraglen - 1 
         tblpos = old_tbegs ( new_cods(ipos) )
         tblseq = old_tbl ( tblpos )
         DO WHILE ( tblseq.GT.0 )
            IF (old_incl(tblseq)) chi_olis1 ( tblseq ) = chi_olis1 ( tblseq ) + 1
            tblpos = tblpos + 1
            tblseq = old_tbl ( tblpos )
         END DO
      END DO
C
C Initialize chi_olis2 with the number of shared oligos for the remaining 
C sequence posisitions,
C
      CALL get_memory ( chi_olis2ptr, old_seqsmax * 4, 'chi_olis2', 'chimera_gain' )
      CALL i4_array_init ( 1, old_seqsmax, chi_olis2, 1, old_seqsmax, 0 )

      DO ipos = new_beg + cl_fraglen, new_end - cl_tuplen + 1
         tblpos = old_tbegs ( new_cods(ipos) )
         tblseq = old_tbl ( tblpos )
         DO WHILE ( tblseq.GT.0 )
            IF (old_incl(tblseq)) chi_olis2 ( tblseq ) = chi_olis2 ( tblseq ) + 1
            tblpos = tblpos + 1
            tblseq = old_tbl ( tblpos )
         END DO
      END DO
C
C Initialize the difference between the sum of oligos shared in the fragments
C combined minus the shared oligos for the full length sequence
C
c$$$      chi_tblmax = 1
c$$$
c$$$      chi_max1 = i4_array_max ( chi_olis1, 1, old_seqsmax, topseq1 )
c$$$      chi_max2 = i4_array_max ( chi_olis2, 1, old_seqsmax, topseq2 )
c$$$
c$$$
c$$$      write (6,'(a,
c$$$      chi_gain (chi_tblmax) = chi_max1 - chi_olis1 ( topseq2 ) 
c$$$     -                      + chi_max2 - chi_olis2 ( topseq1 )
c$$$
c$$$      chi_ndcs (chi_tblmax) = cl_fraglen

      chi_tblmax = 1
      chi_gain (chi_tblmax) = i4_array_max ( chi_olis1, 1, old_seqsmax, topseq1 )
     -                      + i4_array_max ( chi_olis2, 1, old_seqsmax, topseq2 )
     -                      - max_oli
      chi_ndcs (chi_tblmax) = cl_fraglen
C
C Move breakpoint so fragment 1 increases in length while fragment 2 shortens.
C Add and subtract oligo counts, and store in chi_gain (corresponding indices
C in chi_ndcs),
C
      DO breakpnt = new_beg + cl_fraglen - 1 + cl_steplen, 
     -              new_end - cl_tuplen  + 1 - cl_fraglen + 1,
     -              cl_steplen

         DO ipos = breakpnt - cl_steplen + 1, breakpnt
            tblpos = old_tbegs ( new_cods(ipos) )
            tblseq = old_tbl( tblpos )
            DO WHILE ( tblseq.GT.0 )
               IF (old_incl(tblseq)) THEN
                  chi_olis1 ( tblseq ) = chi_olis1 ( tblseq ) + 1
                  chi_olis2 ( tblseq ) = chi_olis2 ( tblseq ) - 1
               END IF
               tblpos = tblpos + 1
               tblseq = old_tbl (tblpos)
            END DO
         END DO

         chi_tblmax = chi_tblmax + 1

c$$$         chi_max1 = i4_array_max ( chi_olis1, 1, old_seqsmax, topseq1 )
c$$$         chi_max2 = i4_array_max ( chi_olis2, 1, old_seqsmax, topseq2 )
c$$$         
c$$$         chi_gain (chi_tblmax) = chi_max1 - chi_olis1 ( topseq2 ) 
c$$$     -                         + chi_max2 - chi_olis2 ( topseq1 )

         chi_gain (chi_tblmax) = i4_array_max ( chi_olis1, 1, old_seqsmax, topseq1 )
     -                         + i4_array_max ( chi_olis2, 1, old_seqsmax, topseq2 )
     -                         - max_oli

         chi_ndcs (chi_tblmax) = breakpnt - new_beg + 1

      END DO

      CALL FREE ( chi_olis1ptr )
      CALL FREE ( chi_olis2ptr )

      RETURN
      END


      SUBROUTINE dtime_write (CPUsec,tim_str,beg,end)
C
C Returns a line of timer information in tim_str(beg:end)
C
      IMPLICIT none

      INTEGER days,hours,minuts,isec

      REAL    frac,CPUsec

      CHARACTER*(*) tim_str

      INTEGER str_beg,str_end
      INTEGER beg,end
C
C------------------------------------------------------------------------
C     
      isec = INT (CPUsec)
      frac = CPUsec-isec

      tim_str = ' '

      IF (isec.GE.86400) THEN 
         days = isec/86400
         isec = MOD (isec,86400)
         IF (days.EQ.1) THEN
            WRITE (tim_str(str_end(tim_str)+1:),'(1x,i2,a)') days,' day,'
         ELSE
            WRITE (tim_str(str_end(tim_str)+1:),'(1x,i2,a)') days,' days,'
         END IF
      END IF

      IF (isec.GE.3600) THEN 
         hours = isec/3600
         isec = MOD (isec,3600)
         IF (hours.EQ.1) THEN
            WRITE (tim_str(str_end(tim_str)+1:),'(1x,i2,a)') hours,' hour,'
         ELSE
            WRITE (tim_str(str_end(tim_str)+1:),'(1x,i2,a)') hours,' hours,'
         END IF
      END IF

      IF (isec.GE.60) THEN 
         minuts = isec/60
         isec = MOD (isec,60)
         IF (minuts.EQ.1) THEN
            WRITE (tim_str(str_end(tim_str)+1:),'(1x,i2,a)') minuts,' minute,'
         ELSE
            WRITE (tim_str(str_end(tim_str)+1:),'(1x,i2,a)') minuts,' minutes,'
         END IF
      END IF

      WRITE (tim_str(str_end(tim_str)+1:),'(1x,F5.2,a8)') isec+frac,' seconds'

      beg = str_beg(tim_str)
      end = str_end(tim_str) 

      RETURN
      END

      SUBROUTINE feel_gbk (inlun,isbase,minseqlen,iseqs,ichrs)
C
C It reads file of one or more Genbank formatted entries; it returns 
C number of sequences (iseqs) and number of characters of the isbase
C kind. It will read ients number of entries ahead from current file 
C pointer position, and it will skip sequences shorter than minseqlen 
C characters.
C
C Program is stopped if terminator lines ('//') dont always follow 
C LOCUS lines and vice versa
C 
      IMPLICIT none

      INTEGER       inlun
      LOGICAL*1     isbase (0:*)
      INTEGER       minseqlen
      INTEGER       iseqs     
      INTEGER       ichrs
C      INTEGER       iwats
C
C Local variables
C
      INCLUDE 'sim_cmdlin.inc'

      CHARACTER*80  inline
      CHARACTER*10  locstr, scrstr
      CHARACTER*8   fun_ID  / 'FEEL_GBK' /
      CHARACTER*1   LF      / 10 / 

      INTEGER     str_beg, str_end
      INTEGER     iloc, itrm
      INTEGER     linend, ipos
      INTEGER     j
C      BYTE        bytech
C   
C -------------------------------------------------------------------------
C
      iseqs = 0
      ichrs = 0

      iloc = 0
      itrm = 0

      DO WHILE (.TRUE.)

         READ (inlun,'(q,a)', END = 98, ERR = 99) linend,inline

         IF (inline(1:5).EQ.'LOCUS') THEN 

            iloc = iloc + 1
            locstr = inline (13:22)

         ELSE IF (inline(1:6).EQ.'ORIGIN') THEN

            READ (inlun,'(q,a)', ERR = 99) linend, inline
            
            j = 0
            
            DO WHILE (inline(1:2).EQ.'  ')

               DO ipos = 11, linend
C                  bytech = ICHAR ( inline(ipos:ipos) )
C                  IF ( bytech.NE.32 ) THEN
                     IF ( isbase(ICHAR(inline(ipos:ipos))) ) j = j + 1
C                  END IF
               END DO

               READ (inlun,'(q,a)', ERR = 99) linend, inline
            END DO

            IF (inline(1:2).EQ.'//') THEN

               itrm = itrm + 1

               IF (iloc.GT.itrm) THEN
                  CALL write_message (cl_errlun, cl_errfnm, 
     -                fun_ID // ': ' //
     -                'Error: Entry previous to ' // locstr //
     -                ' is not properly terminated')
                  STOP
               ELSE IF (iloc.LT.itrm) THEN
                  IF (iloc.EQ.0) THEN
                     CALL write_message (cl_errlun, cl_errfnm, fun_ID // ': ' //
     -                    'Error: First entry is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  ELSE
                     CALL write_message (cl_errlun, cl_errfnm, fun_ID // ': ' //
     -                    'Error: Entry following ' // locstr //
     -                    ' is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  END IF
                  STOP
               END IF

            END IF

            IF ( j.GE.minseqlen ) THEN

               iseqs = iseqs + 1
               ichrs = ichrs + j

            ELSE
               WRITE (scrstr,*) minseqlen
               CALL write_message (cl_errlun, cl_errfnm, fun_ID // ': ' //
     -              'Warning: Entry ' // locstr(str_beg(locstr):str_end(locstr)) // 
     -              ' shorter than ' // scrstr(str_beg(scrstr):str_end(scrstr)) // 
     -              ' residues' // LF // '(entry ignored)' )
            END IF

         END IF
         
      END DO
      
 98   RETURN

 99   CALL write_message (cl_errlun, cl_errfnm, fun_ID // ': ' // 
     -    'Error: Unexpected end of file reached' // LF // 
     -    'Program stop')
      STOP

      END 

      SUBROUTINE feel_gbk_old ( inlun, tuplen, parlen, iseq, ibas )
C
C Returns the number of sequences and bases in a file of one or more Genbank 
C entries, so for example to know how much memory to allocate. Sequences 
C with less than 'parlen' valid Watson-Crick base symbols are ignored. It 
C stops program with informative message if certain format errors: No final
C '//', an unequal number of ORIGIN and LOCUS lines, no ORIGIN line after 
C previous LOCUS line, no LOCUS line after previous ORIGIN line, and no '//' 
C before next LOCUS line.
C 
      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'

      INTEGER       inlun
      INTEGER       tuplen
      INTEGER       parlen
      INTEGER       iseq
      INTEGER       ibas
C
C Local variables 
C
      CHARACTER*80 inline

      INTEGER       iloc, iori
      INTEGER       linend
      INTEGER       i,j

      BYTE          byte_ch

      LOGICAL       locflg

      BYTE WC_base(0:127)
      DATA WC_base  
C               A  B  C  D  E  F  G  H  I  J  K  L  M -
     -  / 65*0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
C               N  O  P  Q  R  S  T  U  V  W  X  Y  Z -
     -          0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,  6*0,
C               a  b  c  d  e  f  g  h  i  j  k  l  m -
     -          1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
C               n  o  p  q  r  s  t  u  v  w  x  y  z -
     -          0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,  5*0 /
C
C -------------------------------------------------------------------------
C    
      iseq = 0  ! sequences   count
      ibas = 0  ! bases         - 
      iloc = 0  ! LOCUS lines   -
      iori = 0  ! ORIGIN lines  - 

      locflg = .FALSE.  ! true after LOCUS line, false after sequence read

      DO WHILE (.TRUE.) 

         READ (inlun,'(q,a)',END=98,ERR=99) linend,inline

         IF (inline(1:5).EQ.'LOCUS') THEN 

            iloc = iloc + 1
            locflg = .TRUE.

         ELSE IF (inline(1:6).EQ.'ORIGIN') THEN

            iori = iori + 1 

            IF (locflg) THEN

               READ (inlun,'(q,a)',END=98,ERR=99) linend,inline

               j = ibas

               DO WHILE (inline(1:2).NE.'//')
                  IF (inline(1:5).EQ.'LOCUS') THEN
                     CALL write_message (cl_errlun,cl_errfnm,
     -                     'Error (feel_gbk): LOCUS ' // inline(13:22) // 
     -                     ' not terminated by ''//''')
                     STOP
                  ELSE
                     DO i = 11,linend
                        READ (inline(i:i),'(a1)') byte_ch   ! not needed on SUN...
                        IF (WC_base(byte_ch)) j = j + 1 
                     END DO
                     READ (inlun,'(q,a)',ERR=99) linend,inline
                  END IF
               END DO

               IF ( j .GE. ibas+parlen ) THEN   
                  iseq = iseq + 1 
                  ibas = j
               END IF

               locflg = .FALSE.

           END IF

         END IF

      END DO
      
 98   IF (iloc.NE.iori) THEN
         CALL write_message (cl_errlun,cl_errfnm, 
     -          'Error (feel_gbk): LOCUS line count differs from ORIGIN count')
         WRITE (cl_errlun,'(2(a,i5))') '                 ' // 
     -         ' LOCUS = ',iloc,'                 ORIGIN = ',iori
         STOP
      END IF

      RETURN

 99   CALL write_message (cl_errlun,cl_errfnm,'Error (feel_gbk): Unexpected EOF')
      STOP

      END 


      SUBROUTINE feel_namlst ( lstlun, fldnum, fldsep, maxndx )

      IMPLICIT none

      INTEGER       lstlun
      INTEGER       fldnum
      CHARACTER*1   fldsep
      INTEGER       maxndx
C
C Local variables
C
      CHARACTER*100 line
      INTEGER       begpos, endpos, maxpos
      INTEGER       ipos, ialf
      LOGICAL       ch_is_alpha
C
C ------------------------------------------------------------------------------
C
C Finds how many lines in which field number 'fldnum' (field separator 
C 'fldsep') have at least one alphabetic character. That number is 
C returned in 'maxndx'.
C
      maxndx = 0

      DO WHILE (.TRUE.)

         READ ( lstlun, '(q,a)', END=99 ) maxpos, line

         CALL str_field ( line, 1, maxpos, fldnum, fldsep, begpos, endpos )

         ialf = 0
         DO ipos = begpos, endpos
            IF (ch_is_alpha(line(ipos:ipos))) ialf = ialf + 1 
         END DO

         IF ( ialf.GT.0 .AND. begpos.GT.0 .AND. endpos.GT.0 ) THEN
            maxndx = maxndx + 1
         END IF

      END DO

 99   RETURN
      END

      SUBROUTINE free_memory (memptr)

      IMPLICIT none

      INTEGER memptr
C
C Return memory to manager
C     
C -------------------------------------------------------------------------------
C
      CALL FREE (memptr) 

      RETURN
      END

      SUBROUTINE get_gbk ( inlun, seqdim, isbase, parlen, basdim,
     -                     seqarr, begsarr, endsarr, sidarr, lidarr )
C
C Extracts short-ID's and sequences from a GenBank formatted alignment. 
C Inlun is the logical unit for file read .. bla bla
C
      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'

      INTEGER       inlun
      INTEGER       seqdim
      LOGICAL*1     isbase(0:*)
      INTEGER       parlen
      INTEGER       basdim
      BYTE          seqarr(basdim)
      INTEGER       begsarr(seqdim)
      INTEGER       endsarr(seqdim)
      CHARACTER*(*) sidarr(seqdim)
      CHARACTER*(*) lidarr(seqdim)
C
C Local variables
C
      CHARACTER*80  inline
      CHARACTER*80  sidline,lidline
      INTEGER       iseq,ibas,iloc,iori
      INTEGER       linend
      INTEGER       i,j

      BYTE          byte_ch

      LOGICAL       locflg,dfnflg,orgflg
C
C -------------------------------------------------------------------------
C 
      iseq = 0  ! sequences   count
      ibas = 0  ! bases         - 
      iloc = 0  ! LOCUS lines   -
      iori = 0  ! ORIGIN lines  - 

      locflg = .FALSE.  ! true after locus, false after sequence read
      dfnflg = .FALSE.  ! true after definition, false after sequence read
      orgflg = .FALSE.  ! true after organism line, false after sequence read

      DO WHILE (.TRUE.) 

         READ (inlun,'(q,a)',END=98,ERR=99) linend,inline

         IF (inline(1:5).EQ.'LOCUS') THEN 

            locflg = .TRUE.
            iloc = iloc + 1
            sidline = inline(13:22)

         ELSE IF (inline(1:10).EQ.'DEFINITION') THEN 

            dfnflg = .TRUE.
            lidline = inline(13:linend)

         ELSE IF (inline(3:10).EQ.'ORGANISM' .AND. .NOT.dfnflg) THEN 

            orgflg = .TRUE.
            lidline = inline(13:linend)

         ELSE IF (inline(1:6).EQ.'ORIGIN') THEN

            iori = iori + 1 

            IF (locflg) THEN

               READ (inlun,'(q,a)',END=98,ERR=99) linend,inline

               j = ibas

               DO WHILE (inline(1:2).NE.'//')
                  IF (inline(1:5).EQ.'LOCUS') THEN
                     CALL write_message (cl_errlun,cl_errfnm,
     -                     'Error (get_gbk): LOCUS ' // inline(13:22) // 
     -                     ' not terminated by ''//''')
                     STOP
                  ELSE
                     DO i = 11,linend
                        byte_ch = ICHAR (inline(i:i))
                        IF (isbase(byte_ch)) THEN
                           j = j + 1 
                           seqarr(j) = byte_ch
                        END IF
                     END DO
                     READ (inlun,'(q,a)',ERR=99) linend,inline
                  END IF
               END DO
            
               IF (j.GE.ibas+parlen) THEN
                  iseq = iseq + 1 
                  begsarr(iseq) = ibas + 1 
                  endsarr(iseq) = j
                  ibas = j
                  sidarr(iseq) = sidline

                  IF (dfnflg.OR.orgflg) THEN
                     lidarr(iseq) = lidline
                  ELSE 
                     lidarr(iseq) = ' '
                  END IF

               END IF

               locflg = .FALSE.
               dfnflg = .FALSE.
               orgflg = .FALSE.

            END IF

         END IF

      END DO

 98   IF (iloc.NE.iori) THEN
         CALL write_message (cl_errlun,cl_errfnm, 
     -          'Error (get_gbk): LOCUS line count differs from ORIGIN count')
         WRITE (cl_errlun,'(2(a,i5))') '                 ' // 
     -         ' LOCUS = ',iloc,'                 ORIGIN = ',iori
         STOP
      END IF

      RETURN

 99   CALL write_message (cl_errlun,cl_errfnm,'Error (get_gbk): Unexpected EOF')
      STOP

      END 

      SUBROUTINE get_memory (memptr,bytes,arrstr,substr)
C
C Allocates 'bytes' bytes of memory, starting at the memory adress memptr 
C     
      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'

      INTEGER bytes
      INTEGER memptr, MALLOC

      CHARACTER*(*) arrstr
      CHARACTER*(*) substr
C
C -------------------------------------------------------------------------------
C
      memptr = MALLOC (bytes)

      IF (memptr.EQ.0) THEN
         CALL write_message ( cl_errlun,cl_errfnm,
     -      'Error: Not enough memory for array ' // arrstr //
     -      ' in procedure ' // substr )
         STOP
      END IF

      RETURN
      END
      SUBROUTINE get_namlst (lstlun, maxndx, sidarr, begndx, endndx, fldnum, fldsep)

      IMPLICIT none

      INTEGER       lstlun
      INTEGER       maxndx
      CHARACTER*(*) sidarr (maxndx)
      INTEGER       begndx
      INTEGER       endndx
      INTEGER       fldnum
      CHARACTER*1   fldsep
C
C Local variables
C
      CHARACTER*200 line
      INTEGER       begpos, endpos, maxpos
      INTEGER       ipos, ialf
      LOGICAL       ch_is_alpha
C
C ------------------------------------------------------------------------------
C
C Read field number 'fldnum' (field separator 'fldsep') into sidarr, starting
C at index 'begndx' and ending at index 'endndx'. Lines are skipped if the 
C field contains no alphabetic characters, or if maxndx is exceeded. Close to
C awk '{print $1}' < file 
C
      endndx = begndx - 1 

      DO WHILE (.TRUE.)

         READ ( lstlun, '(q,a)', END=99 ) maxpos, line

         CALL str_field (line, 1, maxpos, fldnum, fldsep, begpos, endpos)

         ialf = 0
         DO ipos = begpos, endpos
            IF (ch_is_alpha(line(ipos:ipos))) ialf = ialf + 1 
         END DO

         IF ( ialf.GT.0 .AND. begpos.GT.0 .AND. endpos.GT.0 ) THEN
            endndx = endndx + 1
            sidarr(endndx) = line (begpos:endpos)
         END IF

      END DO

 99   RETURN
      END
      SUBROUTINE i2_array_init (dimbeg,dimend,array,inibeg,iniend,value)

      IMPLICIT none

      INTEGER   dimbeg, dimend
      INTEGER*2 array (dimbeg:dimend)
      INTEGER   inibeg, iniend
      INTEGER*2 value
C
C Local variables
C
      INTEGER   i
C
C -------------------------------------------------------------------------
C
      DO i = inibeg, iniend
         array (i) = value
      END DO

      RETURN
      END


      INTEGER FUNCTION i2_array_MAX (i2_array,beg,end,maxndx)
C
C Finds maximum value among INTEGER*2 array elements between beg and 
C end; the index with highest value is returned in maxndx
C
      IMPLICIT none

      INTEGER*2 i2_array(*)
      INTEGER   beg,end,maxndx,i
C
C -------------------------------------------------------------------------
C
      i2_array_MAX = i2_array(beg)
      maxndx = beg

      DO i = beg+1,end
         IF (i2_array(i).GT.i2_array_MAX) THEN
            i2_array_MAX = i2_array(i)
            maxndx = i 
         END IF
      END DO

      RETURN
      END 

      INTEGER FUNCTION i2_array_MIN (i2_array,beg,end,minndx)
C
C Finds minimum value among INTEGER*2 array elements between beg and 
C end; the index with lowest value is returned in minndx
C
      IMPLICIT none

      INTEGER*2 i2_array(*)
      INTEGER   beg,end
      INTEGER   minndx,i
C
C -------------------------------------------------------------------------
C
      i2_array_MIN = i2_array(beg)
      minndx = beg

      DO i = beg+1,end
         IF (i2_array(i).LT.i2_array_MIN) THEN
            i2_array_MIN = i2_array(i)
            minndx = i 
         END IF
      END DO

      RETURN
      END 

      SUBROUTINE i2_bin_io (iostr,lun,lodim,hidim,array)
C
C One dimensional INTEGER*2 binary READ or WRITE, determined by iostr
C 
      IMPLICIT none

      CHARACTER*(*) iostr
      INTEGER   lun
      INTEGER   lodim
      INTEGER   hidim
      INTEGER*2 array (lodim:hidim)
C
C -----------------------------------------------------------------------------
C
      IF (iostr.EQ.'WRITE') THEN

         WRITE (lun) array

      ELSE IF (iostr.EQ.'READ') THEN

         READ (lun) array

      END IF

      RETURN
      END
      SUBROUTINE i4_array_init (dimbeg,dimend,array,inibeg,iniend,value)

      IMPLICIT none

      INTEGER   dimbeg, dimend
      INTEGER*4 array (dimbeg:dimend)
      INTEGER   inibeg, iniend
      INTEGER*4 value
C
C Local variables
C
      INTEGER   i
C
C -------------------------------------------------------------------------
C
      DO i = inibeg, iniend
         array (i) = value
      END DO

      RETURN
      END


      INTEGER FUNCTION i4_array_MAX (i4_array,beg,end,maxndx)
C
C Finds maximum value among INTEGER*4 array elements between beg and 
C end; the index with highest value is returned in maxndx
C
      IMPLICIT none

      INTEGER*4 i4_array(*)
      INTEGER   beg,end,maxndx,i
C
C -------------------------------------------------------------------------
C
      i4_array_MAX = i4_array(beg)
      maxndx = beg

      DO i = beg+1,end
         IF (i4_array(i).GT.i4_array_MAX) THEN
            i4_array_MAX = i4_array(i)
            maxndx = i 
         END IF
      END DO

      RETURN
      END 

      INTEGER FUNCTION i4_array_MIN (i4_array,beg,end,minndx)
C
C Finds minimum value among INTEGER*4 array elements between beg and 
C end; the index with lowest value is returned in minndx
C
      IMPLICIT none

      INTEGER*4 i4_array(*)
      INTEGER   beg,end,minndx,i
C
C -------------------------------------------------------------------------
C
      i4_array_MIN = i4_array(beg)
      minndx = beg

      DO i = beg+1,end
         IF (i4_array(i).LT.i4_array_MIN) THEN
            i4_array_MIN = i4_array(i)
            minndx = i 
         END IF
      END DO

      RETURN
      END 


      SUBROUTINE i4_bin_io (iostr,lun,lodim,hidim,array)
C
C One dimensional INTEGER*4 binary READ or WRITE, determined by iostr
C 
      IMPLICIT none

      CHARACTER*(*) iostr
      INTEGER   lun
      INTEGER   lodim
      INTEGER   hidim
      INTEGER*4 array (lodim:hidim)
C
C -----------------------------------------------------------------------------
C
      IF (iostr.EQ.'WRITE') THEN

         WRITE (lun) array

      ELSE IF (iostr.EQ.'READ') THEN

         READ (lun) array

      END IF

      RETURN
      END
      SUBROUTINE i4_shellsort (a, b, n)
C
C Explain
C      
      IMPLICIT none

      INTEGER a(*),v
      INTEGER b(*),n
      INTEGER i,j,h,bv
C
C ------------------------------------------------------------------------------
C
      h = 1
      DO WHILE ( h.LE.n/9 ) 
         h = 3 * h + 1 
      END DO

      DO WHILE ( h.GT.0 ) 
         i = h + 1 
         DO WHILE ( i.LE.n )
            v = a(i)
            bv = b(i)
            j = i
            DO WHILE ( j.GT.h .AND. a(j-h).LT.v ) 
               a(j) = a(j-h)
               b(j) = b(j-h)
               j = j - h 
            END DO
            a(j) = v
            b(j) = bv
            i = i + 1 
         END DO
         h = h/3
      END DO

      RETURN
      END 
      SUBROUTINE l1_array_init (dimbeg,dimend,array,inibeg,iniend,value)

      IMPLICIT none

      INTEGER   dimbeg, dimend
      LOGICAL*1 array (dimbeg:dimend)
      INTEGER   inibeg, iniend
      LOGICAL*1 value
C
C Local variables
C
      INTEGER   i
C
C -------------------------------------------------------------------------
C
      DO i = inibeg, iniend
         array (i) = value
      END DO

      RETURN
      END

      SUBROUTINE make_iol_seq ( alp_siz, wrd_len, bas_dim, 
     -                          ich_seq, bas_beg, bas_end, iol_seq )

      IMPLICIT none
C
      INTEGER alp_siz              ! in 
      INTEGER wrd_len              ! in
      INTEGER bas_dim              ! in
      BYTE    ich_seq (bas_dim)    ! in
      INTEGER bas_beg              ! in
      INTEGER bas_end              ! in
      INTEGER iol_seq (bas_dim)    ! out
C
C Local variables
C
      INTEGER oli_int, bas_pos, scr_pos
C
C -------------------------------------------------------------------------------
C
      DO bas_pos = bas_beg, bas_end - wrd_len + 1

         oli_int = 0

         DO scr_pos = bas_pos, bas_pos + wrd_len - 1
            oli_int = alp_siz * oli_int + ich_seq (scr_pos)
         END DO

         iol_seq (bas_pos) = oli_int

      END DO

      RETURN
      END
      SUBROUTINE make_lnk_lst (alp_siz,wrd_len,oli_num,seq_dim,bas_dim, 
     -                         seq_min,seq_max,ich_sqs,seq_bgs,seq_nds,
     -                         lnk_bgs,lnk_lst)
      IMPLICIT none
C
C Creates linked list 
C
C lnk_lst: Each element with index i points to next occurrence of the 
C          oligo that starts at i. A zero means last occurrence.
C lnk_bgs: A lookup table with the beginning of each chain, for each 
C          oligo. Again, a zero means it doesnt occur. 
C  bucket: Scratch only, helps create the above.
C
      INTEGER alp_siz
      INTEGER wrd_len
      INTEGER oli_num
      INTEGER seq_dim
      INTEGER bas_dim
      INTEGER seq_min
      INTEGER seq_max
      BYTE    ich_sqs (bas_dim)
      INTEGER seq_bgs (seq_dim)
      INTEGER seq_nds (seq_dim)
      INTEGER lnk_bgs (0:oli_num-1)
      INTEGER lnk_lst (bas_dim)
C     
C Local variables
C
      INTEGER  bucket (0:oli_num-1)    
      POINTER (bucketptr,bucket)
      
      INTEGER iseq,ipos,ioli
      INTEGER bas_pos
C
C -------------------------------------------------------------------------------
C
      CALL get_memory (bucketptr, oli_num * 4, 'bucket', 'make_lnk_lst')

      DO ipos = seq_bgs(seq_min), seq_nds(seq_max)
         lnk_lst(ipos) = 0
      END DO

      DO ioli = 0, oli_num - 1
         bucket(ioli) = 0
         lnk_bgs(ioli) = 0
      END DO

      DO iseq = seq_min,seq_max
         
         DO bas_pos = seq_bgs (iseq), seq_nds (iseq) - wrd_len + 1
            
            ioli = 0
            DO ipos = bas_pos,bas_pos + wrd_len - 1
               ioli = alp_siz * ioli + ich_sqs (ipos)
            END DO

            IF (bucket(ioli).EQ.0) THEN
               lnk_bgs (ioli) = bas_pos
            ELSE
               lnk_lst ( bucket(ioli) ) = bas_pos 
            END IF
            
            bucket (ioli) = bas_pos
            
         END DO
         
      END DO

      CALL FREE (bucketptr)

      RETURN
      END 

      SUBROUTINE output_list (outlun,listlen,outwid,
     -                        old_seqsmax,old_sids,old_lids,oli_adds,old_onums,
     -                        new_onum,new_sid,new_lid,hd_color)
C
C Writes simple ranked list like in this format,
C
C           120  Cln7,III    SA1,PRC,#7,read from M13F+M13R,Gel#4#6#7,B.Key.    
C  ------------------------------------------------------------------------------
C   S_ab   Olis  Short-ID    Full name
C  ------------------------------------------------------------------------------
C   0.950   530  Dsm.sp.2    Desulfuromonas sp.; 16S ribosomal RNA.             
C   0.633  1409  Dsm.acetox  Desulfuromonas acetoxidans str. 11070 or 5071.     
C   0.600  1426  Dsv.spPT2   Desulfovibrio sp. str. PT-2; 16S ribosomal RNA.    
C   0.592  1237  env.MP1991  Iron sulfide containing magnetotactic bacterium.; r
C   0.592  1466  Dsv.desulf  Desulfovibrio desulfuricans.                       
C   0.567  1187  Dsv.gigas   Desulfovibrio gigas.                               
C   0.567  1221  Dsv.vulgar  Desulfovibrio vulgaris str. Hildenborough DSM 644. 
C   0.558  1278  Dcc.multiv  Desulfococcus multivorans.                         
C   0.558   771  env.MP1990  Iron sulfide containing magnetotactic bacterium.; r
C   0.550   517  Dsv.sp      Desulfovibrio sp.; 16S ribosomal RNA.              
C  ------------------------------------------------------------------------------
C
      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'

      INTEGER        outlun
      INTEGER        listlen
      INTEGER        outwid
      INTEGER        old_seqsmax            ! number of sequences in alignment
      CHARACTER*(*)  old_sids(old_seqsmax)  ! database short-ID's
      CHARACTER*(*)  old_lids(old_seqsmax)  ! database full names
      INTEGER        oli_adds(old_seqsmax)  ! shared oligos
      INTEGER        old_onums(old_seqsmax) ! number of distinct database oligos
      INTEGER        new_onum               ! number of distinct oligos in new seq
      CHARACTER*(*)  new_sid                ! short-ID of new sequence
      CHARACTER*(*)  new_lid                ! full name of new sequence
      CHARACTER*(*)  hd_color               ! table header color
C
C Local variables
C 
      REAL       sab_vals (old_seqsmax)
      POINTER   (sab_valsptr,sab_vals)

      INTEGER    seq_ndcs (old_seqsmax)
      POINTER   (seq_ndcsptr,seq_ndcs)

      CHARACTER*999999 dshlin
      POINTER (dshlinptr,dshlin)

      CHARACTER*999999 outstr
      POINTER (outstrptr,outstr)

      INTEGER str_beg,str_end
      INTEGER iout,iseq
      INTEGER listed_seqs

      CHARACTER*1 LF / 10 /
C
C --------------------------------------------------------------------------
C
      CALL get_memory (outstrptr, outwid, 'outstr', 'output_list') 
      CALL get_memory (dshlinptr, outwid, 'dshlin', 'output_list') 

      CALL str_init (outstr, 1, outwid, ' ')
      CALL str_init (dshlin, 1, outwid, '-')
C
C Convert numbers of oligos (shared between the new sequence and each 
C database sequence) to Sab values. These values are simply the oligo
C counts divided by the number of distinct oligos in the new sequence
C or the database sequence, whichever is lowest. That means that a good
C short match will rank higher than a long less good match, for both a 
C partial new sequence and one in the database.
C
      CALL get_memory (sab_valsptr,old_seqsmax*4,'sab_vals','output_list')

      DO iseq = 1, old_seqsmax
         IF (old_onums(iseq).GE.new_onum) THEN
            sab_vals(iseq) = REAL (oli_adds(iseq)) / new_onum
         ELSE
            sab_vals(iseq) = REAL (oli_adds(iseq)) / old_onums(iseq)
         END IF
      END DO
C
C Sort the Sab values in decreasing order (seq_ndcs are pointers to 
C the unsorted indices),
C      
      CALL get_memory (seq_ndcsptr,old_seqsmax*4,'seq_ndcs','output_list') 

      DO iseq = 1, old_seqsmax
         seq_ndcs(iseq) = iseq
      END DO

      CALL r4_shellsort (sab_vals,seq_ndcs,old_seqsmax)
C
C If program is called from mail server, it does not always get a short-ID,
C as it is not a mandatory option. So only write an ID in header if new_sid
C is not RDPMAILSRV. 
C 
      IF (new_sid.EQ.'RDPMAILSRV' .OR. str_end(new_sid).EQ.0) new_sid(1:) = 'Unknown'
      IF (new_sid.EQ.'RDPMAILSRV' .OR. str_end(new_lid).EQ.0) new_lid(1:) = 'Unknown'


      IF ( cl_htmlflag ) THEN

         WRITE (outlun,'(a,i5,a)')
     - '<table cellspacing=0 cellpadding=0>' //LF//
     - '<tr> <td><hr><td><hr><td><hr><td><hr>' //LF//
     - '<tr> <td> S_ab <td> Olis  <td> Short-ID <td> Full name' //LF//
     - '<tr> <td> <hr><td> <hr><td> <hr><td> <hr>' //LF//
     - '<tr> <td align=right> <td align=right> ', new_onum, 
     - ' <td> <font color=' // hd_color // '> ' // 
     -  new_sid (str_beg(new_sid):str_end(new_sid)) // ' </font> ' //
     - ' <td nowrap> <font color=' // hd_color // '> ' //
     -  new_lid (str_beg(new_lid):str_end(new_lid)) // ' </font> '

      ELSE

         WRITE (outstr(9:13),'(i5)') new_onum

         outstr (16:25) = new_sid (str_beg(new_sid):str_end(new_sid))
         outstr (28:outwid) = new_lid (str_beg(new_lid):str_end(new_lid))

         WRITE (outlun,'(a)') outstr (1:outwid)

         WRITE (outlun,'(1x,a)') dshlin (1:outwid)
         WRITE (outlun,'(1x,a)') ' S_ab   Olis  Short-ID    Full name'
         WRITE (outlun,'(1x,a)') dshlin (1:outwid)

      END IF
C
C Print out sorted database entries where Sab > 0,
C
      listed_seqs = 0 
      iout = 1

      DO WHILE ( listed_seqs.LT.listlen .AND. iout.LE.old_seqsmax )

         IF ( sab_vals(iout) .GT. 0.0 ) THEN

            iseq = seq_ndcs (iout)
            
            IF ( cl_htmlflag ) THEN

               WRITE (outlun,'(a,f6.3,a,i5)') 
     - '<tr> <td align=right> ', sab_vals(iout),
     - ' <td align=right> ', old_onums(iseq)

               IF (cl_urlflag) THEN 

                  WRITE (outlun,'(a)') 
     - ' <td> ' //LF// '<a href=' //
     -  cl_urlpre(str_beg(cl_urlpre):str_end(cl_urlpre)) // 
     -        old_sids(iseq) 
     -      (str_beg(old_sids(iseq)):str_end(old_sids(iseq))) // '>'//
     -        old_sids(iseq) 
     -      (str_beg(old_sids(iseq)):str_end(old_sids(iseq))) //
     - '</a>'
               ELSE
                  WRITE (outlun,'(a)') 
     - ' <td> ' // old_sids(iseq) 
     -            (str_beg(old_sids(iseq)):str_end(old_sids(iseq)))
               END IF

               WRITE (outlun,'(a)')
     - ' <td nowrap> ' // old_lids(iseq) 
     -           (str_beg(old_lids(iseq)):str_end(old_lids(iseq)))
     
            ELSE

               WRITE (outstr(2:7),'(f6.3)') sab_vals(iout)
               WRITE (outstr(9:13),'(i5)') old_onums(iseq)
            
               outstr (16:25) = old_sids(iseq) 
     -              ( str_beg(old_sids(iseq)):str_end(old_sids(iseq)) )
            
               IF (str_end(old_lids(iseq)).GT.0) THEN
                  outstr (28:outwid) = old_lids(iseq) 
     -                 ( str_beg(old_lids(iseq)):str_end(old_lids(iseq)) )
               ELSE
                  outstr (28:outwid) = ' ' 
               END IF
               
               WRITE (outlun,'(a)') outstr (1:outwid)

            END IF

            listed_seqs = listed_seqs + 1 

         END IF

         iout = iout + 1

      END DO

      IF ( cl_htmlflag ) THEN

         WRITE (outlun,'(a)')
     - '<tr> <td> <hr><td> <hr><td> <hr><td> <hr>' //LF//
     - '</table>'
      ELSE
         WRITE (outlun,'(1x,a)') dshlin (1:outwid)
      END IF

      CALL FREE (sab_valsptr)
      CALL FREE (seq_ndcsptr)

      CALL FREE (dshlinptr)
      CALL FREE (outstrptr)

C      print*,'
      RETURN
      END 



      SUBROUTINE parse_cmd_line

      IMPLICIT none

      INCLUDE 'sim_cmdlin.inc'
C
C Local variables
C
      CHARACTER*200 argstr
      CHARACTER*1 LF / 10 /

      INTEGER str_beg, str_end
      INTEGER totarg, iarg
      INTEGER IARGC, ACCESS
      INTEGER errsum
C
C -------------------------------------------------------------------------------
C
C Include defaults settings of file names, logical units and flags,
C
      INCLUDE 'sim_deflts.inc'
C
C Read command line arguments; stop program with error message if an 
C argument is not among those recognized
C
      totarg = iargc()
      
      iarg = 1

      DO WHILE (iarg.LE.totarg)

         CALL getarg (iarg,argstr)
         CALL seq_upcase (argstr,str_beg(argstr),str_end(argstr),argstr)

         IF (argstr.EQ.'-A') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_alifnm)
         ELSE IF (argstr.EQ.'-B') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_binfnm)
         ELSE IF (argstr.EQ.'-I') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_newfnm)
         ELSE IF (argstr.EQ.'-U') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_addfnm)
         ELSE IF (argstr.EQ.'-N') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_namfnm)
         ELSE IF (argstr.EQ.'-O') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_outfnm)
         ELSE IF (argstr.EQ.'-T') THEN
            cl_tupflag = .TRUE.
            iarg = iarg + 1 
            CALL getarg(iarg,argstr)
            READ (argstr,*) cl_tuplen
         ELSE IF (argstr.EQ.'-L') THEN
            cl_listflag = .TRUE.
            iarg = iarg + 1 
            CALL getarg(iarg,argstr)
            READ (argstr,*) cl_listlen
         ELSE IF (argstr.EQ.'-W') THEN
            cl_widflag = .TRUE.
            iarg = iarg + 1 
            CALL getarg(iarg,argstr)
            READ (argstr,*) cl_outwid
         ELSE IF (argstr.EQ.'-E') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_errfnm)
         ELSE IF (argstr.EQ.'-CHIMERA') THEN
            cl_chimflag = .TRUE.
         ELSE IF (argstr.EQ.'-SIMPLE') THEN
            cl_simpflag = .TRUE.
         ELSE IF (argstr.EQ.'-HTML') THEN
            cl_htmlflag = .TRUE.
         ELSE IF (argstr.EQ.'-URLBASE') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_urlpre)
         ELSE IF (argstr.EQ.'-D') THEN
            iarg = iarg + 1 
            CALL getarg(iarg,cl_lststr)
         ELSE IF (argstr.EQ.'-S') THEN
            cl_stepflag = .TRUE.
            iarg = iarg + 1
            CALL getarg(iarg,argstr)
            READ (argstr,*) cl_steplen
         ELSE IF (argstr.EQ.'-F') THEN
            cl_fragflag = .TRUE.
            iarg = iarg + 1
            CALL getarg(iarg,argstr)
            READ (argstr,*) cl_fraglen
         ELSE IF (argstr.EQ.'-P') THEN
            cl_partflag = .TRUE.
            iarg = iarg + 1
            CALL getarg(iarg,argstr)
            READ (argstr,*) cl_partlen
         ELSE

            IF (str_end(cl_errfnm).EQ.0) cl_errlun = 6

            CALL write_message (cl_errlun,cl_errfnm,
     -          'Error: Unknown command line option ' //
     -           argstr(str_beg(argstr):str_end(argstr))) 

            STOP

         END IF  

         iarg = iarg + 1 

      END DO

      cl_aliflag = str_end(cl_alifnm) .GT. 0    ! alignment file given
      cl_binflag = str_end(cl_binfnm) .GT. 0    ! binary       -
      cl_newflag = str_end(cl_newfnm) .GT. 0    ! sequence     -
      cl_logflag = str_end(cl_logfnm) .GT. 0    ! log          -
      cl_errflag = str_end(cl_errfnm) .GT. 0    ! error        -
      cl_outflag = str_end(cl_outfnm) .GT. 0    ! output       -
      cl_namflag = str_end(cl_namfnm) .GT. 0    ! namelist     -
      cl_addflag = str_end(cl_addfnm) .GT. 0    ! append       -
      cl_urlflag = str_end(cl_urlpre) .GT. 0    ! urlpre string given
      cl_lstflag = str_end(cl_lststr) .GT. 0    ! list      -     -

      errsum = 0
C
C Validate command line input and stop program with message if files dont 
C exist, flags dont make sense in combination etc. Also assign IO 
C logical unit numbers, if they need be different from defaults,
C
C Error file (-e),
C
      IF (cl_errflag) THEN

         IF (access(cl_errfnm,' ').EQ.0) THEN    ! if it exists, then
            cl_errlun = 6
            CALL write_message (cl_errlun,cl_errfnm,
     -          'Error: Error file ' // cl_errfnm(str_beg(cl_errfnm)
     -               :str_end(cl_errfnm)) // ' already exists')
            errsum = errsum + 1 
         END IF

      ELSE
         cl_errlun = 6
      END IF
C
C Output file (-e),
C
      IF (cl_outflag) THEN

         IF (access(cl_outfnm,' ').EQ.0) THEN    ! if it exists, then
            CALL write_message (cl_errlun,cl_errfnm,
     -          'Error: Output file ' // cl_outfnm(str_beg(cl_outfnm)
     -               :str_end(cl_outfnm)) // ' already exists')
            errsum = errsum + 1 
         END IF

      ELSE
         cl_outlun = 6
      END IF
C
C User appended file (-u),
C
      IF (cl_addflag) THEN

         IF (access(cl_addfnm,'r').NE.0) THEN    ! if it exists, then
            CALL write_message (cl_errlun,cl_errfnm,
     -          'Error: Append file ' // cl_addfnm(str_beg(cl_addfnm)
     -               :str_end(cl_addfnm)) // ' does not exist as readable')
            errsum = errsum + 1 
         END IF

      END IF
C
C Namelist file (-n),
C
      IF (cl_namflag) THEN

         IF (access(cl_namfnm,'r').NE.0) THEN    ! if it exists, then
            CALL write_message (cl_errlun,cl_errfnm,
     -          'Error: Name list file ' // cl_namfnm(str_beg(cl_namfnm)
     -               :str_end(cl_namfnm)) // ' does not exist as readable')
            errsum = errsum + 1 
         END IF

      END IF
C
C Sequences to be analyzed (-i),
C
      IF (cl_newflag) THEN
                        
         IF (access(cl_newfnm,'r').NE.0) THEN    ! if not readable, then
            CALL write_message (cl_errlun,cl_errfnm,
     -           'Error: New sequence file ' // 
     -            cl_newfnm(str_beg(cl_newfnm):str_end(cl_newfnm)) // 
     -           ' does not exist as readable')
            errsum = errsum + 1   
         END IF

      ELSE

         CALL write_message (cl_errlun,cl_errfnm,
     -        'Error: New sequence file not specified')
         errsum = errsum + 1   

      END IF
C
C If no binary file or alignment file given, stop program with message,
C
      IF (.NOT.cl_aliflag .AND. .NOT.cl_binflag) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -        'Error: Neither alignment nor binary file specified')
         errsum = errsum + 1  
      END IF
C
C Otherwise, use binary if readable. If it isnt, use alignment file,
C 
      IF (cl_binflag .AND. access(cl_binfnm,'r').EQ.0) THEN

         CONTINUE !  cl_aliflag = .FALSE.    ! redundant isnt it

      ELSE IF (cl_binflag .AND. access(cl_binfnm,'r').NE.0) THEN

         IF (cl_aliflag) THEN
            IF (access(cl_alifnm,'r').NE.0) THEN
               CALL write_message (cl_errlun,cl_errfnm,
     -              'Error: Neither binary file ' // 
     -              cl_binfnm(str_beg(cl_binfnm):str_end(cl_binfnm)) // LF // 
     -              'nor alignment file ' // 
     -              cl_alifnm(str_beg(cl_alifnm):str_end(cl_alifnm)) // 
     -              ' exist as readable')
               errsum = errsum + 1  
            ELSE
               cl_binflag = .FALSE.
            END IF
         ELSE
            CALL write_message (cl_errlun,cl_errfnm,
     -           'Error: Binary file ' // 
     -           cl_binfnm(str_beg(cl_binfnm):str_end(cl_binfnm)) // 
     -           ' does not exist as readable')
            errsum = errsum + 1  
         END IF

      ELSE IF (cl_aliflag) THEN

         IF (access(cl_alifnm,'r').NE.0) THEN
            CALL write_message (cl_errlun,cl_errfnm,
     -           'Error: Alignment file ' // 
     -           cl_alifnm(str_beg(cl_alifnm):str_end(cl_alifnm)) // 
     -           ' does not exist as readable')
            errsum = errsum + 1  
         ELSE
            cl_binflag = .FALSE.
         END IF

      END IF
C
C If no output file given, write to stdout,
C
      IF (.NOT.cl_outflag) cl_outlun = 6
C
C Is -simple given with -chimera,
C
      IF (cl_simpflag .AND. cl_chimflag) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -        'Error: -simple option is incompatible with -chimera option')
         errsum = errsum + 1
      END IF
C
C Is cl_listlen reasonable,
C
      IF (cl_listflag .AND. cl_listlen.LE.0) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -        'Error: List length must have a positive value')
         errsum = errsum + 1  
      END IF
C
C Is cl_partlen reasonable,
C
      IF (cl_partflag .AND. cl_partlen.LT.cl_tuplen) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -        'Error: Min. sequence length must be longer than tuple value')
         errsum = errsum + 1  
      END IF
C
C Is cl_outwid reasonable,
C
      IF (cl_widflag .AND. cl_outwid.LE.60) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -        'Error: Output must be wider than 60 characters')
         errsum = errsum + 1  
      END IF
C
C Minimum fragment length given, but no -chimera flag is not good,
C
      IF (cl_fragflag .AND. .NOT.cl_chimflag) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -       'Error: Minimum fragment length only applies to -chimera option')
         errsum = errsum + 1  
      END IF

      IF (cl_chimflag .AND. cl_fraglen.LE.0) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -       'Error: Minimum fragment length must be a positive number')
         errsum = errsum + 1  
      END IF
C
C Step length given, but no -chimera flag is not good,
C
      IF (cl_stepflag .AND. .NOT.cl_chimflag) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -       'Error: Step length only applies to -chimera option')
         errsum = errsum + 1  
      END IF

      IF (cl_chimflag .AND. cl_steplen.LE.0) THEN
         CALL write_message (cl_errlun,cl_errfnm,
     -       'Error: Step length must be a positive number')
         errsum = errsum + 1  
      END IF
C
C If tuple length specified, check validity. Also dont use the binary 
C and protest if no ascii file given
C
      IF (cl_tupflag) THEN

         IF (cl_tuplen.LT.1 .OR. cl_tuplen.GT.10) THEN
            CALL write_message (cl_errlun,cl_errfnm,
     -           'Error: Tuple must be within the range 1 - 10')
            errsum = errsum + 1  
         END IF

         IF (cl_aliflag) THEN
            IF (access(cl_alifnm,'r').NE.0) THEN
               CALL write_message (cl_errlun,cl_errfnm,
     -           'Error: Alignment file ' // 
     -           cl_alifnm(str_beg(cl_alifnm):str_end(cl_alifnm)) // 
     -           ' does not exist as readable')
               errsum = errsum + 1  
            END IF
         ELSE
            CALL write_message (cl_errlun,cl_errfnm,
     -         'Error: The -t option requires specification of an '//
     -         'ASCII alignment file')
            errsum = errsum + 1  
         END IF

      END IF

      IF (errsum.GT.0) STOP

      RETURN
      END
      SUBROUTINE r4_array_init (dimbeg,dimend,array,inibeg,iniend,value)

      IMPLICIT none

      INTEGER   dimbeg, dimend
      REAL      array (dimbeg:dimend)
      INTEGER   inibeg, iniend
      REAL      value
C
C Local variables
C
      INTEGER   i
C
C -------------------------------------------------------------------------
C
      DO i = inibeg, iniend
         array (i) = value
      END DO

      RETURN
      END

      REAL FUNCTION r4_array_MAX (r4_array,beg,end,maxndx)
C
C Finds maximum value among real array elements between beg and 
C end; the index with highest value is returned in maxndx
C
      IMPLICIT none

      REAL    r4_array(*)
      INTEGER beg,end,maxndx,i
C
C -------------------------------------------------------------------------
C
      r4_array_MAX = r4_array(beg)
      maxndx = beg

      DO i = beg+1,end
         IF (r4_array(i).GT.r4_array_MAX) THEN
            r4_array_MAX = r4_array(i)
            maxndx = i 
         END IF
      END DO

      RETURN
      END 

      REAL FUNCTION r4_array_MIN (r4_array,beg,end,minndx)
C
C Finds minimum value among real array elements between beg and 
C end; the index with lowest value is returned in minndx
C
      IMPLICIT none

      REAL    r4_array(*)
      INTEGER beg,end,minndx,i
C
C -------------------------------------------------------------------------
C
      r4_array_MIN = r4_array(beg)
      minndx = beg

      DO i = beg+1,end
         IF (r4_array(i).LT.r4_array_MIN) THEN
            r4_array_MIN = r4_array(i)
            minndx = i 
         END IF
      END DO

      RETURN
      END 
      SUBROUTINE r4_shellsort (a, b, n)
C
C Explain
C      
      IMPLICIT none

      REAL a(*),v
      INTEGER b(*),n
      INTEGER i,j,h,bv
C
C ------------------------------------------------------------------------------
C
      h = 1
      DO WHILE ( h.LE.n/9 ) 
         h = 3 * h + 1 
      END DO

      DO WHILE ( h.GT.0 ) 
         i = h + 1 
         DO WHILE ( i.LE.n )
            v = a(i)
            bv = b(i)
            j = i
            DO WHILE ( j.GT.h .AND. a(j-h).LT.v ) 
               a(j) = a(j-h)
               b(j) = b(j-h)
               j = j - h 
            END DO
            a(j) = v
            b(j) = bv
            i = i + 1 
         END DO
         h = h/3
      END DO

      RETURN
      END 

      SUBROUTINE sa_bin_io (iostr,lun,lodim,hidim,array)
C
C One dimensional string array binary write; how to get rid of loop? 
C 
      IMPLICIT none

      CHARACTER*(*) iostr
      INTEGER   lun
      INTEGER   lodim
      INTEGER   hidim

      CHARACTER*(*) array (lodim:hidim)
C
C Local variables
C
      INTEGER i
C
C -----------------------------------------------------------------------------
C
      IF (iostr.EQ.'WRITE') THEN

         DO i = lodim, hidim
            WRITE (lun) array(i)
         END DO

      ELSE IF (iostr.EQ.'READ') THEN

         DO i = lodim, hidim
            READ (lun) array(i)
         END DO

      END IF

      RETURN
      END


      SUBROUTINE seq_locase (in_seq,beg,end,out_seq)

      IMPLICIT none

      BYTE    in_seq (*)
      INTEGER beg
      INTEGER end
      BYTE    out_seq (*)
C
C Local variables
C
      INTEGER i
C
C -------------------------------------------------------------------------------
C
      DO i = beg,end
         IF (in_seq(i).GE.65 .AND. in_seq(i).LE.90) THEN
            out_seq(i) = in_seq(i) + 32
         ELSE
            out_seq(i) = in_seq(i)
         END IF
      END DO

      RETURN
      END

      SUBROUTINE seq_replace (in_seq,beg,end,in_ich,out_ich,out_seq)

      IMPLICIT none

      BYTE    in_seq (*)
      INTEGER beg
      INTEGER end
      BYTE    in_ich
      BYTE    out_ich
      BYTE    out_seq (*)
C
C Local variables
C
      INTEGER i
C
C -------------------------------------------------------------------------------
C
      DO i = beg,end
         IF (in_seq(i).EQ.in_ich) THEN
            out_seq(i) = out_ich
         ELSE
            out_seq(i) = in_seq(i)
         END IF
      END DO

      RETURN
      END

      SUBROUTINE seq_upcase (in_seq,beg,end,out_seq)

      IMPLICIT none

      BYTE    in_seq (*)
      INTEGER beg
      INTEGER end
      BYTE    out_seq (*)
C
C Local variables
C
      INTEGER i
C
C -------------------------------------------------------------------------------
C
      DO i = beg,end
         IF (in_seq(i).GE.97 .AND. in_seq(i).LE.122) THEN
            out_seq(i) = in_seq(i) - 32
         ELSE
            out_seq(i) = in_seq(i)
         END IF
      END DO

      RETURN
      END

      INTEGER FUNCTION str_beg(str)

      IMPLICIT none

      CHARACTER*(*) str
      CHARACTER*1   null /0/
C
C -------------------------------------------------------------------- 
C
      DO str_beg = 1,len(str)
         IF (str(str_beg:str_beg).NE.' ' .AND. 
     -       str(str_beg:str_beg).NE.null) RETURN 
      END DO

      RETURN
      END


      INTEGER FUNCTION str_end(str)

      IMPLICIT none

      CHARACTER*(*) str
      CHARACTER*1   null /0/
C
C --------------------------------------------------------------------
C
      DO str_end = len(str),1,-1
         IF (str(str_end:str_end).NE.' ' .AND.
     -       str(str_end:str_end).NE.null) RETURN
      END DO

      RETURN
      END
         

      INTEGER FUNCTION str_end_ch (str,ch)

      IMPLICIT none
 
      CHARACTER*(*) str
      CHARACTER*1   ch
C
C -------------------------------------------------------------------- 
C
      DO str_end_ch = len(str),1,-1
         IF (str(str_end_ch:str_end_ch).EQ.ch) RETURN
      END DO

      RETURN
      END
      SUBROUTINE str_field (line, begndx, endndx, fldnum, fldsep, begpos, endpos)
C
C      
      IMPLICIT none

      CHARACTER*(*) line
      INTEGER       begndx
      INTEGER       endndx
      INTEGER       fldnum
      CHARACTER*1   fldsep
      INTEGER       begpos
      INTEGER       endpos
C
C Local variables
C
      INTEGER ifld
      LOGICAL sepflg
C
C -------------------------------------------------------------------------------
C
      ifld = 1
      begpos = begndx
      endpos = begpos
      sepflg = .TRUE.

      DO WHILE (ifld.LE.fldnum)

         begpos = endpos

         DO WHILE (begpos.LE.endndx .AND. sepflg)
            IF (line(begpos:begpos).EQ.fldsep) THEN
               begpos = begpos + 1 
            ELSE
               sepflg = .FALSE.
            END IF
         END DO

         endpos = begpos

         DO WHILE (endpos.LE.endndx .AND. .NOT.sepflg)
            IF (line(endpos:endpos).NE.fldsep) THEN
               endpos = endpos + 1 
            ELSE
               sepflg = .TRUE.
            END IF
         END DO

         ifld = ifld + 1
         
      END DO

      endpos = endpos - 1

      IF (begpos.GT.endndx) THEN
         begpos = 0             ! will hopefully crash most lines
         endpos = 0             !    -     - 
      END IF

      RETURN
      END
      SUBROUTINE str_init (string,inibeg,iniend,ch)

      IMPLICIT none

      CHARACTER*(*) string
      INTEGER       inibeg, iniend
      CHARACTER*1   ch
C
C Local variables
C
      INTEGER   i
C
C -------------------------------------------------------------------------
C
      DO i = inibeg, iniend
         string (i:i) = ch
      END DO

      RETURN
      END

 
      SUBROUTINE str_locase (str,beg,end)
C
C Changes upper case characters to lower case from beg to end
C
      IMPLICIT none

      CHARACTER*(*) str

      INTEGER beg,end
      INTEGER i 
C
C ------------------------------------------------------------------------
C 
      DO i = beg,end
         IF (ICHAR(str(i:i)).GE.65 .AND. ICHAR(str(i:i)).LE.90) 
     -       str(i:i) = CHAR (ICHAR (str(i:i)) + 32)
      END DO

      RETURN
      END 
      SUBROUTINE str_lst_lookup (sortorder, ndim, strarr, str, strndx)
C
C returns the index of string str in the n-dimensional array of 
C strings by a binary search procedure. If not found, a zero is 
C returned.
C
      IMPLICIT none
 
      CHARACTER*10  sortorder
      INTEGER       ndim
      CHARACTER*(*) strarr(ndim)
      CHARACTER*(*) str
      INTEGER       strndx
C
C Local variables
C
      INTEGER       index, hi_index, lo_index
      LOGICAL       hit
C
C-----------------------------------------------------------------------
C
      CALL str_upcase ( sortorder, 1, LEN(sortorder) )

      lo_index = 1
      hi_index = ndim
 
      hit = .FALSE.

      IF ( sortorder.EQ.'ASCENDING' ) THEN
 
         DO WHILE ( .NOT.hit .AND. lo_index.LE.hi_index )
            
            index = (hi_index+lo_index) / 2
            
            IF (str.LT.strarr(index)) THEN
               hi_index = index-1
            ELSE IF (str.GT.strarr(index)) THEN
               lo_index = index+1
            ELSE 
               hit = .TRUE.
            END IF
            
         END DO

      ELSE IF ( sortorder.EQ.'DESCENDING' ) THEN
 
         DO WHILE ( .NOT.hit .AND. lo_index.LE.hi_index )
            
            index = (hi_index+lo_index) / 2
            
            IF (str.GT.strarr(index)) THEN
               hi_index = index-1
            ELSE IF (str.LT.strarr(index)) THEN
               lo_index = index+1
            ELSE 
               hit = .TRUE.
            END IF
            
         END DO

      END IF

      IF (hit) THEN
        strndx = index
      ELSE
        strndx = 0
      END IF
 
      RETURN
      END 
      SUBROUTINE str_shellsort (a,b,n)
      
      IMPLICIT none
      
      INTEGER n
      CHARACTER*(*) a(n) 
      INTEGER b(n)
C
C Local variables
C
      CHARACTER*80 v
      POINTER (vptr,v)

      INTEGER i,j,h,bv
C
C ------------------------------------------------------------------------------
C
      CALL get_memory (vptr,LEN(a(1)),'v','str_shellsort')

      h = 1
      DO WHILE ( h.LE.n/9 ) 
         h = 3 * h + 1 
      END DO

      DO WHILE ( h.GT.0 ) 
         i = h + 1 
         DO WHILE ( i.LE.n )
            v = a(i)
            bv = b(i)
            j = i
            DO WHILE ( j.GT.h .AND. a(j-h).LT.v ) 
               a(j) = a(j-h)
               b(j) = b(j-h)
               j = j - h 
            END DO
            a(j) = v
            b(j) = bv
            i = i + 1 
         END DO
         h = h/3
      END DO

      CALL FREE (vptr)

      RETURN
      END 
 
      SUBROUTINE str_upcase (str,beg,end)
C
C Changes lower case characters to upper case from beg to end
C
      IMPLICIT none

      CHARACTER*(*) str

      INTEGER beg,end
      INTEGER i 
C
C ------------------------------------------------------------------------
C 
      DO i = beg,end
         IF (ICHAR(str(i:i)).GE.97 .AND. ICHAR(str(i:i)).LE.122) 
     -       str(i:i) = CHAR (ICHAR (str(i:i)) - 32)
      END DO

      RETURN
      END 
      SUBROUTINE text_wrap (textstr,lftmarg,rgtmarg,sepchar,wrapstr)
C
C IMPROVE
C
      CHARACTER*(*) textstr
      INTEGER       lftmarg
      INTEGER       rgtmarg
      CHARACTER*1   sepchar
      CHARACTER*(*) wrapstr
C
C Local variables
C
      CHARACTER*1 LF / 10 / 
      INTEGER textpos, wrappos
      INTEGER textlen, linewid
C
C ---------------------------------------------------------------------------
C
      textlen = LEN (textstr)

      linewid = rgtmarg - lftmarg + 1
      textpos = linewid

      DO WHILE (textpos.LT.textlen)
         DO WHILE (textstr(textpos:textpos).NE.sepchar)
            textpos = textpos - 1 
         END DO
         textstr(textpos:textpos) = LF
         textpos = textpos + linewid
      END DO

      wrapstr(1:lftmarg) = ' ' 
      wrappos = lftmarg

      DO textpos = 1,textlen
         wrappos = wrappos + 1 
         IF (textstr(textpos:textpos).EQ.LF) THEN
            wrapstr(wrappos:wrappos) = LF
            wrapstr(wrappos+1:wrappos+lftmarg) = ' '
            wrappos = wrappos + lftmarg
         ELSE
            wrapstr(wrappos:wrappos) = textstr(textpos:textpos)
         END IF
      END DO

      wrapstr (wrappos+1:) = ' '

      RETURN
      END
      SUBROUTINE To_Code4 (bytarr,beg,end)
C
C The incoming ASCII characters (bytarr(beg) through bytarr(end)) are 
C translated to the following code:
C
C     ASCII    Meaning   Code 
C    ------   --------   -----
C         A =  A           0        (Adenine)
C       U/T =        U     1        (Uracil/Thymine)
C         G =      G       2        (Guanine)
C         C =    C         3        (Cytosine)
C
      IMPLICIT none

      BYTE    bytarr(*)
      BYTE    Table(0:127)
      INTEGER beg,end
      INTEGER i

      DATA Table  
C               A  B  C  D  E  F  G  H  I  J  K  L  M -
     -  / 65*0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
C               N  O  P  Q  R  S  T  U  V  W  X  Y  Z -
     -          0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,  6*0,
C               a  b  c  d  e  f  g  h  i  j  k  l  m -
     -          0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
C               n  o  p  q  r  s  t  u  v  w  x  y  z -
     -          0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,  5*0 /
C
C--------------------------------------------------------------------
C
      DO i = beg,end
         BytArr(i) = Table (BytArr(i))
      END DO

      RETURN
      END

      SUBROUTINE write_message (lun,fnm,str) 

      INCLUDE 'sim_cmdlin.inc'
C
C Improve
C
      IMPLICIT none
C
C Writes messages
C
      INTEGER       lun
      CHARACTER*(*) fnm
      CHARACTER*(*) str
C
C Local variables
C
      LOGICAL opnflg
      INTEGER opnerr
      CHARACTER*1 LF / 10 /
      CHARACTER*2000 outstr
      INTEGER str_beg,str_end
      INTEGER i,iout
C
C -------------------------------------------------------------------------------
C
      INQUIRE (UNIT = lun, OPENED = opnflg) 

      IF (.NOT.opnflg)
     -   OPEN (UNIT = lun,
     -         FILE = fnm,
     -       STATUS = 'NEW',
     -       IOSTAT = opnerr)

      IF (opnerr.NE.0) THEN
 
         IF (cl_htmlflag) THEN
            WRITE (6,'(a)') 
     - 'Content-Type: text/html' //LF//LF//
     - '<HTML>' //LF//
     - '<HEAD>' //LF//
     - '<TITLE> SIMRANK error </TITLE>' //LF//
     - '</HEAD>' //LF//
     - '<BODY>' //LF//
     - '<H2> SIMRANK error: </H2> Cannot open new file ' //
     -  fnm(str_beg(fnm):str_end(fnm)) // '<BR>' //LF//
     - 'it may already exist, check also permissions' //LF//
     - '</BODY>' //LF//
     - '</HTML>'

         ELSE
            WRITE (6,'(a)') ' *********** ' //LF// 
     -                   ' ** SIMRNK: WRITE_MESSAGE: Error: Cannot open' //
     -                   ' new file ' // fnm(str_beg(fnm):str_end(fnm)) //LF//
     -                   ' **      it may already exist, check also' //
     -                   ' permissions' //LF//
     -                   ' *********** '
         END IF
         STOP
      END IF

      IF (cl_htmlflag) THEN
         WRITE (lun,'(a)') 
     - 'Content-Type: text/html' //LF//LF//
     - '<HTML>' //LF//
     - '<HEAD>' //LF//
     - '<TITLE> SIMRANK error </TITLE>' //LF//
     - '</HEAD>' //LF//
     - '<BODY>' //LF//
     - '<H2> SIMRANK error: </H2> ' //LF//
     -  str(str_beg(str):str_end(str)) //LF//
     - '</BODY>' //LF//
     - '</HTML>'
      ELSE

         iout = 26              !1234567890123     4      567890123456
         outstr(1:iout) = ' *********** ' // LF // ' ** SIMRNK: '
         
         DO i = 1,str_end(str)
            iout = iout + 1
            outstr(iout:iout) = str(i:i)
            IF (str(i:i).EQ.LF) THEN
               outstr(iout+1:iout+12) = ' **         '
               iout = iout + 12
            END IF
         END DO
         
         outstr(iout+1:iout+14) = LF // ' *********** '
         iout = iout + 14
         
         WRITE (lun,'(a)' ) outstr(1:iout)

      END IF

      RETURN
      END
