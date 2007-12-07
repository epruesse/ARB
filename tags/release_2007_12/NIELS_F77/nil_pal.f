      PROGRAM PAL     ! PreALignment
C
C Niels Larsen, niels@vitro.cme.msu.edu
C
C Version 0.0, 22 january, 1993.
C Version 0.5, 15 february, 1993.
C Version 1.0, 15 march, 1993.
C Version 1.1, 26 july, 1993.
C      Better binary IO and memory allocation.
C Version 1.2, 05 august, 1993.
C      Safer IO, read all new sequences into memory, improved 
C      -complete option
C Version 1.3, 12 august, 1994.
C      Better command line 
C      Key/table new sequence input option
C      Key/sab/table outputs
C Version 1.4, 30 september, 1994
C      Key/table old sequence (db) input option
C Version 1.5, 24 october, 1994
C      Improved ruler annotation
C Version 1.6, 5 november, 1994
C      Bug fixes
C Version 1.7, 3 February, 1997
C      Port to Linux using Absoft f77 compiler v 3.4a. 
C      Two memory bugs fixed that SUN f77 accepted. Much
C      better speed, Absoft is a much better compiler. 
C
C TODO:  Cut the memory. Also this program is by no means a finished
C        thing, just a snapshot of development as it stopped in 93..
C
C ---------------------------------------------------------------------------
C
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
      INCLUDE 'pal_settings.inc'
      INCLUDE 'pal_stacks.inc'
      INCLUDE 'pal_res_eqv.inc'

C      CHARACTER*2000 garbstr
C
C ---------------------------------------------------------------------------
C
C Dynamically allocated arrays for aligned entries
C The bounds need work
C
      BYTE       old_seqs (old_bytdim)   ! cat'ed sequences
      POINTER   (old_seqsptr,old_seqs) 

      BYTE       old_alis (old_bytdim)   ! cat'ed aligned sequences
      POINTER   (old_alisptr,old_alis) 

      INTEGER    old_lnklst (old_bytdim)   ! linked list
      POINTER   (old_lnklstptr,old_lnklst)

      CHARACTER*(old_sidswid)  old_sids (old_seqsdim)  ! short-ID's 
      POINTER   (old_sidsptr,old_sids) 

      CHARACTER*(old_lidswid)  old_lids (old_seqsdim)  ! full names 
      POINTER   (old_lidsptr,old_lids) 

      INTEGER    old_seqbgs (old_seqsdim)  ! begs of seqs in old_seqs 
      POINTER   (old_seqbgsptr,old_seqbgs)

      INTEGER    old_alibgs (old_seqsdim)  ! begs of aligned seqs in old_aseqs 
      POINTER   (old_alibgsptr,old_alibgs)

      INTEGER    old_seqnds (old_seqsdim)  ! ends of seqs in old_seqs
      POINTER   (old_seqndsptr,old_seqnds)

      INTEGER    old_alinds (old_seqsdim)  ! ends of aligned seqs in old_aseqs
      POINTER   (old_alindsptr,old_alinds)

      INTEGER    old_onums (old_seqsdim)  ! # different tuplen-mers per seq
      POINTER   (old_onumsptr,old_onums)

      INTEGER    old_olibgs (0:tupdimmax-1)      ! first occurence of each oligo
      POINTER   (old_olibgsptr,old_olibgs)  ! the upper bound doesnt matter

      INTEGER*2  old_tbl (old_seqsdim * old_basdim + tupdimmax)
      POINTER   (old_tblptr,old_tbl)

      INTEGER    old_tbegs (0:tupdimmax)      ! index into old_tbl
      POINTER   (old_tbegsptr,old_tbegs)   ! upper bound doesnt matter

      REAL       old_comps (old_seqsdim)
      POINTER   (old_compsptr,old_comps)
C 
C ---------------------------------------------------------------------------
C
C Dynamically allocated arrays for unaligned entries  
C
      BYTE       new_seqs (new_bytdim)   ! cat'ed unaligned sequences
      POINTER   (new_seqsptr,new_seqs) 

      CHARACTER*(new_sidswid)  new_sids (new_seqsdim)  ! short-ID's 
      POINTER   (new_sidsptr,new_sids) 

      CHARACTER*(new_lidswid)  new_lids (new_seqsdim)  ! full names 
      POINTER   (new_lidsptr,new_lids) 

      INTEGER    new_begs (new_seqsdim)  ! begs of aligned seqs in new_aseqs 
      POINTER   (new_begsptr,new_begs)

      INTEGER    new_ends (new_seqsdim)  ! ends of aligned seqs in new_aseqs
      POINTER   (new_endsptr,new_ends)

      INTEGER    new_abgs (new_seqsdim)  !  beginnings of annotation text 
      POINTER   (new_abgsptr,new_abgs)

      INTEGER    new_ands (new_seqsdim)  !  ends of annotation text
      POINTER   (new_andsptr,new_ands)

      CHARACTER*(new_textwid)  new_text (new_bytdim)  ! input memory text
      POINTER   (new_textptr, new_text) 

      REAL       new_sabs (old_seqsdim)  !  Sab's of one new sequence
      POINTER   (new_sabsptr,new_sabs)

      INTEGER    new_ndcs (old_seqsdim)  !  Indices of Sab's 
      POINTER   (new_ndcsptr,new_ndcs)

      INTEGER x_outlen, y_outlen
      INTEGER err_cod

      INTEGER new_lines
      INTEGER new_seqsmax
      INTEGER new_abyts
C
C ---------------------------------------------------------------------------
C
C Variables for align subroutine
C
      BYTE       x_seq (new_bytdim)
      POINTER   (x_seq_ptr,x_seq)

      BYTE       y_seq (new_bytdim)
      POINTER   (y_seq_ptr,y_seq)

      BYTE       x_alinks (new_bytdim)
      POINTER   (x_alinks_ptr, x_alinks)

      BYTE       y_alinks (new_bytdim)
      POINTER   (y_alinks_ptr, y_alinks)

      BYTE       x_ali (new_bytdim)
      POINTER   (x_ali_ptr,x_ali)

      BYTE       y_ali (new_bytdim)
      POINTER   (y_ali_ptr,y_ali)

      BYTE       a_mrk (new_bytdim)
      POINTER   (a_mrk_ptr,a_mrk)

      BYTE       x_gapali (new_bytdim)     ! output template sequence
      POINTER   (x_gapali_ptr,x_gapali) 

      BYTE       y_gapali (new_bytdim)     ! output aligned sequence
      POINTER   (y_gapali_ptr,y_gapali) 

      BYTE       a_gapmrk (new_bytdim)     ! output alignment certainty
      POINTER   (a_gapmrk_ptr,a_gapmrk) 


      INTEGER    x_seqlen,y_seqlen

c$$$      BYTE       debug(old_basdim)
c$$$      POINTER   (debug_ptr,debug)
C 
C ---------------------------------------------------------------------------
C
C Observed dimensions and sizes
C
      INTEGER    bin_tupdim       !
      INTEGER    bin_tuplen       ! 
      INTEGER    bin_basdim       !  dimensions saved and read from bin file
      INTEGER    bin_seqsdim      !  used to check with current program 
      INTEGER    bin_sidswid      !  parameter settings
      INTEGER    bin_lidswid      ! 

      INTEGER    old_seqsmax
      INTEGER    old_tblmax
      INTEGER    old_sbyts
      INTEGER    old_abyts

      INTEGER    bin_seqsmax
      INTEGER    bin_tblmax
      INTEGER    bin_abyts
C
C Local variables
C
C Output buffer, since write loops dont always work reliably (grr)
C
      CHARACTER*(new_basdim)  outbuf
      POINTER   (outbufptr,outbuf)  
      
      INTEGER str_beg,str_end
      INTEGER iseq, jseq, ires, imis, ipos, ibyt, inew, ilen
      INTEGER tmplndx, aliend
      INTEGER i
      INTEGER keylines, tbllines
      INTEGER out_tmpl
      BYTE    missch

      LOGICAL hit_flag

      CHARACTER*4000 usestr
      CHARACTER*1000 hdrstr
      CHARACTER*400  options
      CHARACTER*10 probid
      CHARACTER*20 intstr1, intstr2
      CHARACTER*7  pctstr
      CHARACTER*1 LF / 10 / 
      BYTE ich1,ich2

      INTEGER file_size

      CHARACTER*24 dat_str
      CHARACTER*3  month
      CHARACTER*3  pidstr, vidstr
      INTEGER      date, year

      INTEGER tupdim, tuplen
      INTEGER lun_alloc

c$$$      CHARACTER*10 ieee_id
c$$$      INTEGER inx,over,under,div,inv,accrued
c$$$      INTEGER ieee_err
c$$$      INTEGER IEEE_HANDLER,IEEE_FLAGS,SIGPFE_ABORT
C
c$$$      REAL    timarr(2)
c$$$      CHARACTER*100 tim_str
C
C ---------------------------------------------------------------------------
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
C
C -----------------------------------------------------------------------------
C
C Global author etc stamps,
C
      DATA author_ID  / 'Niels Larsen' /
      DATA email_ID   / 'niels@vitro.cme.msu.edu' /
      DATA program_ID / 'PAL' /
      DATA version_ID / '1.7' /
      DATA revdate_ID / '3 february 1997' / 
C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
C     Get command line and set up IO defaults
C 
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
C Initialize indices into the arrays that store flags, logical units 
C and file names,
C
      ndx_ingbk  = 1
      ndx_inkey  = 2
      ndx_intbl  = 3
      ndx_dbbin  = 4
      ndx_dbgbk  = 5
      ndx_dbkey  = 15
      ndx_dbtbl  = 16
      ndx_bpmat  = 6
      ndx_outgbk = 7
      ndx_outkey = 8 
      ndx_outsab = 9
      ndx_outtbl = 10 
      ndx_outhum = 11
      ndx_outrul = 12
      ndx_outsrv = 17
      ndx_log    = 13
      ndx_err    = 14
C
C Initialize flags and default values, 
C
      cl_dbgapsflag = .TRUE.
      cl_htmlflag   = .FALSE.
      cl_t2uflag    = .FALSE.
      cl_u2tflag    = .FALSE.
      cl_upcaseflag = .FALSE.
      cl_locaseflag = .FALSE.
      cl_completeflag = .FALSE.
      cl_tplflag    = .FALSE.
      cl_debugflag  = .FALSE.
      cl_outbinflag = .TRUE.

      cl_listwid = 60
      cl_templates = 1
      cl_completepct = 90.0
      cl_olilen = 7
C
C String which briefly explains program usage, 
C
      usestr = LF //  '                 Program ' // 
     -             program_ID // ' v ' // version_ID // ', ' // revdate_ID           //LF//
     - '             ' // author_ID // ', ' // email_ID                          //LF//LF//
     - ' One or more sequences are PreALigned with the most similar sequence(s).'    //LF//
     - ' The output shows where solidly aligned regions are.  For details see '      //LF//
     - ' the README file.  It will currently MISTREAT proteins.  Usage: '        //LF//LF//
     - ' Help :    -h -help '                                                    //LF//LF//
     - ' Input :   -in_genbank  file_name   (Mandatory if no other input) '          //LF//
     - '           -in_sids     file_name   (Mandatory with -in_table) '             //LF//
     - '           -in_seqs     file_name   (Mandatory if no other input) '          //LF//
     - '           -db_binary   file_name   (Mandatory unless -nobinary; fast) '     //LF//
     - '           -db_genbank  file_name   (Mandatory if no other db input; slow) ' //LF//
     - '           -db_sids     file_name   (Mandatory with -db_table) '             //LF//
     - '           -db_seqs     file_name   (mandatory if no other input) '          //LF//
     - '         [ -bp_matrix   file_name ] (Optional) '                         //LF//LF//
     - ' Output: [ -out_genbank  [ file_name ] ]  To screen if omitted '             //LF//
     - '         [ -out_human    [ file_name ] ]       -     -     -   '             //LF//
     - '         [ -out_sids     [ file_name ] ]       -     -     -   '             //LF//
     - '         [ -out_sabs     [ file_name ] ]       -     -     -   '             //LF//
     - '         [ -out_rulers   [ file_name ] ]       -     -     -   '             //LF//
     - '         [ -out_seqs     [ file_name ] ]       -     -     -   '             //LF//
     - '         [ -out_serv     [ file_name ] ]       -     -     -   '             //LF//
c     - '         [ -logs         [ file_name ] ]       -     -     -   '             //LF//
     - '         [ -errors       [ file_name ] ]       -     -     -   '         //LF//LF//
     - ' Options : [ -templates integer   ]   D = 1 '                                //LF//
     - '           [ -oligo_len integer   ]   D = 7 '                                //LF//
     - '           [ -complete_pct number ]   D = 90.0  (to exclude partials) '      //LF//
     - '           [ -listwid integer     ]   D = 80  (with -out_humane) '           //LF//
     - '           [ -listhtml            ]   D = no html                      '     //LF//
     - '           [ -nodbgaps            ]   D = gaps from database included '      //LF//
     - '           [ -nobinary            ]   D = write binary if not existing '     //LF//
     - '           [ -t2u -u2t            ]   D = leave as is '                      //LF//
     - '           [ -upper -lower        ]   D = dont change cases '                //LF
C
C 
C Corresponding list of valid options, used during command line parsing,
C
      options = '-h -help ' //
     - '-in_genbank -in_sids -in_seqs -db_binary -db_genbank ' //
     - '-db_sids -db_seqs -bp_matrix -out_genbank -out_sids '  // 
     - '-out_sabs -out_rulers -out_seqs -out_human -logs -errors ' //
     - '-templates -oligo_len -complete_pct -listwid -nodbgaps ' //
     - '-t2u -u2t -nobinary -upper -lower -listhtml -out_serv'
C
C Interpret command line, set flags and switches.  Stop program with error 
C messages if there is nonsense on the command line, mandatory arguments are 
C missing, something contradicts etc,
C
      CALL parse_cmd_line (usestr(1:str_end(usestr)),options(1:str_end(options)))
C      print*,'after parse command line'

C      WRITE (6,'(a, i6, 1x, l1)') ' after parse', ndx_outgbk, IO_flags(ndx_outgbk)
C 10    FORMAT (a,1x,l1,1x,i4,1x,a40)

C      WRITE (6,10) '-in_genbank  ', IO_flags(ndx_ingbk), IO_luns(ndx_ingbk), IO_fnms(ndx_ingbk)
C      WRITE (6,10) '-in_keys     ', IO_flags(ndx_inkey), IO_luns(ndx_inkey), IO_fnms(ndx_inkey)
C      WRITE (6,10) '-in_table    ', IO_flags(ndx_intbl), IO_luns(ndx_intbl), IO_fnms(ndx_intbl)
C      WRITE (6,10) '-db_binary   ', IO_flags(ndx_dbbin), IO_luns(ndx_dbbin), IO_fnms(ndx_dbbin)
C      WRITE (6,10) '-db_genbank  ', IO_flags(ndx_dbgbk), IO_luns(ndx_dbgbk), IO_fnms(ndx_dbgbk)
C      WRITE (6,10) '-bp_matrix   ', IO_flags(ndx_bpmat), IO_luns(ndx_bpmat), IO_fnms(ndx_bpmat)
C      WRITE (6,10) '-out_genbank ', IO_flags(ndx_outgbk), IO_luns(ndx_outgbk), IO_fnms(ndx_outgbk)
C      WRITE (6,10) '-out_keys    ', IO_flags(ndx_outkey), IO_luns(ndx_outkey), IO_fnms(ndx_outkey)
C      WRITE (6,10) '-out_sabs    ', IO_flags(ndx_outsab), IO_luns(ndx_outsab), IO_fnms(ndx_outsab)
C      WRITE (6,10) '-out_ruler   ', IO_flags(ndx_outrul), IO_luns(ndx_outrul), IO_fnms(ndx_outrul)
C      WRITE (6,10) '-out_table   ', IO_flags(ndx_outtbl), IO_luns(ndx_outtbl), IO_fnms(ndx_outtbl)
C      WRITE (6,10) '-out_humane  ', IO_flags(ndx_outhum), IO_luns(ndx_outhum), IO_fnms(ndx_outhum)
C      WRITE (6,10) '-logs        ', IO_flags(ndx_log), IO_luns(ndx_log), IO_fnms(ndx_log)
C      WRITE (6,10) '-errors      ', IO_flags(ndx_err), IO_luns(ndx_err), IO_fnms(ndx_err)
C      WRITE (6,'(a,1x,l1,1x,i5)') '-templates', cl_tplflag, cl_templates 
C      WRITE (6,'(a,1x,i5)') '-listwid  ', cl_listwid
C      WRITE (6,'(a,1x,l1)') '-nobinary ', cl_outbinflag
C      WRITE (6,'(a,1x,l1)') '-nodbgaps ', cl_dbgapsflag
C      WRITE (6,'(a,1x,l1)') '-t2u ', cl_t2uflag
C      WRITE (6,'(a,1x,l1)') '-u2t ', cl_u2tflag
C      WRITE (6,'(a,1x,l1,1x,f7.2)') '-complete_pct', cl_completeflag, cl_completepct
C      WRITE (6,'(a,1x,l1)') '-upper', cl_upcaseflag
C      WRITE (6,'(a,1x,l1)') '-lower', cl_locaseflag

c      STOP

C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
C     Initialize alignment related values and matrices
C 
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
      x_max_gap = 0.1
      y_max_gap = 0.1
      consec_pen = 0.001      ! strictness
c      permut_pen = 0.005      ! strictness

      p_mtc = 0.25      ! probability of match *improve*
      p_mis = 0.75      !        -    mismatch     -

      x_gap5 = ICHAR('~')   ! leading gap
      x_gap4 = ICHAR('-')   ! internal gap
      x_gap3 = ICHAR('~')   ! trailing gap

      y_gap5 = ICHAR('~')   
      y_gap4 = ICHAR('-')
      y_gap3 = ICHAR('~')

      tuplen = cl_olilen
      tupdim = 4 ** tuplen
C
C Stack index variables, then easier to change stacks. 
C
      xl = 1  ! x lower  rectangle boundary
      xh = 2  ! x higher rectangle boundary 
      yl = 3  ! y lower  rectangle boundary
      yh = 4  ! y higher rectangle boundary 
      cc = 5  ! match character (debug only)
      xm = 6  ! x lower limit of search space
      ym = 7  ! y   -     -         -      -
      ms = 8  ! not used
C
C Symbols for the ruler string that tells where problems are
C
      end5_ch = ICHAR('>')   ! marks uncertain alignment at 5 end
      end3_ch = ICHAR('<')   !    -      -          -       3  -
      ambx_ch = ICHAR('@')   ! denotes db seq expansion
      amby_ch = ICHAR('+')   !        new  -      -
      crtn_ch = ICHAR('-')   ! marks solid alignment
      unal_ch = ICHAR('?')   ! marks no alignment at all
C
C Set up pairing matrix  
C
      CALL bas_eqv_init
C
C Load scoring matrix if specified, otherwise use default,
C 
      IF ( IO_flags(ndx_bpmat) ) THEN

         IO_luns(ndx_bpmat) = lun_alloc (ndx_max, err_cod)

         OPEN ( UNIT = IO_luns(ndx_bpmat), 
     -          FILE = IO_fnms(ndx_bpmat),
     -        STATUS = 'OLD' )

         CALL Res_Sco_load ( IO_luns(ndx_bpmat) )

         CLOSE ( UNIT = IO_luns(ndx_bpmat) ) 

      ELSE
         CALL Res_Sco_dflt 
      END IF

C      print*,' after pairing and scoring matrices'
C
C --------------------------------------------------------------------------------
C
c$$$C This section for debug only, it can be commented away without affecting 
c$$$C rest of program. It prompts for two strings from stdin, and shows their
c$$$C alignment,
c$$$C
c$$$      IF (cl_debugflag) THEN
c$$$         
c$$$         DO WHILE (.TRUE.)
c$$$
c$$$            WRITE (6,'(a$)') ' Seq 1: '
c$$$            READ  (5,'(a)') argstr
c$$$            x_seqlen = 0
c$$$            DO i = 1,str_end(argstr)
c$$$               IF (IUB_base(ICHAR (argstr(i:i)))) x_seqlen = x_seqlen + 1 
c$$$            END DO
c$$$            CALL get_memory (x_seq_ptr,x_seqlen,'x_seq',program_ID)
c$$$            x_seqlen = 0
c$$$            DO i = 1,str_end(argstr)
c$$$               IF (IUB_base(ICHAR (argstr(i:i)))) THEN
c$$$                  x_seqlen = x_seqlen + 1 
c$$$                  x_seq(x_seqlen) = ICHAR (argstr(i:i))
c$$$               END IF
c$$$            END DO
c$$$
c$$$            WRITE (6,'(a$)') ' Seq 2: '
c$$$            READ  (5,'(a)') argstr
c$$$            y_seqlen = 0
c$$$            DO i = 1,str_end(argstr)
c$$$               IF (IUB_base(ICHAR (argstr(i:i)))) y_seqlen = y_seqlen + 1 
c$$$            END DO
c$$$            CALL get_memory (y_seq_ptr,y_seqlen,'y_seq',program_ID)
c$$$            y_seqlen = 0
c$$$            DO i = 1,str_end(argstr)
c$$$               IF (IUB_base(ICHAR (argstr(i:i)))) THEN
c$$$                  y_seqlen = y_seqlen + 1 
c$$$                  y_seq(y_seqlen) = ICHAR (argstr(i:i))
c$$$               END IF
c$$$            END DO
c$$$            
c$$$            IF (x_seqlen.EQ.0 .OR. y_seqlen.EQ.0) STOP
c$$$
c$$$            x_aliend = 2 * MAX (y_seqlen,x_seqlen)
c$$$            y_aliend = x_aliend
c$$$
c$$$            CALL get_memory (x_ali_ptr,x_aliend,'x_ali',program_ID)
c$$$            CALL get_memory (y_ali_ptr,y_aliend,'y_ali',program_ID)
c$$$            CALL get_memory (a_mrk_ptr,x_aliend,'a_mrk',program_ID)
c$$$            CALL get_memory (y_seq_ptr,y_seqlen,'y_seq',program_ID)
c$$$            CALL get_memory (debug_ptr,x_alilen,'debug',program_ID)
c$$$
c$$$            CALL align ( 4,6,4**6,
c$$$     -                x_seqlen,x_seq,x_alilen,x_ali,
c$$$     -                y_seqlen,y_seq,y_alilen,y_ali,
c$$$     -                a_mrk,
c$$$     -                err_cod,err_str,debug)
c$$$            
c$$$            CALL safe_compare ( x_seq,1,x_seqlen,x_ali,1,x_alilen,err_cod)
c$$$            IF (err_cod.NE.0) CALL write_message (6,' ','Error: x_seq != x_ali')
c$$$
c$$$            CALL safe_compare ( y_seq,1,y_seqlen,y_ali,1,y_alilen,err_cod)
c$$$            IF (err_cod.NE.0) CALL write_message (6,' ','Error: y_seq != y_ali')
c$$$
c$$$            CALL simple_out (6,x_alilen,x_alilen,x_ali,y_ali,
c$$$     -                       'Seq 1     ','Seq 2     ',a_mrk,debug)
c$$$            WRITE (6,'(a)')
c$$$
c$$$            CALL FREE (x_seq_ptr)
c$$$            CALL FREE (y_seq_ptr)
c$$$            CALL FREE (x_ali_ptr)
c$$$            CALL FREE (y_ali_ptr)
c$$$            CALL FREE (a_mrk_ptr)
c$$$            CALL FREE (debug_ptr)
c$$$            CALL FREE (x_ali_ptr)
c$$$
c$$$         END DO
c$$$
c$$$      END IF

C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C                                                                          
C     "DATABASE" input from here on                                        
C                                                                          
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
C ------------------------------------------------------------
C Binary input (PAL specific, format allowed to change freely)
C ------------------------------------------------------------
C
      IF (IO_flags(ndx_dbbin)) THEN

         IO_luns(ndx_dbbin) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_dbbin),
     -         FILE = IO_fnms(ndx_dbbin), 
     -       STATUS = 'OLD',
     -         FORM = 'UNFORMATTED')
C     -       BUFFER = 1000000)           ! Absoft
C     -      FILEOPT = 'BUFFER=1000000')  ! SUN specific
C     -    BLOCKSIZE = 1000000)          ! VAX specific
 
C Read program ID, stop with error message if unrecognized,
C
         READ (IO_luns(ndx_dbbin)) pidstr

         IF (pidstr.NE.program_ID) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -          'Error: Binary file program-ID mismatch:' // LF //
     -          'File ID is ' // pidstr // ' but should be ' // 
     -           program_ID // LF //
     -          'Solution: Delete ' // 
     -           IO_fnms(ndx_dbbin)(str_beg(IO_fnms(ndx_dbbin)):
     -                              str_end(IO_fnms(ndx_dbbin))) // 
     -          ' and restart the program')
            STOP
         END IF
C 
C Read version ID, stop with error message if unrecognized,
C
         READ (IO_luns(ndx_dbbin)) vidstr

         IF (vidstr.NE.version_ID) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -          'Error: Binary file version-ID mismatch:' // LF //
     -          'File ID is ' // vidstr // ' but should be ' // 
     -           version_ID // LF //
     -          'Solution: Delete ' // 
     -           IO_fnms(ndx_dbbin)(str_beg(IO_fnms(ndx_dbbin)):
     -                              str_end(IO_fnms(ndx_dbbin))) // 
     -          ' and restart the program')
            STOP
         END IF
C
C Parameters for comparison with settings in this program version 
C
         READ (IO_luns(ndx_dbbin)) bin_tuplen,
     -                    bin_tupdim, bin_seqsdim, bin_basdim,
c     -                    bin_sbyts, bin_abyts, 
     -                   bin_abyts, 
     -                    bin_sidswid, bin_lidswid,
     -                    bin_seqsmax, bin_tblmax

         old_seqsmax = bin_seqsmax
         old_tblmax = bin_tblmax
c         old_sbyts = bin_sbyts 
         old_abyts = bin_abyts 
C
C Stop program if program mismatches
C
         IF (.NOT. (
     -            bin_tuplen.EQ.tuplen        .AND.
     -            bin_tupdim.EQ.tupdim        .AND.
     -            bin_seqsdim.EQ.old_seqsdim  .AND.
     -            bin_basdim.EQ.old_basdim    .AND.
     -            bin_sidswid.EQ.old_sidswid  .AND.
     -            bin_lidswid.EQ.old_lidswid
     -             ) ) THEN

            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -          'Error: Program parameters mismatch in ' // 
     -           IO_fnms(ndx_dbbin)(str_beg(IO_fnms(ndx_dbbin)):
     -                              str_end(IO_fnms(ndx_dbbin))) )

            WRITE (IO_luns(ndx_err),'(2(a,i6),5(/2(a,i6)))') 
     - ' **     bin_tuplen = ',bin_tuplen,   '      tuplen = ',tuplen,
     - ' **     bin_tupdim = ',bin_tupdim,   '      tupdim = ',tupdim,
     - ' **    bin_seqsdim = ',bin_seqsdim,  ' old_seqsdim = ',old_seqsdim,
     - ' **     bin_basdim = ',bin_basdim,   '  old_basdim = ',old_basdim,
     - ' **    bin_sidswid = ',bin_sidswid,  ' old_sidswid = ',old_sidswid,
     - ' **    bin_lidswid = ',bin_lidswid,  ' old_lidswid = ',old_lidswid

            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -          'You may recompile the program, or use the easier way: ' //LF//
     -          'regenerate the binary file by deleting it' )
            STOP

         END IF
C
C Get memory and load arrays with binary read
C
         CALL get_memory (old_sidsptr, old_seqsmax * old_sidswid, 'old_sids', program_ID)
         CALL get_memory (old_lidsptr, old_seqsmax * old_lidswid, 'old_lids', program_ID)
         CALL get_memory (old_alibgsptr, old_seqsmax * 4, 'old_alibgs', program_ID)
         CALL get_memory (old_alindsptr, old_seqsmax * 4, 'old_alinds', program_ID)
         CALL get_memory (old_compsptr, old_seqsmax * 4, 'old_comps', program_ID)
         CALL get_memory (old_onumsptr, old_seqsmax * 4, 'old_onums', program_ID)
         CALL get_memory (old_tbegsptr, tupdim * 4, 'old_tbegs', program_ID)
         CALL get_memory (old_tblptr, old_tblmax * 2, 'old_tbl', program_ID)
         CALL get_memory (old_alisptr, old_abyts, 'old_alis', program_ID)

         CALL sa_bin_io ('READ',IO_luns(ndx_dbbin),1,old_seqsmax,old_sids)  
         CALL sa_bin_io ('READ',IO_luns(ndx_dbbin),1,old_seqsmax,old_lids)  
         CALL i4_bin_io ('READ',IO_luns(ndx_dbbin),1,old_seqsmax,old_alibgs)
         CALL i4_bin_io ('READ',IO_luns(ndx_dbbin),1,old_seqsmax,old_alinds)
         CALL r4_bin_io ('READ',IO_luns(ndx_dbbin),1,old_seqsmax,old_comps) 
         CALL i4_bin_io ('READ',IO_luns(ndx_dbbin),1,old_seqsmax,old_onums)
         CALL i4_bin_io ('READ',IO_luns(ndx_dbbin),0,tupdim-1,old_tbegs)
         CALL i2_bin_io ('READ',IO_luns(ndx_dbbin),1,old_tblmax,old_tbl)
         CALL by_bin_io ('READ',IO_luns(ndx_dbbin),1,old_abyts,old_alis)

         GO TO 200

      END IF
C
C -------------------------------------------------------
C Genbank input format (concatenated entries, ASCII only)
C -------------------------------------------------------
C
      IF (IO_flags(ndx_dbgbk)) THEN
C
C Input multi-entry ASCII genbank file. The file is read twice, first find out
C how much memory is needed, allocate and then load. I wish I knew how to keep
C adding to a memory allocation. This is very slow, but the binary read is used
C most of the time 
C 
         IO_luns(ndx_dbgbk) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_dbgbk),
     -         FILE = IO_fnms(ndx_dbgbk),
     -       STATUS = 'OLD')
C
C Find number of sequences, Watson-Crick bases (other than AaUuTtGgCc are 
C ignored) and characters,
C
         CALL feel_gbk ( IO_luns(ndx_dbgbk), WC_base, 50, 
     -                   old_seqsmax, old_abyts, old_sbyts )
C         print*,'after feel gbk:'
C         print*,'   old_seqsmax, old_abyts, old_sbyts = ',old_seqsmax, old_abyts, old_sbyts
         
C
C Stop program if no valid sequences
C
         IF (old_seqsmax.LT.1) THEN
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -          'Error: No valid entries in ' // 
     -          IO_fnms(ndx_dbgbk)(str_beg(IO_fnms(ndx_dbgbk)):
     -                             str_end(IO_fnms(ndx_dbgbk))) )
            STOP
         END IF
C
C Stop program if the sequences overflow dynamic memory limits,
C
         IF (.NOT. ( 
     -            old_seqsmax .LE. old_seqsdim
     -        .AND. old_sbyts .LE. old_seqsdim * old_basdim
     -        .AND. old_abyts .LE. old_seqsdim * old_basdim
     -             ) ) THEN

            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -          'Error: Needed memory for ' // 
     -           IO_fnms(ndx_dbgbk)(str_beg(IO_fnms(ndx_dbgbk)):
     -                              str_end(IO_fnms(ndx_dbgbk))) //
     -          ' overflows dynamic limits :')

            WRITE (IO_luns(ndx_err),'(2(a,i9)/2(a,i9)/2(a,i9))')
     -         ' **      old_seqsmax = ',old_seqsmax,
     -               '   old_seqsdim = ',old_seqsdim,
     -         ' **      old_sbyts   = ',old_sbyts,
     -               '   old_bytdim  = ',old_bytdim,
     -         ' **      old_abyts   = ',old_abyts,
     -               '   old_bytdim  = ',old_bytdim

            CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err),
     -          'Solution: Increase dynamic maxima in params.inc and recompile')
            STOP

         END IF
C
C Allocate sequence related memory for
C
C  old_sids   :  Short-ID's for each sequence
C  old_lids   :  Full names for each sequence
C  old_seqs   :  Holds all sequences as one long composite sequence
C  old_seqbgs :  Pointers to where in old_seqs each sequence begins
C  old_seqnds :  Pointers to where in old_seqs each sequence ends
C  old_alis   :  Holds all aligned sequences as one long composite 
C  old_alibgs :  Pointers to where in old_alis each sequence begins
C  old_alinds :  Pointers to where in old_alis each sequence ends
C
         CALL get_memory (old_sidsptr, old_seqsmax * old_sidswid, 'old_sids', program_ID)
         CALL get_memory (old_lidsptr, old_seqsmax * old_lidswid, 'old_lids', program_ID)
         CALL get_memory (old_seqsptr, old_sbyts, 'old_seqs', program_ID)
         CALL get_memory (old_seqbgsptr, old_seqsmax * 4, 'old_seqbgs', program_ID)
         CALL get_memory (old_seqndsptr, old_seqsmax * 4, 'old_seqnds', program_ID)
         CALL get_memory (old_alisptr, old_abyts, 'old_alis', program_ID)
         CALL get_memory (old_alibgsptr, old_seqsmax * 4, 'old_alibgs', program_ID)
         CALL get_memory (old_alindsptr, old_seqsmax * 4, 'old_alinds', program_ID)
C
C Read Genbank alignment
C
         REWIND (UNIT = IO_luns(ndx_dbgbk))

         CALL get_gbk (IO_luns(ndx_dbgbk), WC_base, old_seqsmax, 50,
     -                 old_seqs, old_seqbgs, old_seqnds,
     -                 old_alis, old_alibgs, old_alinds,
     -                 old_sids, old_lids)

C         print*,' after read genbank, old_seqsmax = ',old_seqsmax
C         STOP 'stopped after get_gbk'

         CLOSE (UNIT = IO_luns(ndx_dbgbk))

         GO TO 100

      END IF
C
C -----------------------
C KEY/TABLE input format 
C -----------------------
C
      IF ( IO_flags(ndx_dbkey) .AND. IO_flags(ndx_dbtbl) ) THEN

         IO_luns(ndx_dbkey) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_dbkey),
     -         FILE = IO_fnms(ndx_dbkey),
     -       STATUS = 'OLD')

         CALL read_lines (IO_luns(ndx_dbkey), keylines)

         REWIND (IO_luns(ndx_dbkey))

         IO_luns(ndx_dbtbl) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_dbtbl),
     -         FILE = IO_fnms(ndx_dbtbl),
     -       STATUS = 'OLD')

         CALL read_lines (IO_luns(ndx_dbtbl), tbllines)

         IF (keylines.GT.0 .AND. tbllines.GT.0 .AND. 
     -       keylines.EQ.tbllines) THEN
            old_seqsmax = keylines
         ELSE
            WRITE (intstr1,*) keylines
            WRITE (intstr2,*) tbllines
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -         'Error: Entries in ' // 
     -          IO_fnms(ndx_dbkey)(str_beg(IO_fnms(ndx_dbkey)):
     -                             str_end(IO_fnms(ndx_dbkey))) // 
     -         ': ' // intstr1(str_beg(intstr1):str_end(intstr1)) //LF//
     -         '       Entries in ' // 
     -          IO_fnms(ndx_dbtbl)(str_beg(IO_fnms(ndx_dbtbl)):
     -                             str_end(IO_fnms(ndx_dbtbl))) // 
     -         ': ' // intstr2(str_beg(intstr2):str_end(intstr2))  )
            STOP
         END IF

         REWIND (IO_luns(ndx_dbtbl))

         old_abyts = file_size (IO_luns(ndx_dbtbl)) 
C
C Allocate sequence related memory for
C
C  old_sids   :  Short-ID's for each sequence
C  old_lids   :  Full names for each sequence
C  old_seqs   :  Holds all IUB sequences as one long byte array
C  old_seqbgs   :  Pointers to where in old_seqs each sequence begins
C  old_seqnds   :  Pointers to where in old_seqs each sequence ends
C
         CALL get_memory (old_sidsptr, old_seqsmax * old_sidswid, 'old_sids', program_ID)
         CALL get_memory (old_lidsptr, old_seqsmax * old_lidswid, 'old_lids', program_ID)

c         old_lids(1:old_seqsmax*old_lidswid) = ' '
         
         CALL get_keys (IO_luns(ndx_dbkey),old_seqsmax,old_sids,1,old_seqsmax,1,' ')
         
         CALL get_memory (old_seqsptr, old_abyts,       'old_seqs', program_ID)
         CALL get_memory (old_seqbgsptr, old_seqsmax * 4, 'old_seqbgs', program_ID)
         CALL get_memory (old_seqndsptr, old_seqsmax * 4, 'old_seqnds', program_ID)
         CALL get_memory (old_alisptr, old_abyts,       'old_alis', program_ID)
         CALL get_memory (old_alibgsptr, old_seqsmax * 4, 'old_alibgs', program_ID)
         CALL get_memory (old_alindsptr, old_seqsmax * 4, 'old_alinds', program_ID)
C
C Extract both aligned and unaligned sequences with 50 or more bases
C
         CALL get_seqlst (IO_luns(ndx_dbtbl), WC_base, 50, 
     -                    old_seqsmax, old_abyts, 
     -                    old_seqs, old_seqbgs, old_seqnds,
     -                    old_alis, old_alibgs, old_alinds )

         old_sbyts = old_seqnds(old_seqsmax)
         
         CLOSE ( UNIT = IO_luns(ndx_dbkey) )
         CLOSE ( UNIT = IO_luns(ndx_dbtbl) )

         GO TO 100

      END IF
C
C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C                                                                           
C     Set up "DATABASE" sequences for rapid search (skip if binary already) 
C                                                                           
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
 100  CONTINUE
C
C  old_comps  :  Percent occurrence of 'missing residue character'
C
      CALL get_memory (old_compsptr, old_seqsmax * 4, 'old_comps', program_ID)
C
C Count the number of characters that symbolize missing residues
C
      missch = ICHAR ('.')

      DO iseq = 1, old_seqsmax
         imis = 0
         DO ires = old_alibgs(iseq), old_alinds(iseq)
            IF ( old_alis (ires) .EQ. missch ) imis = imis + 1
         END DO
         old_comps (iseq) = 100.0 - 100 * REAL (imis) 
     -        / (old_alinds(iseq) - old_alibgs(iseq) + 1)
      END DO
C     
C Print warnings of unusual characters in database alignment: Other than
C IUB symbols plus '.', '~' and '-',
C     
      CALL check_seqs (old_alinds(old_seqsmax), old_seqsmax,
     -     'ABCDGHKMNRSTUVWYabcdghkmnrstuvwy.~-', old_alis,
     -     1, old_seqsmax, old_alibgs, old_alinds, old_sids)
C
C Translate old_seqs to A = 0, U = 1, G = 2, C = 3
C       
      CALL To_Code_4 (old_seqs,1,old_sbyts,1,old_sbyts,old_seqs)
C     
C Create linked list
C     
      CALL get_memory (old_olibgsptr, tupdim * 4, 'old_olibgs', program_ID)
      CALL get_memory (old_lnklstptr, old_sbyts * 4, 'old_lnklst', program_ID)
      
      CALL make_lnk_lst ( 4, tuplen, tupdim, old_seqsmax, old_sbyts,
     -     old_seqs, 1, old_seqsmax, old_seqbgs, old_seqnds,
     -     old_olibgs, old_lnklst )
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
      ibyt = ( old_sbyts + tupdim ) * 2
      CALL get_memory (old_tblptr, ibyt, 'old_tbl', program_ID)
      CALL get_memory (old_tbegsptr, tupdim * 4, 'old_tbegs', program_ID)
      CALL get_memory (old_onumsptr, old_seqsmax * 4, 'old_onums', program_ID)
      
      CALL make_oli_tables ( tupdim, old_seqsmax, old_sbyts, ibyt,
     -                       old_seqnds, old_olibgs, old_lnklst, 
     -                       old_tblmax, old_tbl, old_tbegs, old_onums )

      CALL FREE (old_olibgsptr)
      CALL FREE (old_lnklstptr)
      
      CALL FREE (old_seqbgsptr)
      CALL FREE (old_seqndsptr)
      
C      print*,' after setup sequences for search'
C
C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C                                                          
C     "DATABASE" binary output (if requested)              
C                                                          
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
      IF (cl_outbinflag) THEN

         IO_luns(ndx_dbbin) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_dbbin),
     -         FILE = IO_fnms(ndx_dbbin),
     -       STATUS = 'NEW',
     -         FORM = 'UNFORMATTED')
C     -       BUFFER = 1000000)           ! Absoft
C     -      FILEOPT = 'BUFFER=1000000') ! SUN specific
C     -    BLOCKSIZE = 1000000)          ! VAX specific
C     
C Write program-ID and version-ID into file, so it will only read its own files
C
         WRITE (IO_luns(ndx_dbbin)) program_ID
         WRITE (IO_luns(ndx_dbbin)) version_ID
C     
C Parameters for comparison when binary file is read next time
C
         WRITE (IO_luns(ndx_dbbin)) tuplen, 
     -                     tupdim, old_seqsdim, old_basdim,
c     -                        old_sbyts, old_abyts,
     -                       old_abyts,
     -                       old_sidswid, old_lidswid,
     -                       old_seqsmax, old_tblmax
C
C Save arrays using binary write
C
         CALL sa_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_seqsmax,old_sids) ! string array
         CALL sa_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_seqsmax,old_lids) ! string array
         CALL i4_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_seqsmax,old_alibgs)
         CALL i4_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_seqsmax,old_alinds)
         CALL r4_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_seqsmax,old_comps) ! real array
         CALL i4_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_seqsmax,old_onums)
         CALL i4_bin_io ('WRITE',IO_luns(ndx_dbbin),0,tupdim-1,old_tbegs)
         CALL i2_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_tblmax,old_tbl)
         CALL by_bin_io ('WRITE',IO_luns(ndx_dbbin),1,old_abyts,old_alis) ! byte array
         
       END IF
C
C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C                                                          
C     Read input sequences in different formats            
C                                                          
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
 200  CONTINUE
C
C -----------------------
C KEY/TABLE input format 
C -----------------------
C
      IF ( IO_flags(ndx_inkey) .AND. IO_flags(ndx_intbl) ) THEN

         IO_luns(ndx_inkey) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_inkey),
     -         FILE = IO_fnms(ndx_inkey),
     -       STATUS = 'OLD')

         CALL read_lines (IO_luns(ndx_inkey), keylines)

         REWIND (IO_luns(ndx_inkey))

         IO_luns(ndx_intbl) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_intbl),
     -         FILE = IO_fnms(ndx_intbl),
     -       STATUS = 'OLD')

         CALL read_lines (IO_luns(ndx_intbl), tbllines)

         REWIND (IO_luns(ndx_intbl))

         IF (keylines.GT.0 .AND. tbllines.GT.0 .AND. 
     -       keylines.EQ.tbllines) THEN
            new_seqsmax = keylines
         ELSE
            WRITE (intstr1,*) keylines
            WRITE (intstr2,*) tbllines
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -         'Error: Entries in ' // 
     -          IO_fnms(ndx_inkey)(str_beg(IO_fnms(ndx_inkey)):
     -                             str_end(IO_fnms(ndx_inkey))) // 
     -         ': ' // intstr1(str_beg(intstr1):str_end(intstr1)) //LF//
     -         '       Entries in ' // 
     -          IO_fnms(ndx_intbl)(str_beg(IO_fnms(ndx_intbl)):
     -                             str_end(IO_fnms(ndx_intbl))) // 
     -         ': ' // intstr2(str_beg(intstr2):str_end(intstr2))  )
            STOP
         END IF
C
C Allocate sequence related memory for
C
C  new_sids   :  Short-ID's for each sequence
C  new_seqs   :  Holds all IUB sequences as one long byte array
C  new_begs   :  Pointers to where in new_seqs each sequence begins
C  new_ends   :  Pointers to where in new_seqs each sequence ends
C
         CALL get_memory (new_sidsptr, new_seqsmax * new_sidswid, 'new_sids', program_ID)
         
         CALL get_keys (IO_luns(ndx_inkey),new_seqsmax,new_sids,1,new_seqsmax,1,' ')
         
         new_abyts = file_size (IO_luns(ndx_intbl)) 

         CALL get_memory (new_seqsptr, new_abyts, 'new_seqs', program_ID)
         CALL get_memory (new_begsptr, new_seqsmax * 4, 'new_begs', program_ID)
         CALL get_memory (new_endsptr, new_seqsmax * 4, 'new_ends', program_ID)
         
         CALL get_tbl (IO_luns(ndx_intbl), IUB_base, 50, 
     -                new_seqsmax, new_abyts, 
     -                new_seqs, new_begs, new_ends )

         CLOSE ( UNIT = IO_luns(ndx_inkey) )
         CLOSE ( UNIT = IO_luns(ndx_intbl) )

         GO TO 300

      END IF
C
C
C ---------------------
C GENBANK input format
C ---------------------
C
C First load whole Genbank file of new sequences as a CHARACTER*80 text array
C in memory (if specified).  
C
      IF (IO_flags(ndx_ingbk)) THEN

C         print*,'at genbank input format'

         IO_luns(ndx_ingbk) = lun_alloc (ndx_max, err_cod)

         OPEN (UNIT = IO_luns(ndx_ingbk),
     -         FILE = IO_fnms(ndx_ingbk),
     -       STATUS = 'OLD')
C
C Find number of lines first to know how much memory to reserve, then fill 
C text array. This takes extra memory, but is somewhat faster and no annotation
C will disappear. 
C
         CALL read_lines ( IO_luns(ndx_ingbk), new_lines )
         CALL get_memory ( new_textptr, new_lines * 80, 'new_text', program_ID)         
         REWIND ( UNIT = IO_luns(ndx_ingbk) ) 
         CALL read_text ( IO_luns(ndx_ingbk), new_lines, new_text )         

         CLOSE ( UNIT = IO_luns(ndx_ingbk) )
C
C Then parse the text array:  Create pointers to starts/ends of annotations 
C in the text array, and to the sequences in the byte array. Determine number 
C of sequences (new_seqsmax) and IUB bases (new_abyts) and only include 
C sequences that are more than 50 IUB_bases long,
C
         CALL feel_gbtxt ( new_lines, new_text,
     -                     IUB_base, 50, 
     -                     new_seqsmax, new_abyts )
C     
C Stop program if no valid sequences,
C
         IF ( new_seqsmax.LT.1 ) THEN
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -           'Error: No valid IUB entries in ' // 
     -           IO_fnms(ndx_ingbk)(str_beg(IO_fnms(ndx_ingbk)):
     -                              str_end(IO_fnms(ndx_ingbk))) )
            STOP
         END IF
C     
C Stop program if the sequences overflow dynamic memory limits,
C
         IF (.NOT. ( 
     -           new_seqsmax .LE. new_seqsdim
     -       .AND. new_abyts .LE. new_seqsdim * new_basdim
     -      ) ) THEN
         
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: Needed memory for ' // 
     -            IO_fnms(ndx_ingbk)(str_beg(IO_fnms(ndx_ingbk)):
     -                               str_end(IO_fnms(ndx_ingbk))) //
     -           ' overflows dynamic limits :')
         
            WRITE (IO_luns(ndx_err),'(2(a,i9)/2(a,i9))')
     -        ' **      new_seqsmax = ',new_seqsmax,
     -              '   new_seqsdim = ',new_seqsdim,
     -        ' **      new_abyts   = ',new_abyts,
     -              '   new_bytdim  = ',new_bytdim
         
            CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err),
     -           'Solution: Increase dynamic maxima in params.inc and recompile')
            STOP
         
         END IF
C
C Allocate sequence related memory for
C
C  new_seqs   :  Holds all IUB sequences as one long byte array
C  new_abgs   :  Pointers to where in new_text each annotation starts
C  new_ands   :  Pointers to where in new_text each annotation ends
C  new_begs   :  Pointers to where in new_seqs each sequence begins
C  new_ends   :  Pointers to where in new_seqs each sequence ends
C  new_sids   :  Short-ID's for each sequence
C  new_lids   :  Full names for each sequence
C
         CALL get_memory (new_seqsptr, new_abyts, 'new_seqs', program_ID)
         CALL get_memory (new_abgsptr, new_seqsmax * 4, 'new_abgs', program_ID)
         CALL get_memory (new_andsptr, new_seqsmax * 4, 'new_ands', program_ID)
         CALL get_memory (new_begsptr, new_seqsmax * 4, 'new_begs', program_ID)
         CALL get_memory (new_endsptr, new_seqsmax * 4, 'new_ends', program_ID)
         CALL get_memory (new_sidsptr, new_seqsmax * new_sidswid, 'new_sids', program_ID)
         CALL get_memory (new_lidsptr, new_seqsmax * new_lidswid, 'new_lids', program_ID)
C     
C Load new unaligned Genbank sequences
C
         CALL load_gbtxt ( new_lines, new_text, IUB_base, 50, new_seqsmax, new_abyts,
     -                     new_seqs, new_begs, new_ends, new_abgs, new_ands,
     -                     new_sids, new_lids )

c$$$      DO i = 1, new_seqsmax
c$$$         do j = new_abgs(i), new_ands(i)
c$$$            write (6,'(a)') new_text(j)
c$$$         end do
c$$$      end do
c$$$      stop

         GO TO 300

      END IF

 300  CONTINUE
C      print*,'after 300'
C
C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C                                                                           
C     Optional input sequence modification before search
C                                                                           
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
C Do DNA -> RNA conversion, or vice versa,
C
      IF (cl_t2uflag) THEN
         
         ich1 = ICHAR('T')
         ich2 = ICHAR('U')
         CALL seq_replace ( new_seqs, 1, new_ends(new_seqsmax), ich1, ich2, new_seqs )
         
         ich1 = ICHAR('t')
         ich2 = ICHAR('u')
         CALL seq_replace ( new_seqs, 1, new_ends(new_seqsmax), ich1, ich2, new_seqs )
         
      ELSE IF (cl_u2tflag) THEN
         
         ich1 = ICHAR('U')
         ich2 = ICHAR('T')
         CALL seq_replace ( new_seqs, 1, new_ends(new_seqsmax), ich1, ich2, new_seqs )
         
         ich1 = ICHAR('u')
         ich2 = ICHAR('t')
         CALL seq_replace ( new_seqs, 1, new_ends(new_seqsmax), ich1, ich2, new_seqs )
         
      END IF
C     
C Change to upper or lower case if requested,
C
      IF (cl_locaseflag) THEN
         
         CALL seq_locase ( new_seqs, 1, new_ends(new_seqsmax), new_seqs )
         
      ELSE IF (cl_upcaseflag) THEN
         
         CALL seq_upcase ( new_seqs, 1, new_ends(new_seqsmax), new_seqs )
         
      END IF

C      print*,'after sequence conversion'
c$$$C
c$$$C
c$$$C --------------------------------------
c$$$C                                                                             
c$$$
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_outgbk, IO_flags(ndx_outgbk)
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_outkey, IO_flags(ndx_outkey)
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_outsab, IO_flags(ndx_outsab)
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_outtbl, IO_flags(ndx_outtbl)
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_outhum, IO_flags(ndx_outhum)
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_outrul, IO_flags(ndx_outrul)
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_log, IO_flags(ndx_log)
c$$$      WRITE (6,'(i6, 1x, l1)') ndx_err, IO_flags(ndx_err)
C
C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C                                                                             
C     Major program loop                                                      
C                                                                             
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C
C Assign and open logical units for various outputs.  'Assign_lun' listens
C to which channels are busy and picks a free one; that way there is never
C an IO collision. 
C                                                                             
C -OUT_GENBANK, -OUT_KEYS, -OUT_SABS, -OUT_RULER, -OUT_TABLE, -OUT_HUMANE,
C -OUT_SERV, -LOGS, -ERRORS
C
      IF (IO_flags(ndx_outgbk)) CALL assign_lun (IO_fnms(ndx_outgbk),
     -                       IO_luns(ndx_outgbk),IO_flags(ndx_outgbk))
      IF (IO_flags(ndx_outkey)) CALL assign_lun (IO_fnms(ndx_outkey),
     -                       IO_luns(ndx_outkey),IO_flags(ndx_outkey))
      IF (IO_flags(ndx_outsab)) CALL assign_lun (IO_fnms(ndx_outsab),
     -                       IO_luns(ndx_outsab),IO_flags(ndx_outsab))
      IF (IO_flags(ndx_outrul)) CALL assign_lun (IO_fnms(ndx_outrul),
     -                       IO_luns(ndx_outrul),IO_flags(ndx_outrul))
      IF (IO_flags(ndx_outtbl)) CALL assign_lun (IO_fnms(ndx_outtbl),
     -                       IO_luns(ndx_outtbl),IO_flags(ndx_outtbl))
      IF (IO_flags(ndx_outhum)) CALL assign_lun (IO_fnms(ndx_outhum),
     -                       IO_luns(ndx_outhum),IO_flags(ndx_outhum))
      IF (IO_flags(ndx_outsrv)) CALL assign_lun (IO_fnms(ndx_outsrv),
     -                       IO_luns(ndx_outsrv),IO_flags(ndx_outsrv))
      IF (IO_flags(ndx_log)) CALL assign_lun (IO_fnms(ndx_log),
     -                       IO_luns(ndx_log),IO_flags(ndx_log))

C
C Memory for similarity scores and their indices,
C
      CALL get_memory ( new_sabsptr, old_seqsmax * 4, 'new_sabs', program_ID ) 
      CALL get_memory ( new_ndcsptr, old_seqsmax * 4, 'new_ndcs', program_ID ) 
C
C Program loop
C      
C      print*,'new_seqsmax'

      DO inew = 1, new_seqsmax

C         print*,'inew = ',inew
C
C Calculate similarity values (~Sab) for each sequence to be aligned; this is 
C the essence of what SIMILARITY_RANK does,
C
         CALL calc_sabs ( old_seqsmax, old_basdim, tupdim, old_tblmax, 
     -                    new_begs(inew), new_ends(inew), new_seqs, 
     -                    tuplen, old_onums, old_tbegs, old_tbl, new_sabs )
C         print*,'after calc_sabs'
C
C Exclude matches with sequences less complete than cl_completepct,
C 
         DO iseq = 1, old_seqsmax 
            IF ( old_comps(iseq).LT.cl_completepct ) new_sabs(iseq) = 0.0
         END DO
C
C Sort Sab's, but keep their original indices in new_ndcs,
C
         DO iseq = 1, old_seqsmax
            new_ndcs (iseq) = iseq
         END DO

         CALL r4_shellsort ( new_sabs, new_ndcs, old_seqsmax )
C         print*,'after shellsort'
C
C Use as template the most similar sequence unless that sequence is a 
C poor match,
C
         IF ( new_sabs(1).GT.0 ) THEN
            tmplndx = new_ndcs (1)
         ELSE
            CALL write_message ( IO_luns(ndx_err),IO_fnms(ndx_err),
     -         'Error: Unable to find a template for new entry ' // 
     -          new_sids(inew)(str_beg(new_sids):str_end(new_sids)) // '.'  //LF//
     -         'Possible explanations: No sequence satisfies the -complete' //LF//
     -         'option, your sequence shares no oligos with any sequence'   //LF//
     -         'in the specified RDP alignment, or maybe it is even a '     //LF// 
     -         'program error; then please mail your data to ' // author_ID // ',' //LF//
     -         email_ID )
            GO TO 500
         END IF
C
C Make copies (x_seq and y_seq) of valid base symbols in the original template 
C sequence and unaligned sequence.  Although redundant, that makes it possible
C to ensure that no sequence has been changed during alignment.
C
         ibyt = old_alinds (tmplndx) - old_alibgs (tmplndx) + 1
         CALL get_memory ( x_seq_ptr, ibyt, 'x_seq', program_ID )

         x_seqlen = 0
         DO ipos = old_alibgs(tmplndx), old_alinds(tmplndx)
            IF ( IUB_base(old_alis(ipos)) ) THEN
               x_seqlen = x_seqlen + 1
               x_seq (x_seqlen) = old_alis (ipos)
            END IF
         END DO

         ibyt = new_ends (inew) - new_begs (inew) + 1
         CALL get_memory ( y_seq_ptr, ibyt, 'y_seq', program_ID )

         y_seqlen = 0
         DO ipos = new_begs(inew), new_ends(inew)
            y_seqlen = y_seqlen + 1
            y_seq (y_seqlen) = new_seqs (ipos)
         END DO
C
C These are tracer arrays that are filled during alignment; they have
C the same length as the sequences.  The alignment can then be 
C reconstructed,
C
C  x_alinks   0000111111000001111110000...
C                //////       \\\\\\
C  y_alinks   0011111100000000011111100...
C
C
         CALL get_memory (x_alinks_ptr, x_seqlen, 'x_alinks', program_ID)
         CALL get_memory (y_alinks_ptr, y_seqlen, 'y_alinks', program_ID)
C
C Align x_seq with y_seq.  Dont look into this routine, your brain 
C may be damaged; there are spaghetti at places. 
C
C         print*,'before seqmatch'
         CALL seq_match ( 4, 6, 4 ** 6,
     -                   1, x_seqlen, x_seq, x_alinks,
     -                   1, y_seqlen, y_seq, y_alinks,
     -                   hit_flag )
C         print*,'after seqmatch'
C
C Skip to next entry if no matches
C
         IF ( .NOT.hit_flag ) THEN
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -           'Warning: Entry ' // new_sids(inew)
     -           (str_beg(new_sids(inew)):str_end(new_sids(inew))) // 
     -           ' could not be aligned reliably'                  // LF //
     -           'with any sequences in the specified alignment'   // LF // 
     -           'No output produced' )
            CALL FREE (x_seq_ptr)
            CALL FREE (y_seq_ptr)
            CALL FREE (x_alinks_ptr)
            CALL FREE (y_alinks_ptr)
            GO TO 500
         END IF
C
C Trace and load aligned sequences into x_ali, y_ali and a_mrk, which
C marks certain and uncertain regions. This may seem a little redundant
C but it removes arbitrary limits, and permits better reuse of functions.
C
         CALL feel_outsiz ( 1, x_seqlen, x_alinks, 
     -                      1, y_seqlen, y_alinks, aliend )
C         print*,'after outsiz'

         CALL get_memory (x_ali_ptr, aliend, 'x_ali', program_ID)
         CALL get_memory (y_ali_ptr, aliend, 'y_ali', program_ID)
         CALL get_memory (a_mrk_ptr, aliend, 'a_mrk', program_ID)

         CALL load_outali ( 1, x_seqlen, x_seq, x_alinks,
     -                      1, y_seqlen, y_seq, y_alinks, 
     -                      1, aliend, x_ali, y_ali, a_mrk )
C         print*,'after load_outali'

         CALL FREE (x_seq_ptr)
         CALL FREE (y_seq_ptr)
         CALL FREE (x_alinks_ptr)
         CALL FREE (y_alinks_ptr)
C
C Given x_ali, y_ali, a_mrk (of size aliend):  Produce final output 
C arrays x_gapali, y_gapali and a_gapmrk.  
C
         IF (.NOT.cl_dbgapsflag) THEN
C 
C If -nodbgaps option, copy all three arrays into final output arrays
C without change
C
            CALL get_memory (x_gapali_ptr, aliend, 'x_gapali', program_ID)
            CALL get_memory (y_gapali_ptr, aliend, 'y_gapali', program_ID)
            CALL get_memory (a_gapmrk_ptr, aliend, 'a_gapmrk', program_ID)
            
            DO i = 1, aliend
               x_gapali(i) = x_ali(i)
               y_gapali(i) = y_ali(i)
               a_gapmrk(i) = a_mrk(i)
            END DO
            x_outlen = aliend
            y_outlen = aliend

         ELSE
C 
C Gap reinsertion
C
            ibyt = old_alinds(tmplndx) - old_alibgs(tmplndx) + 1 
     -           + MAX ( aliend-x_seqlen, aliend-y_seqlen )

            CALL get_memory (x_gapali_ptr, ibyt, 'x_gapali', program_ID)

            x_outlen = 0
            DO i = old_alibgs(tmplndx), old_alinds(tmplndx)
               x_outlen = x_outlen + 1
               x_gapali(x_outlen) = old_alis(i)
            END DO

C            DO i = 1,x_outlen
C               garbstr(i:i) = CHAR(x_gapali(i))
C            END DO
C            WRITE (6,'(a)') garbstr(1:str_end(garbstr))

            CALL get_memory (y_gapali_ptr, ibyt, 'y_gapali', program_ID)
            CALL get_memory (a_gapmrk_ptr, ibyt, 'a_gapmrk', program_ID)

C           print*,'before gap_reins'
            CALL gap_reins ( x_outlen, x_gapali, 
     -                       aliend, x_ali, y_ali, a_mrk, 
     -                       ibyt, y_gapali, a_gapmrk, y_outlen )
C            print*,'after gap_reins'
            
            DO i = x_outlen+1, y_outlen
               x_gapali(i) = ICHAR(' ')
            END DO
               
         END IF

         CALL FREE (x_ali_ptr)
         CALL FREE (y_ali_ptr)
         CALL FREE (a_mrk_ptr)
C
C Compare IUB-bases in input and output sequences, and write error if 
C different.  This is so that I can sleep at night. 
C
C         CALL safe_compare ( 1, new_ends(inew)-new_begs(inew)+1, 
C     -                       new_seqs(new_begs(inew)),
C     -                       1, y_outlen, y_gapali, IUB_base, err_cod )
         CALL safe_compare ( new_begs(inew), new_ends(inew), new_seqs,
     -                       1, y_outlen, y_gapali, 
     -                       IUB_base, err_cod )

         IF (err_cod.NE.0) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -         'Error: Entry ' // new_sids(inew)
     -         (str_beg(new_sids(inew)):str_end(new_sids(inew))) // 
     -         ' not written, output sequence != input sequence' //LF//
     -         'It''s possibly a program error, contact ' // author_ID // ',' //LF//
     -         email_ID)
            GO TO 500
         END IF
C
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C                                                                           
C     Output 
C                                                                           
C |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
C 
C First determine number of template keys and sabs to put out; cl_templates 
C sets that, but there may be fewer,
C
         iseq = MIN (cl_templates, old_seqsmax)
         out_tmpl = 0

         DO WHILE ( out_tmpl+1.LE.iseq .AND. new_sabs(out_tmpl+1).GT.0 ) 
            out_tmpl = out_tmpl + 1
         END DO
C
C OUT_GENBANK  (this should probably disappear).  
C
         IF ( IO_flags(ndx_outgbk) ) THEN
C
C Date stamp,
C
            CALL fdate (dat_str)
            READ (dat_str(9:10),*) date
            CALL seq_upcase ( dat_str(5:7),1,3,month )
            READ (dat_str(21:24),*) year
C
C Template sequence, if asked for,
C
            IF ( cl_templates.GT.0 ) THEN
               
               WRITE (hdrstr,'(a,i4,a,i2.2,a,i4,a)')
     -            'LOCUS       ' // old_sids(tmplndx)(1:10) // '   ', 
     -                x_outlen, ' bp   rRNA' //
     -            '             RNA       ', date, '-' // month // 
     -                '-', year, '       ' //LF//
     -            'DEFINITION  ' // old_lids(tmplndx)(1:
     -                 MAX(str_end(old_lids(tmplndx)),1)) //LF//
     -            'COMMENTS    ' // 'Your submitted sequence was aligned ' //
     -                'against this most similar' //LF//
     -                '            RDP sequence'  //LF// 
     -            'ORIGIN'

               CALL write_gbk_ent ( IO_luns(ndx_outgbk),
     -                              1, 1, 1, hdrstr,
     -                              x_outlen, 1, x_outlen, x_gapali )
            END IF
C
C Then the aligned sequence in Genbank format, 
C
            CALL write_gbk_ent ( IO_luns(ndx_outgbk),
     -                           new_ands(new_seqsmax), new_abgs(inew),
     -                           new_ands(inew), new_text, 
     -                           y_outlen, 1, y_outlen, y_gapali )
C
C Finally Genbank formatted entry that annotates problem spots during 
C the alignment,
C
            WRITE (probid,'(i10)') inew
            DO i = 1,10
               IF (probid(i:i).EQ.' ') probid(i:i) = CHAR (amby_ch)
            END DO

            WRITE (pctstr,'(f7.3)') new_sabs(1) 

            WRITE (hdrstr,'(a,i4,a,i2.2,a,i4,a)') 
     -         'LOCUS       ' // probid // '   ', y_outlen, 
     -             ' bp    NNN             NNN' // 
     -             '       ', date, '-' // month // '-', year, '       '         //LF//
     -         'DEFINITION  ' // CHAR(ambx_ch) // '''s and ' // CHAR(amby_ch) //
     -                          'mark uncertain alignment' //LF//
     -         'COMMENTS    ' // new_sids(inew)(str_beg(new_sids(inew)):
     -                                          str_end(new_sids(inew))) // 
     -             ' (submitted) highest S_ab value ' // 
     -             pctstr(str_beg(pctstr):str_end(pctstr)) // ' versus ' // 
     -             old_sids(tmplndx)(str_beg(old_sids(tmplndx)):
     -                               str_end(old_sids(tmplndx)))             //LF//
     -             '            Note: This S_ab value may differ slightly from ' // 
     -             'those returned' //LF// '            by SIMILARITY_RANK in '  // 
     -             'case the submitted sequence contains' //LF// 
     -             '            ambiguity codes.'                           //LF//
     -             '            This output is produced by program ' //program_ID//
     -             ' (v ' // version_ID // '), written by'                  //LF//
     -             '            '//author_ID//', '//email_ID   //LF//
     -         'ORIGIN'

            CALL write_gbk_ent ( IO_luns(ndx_outgbk),
     -                           1, 1, 1, hdrstr,
     -                           y_outlen, 1, y_outlen, a_gapmrk )
         END IF
C         print*,'after write genbank'
C
C OUT_KEYS 
C 
         IF ( IO_flags(ndx_outkey) ) THEN
            WRITE (IO_luns(ndx_outkey),'(a,$)') new_sids(inew)
     -            (str_beg(new_sids(inew)):str_end(new_sids(inew)))
            DO iseq = 1, out_tmpl
               jseq = new_ndcs(iseq) 
               WRITE (IO_luns(ndx_outkey),'(1x,a,$)') old_sids(jseq) 
     -               (str_beg(old_sids(jseq)):str_end(old_sids(jseq)))
            END DO
            WRITE (IO_luns(ndx_outkey),'()')
         END IF
C         print*,'after out_keys'
C
C OUT_SABS
C
         IF ( IO_flags(ndx_outsab) ) THEN
            WRITE (intstr1,*) new_sabs(1)
            WRITE (IO_luns(ndx_outsab),'(a,$)') 
     -              intstr1(str_beg(intstr1):str_end(intstr1))
            DO iseq = 2, out_tmpl
               WRITE (intstr1,*) new_sabs(iseq)
               WRITE (IO_luns(ndx_outsab),'(1x,a,$)') 
     -                 intstr1(str_beg(intstr1):str_end(intstr1))
            END DO
            WRITE (IO_luns(ndx_outsab),'()')
         END IF
C         print*,'after out_sabs'
C
C OUT_TABLE
C
C Would have been simpler with implicit write, but they dont work 
C reliably with SUN f77 v2.0
C
         IF ( IO_flags(ndx_outtbl) ) THEN
C
C Write newly aligned sequence
C
            CALL get_memory (outbufptr, y_outlen, 'outbuf', program_ID)
            DO ires = 1, y_outlen
               outbuf(ires:ires) = CHAR(y_gapali(ires))
            END DO
C            WRITE (IO_luns(ndx_outtbl),'(a<y_outlen>,$)') outbuf(1:y_outlen)
            WRITE (IO_luns(ndx_outtbl),'(a,$)') outbuf(1:y_outlen)
            CALL FREE (outbufptr)
C
C Write template sequences on the same line, separated by one blank,
C
            DO iseq = 1, out_tmpl
               jseq = new_ndcs(iseq)
               ilen = old_alinds(jseq) - old_alibgs(jseq) + 1
               CALL get_memory (outbufptr, ilen, 'outbuf', program_ID)
               ipos = 0
               DO ires = old_alibgs(jseq), old_alinds(jseq)
                  ipos = ipos + 1
                  outbuf(ipos:ipos) = CHAR(old_alis(ires))
               END DO
               WRITE (IO_luns(ndx_outtbl),'(1x,a,$)') outbuf(1:ipos)
C               WRITE (IO_luns(ndx_outtbl),'(1x,a<ipos>,$)') outbuf(1:ipos)
               CALL FREE (outbufptr)
            END DO
               
            WRITE (IO_luns(ndx_outtbl),'()')

         END IF
C         print*,'after out_table'

C
C OUT_RULER
C
C Would have been simpler with implicit write, but they dont work 
C reliably with SUN f77 v2.0
C
         IF ( IO_flags(ndx_outrul) ) THEN
            CALL get_memory (outbufptr, y_outlen, 'outbuf', program_ID)
            DO ires = 1, y_outlen
               outbuf(ires:ires) = CHAR(a_gapmrk(ires))
            END DO
            WRITE (IO_luns(ndx_outrul),'(a)') outbuf(1:y_outlen)
C            WRITE (IO_luns(ndx_outrul),'(a<y_outlen>)') outbuf(1:y_outlen)
            CALL FREE (outbufptr)
         END IF
C         print*,'after out_ruler'
C
C OUT_HUMANE
C
         IF ( IO_flags(ndx_outhum) ) THEN
            CALL write_humane (IO_luns(ndx_outhum), cl_listwid, y_outlen, 
     -                         x_gapali, y_gapali, a_gapmrk,
     -                         old_sids(tmplndx), new_sids(inew) )
         END IF
C
C OUT_SERV  (format that converts into something read by distance matrix programs
C
         IF ( IO_flags(ndx_outsrv) ) THEN
C ruler
            CALL get_memory (outbufptr, y_outlen, 'outbuf', program_ID)
            DO ires = 1, y_outlen
               outbuf(ires:ires) = CHAR(a_gapmrk(ires))
            END DO
            WRITE (IO_luns(ndx_outsrv),'(a)') 'MASKID    '//' '//'RULER'
            WRITE (IO_luns(ndx_outsrv),'(a)') 'MASK      '//' '//outbuf(1:y_outlen)
            CALL FREE (outbufptr)
C new
            CALL get_memory (outbufptr, y_outlen, 'outbuf', program_ID)
            DO ires = 1, y_outlen
               outbuf(ires:ires) = CHAR(y_gapali(ires)) 
            END DO
            WRITE (IO_luns(ndx_outsrv),'(a)') 'PALSEQID  '//' '//new_sids(inew)(str_beg(new_sids(inew)):str_end(new_sids(inew)))
            WRITE (IO_luns(ndx_outsrv),'(a)') 'PALSEQ    '//' '//outbuf(1:y_outlen)
            CALL FREE (outbufptr)
C templates
            DO iseq = 1, out_tmpl
               jseq = new_ndcs(iseq)
               ilen = old_alinds(jseq) - old_alibgs(jseq) + 1
               CALL get_memory (outbufptr, ilen, 'outbuf', program_ID)
               ipos = 0
               DO ires = old_alibgs(jseq), old_alinds(jseq)
                  ipos = ipos + 1
                  outbuf(ipos:ipos) = CHAR(old_alis(ires))
               END DO
               WRITE (IO_luns(ndx_outsrv),'(a)') 'RDPSEQID  '//' '//old_sids(jseq)(str_beg(old_sids(jseq)):str_end(old_sids(jseq)))
               WRITE (IO_luns(ndx_outsrv),'(a)') 'RDPSEQ    '//' '//outbuf(1:y_outlen)
               CALL FREE (outbufptr)
            END DO

         END IF

         CALL FREE (a_gapmrk_ptr) 
C         print*,'after a_gapmrk_ptr'
         CALL FREE (y_gapali_ptr)
C         print*,'after y_gapali_ptr'
         CALL FREE (x_gapali_ptr)
C         print*,'after x_gapali_ptr'

C         print*,'end of main loop'
         
 500  END DO

C      print*,'after 500'
C
C Free output channels other than stdout,
C
      IF (IO_luns(ndx_outgbk).NE.6 .AND. IO_flags(ndx_outgbk))
     -        CLOSE ( UNIT = IO_luns(ndx_outgbk) )
      IF (IO_luns(ndx_outkey).NE.6 .AND. IO_flags(ndx_outkey))
     -        CLOSE ( UNIT = IO_luns(ndx_outkey) )
      IF (IO_luns(ndx_outsab).NE.6 .AND. IO_flags(ndx_outsab))
     -        CLOSE ( UNIT = IO_luns(ndx_outsab) )
      IF (IO_luns(ndx_outrul).NE.6 .AND. IO_flags(ndx_outrul))
     -        CLOSE ( UNIT = IO_luns(ndx_outrul) )
      IF (IO_luns(ndx_outtbl).NE.6 .AND. IO_flags(ndx_outtbl))
     -        CLOSE ( UNIT = IO_luns(ndx_outtbl) )
      IF (IO_luns(ndx_outhum).NE.6 .AND. IO_flags(ndx_outhum))
     -        CLOSE ( UNIT = IO_luns(ndx_outhum) )
      IF (IO_luns(ndx_log).NE.6 .AND. IO_flags(ndx_log))
     -        CLOSE ( UNIT = IO_luns(ndx_log) )
      IF (IO_luns(ndx_err).NE.6 .AND. IO_flags(ndx_err))
     -        CLOSE ( UNIT = IO_luns(ndx_err) )

C      print*,'after close'
C
C Free memory,
C
      CALL FREE (old_compsptr)
      CALL FREE (old_tblptr)
      CALL FREE (old_tbegsptr)
      CALL FREE (old_onumsptr)
      CALL FREE (old_alisptr)
      CALL FREE (old_alibgsptr)
      CALL FREE (old_alindsptr)
      CALL FREE (old_sidsptr)
      CALL FREE (old_lidsptr)

      CALL FREE (new_seqsptr)
      CALL FREE (new_sidsptr)
      CALL FREE (new_lidsptr)
      CALL FREE (new_begsptr)
      CALL FREE (new_endsptr)
      CALL FREE (new_abgsptr)
      CALL FREE (new_andsptr)
      CALL FREE (new_sabsptr)

C      print*,'after last free'

c      CALL FREE (new_higndcs)

C
C For C pre-processor; it is for signalling floating points errors
C
c$$$# define inexact    0
c$$$# define division   1
c$$$# define underflow  2
c$$$# define overflow   3
c$$$# define invalid    4
c$$$C
c$$$C Establish signal handlers for division by zero, underflow, overflow
c$$$C and invalid operand.
c$$$C
c$$$      ieee_err = ieee_handler('set','division' ,SIGPFE_ABORT)
c$$$      ieee_err = ieee_handler('set','underflow',SIGPFE_ABORT)
c$$$      ieee_err = ieee_handler('set','overflow', SIGPFE_ABORT)
c$$$      ieee_err = ieee_handler('set','invalid',  SIGPFE_ABORT)


C
C Print out floating point exception flags, if set
C
c$$$      accrued = IEEE_FLAGS ('get','exception',' ',ieee_id)
c$$$
c$$$      inx   = AND (RSHIFT(accrued,inexact),1)
c$$$      div   = AND (RSHIFT(accrued,division),1)
c$$$      under = AND (RSHIFT(accrued,underflow),1)
c$$$      over  = AND (RSHIFT(accrued,overflow),1)
c$$$      inv   = AND (RSHIFT(accrued,invalid),1)
c$$$
c$$$C      IF (inx.NE.0) WRITE (IO_luns(ndx_err),'(a)') 
c$$$C     -     ' ***  0 *** FPE error: Inexact ***'
c$$$      IF (div.NE.0) WRITE (IO_luns(ndx_err),'(a)')
c$$$     -     ' ***  1 *** FPE error: Divide by zero ***'
c$$$      IF (under.NE.0) WRITE (IO_luns(ndx_err),'(a)') 
c$$$     -     ' ***  2 *** FPE error: Underflow ***'
c$$$      IF (over.NE.0) WRITE (IO_luns(ndx_err),'(a)') 
c$$$     -     ' ***  3 *** FPE error: Overflow ***'
c$$$      IF (inv.NE.0) WRITE (IO_luns(ndx_err),'(a)') 
c$$$     -     ' ***  4 *** FPE error: Invalid operand ***'

      END

      SUBROUTINE by_test_io (iostr,lun,lodim,hidim,array)
C
C One dimensional BYTE binary READ or WRITE, determined by iostr
C 
      IMPLICIT none

      CHARACTER*(*) iostr
      INTEGER   lun
      INTEGER   lodim
      INTEGER   hidim
      BYTE      array (lodim:hidim)
C
C -----------------------------------------------------------------------------
C
      IF (iostr.EQ.'WRITE') THEN

         WRITE (lun,'(a)') array

      ELSE IF (iostr.EQ.'READ') THEN

         READ (lun,'(a)') array

      END IF

      RETURN
      END








C         write (6,'(a,i)') ' bin_tuplen = ',bin_tuplen
C         write (6,'(a,i)') ' bin_tupdim = ',bin_tupdim
C         write (6,'(a,i)') ' bin_seqsdim= ',bin_seqsdim
C         write (6,'(a,i)') ' bin_basdim = ',bin_basdim
C         write (6,'(a,i)') ' bin_sidswid= ',bin_sidswid
C         write (6,'(a,i)') ' bin_lidswid= ',bin_lidswid
C         write (6,'(a,i)') ' bin_int4bufdim = ',bin_int4bufdim
C         write (6,'(a,i)') ' bin_bytbufdim = ',bin_bytbufdim

C         write (6,'(a,i)') '     tuplen = ',tuplen
C         write (6,'(a,i)') '     tupdim = ',tupdim
C         write (6,'(a,i)') ' old_seqsdim= ',old_seqsdim
C         write (6,'(a,i)') ' old_basdim = ',old_basdim
C         write (6,'(a,i)') ' old_sidswid= ',old_sidswid
C         write (6,'(a,i)') ' old_lidswid= ',old_lidswid
C         write (6,'(a,i)') '  int4bufdim = ',int4bufdim
C         write (6,'(a,i)') '  bytbufdim = ',bytbufdim

c$$$         CALL dtime (timarr)
c$$$         CALL dtime (timarr)
c$$$         CALL dtime_write (timarr,tim_str,i,j)
c$$$         WRITE (IO_luns(ndx_err),'(a)') ' align time : ' // tim_str(i:j)
c$$$            do i = 1, MAX (x_outlen,y_outlen)
c$$$               write (6,'(a1,a1,a1)') char(x_gapali(i)),char(a_gapmrk(i)),
c$$$     -                                char(y_gapali(i))
c$$$            END DO
c$$$
c$$$            print*,' outwid,alilen = ',cl_listwid,MAX (x_outlen,y_outlen)
c$$$            write (6,'(a,1x,a)') old_sids(tmplndx), new_sids(inew)

c$$$C
c$$$C Print warnings of unusual characters in database templatealignment: Other than
c$$$C IUB symbols plus '.', '~' and '-',
c$$$C
c$$$         CALL check_seqs (old_abyts, old_seqsmax,
c$$$     -                   'ABCDGHKMNRSTUVWYabcdghkmnrstuvwy.~-', old_alis,
c$$$     -                    new_higndcs(inew),new_higndcs(inew),
c$$$     -                    old_alibgs, old_alinds, old_sids)
c$$$
c$$$

c$$$         DO i = 1, old_seqsmax
c$$$            write (6,'(a,1x,2i6)') old_sids(i),old_seqbgs(i),old_seqnds(i)
c$$$            write (6,'(a,1x,2i6/)') old_sids(i),old_alibgs(i),old_alinds(i)
c$$$         END DO
c$$$
c$$$         print*,' old_seqsmax,old_sbyts,old_abyts = ', 
c$$$     -            old_seqsmax,old_sbyts,old_abyts
         
c$$$         DO i = 1,old_seqsmax
c$$$            write(6,'(a,1x,,2i8)') old_sids(i),old_seqbgs(i),old_seqnds(i)
c$$$            write(6,'(a,1x,2i8/)') old_sids(i),old_alibgs(i),old_alinds(i)
c$$$         END DO
c$$$
c$$$         do i = 7978, 8000
c$$$            write (6,'(a,1x,i3)') char(old_alis(i)), old_alis(i)
c$$$         end do
c$$$c         stop
c$$$         DO i = 1, y_outlen
c$$$            IF (.not.(
c$$$     -            y_gapali(i).EQ.126   .OR.
c$$$     -            y_gapali(i).EQ.46    .OR.
c$$$     -            y_gapali(i).EQ.45    .OR.
c$$$     -            (y_gapali(i).LE.122.AND.y_gapali(i).GE.97)  .OR.
c$$$     -            (y_gapali(i).LE.90.AND.y_gapali(i).GE.65))) THEN 
c$$$c            IF (.NOT.ch_is_alpha(CHAR(y_gapali(i)))) THEN
c$$$               write (6,'(i4,1x,i4)') i,y_gapali(i)
c$$$               stop
c$$$            END IF
c$$$         END DO
c$$$

C
C From here on only the sequences are needed
c$$$C     
c$$$C For each new sequence, store the index of the most similar database sequence
c$$$C and its S_ab value (when compared with a given new sequence) in arrays 
c$$$C (new_higndcs) and (new_higSabs).  These indices are then used as templates 
c$$$C during the alignment. The (large) similarity tables can then be deallocated 
c$$$C after this. The following is the essence of what SIMILARITY_RANK does.
c$$$C 
c$$$      CALL get_memory ( new_higSabsptr, new_seqsmax * 4, 'new_higSabs', program_ID )
c$$$      CALL get_memory ( new_higndcsptr, new_seqsmax * 4, 'new_higndcs', program_ID )
c$$$
c$$$      CALL get_memory ( mat_addsptr, old_seqsmax * 4, 'mat_adds', program_ID ) 
c$$$      CALL get_memory ( bucketptr, tupdim * 4, 'bucket', program_ID)
c$$$
c$$$      CALL get_memory ( new_seqsptr, new_abyts, 'new_seqs', program_ID ) 
c$$$
c$$$      SUBROUTINE find_relatives (beg, end, bytseq, sabs, ndcs, errstat)
c$$$
c$$$      IMPLICIT none
c$$$
c$$$      INTEGER beg
c$$$      INTEGER end
c$$$      BYTE    bytseq (*)
c$$$      INTEGER sabs (*)
c$$$      INTEGER ndcs (*)
c$$$      INTEGER errstat
c$$$C
c$$$C Local variables
c$$$C     
c$$$      INTEGER i
c$$$C
c$$$C -----------------------------------------------------------------------------
c$$$C
c$$$      
c$$$
c$$$      DO inew = 1, new_seqsmax
c$$$
c$$$         CALL To_Code_4 ( new_seqs, 1, new_abyts, begres, endres, new_seqs )
c$$$         
c$$$         CALL i4_array_init ( 0, tupdim-1, bucket, 0, tupdim-1, 0 )
c$$$         CALL i4_array_init ( 1, old_seqsmax, mat_adds, 1, old_seqsmax, 0 )
c$$$         
c$$$         new_onum = 0
c$$$
c$$$         DO ires = begres, endres - tuplen + 1
c$$$
c$$$            oli_int = 0
c$$$
c$$$            DO ipos = ires, ires + tuplen - 1
c$$$               oli_int = 4 * oli_int + new_seqs (ipos)
c$$$            END DO
c$$$
c$$$            IF ( bucket(oli_int).EQ.0 ) THEN
c$$$
c$$$               new_onum = new_onum + 1 
c$$$
c$$$               tblpos = old_tbegs(oli_int)
c$$$               DO WHILE ( old_tbl(tblpos).GT.0 )
c$$$                  mat_adds ( old_tbl(tblpos) ) = mat_adds ( old_tbl(tblpos) ) + 1
c$$$                  tblpos = tblpos + 1 
c$$$               END DO
c$$$
c$$$               bucket (oli_int) = bucket (oli_int) + 1
c$$$
c$$$            END IF
c$$$
c$$$         END DO
c$$$         
c$$$         higSab = 0
c$$$         higndx = 0
c$$$
c$$$         DO iold = 1, old_seqsmax
c$$$
c$$$            IF ( old_comps(iold).GE.cl_completepct ) THEN
c$$$
c$$$               Sab = REAL( mat_adds(iold) ) 
c$$$     -              / MIN ( new_onum, old_onums(iold) )
c$$$
c$$$               IF (Sab.GT.higSab) THEN
c$$$                  higSab = Sab
c$$$                  higndx = iold
c$$$               END IF
c$$$
c$$$            END IF
c$$$
c$$$         END DO
c$$$
c$$$         new_higSabs (inew) = higSab
c$$$         new_higndcs (inew) = higndx
c$$$
c$$$         IF (higSab.LE.0) THEN
c$$$            CALL write_message ( IO_luns(ndx_err),IO_fnms(ndx_err),
c$$$     -          'Error: Unable to find a template for new entry ' // 
c$$$     -           new_sids(inew)(str_beg(new_sids):str_end(new_sids)) // '.'   // LF //
c$$$     -           'Possible explanations: No sequence satisfies the -complete' // LF //
c$$$     -           'option, your sequence shares no 8-mers with any sequence'   // LF //
c$$$     -           'in the specified RDP alignment, or maybe it is a program'   // LF // 
c$$$     -           'error; then please mail your data to Niels Larsen,'         // LF //
c$$$     -           'niels@darwin.life.uiuc.edu' )
c$$$         END IF
c$$$
c$$$      END DO
c$$$
c$$$      CALL FREE (mat_addsptr)
c$$$      CALL FREE (bucketptr)
c$$$      CALL FREE (new_seqsptr)
C
c$$$C
c$$$C -OUT_GENBANK   This includes a Genbank formatted ruler output
c$$$C This option may disappear some day
c$$$C
c$$$         CALL 
c$$$
c$$$         ELSE
c$$$
c$$$c$$$            IF (.NOT.cl_stdoutflag) THEN
c$$$c$$$
c$$$c$$$               OPEN (UNIT = IO_luns(ndx_outgbk),
c$$$c$$$     -               FILE = new_sids(inew)(str_beg(new_sids(inew))
c$$$c$$$     -                        :str_end(new_sids(inew))) // '.palgb', 
c$$$c$$$     -             STATUS = 'NEW',
c$$$c$$$     -             IOSTAT = iostat)
c$$$c$$$
c$$$c$$$               IF (iostat.NE.0) THEN
c$$$c$$$                  CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
c$$$c$$$     -                 'Error: File ' // new_sids(inew)
c$$$c$$$     -                 (str_beg(new_sids(inew)):str_end(new_sids(inew))) // '.palgb' //
c$$$c$$$     -                 ' already exists' // LF //
c$$$c$$$     -                 'Entry ' // new_sids(inew)
c$$$c$$$     -                 (str_beg(new_sids(inew)):str_end(new_sids(inew))) // 
c$$$c$$$     -                 ' not written')
c$$$c$$$                  GO TO 500
c$$$c$$$               END IF
c$$$c$$$
c$$$c$$$            END IF
c$$$c$$$            
c$$$            CALL fdate (dat_str)
c$$$            READ (dat_str(9:10),*) date
c$$$            CALL seq_upcase ( dat_str(5:7),1,3,month )
c$$$            READ (dat_str(21:24),*) year
c$$$C
c$$$C If -template option, include template sequence in output,
c$$$C
c$$$            IF ( cl_tplflag ) THEN
c$$$               
c$$$               WRITE (hdrstr,'(a,i4,a,i2.2,a,i4,a)')
c$$$     - 'LOCUS       ' // old_sids(tmplndx)(1:10) // '   ', x_outlen,' bp   rRNA' //
c$$$     - '             RNA       ', date, '-' // month // '-', year, '       ' // LF //
c$$$     - 'DEFINITION  ' // old_lids(tmplndx)(1:MAX(str_end(old_lids(tmplndx)),1)) // LF //
c$$$     - 'COMMENTS    ' // 'Your submitted sequence was aligned against this most ' //
c$$$     -                  'similar' // LF //
c$$$     - '            RDP sequence' // LF // 
c$$$     - 'ORIGIN'
c$$$
c$$$               CALL write_gbk_ent ( IO_luns(ndx_outgbk),
c$$$     -                              1, 1, 1, hdrstr,
c$$$     -                              x_outlen, 1, x_outlen, x_gapali )
c$$$            END IF
c$$$C
c$$$C Write new aligned sequence
c$$$C
c$$$            print*,' new_ands(new_seqsmax), new_abgs(inew) = ',
c$$$     -               new_ands(new_seqsmax), new_abgs(inew)
c$$$            print*,' inew, new_ands(inew) = ', inew, new_ands(inew)
c$$$            print*,' y_outlen, y_gapali = ',y_outlen, y_gapali 
c$$$            CALL write_gbk_ent ( IO_luns(ndx_outgbk),
c$$$     -                           new_ands(new_seqsmax), new_abgs(inew),
c$$$     -                           new_ands(inew), new_text, 
c$$$     -                           y_outlen, 1, y_outlen, y_gapali )
c$$$C
c$$$C Write special entry that shows where alignment is certain and where not,
c$$$C
c$$$            WRITE (probid,'(i10)') inew
c$$$            DO i = 1,10
c$$$               IF (probid(i:i).EQ.' ') probid(i:i) = CHAR (amb_ich)
c$$$            END DO
c$$$c            WRITE (pctstr,'(f7.3)') new_higSabs(inew)
c$$$
c$$$            WRITE (hdrstr,'(a,i4,a,i2.2,a,i4,a)') 
c$$$     - 'LOCUS       ' // probid // '   ', y_outlen, ' bp    NNN             NNN' // 
c$$$     - '       ', date, '-' // month // '-', year, '       ' // LF //
c$$$     - 'DEFINITION  ' // CHAR(amb_ich) // '''s mark uncertain alignment'   // LF //
c$$$     - 'COMMENTS    ' // 
c$$$     -       new_sids(inew)(str_beg(new_sids(inew)):str_end(new_sids(inew))) // 
c$$$     -     ' (submitted) highest S_ab value ' // 
c$$$     -       pctstr(str_beg(pctstr):str_end(pctstr)) // ' versus ' // 
c$$$     -       old_sids(tmplndx)(str_beg(old_sids(tmplndx)):
c$$$     -                         str_end(old_sids(tmplndx)))                         // LF //
c$$$     - '            Note: This S_ab value may differ slightly from those returned' // LF //
c$$$     - '            by SIMILARITY_RANK in case the submitted sequence contains'    // LF //
c$$$     - '            ambiguity codes.'                                              // LF //
c$$$     - '            This output is produced by program ' // program_ID  //
c$$$     -              ' (v ' // version_ID // '), written by'                 // LF //
c$$$     - '            Niels Larsen, niels@darwin.life.uiuc.edu'               // LF //
c$$$     - 'ORIGIN'
c$$$
c$$$            CALL write_gbk_ent ( IO_luns(ndx_outgbk),
c$$$     -                           1, 1, 1, hdrstr,
c$$$     -                           y_outlen, 1, y_outlen, a_gapmrk )
c$$$
c$$$c$$$            IF (.NOT.cl_stdoutflag) CLOSE (UNIT = IO_luns(ndx_outgbk))
c$$$
c$$$         END IF
c$$$
c$$$
c$$$
c$$$
c$$$
c$$$c  How many template sequences to include in the output?  
c$$$
c$$$
c$$$
c$$$C List output
c$$$C
c$$$         IF (cl_outhumflag) THEN
c$$$
c$$$c$$$            IF (.NOT.cl_stdoutflag) THEN
c$$$c$$$
c$$$c$$$               OPEN (UNIT = IO_luns(ndx_outgbk),
c$$$c$$$     -               FILE = new_sids(inew)(str_beg(new_sids(inew))
c$$$c$$$     -                        :str_end(new_sids(inew))) // '.palst', 
c$$$c$$$     -             STATUS = 'NEW', 
c$$$c$$$     -             IOSTAT = iostat)
c$$$c$$$
c$$$c$$$               IF (iostat.NE.0) THEN
c$$$c$$$                  CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
c$$$c$$$     -                 'Error: File ' // new_sids(inew)
c$$$c$$$     -                 (str_beg(new_sids(inew)):str_end(new_sids(inew))) // '.palst' //
c$$$c$$$     -                 ' already exists' // LF //
c$$$c$$$     -                 'Entry ' // new_sids(inew)
c$$$c$$$     -                 (str_beg(new_sids(inew)):str_end(new_sids(inew))) // 
c$$$c$$$     -                 ' not written')
c$$$c$$$                  GO TO 500
c$$$c$$$               END IF
c$$$c$$$
c$$$c$$$            END IF
c$$$
c$$$
c$$$
c$$$         CALL FREE (x_gapali_ptr)
c$$$         CALL FREE (y_gapali_ptr)
c$$$         CALL FREE (a_gapmrk_ptr) 
      SUBROUTINE assign_lun ( fnm, lun, ioflag )

      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      CHARACTER*(*) fnm
      INTEGER lun
      LOGICAL ioflag
C
C Local variables
C
      CHARACTER*1 LF / 10 /
      INTEGER str_beg, str_end, lun_alloc
      INTEGER err_cod, io_stat
C
C ---------------------------------------------------------------------------------
C
      IF ( str_end(fnm).GT.0 ) THEN

         lun = lun_alloc (ndx_max,err_cod)

         IF ( err_cod.NE.0)  THEN
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -         'Error:  Attempt to reserver more than max. allowed' //LF//
     -         '        number of IO-channels.  No output produced.' )
            ioflag = .FALSE.
            RETURN
         END IF

         OPEN (UNIT = lun,
     -         FILE = fnm,
     -       STATUS = 'NEW', 
     -       IOSTAT = io_stat)

         IF (io_stat.NE.0) THEN
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -         'Error:  File ' // fnm(str_beg(fnm):str_end(fnm)) //
     -                 ' already exists' // LF //
     -         '        No output produced. ' )
            ioflag = .FALSE.
            RETURN
         END IF

      ELSE
         IO_luns(ndx_outgbk) = 6
      END IF

      RETURN
      END 

      SUBROUTINE Bas_Eqv_init

      IMPLICIT none

      INCLUDE 'pal_res_eqv.inc'

      INTEGER str_beg,str_end
C
C Local variables
C
      INTEGER strbeg,strend
      INTEGER i,j
      INTEGER itbl

      CHARACTER*80 par_tbl(16)
      CHARACTER*1  ch2
      INTEGER      ich1,ich2
C
C ------------------------------------------------------------------------------
C
C Edit this table to get different matching schemes, but please only include
C IUB base symbols or blanks. 
C
C             this one   matches these
C                    |
      par_tbl( 1) = 'U   U T K Y B W D H                '
      par_tbl( 2) = 'T   T U K Y B W D H                '
      par_tbl( 3) = 'G   G K S B R D V                  '
      par_tbl( 4) = 'K   K U T G Y S B W R D H V        '
      par_tbl( 5) = 'C   C Y S B M H V                  '
      par_tbl( 6) = 'Y   Y U T K C S B W D M H V        '
      par_tbl( 7) = 'S   S G K C Y B R D M H V          '
      par_tbl( 8) = 'B   B U T G K C Y S W R D M H V    '
      par_tbl( 9) = 'A   A W R D M H V                  '
      par_tbl(10) = 'W   W U T K Y B A R D M H V        '
      par_tbl(11) = 'R   R G K S B A W D M H V          '
      par_tbl(12) = 'D   D U T G K Y S B A W R M H V    '
      par_tbl(13) = 'M   M C Y S B A W R D H V          '
      par_tbl(14) = 'H   H U T K C Y S B A W R D M V    '
      par_tbl(15) = 'V   V G K C Y S B A W R D M H      '
      par_tbl(16) = 'N   N                              '
C
C Set up bas_eqv matrix, so true where ICHAR values match, false otherwise
C
      DO i = 0,127
         DO j = 0,127
            bas_eqv (i,j) = 0    ! init with .FALSE.
         END DO
      END DO

      DO itbl = 1,16

         strbeg = str_beg (par_tbl(itbl))
         strend = str_end (par_tbl(itbl))

         ich1 = ICHAR (par_tbl(itbl)(strbeg:strbeg))

         DO i = strbeg + 1, strend

            ch2 = par_tbl(itbl)(i:i)

            IF (ch2.NE.' ') THEN

               ich2 = ICHAR (ch2)

               bas_eqv (ich1   , ich2   ) = 1   ! upper-upper
               bas_eqv (ich1   , ich2+32) = 1   ! upper-lower
               bas_eqv (ich1+32, ich2   ) = 1   ! lower-upper
               bas_eqv (ich1+32, ich2+32) = 1   ! upper-upper

            END IF

         END DO

      END DO

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


      SUBROUTINE by_bin_io (iostr,lun,lodim,hidim,array)
C
C One dimensional BYTE binary READ or WRITE, determined by iostr
C 
      IMPLICIT none

      CHARACTER*(*) iostr
      INTEGER   lun
      INTEGER   lodim
      INTEGER   hidim
      BYTE      array (lodim:hidim)
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

      SUBROUTINE calc_sabs (seqmax, basmax, tupmax, tblmax, beg, end, bytseq, 
     -                      tuplen, db_olinums, tblbegs, tbl, sabs)

      IMPLICIT none

      INTEGER   seqmax
      INTEGER   basmax
      INTEGER   tupmax
      INTEGER   tblmax
      INTEGER   beg
      INTEGER   end
      BYTE      bytseq (*)
      INTEGER   tuplen
      INTEGER   db_olinums (seqmax)
      INTEGER   tblbegs (0:tupmax-1)
      INTEGER*2 tbl (tblmax)
      REAL      sabs (seqmax)
C
C Local variables
C
      INTEGER  matadd (seqmax)
      POINTER (matadd_ptr, matadd)

      BYTE     codseq (basmax)
      POINTER (codseq_ptr, codseq)

      INTEGER  bucket (0:tupmax-1)
      POINTER (bucket_ptr,bucket)

      CHARACTER*14 program_ID / 'FIND_RELATIVES' /

      INTEGER olinum, oliint, tblpos
      INTEGER ipos, ires, iseq
C
C -----------------------------------------------------------------------------
C
      CALL get_memory ( matadd_ptr, seqmax * 4, 'matadd', program_ID ) 
      CALL get_memory ( bucket_ptr,  tupmax * 4, 'bucket', program_ID )
      CALL get_memory ( codseq_ptr,   end-beg+1, 'codseq', program_ID ) 

      CALL To_Code_4 ( bytseq(beg), 1, end-beg+1, 1, end-beg+1, codseq )
      
      CALL i4_array_init ( 0, tupmax-1, bucket, 0, tupmax-1, 0 )
      CALL i4_array_init ( 1, seqmax, matadd, 1, seqmax, 0 )
         
      olinum = 0

      DO ires = beg, end - tuplen + 1

         oliint = 0

         DO ipos = ires-beg+1, ires-beg + tuplen
            oliint = 4 * oliint + codseq (ipos)
         END DO
         
         IF ( bucket(oliint).EQ.0 ) THEN
            
            olinum = olinum + 1 
            
            tblpos = tblbegs(oliint)
            DO WHILE ( tbl(tblpos).GT.0 )
               matadd ( tbl(tblpos) ) = matadd ( tbl(tblpos) ) + 1
               tblpos = tblpos + 1 
            END DO
            
            bucket (oliint) = bucket (oliint) + 1
            
         END IF
         
      END DO

      CALL FREE (bucket_ptr)
      CALL FREE (codseq_ptr)
C
C Get Sab's 
C

      DO iseq = 1, seqmax
         sabs(iseq) = REAL( matadd(iseq) ) / MIN ( olinum, db_olinums(iseq) )
      END DO

      CALL FREE (matadd_ptr)

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
      SUBROUTINE check_seqs (resmax, seqmax, acptstr, seqs, 
     -                       seqbeg, seqend, begs, ends, sids)
C
C Unfinished, improve
C
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      INTEGER       resmax
      INTEGER       seqmax
      CHARACTER*(*) acptstr
      BYTE          seqs (resmax)
      INTEGER       seqbeg
      INTEGER       seqend
      INTEGER       begs (seqmax)
      INTEGER       ends (seqmax)
      CHARACTER*(*) sids (seqmax)
C
C Local variables
C     
      INTEGER       str_beg, str_end
      INTEGER       bucket (0:255)
      INTEGER       i
      BYTE          ich
      INTEGER       outpos
      INTEGER       iseq, ires

      CHARACTER*999 outstr
      CHARACTER*20  numstr
      CHARACTER*1   LF   / 10 /
      LOGICAL       accept (0:255)
      LOGICAL       flag
C
C --------------------------------------------------------------------------------
C
      DO i = 0, 255
         accept (i) = .FALSE.
      END DO

      DO i = 1, LEN (acptstr)
         accept ( ICHAR(acptstr(i:i)) ) = .TRUE.
      END DO

      DO iseq = seqbeg, seqend

         CALL i4_array_init (0,255,bucket,0,255,0)
         flag = .FALSE.

         DO ires = begs (iseq), ends (iseq)
            ich = seqs(ires)
            IF (.NOT.accept(ich)) THEN
c               write(6,'(a,1x,i3,1x,i8)') char(ich), ich, ires
               bucket (ich) = bucket(ich) + 1
               flag = .TRUE.
            END IF
         END DO

         IF (flag) THEN
            outpos = 0
            DO i = 0,255
               IF (bucket(i).GT.0) THEN
                  WRITE (numstr,*) bucket(i)
                  outstr(outpos+1:outpos+37) = '            "' // 
     -                      CHAR (i) // '" ' // numstr // LF
                  outpos = outpos + 37
               END IF
            END DO
            outpos = outpos - 1 
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -          'Warning: Unrecognized symbols in ' // 
     -          sids(iseq)(str_beg(sids(iseq)):str_end(sids(iseq))) // 
     -          ' :' // LF // '         Symbol  frequency' // LF // 
     -          outstr(1:outpos) )
         END IF

      END DO

      RETURN
      END
      INTEGER FUNCTION dia_begs (x_low_bnd,x_hig_bnd,
     -                           y_low_bnd,y_hig_bnd,
     -                           x_ini_lim,y_ini_lim,
     -                           dia_len,err_cod)
C
C Counts the number of start positions of windows (of length dia_len) 
C that move along diagonals. Windows move in the area spanned by 
C (x_low_bnd,y_low_bnd) and (x_hig_bnd,y_hig_bnd); within that area
C x_ini_lim and y_ini_lim sometimes constrain the starting positions
C
      IMPLICIT none

      INTEGER x_low_bnd,x_hig_bnd  ! in
      INTEGER y_low_bnd,y_hig_bnd  ! in
      INTEGER x_ini_lim,y_ini_lim  ! in 
      INTEGER dia_len              ! in
      INTEGER err_cod              ! out
C
C Local variables
C
      INTEGER i,x,y
C
C --------------------------------------------------------------------------
C
      dia_begs = 0

      IF (dia_len .GT. x_hig_bnd - x_low_bnd + 1  .OR.
     -    dia_len .GT. y_hig_bnd - y_low_bnd + 1)  THEN

         err_cod = -1 
         RETURN

      ELSE
         err_cod = 0
      END IF

      x = x_hig_bnd - dia_len + 1
      y = y_hig_bnd - dia_len + 1

      dia_begs = ( x - x_low_bnd + 1 ) * ( y - y_low_bnd + 1 )

      IF (x.GT.x_ini_lim) THEN
         i = x - x_ini_lim
         dia_begs = dia_begs - ( i * i - i ) / 2 - i
      END IF

      IF (y.GT.y_ini_lim) THEN
         i = y - y_ini_lim
         dia_begs = dia_begs - ( i * i - i ) / 2 - i
      END IF

      RETURN
      END
      SUBROUTINE do_sim_list (tuplen,tupdim,
     -                        old_seqsmax,old_sbegs,old_sends,
     -                        old_tbl,old_tbegs,old_onum,
     -                        new_seqs,new_beg,new_end,
     -                        mat_pcts)
C
C Returns mat_pcts, where each element is the percentage of tuplen'mers
C shared with new sequence from new_seqs(new_beg) to new_seqs(new_end)
C
      IMPLICIT none

      INTEGER tuplen               ! in
      INTEGER tupdim              ! in    
      INTEGER old_seqsmax         ! in    
      INTEGER old_sbegs(*)        ! in    
      INTEGER old_sends(*)        ! in    
      INTEGER*2 old_tbl(*)        ! in
      INTEGER old_tbegs(0:tupdim-1)      ! in
      INTEGER old_onum(*)         ! in
      BYTE    new_seqs(*)         ! in
      INTEGER new_beg             ! in
      INTEGER new_end             ! in
      REAL    mat_pcts(*)         ! out
C
C Local variables
C
      INTEGER    mat_adds(old_seqsmax)
      POINTER   (mat_addsptr,mat_adds)

      INTEGER    new_nums (0:tupdim-1)
      POINTER   (new_numsptr,new_nums)

      INTEGER   ipos,ioli,iseq
      INTEGER   new_onum,new_len
      INTEGER   baspos,intcod,tblpos,old_iseq
C
C --------------------------------------------------------------------------------
C
C Init tables
C
      CALL get_memory (new_numsptr, tupdim * 4, 'new_nums', 'do_sim_list')

      DO ioli = 0, tupdim - 1 
         new_nums(ioli) = 0 
      END DO
C     
C Generate unique integers for each tuplen-mer and store their numbers
C
      new_onum = 0

      DO baspos = new_beg, new_end - tuplen + 1

         intcod = 0
         DO ipos = baspos, baspos + tuplen - 1
            intcod = 4 * intcod + new_seqs(ipos)
         END DO

         IF (new_nums(intcod).EQ.0) new_onum = new_onum + 1 

         new_nums(intcod) = new_nums(intcod) + 1 
         
      END DO

      CALL FREE (new_numsptr) 
C
C Init similarity matrix 
C
      CALL get_memory (mat_addsptr, old_seqsmax * 4, 'mat_adds', 'do_sim_list' ) 

      DO iseq = 1,old_seqsmax
         mat_adds(iseq) = 0 
      END DO
C
C Comparison section (YES thats all)
C
      DO ioli = 0,tupdim
 
         IF (new_nums(ioli).GT.0 .AND. 
     -       old_tbl(old_tbegs(ioli)).GT.0) THEN
               
            tblpos = old_tbegs(ioli)
               
            DO WHILE (old_tbl(tblpos).GT.0)
               old_iseq = old_tbl(tblpos)
               mat_adds(old_iseq) = mat_adds(old_iseq) + 1
               tblpos = tblpos + 1 
            END DO

         END IF
            
      END DO
C
C Divide number of different matching oligos with shortest sequence
C length so a short good match ranks higher than a less good longer one
C
      new_len = new_end - new_beg + 1

      DO iseq = 1,old_seqsmax
         IF (old_sends(iseq)-old_sbegs(iseq).GE.new_len) THEN
             mat_pcts(iseq) = 100.0 * REAL(mat_adds(iseq)) / 
     -           REAL (new_onum)
         ELSE
             mat_pcts(iseq) = 100.0 * REAL(mat_adds(iseq)) / 
     -           REAL (old_onum(iseq))
         END IF
      END DO
      
      CALL FREE (mat_addsptr)

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
      SUBROUTINE feel_gbk (inlun,isbase,minseqlen,iseqs,ichrs,iwats)
C
C It reads file of one or more Genbank formatted entries; it returns 
C number of sequences (iseqs) and number of non-blank sequence characters 
C (ibyts). It will read ients number of entries ahead from current file 
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
      INTEGER       iwats
C
C Local variables
C
      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      CHARACTER*80  inline
      CHARACTER*10  locstr, scrstr
      CHARACTER*8   fun_ID  / 'FEEL_GBK' /
      CHARACTER*1   LF      / 10 / 

      INTEGER     str_beg, str_end
      INTEGER     iloc, itrm
      INTEGER     linend, ipos
      INTEGER     i, j
      BYTE        bytech
C   
C -------------------------------------------------------------------------
C
      iseqs = 0
      ichrs = 0
      iwats = 0

      iloc = 0
      itrm = 0

      DO WHILE (.TRUE.)

         READ (inlun,'(q,a)', END = 98, ERR = 99) linend,inline

         IF (inline(1:5).EQ.'LOCUS') THEN 

            iloc = iloc + 1
            locstr = inline (13:22)

         ELSE IF (inline(1:6).EQ.'ORIGIN') THEN

            READ (inlun,'(q,a)', ERR = 99) linend, inline
            
            i = 0
            j = 0
            
            DO WHILE (inline(1:2).EQ.'  ')

               DO ipos = 11, linend
                  bytech = ICHAR ( inline(ipos:ipos) )
                  IF ( bytech.NE.32 ) THEN
                     i = i + 1
                     IF ( isbase(bytech) ) j = j + 1
                  END IF
               END DO

               READ (inlun,'(q,a)', ERR = 99) linend, inline
            END DO

            IF (inline(1:2).EQ.'//') THEN

               itrm = itrm + 1

               IF (iloc.GT.itrm) THEN
                  CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
     -                fun_ID // ': ' //
     -                'Error: Entry previous to ' // locstr //
     -                ' is not properly terminated')
                  STOP
               ELSE IF (iloc.LT.itrm) THEN
                  IF (iloc.EQ.0) THEN
                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
     -                    'Error: First entry is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  ELSE
                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
     -                    'Error: Entry following ' // locstr //
     -                    ' is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  END IF
                  STOP
               END IF

            END IF

            IF ( j.GE.minseqlen ) THEN

               iseqs = iseqs + 1
               ichrs = ichrs + i
               iwats = iwats + j

            ELSE
               WRITE (scrstr,*) minseqlen
               CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
     -              'Warning: Entry ' // locstr(str_beg(locstr):str_end(locstr)) // 
     -              ' shorter than ' // scrstr(str_beg(scrstr):str_end(scrstr)) // 
     -              ' residues' // LF // '(entry ignored)' )
            END IF

         END IF
         
      END DO
      
 98   RETURN

 99   CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' // 
     -    'Error: Unexpected end of file reached' // LF // 
     -    'Program stop')
      STOP

      END 
      SUBROUTINE feel_gbk_seqs (ent_maxlin,ent_txt,ent_lin,
     -                          entries,tuplen,
     -                          iseq,iwcs,iubs,ichs)
C
C Returns number of sequences, IUB bases and characters (within the sequence)
C in a file of one or more Genbank entries. Entries with less than tuplen 
C IUB bases are ignored. Program stops if following format errors occur: 
C No final '//', unequal number of ORIGIN and LOCUS lines, no ORIGIN line 
C after last LOCUS line, no LOCUS line after last ORIGIN line, and no '//' 
C before next LOCUS line.
C 
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc' 
      INCLUDE 'pal_res_eqv.inc'   ! contains base logicals

      INTEGER       ent_maxlin       
      CHARACTER*(80) ent_txt(ent_maxlin)
      INTEGER       ent_lin
      INTEGER       entries        ! in     # entries to read
      INTEGER       tuplen         ! in     oligo length
      INTEGER       iseq           ! out    # sequences
      INTEGER       iwcs           ! out    # WC bases
      INTEGER       iubs           ! out    # IUB bases
      INTEGER       ichs           ! out    # characters
C
C Local variables
C
      INTEGER     ient,iloc,iori,ilin
      INTEGER     str_end
      INTEGER     i,j,k,n
      BYTE        ich

      LOGICAL locflg
C   
C -------------------------------------------------------------------------
C
      iseq = 0   ! # of sequences
      iwcs = 0   ! # of AUCG bases
      iubs = 0   ! # of IUB bases
      ichs = 0   ! # of non-blank characters

      ient = 0   ! # of entries read
      iloc = 0   ! # of LOCUS lines
      iori = 0   ! # of ORIGIN lines
      ilin = 0   ! # of lines read

      locflg = .FALSE.  ! true after LOCUS line, false after sequence read

      DO WHILE (ient.LT.entries .AND. ilin.LE.ent_lin) 

         ilin = ilin + 1 

         IF (ent_txt(ilin)(1:5).EQ.'LOCUS') THEN 

            iloc = iloc + 1
            locflg = .TRUE.

         ELSE IF (ent_txt(ilin)(1:6).EQ.'ORIGIN') THEN

            iori = iori + 1 

            IF (locflg) THEN

               ilin = ilin + 1

               j = iwcs
               n = iubs
               k = ichs

               DO WHILE (ent_txt(ilin)(1:2).NE.'//')

                  IF (ent_txt(ilin)(1:5).EQ.'LOCUS') THEN

                     CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err), 
     -                   'FEEL_GBK_SEQS: Error: ' // 'LOCUS ' // 
     -                    ent_txt(ilin)(13:22) // ' not terminated by ''//''')
                     STOP

                  ELSE

                     DO i = 11,str_end(ent_txt(ilin))
                        ich = ICHAR(ent_txt(ilin)(i:i))
                        IF (ich.NE.ICHAR(' ')) THEN
                           k = k + 1
                           IF (WC_base(ich)) j = j + 1
                           IF (IUB_base(ich)) n = n + 1
                        END IF
                     END DO

                     ilin = ilin + 1 

                  END IF

               END DO

               IF (j .GE. iwcs + tuplen) THEN
                  iseq = iseq + 1
                  ient = ient + 1 
                  iwcs = j
                  iubs = n
                  ichs = k
               END IF

               locflg = .FALSE.

           END IF

         END IF

      END DO
      
 98   IF (iloc.NE.iori) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err), 
     -        'FEEL_GBK_SEQS: ' // 'LOCUS line count != ORIGIN count')
         STOP
      END IF

      RETURN

 99   CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -     'FEEL_GBK_SEQS: Error: Unexpected EOF')
      STOP

      END 
      SUBROUTINE feel_gbtxt ( maxlin, gbtext, 
     -                        isbase, minlen,
     -                         iseqs, ibyts )
C
C It looks through one or more Genbank formatted entries and returns 
C number of sequences (iseqs) and number of characters (ibyts) that 
C are flagged by isbase.
C
C Program is stopped if terminator lines ('//') dont always follow 
C LOCUS lines and vice versa
C 
      IMPLICIT none

      INTEGER       maxlin
      CHARACTER*(*) gbtext (maxlin)
      LOGICAL*1     isbase (0:*)
      INTEGER       minlen
      INTEGER       iseqs     
      INTEGER       ibyts
C
C Local variables
C
      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      CHARACTER*10  locstr, scrstr
      CHARACTER*8   fun_ID  / 'FEEL_GBK' /
      CHARACTER*1   LF      / 10 / 

      INTEGER     str_beg, str_end
      INTEGER     iloc, itrm, ilin, ipos
      INTEGER     i
C   
C -------------------------------------------------------------------------
C
      iseqs = 0
      ibyts = 0

      iloc = 0
      itrm = 0
      ilin = 1

      DO WHILE ( ilin.LE.maxlin )

         IF ( gbtext(ilin)(1:5).EQ.'LOCUS' ) THEN 

            iloc = iloc + 1
            locstr = gbtext(ilin)(13:22)

         ELSE IF ( gbtext(ilin)(1:6).EQ.'ORIGIN' ) THEN

            ilin = ilin + 1 

            i = 0
            
            DO WHILE ( gbtext(ilin)(1:2).EQ.'  ' )

               DO ipos = 11, 80
                  IF ( isbase(ICHAR(gbtext(ilin)(ipos:ipos))) ) i = i + 1
               END DO

               ilin = ilin + 1

            END DO

            IF ( gbtext(ilin)(1:2).EQ.'//' ) THEN

               itrm = itrm + 1

               IF (iloc.GT.itrm) THEN
                  CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
     -                fun_ID // ': ' //
     -                'Error: Entry previous to ' // locstr //
     -                ' is not properly terminated')
                  STOP
               ELSE IF (iloc.LT.itrm) THEN
                  IF (iloc.EQ.0) THEN
                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
     -                    'Error: First entry is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  ELSE
                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
     -                    'Error: Entry following ' // locstr //
     -                    ' is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  END IF
                  STOP
               END IF

            END IF

            IF ( i.GE.minlen ) THEN

               iseqs = iseqs + 1
               ibyts = ibyts + i

            ELSE
               WRITE (scrstr,*) minlen
               CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
     -              fun_ID // ': ' //
     -              'Warning: Entry ' // locstr // ' shorter than ' //
     -              scrstr(str_beg(scrstr):str_end(scrstr)) // 
     -              ' residues' // LF // '(entry ignored)' )
            END IF

         END IF
         
         ilin = ilin + 1

      END DO
      
      RETURN
      END 
      SUBROUTINE feel_outsiz ( x_seqbeg, x_seqend, x_links,
     -                         y_seqbeg, y_seqend, y_links, 
     -                          alibyts )
C
C Terrible routine here, clean up, improve
C
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
      INCLUDE 'pal_settings.inc'

      INTEGER x_seqbeg
      INTEGER x_seqend
      BYTE    x_links (x_seqbeg:x_seqend)
      INTEGER y_seqbeg
      INTEGER y_seqend
      BYTE    y_links (y_seqbeg:y_seqend)
      INTEGER alibyts
C
C Local variables
C
      CHARACTER*11 fun_ID  / 'FEEL_OUTSIZ' /
      CHARACTER*1      LF  /            10 /

      INTEGER xlnks, ylnks
      INTEGER x_seqpos, y_seqpos
      INTEGER alipos, seqpos
C
C --------------------------------------------------------------------------
C
      xlnks = 0
      DO seqpos = x_seqbeg, x_seqend
         IF ( x_links(seqpos).GT.0 ) xlnks = xlnks + 1 
      END DO

      ylnks = 0
      DO seqpos = y_seqbeg, y_seqend
         IF ( y_links(seqpos).GT.0 ) ylnks = ylnks + 1 
      END DO
C
C If there are matches in one seq and not the other, alert and return,
C
      IF ( xlnks.GT.0 .AND. ylnks.EQ.0 ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: Matches only marked in x_seq'      // LF // 
     -        'This is most likely a bug; program stopped. Contact' // LF // 
     -        author_ID//', '//email_ID ) 
         STOP
      END IF
      
      IF ( ylnks.GT.0 .AND. xlnks.EQ.0 ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: Matches only marked in y_seq'      // LF // 
     -        'This is most likely a bug; program stopped. Contact' // LF // 
     -        author_ID//', '//email_ID ) 
         STOP
      END IF
C
C No matches found, return length of longest sequence,
C      
      IF ( xlnks.EQ.0 .AND. ylnks.EQ.0 ) THEN
         alibyts = MAX ( x_seqend-x_seqbeg, y_seqend-y_seqbeg ) + 1
         RETURN
      END IF
C
C This only works if there is a matching segment,
C
      x_seqpos = x_seqbeg      
      DO WHILE (x_links(x_seqpos).EQ.0) 
         x_seqpos = x_seqpos + 1 
      END DO

      y_seqpos = y_seqbeg    
      DO WHILE (y_links(y_seqpos).EQ.0) 
         y_seqpos = y_seqpos + 1 
      END DO

      alipos = 0
C
C Dangling x 5' end
C
      IF (x_seqpos.GE.y_seqpos) THEN
         x_seqpos = x_seqpos - y_seqpos + 1
         y_seqpos = y_seqbeg
         DO seqpos = x_seqbeg, x_seqpos-1   ! improve all this later
            alipos = alipos + 1 
         END DO
C
C Dangling y 5' end
C
      ELSE IF (y_seqpos.GT.x_seqpos) THEN
         y_seqpos = y_seqpos - x_seqpos + 1 
         x_seqpos = x_seqbeg
         DO seqpos = y_seqbeg, y_seqpos-1
            alipos = alipos + 1
         END DO
      END IF
C
C Alignment middle
C
      DO WHILE (x_seqpos.LE.x_seqend   .AND.
     -          y_seqpos.LE.y_seqend)

         alipos = alipos + 1

         IF ( x_links(x_seqpos).EQ.0 .AND. y_links(y_seqpos).EQ.0 ) THEN
            x_seqpos = x_seqpos + 1
            y_seqpos = y_seqpos + 1 
         ELSE IF ( x_links(x_seqpos).EQ.1 .AND. y_links(y_seqpos).EQ.1 ) THEN
            x_seqpos = x_seqpos + 1
            y_seqpos = y_seqpos + 1 
         ELSE IF ( x_links(x_seqpos).EQ.0 .AND. y_links(y_seqpos).EQ.1 ) THEN
            x_seqpos = x_seqpos + 1
         ELSE IF ( x_links(x_seqpos).EQ.1 .AND. y_links(y_seqpos).EQ.0 ) THEN
            y_seqpos = y_seqpos + 1 
         ELSE
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -          'Error: Alignment tracing went wrong, there is a bug.' //LF//
     -          author_ID//', '//email_ID)
            STOP
         END IF

      END DO
C
C Dangling 3' end if any
C
      DO WHILE (x_seqpos.LE.x_seqend)
         alipos = alipos + 1
         x_seqpos = x_seqpos + 1 
      END DO

      DO WHILE (y_seqpos.LE.y_seqend)
         alipos = alipos + 1
         y_seqpos = y_seqpos + 1 
      END DO

      alibyts = alipos

      RETURN
      END
      INTEGER FUNCTION file_size ( lun )

      IMPLICIT none
      
      INTEGER lun
C
C Local variables
C
      INTEGER statarr(13)
      INTEGER iostat, fstat
C
C -------------------------------------------------------------------------------
C
      iostat = fstat (lun, statarr)
      file_size = statarr(8)
      
      RETURN 
      END 
      SUBROUTINE gap_reins ( tlen, tmpl, 
     -                       alen, dbali, inali, inrul,
     -                       olen, outali, outrul, opos ) 
C
C CLEAN UP + EXPLAIN. 
C
      IMPLICIT none

      INCLUDE 'pal_res_eqv.inc'
      INCLUDE 'pal_settings.inc'
      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
C
C Input
C
      INTEGER tlen         ! Length of template sequence
      BYTE    tmpl(tlen)   ! Template sequence containing gaps to be inserted
      INTEGER alen         ! Length of following three 
      BYTE    dbali(alen)  ! Aligned db seq, with original gaps removed
      BYTE    inali(alen)  ! Aligned incoming sequence 
      BYTE    inrul(alen)  ! Aligned ruler which marks uncertain spots
C
C Output
C
      INTEGER olen          ! Max length of output seq with reinserted gaps
      BYTE    outali(olen)  ! Output seq with original gaps reinserted
      BYTE    outrul(olen)  ! Output ruler with original gaps reinserted
      INTEGER opos          ! Length of output seq with reinserted gaps

      INTEGER tpos, apos
      INTEGER diff, t_diff, ires

      INTEGER seq_nxtarr, seq_nxtstr

      INTEGER crtn_beg, crtn_end
      INTEGER ambg_beg, ambg_end

      INTEGER t_crtn_beg, t_crtn_end
      INTEGER t_ambg_beg, t_ambg_end

      INTEGER i
      
      CHARACTER*1 LF / 10 / 
C
C ------------------------------------------------------------------------------
C
      apos = 1
      tpos = 1
      opos = 0 

      DO WHILE (apos.LE.alen)

         IF (inrul(apos).EQ.crtn_ch) THEN
C
C Find beg/end indices of unambigous regions in new sequence (inali) as
C well as in its template (tmpl),
C
            crtn_beg = apos
            crtn_end = seq_nxtstr (1,'.NE.','>',CHAR(crtn_ch),inrul,crtn_beg,alen) - 1

            t_crtn_beg = tpos
            t_crtn_end = seq_nxtarr (crtn_end-crtn_beg+2,'.EQ.','>',
     -                               IUB_base,tmpl,t_crtn_beg,tlen) - 1

            DO tpos = t_crtn_beg, t_crtn_end

               opos = opos + 1
               outrul(opos) = crtn_ch

               IF (IUB_base(tmpl(tpos))) THEN
                  outali(opos) = inali(apos) ! load the residue
                  apos = apos + 1
               ELSE
                  outali(opos) = tmpl(tpos) ! otherwise the non-residue
               END IF

            END DO

            apos =   crtn_end + 1
            tpos = t_crtn_end + 1

         ELSE
C
C Find beg/end indices of ambigous regions in both aligned sequence 
C and template,
C
            ambg_beg = apos
            ambg_end = seq_nxtstr (1,'.EQ.','>',CHAR(crtn_ch),inrul,ambg_beg,alen) - 1

            t_ambg_beg = tpos

            ires = 0
            DO i = ambg_beg, ambg_end 
               IF (IUB_base(dbali(i))) ires = ires + 1
            END DO

            t_ambg_end = seq_nxtarr (ires+1,'.EQ.','>',IUB_base,tmpl,t_ambg_beg,tlen) - 1

              diff = ambg_end-ambg_beg
            t_diff = t_ambg_end-t_ambg_beg

            ires = 0
            DO i = ambg_beg, ambg_end
               IF (IUB_base(inali(i))) ires = ires + 1
            END DO

            apos = ambg_beg-1
C
C Beginning of alignment
C
            IF (ambg_beg.EQ.1) THEN

               IF (t_diff.GE.diff) THEN
                  DO i = 1, t_ambg_end-ires
                     opos = opos + 1
                     outali(opos) = y_gap5
                     outrul(opos) = end5_ch
                  END DO
                  DO i = 1, ires
                     opos = opos + 1
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = amby_ch
                  END DO
               ELSE
                  DO i = 1, diff-t_diff
                     opos = opos + 1
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = ambx_ch
                  END DO
                  DO i = diff-t_diff+1, ambg_end
                     opos = opos + 1
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = amby_ch
                  END DO
               END IF
C
C At the end
C
            ELSE IF (ambg_end.EQ.alen) THEN

               IF (t_diff.GE.diff) THEN
                  DO i = 1, ires
                     opos = opos + 1 
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = amby_ch
                  END DO
                  DO i = 1, t_diff-ires+1
                     opos = opos + 1
                     outali(opos) = y_gap3
                     outrul(opos) = end3_ch
                  END DO
               ELSE
                  DO i = 1, ires - (diff-t_diff) 
                     opos = opos + 1
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = amby_ch
                  END DO
                  DO i = 1, diff-t_diff 
                     opos = opos + 1
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = ambx_ch
                  END DO
               END IF
C
C Middle
C               
            ELSE IF (  ambg_beg.GT.1 .AND.   ambg_end.LT.alen  .AND.
     -               t_ambg_beg.GT.1 .AND. t_ambg_end.LT.tlen ) THEN

               IF (t_diff.GE.diff) THEN
                  DO i = 1, ires
                     opos = opos + 1 
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = amby_ch
                  END DO
                  DO i = 1, t_diff-ires+1
                     opos = opos + 1
                     outali(opos) = y_gap4
                     outrul(opos) = amby_ch
                  END DO
               ELSE
                  DO i = 1, ires - (diff-t_diff) 
                     opos = opos + 1
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = amby_ch
                  END DO
                  DO i = 1, diff-t_diff 
                     opos = opos + 1
                     apos = seq_nxtarr (1,'.EQ.','>',IUB_base,inali,apos+1,alen)
                     outali(opos) = inali(apos)
                     outrul(opos) = ambx_ch
                  END DO
               END IF
            ELSE
               CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -               'GAPS_REINS: Error: Indices out of register.' //LF//
     -               'This is a bug, please contact '//author_ID//',' //LF//
     -                email_ID  //LF// 
     -                'Program stopped')
               STOP
            END IF
                  
            apos =   ambg_end + 1
            tpos = t_ambg_end + 1
         END IF

      END DO

      RETURN
      END


      INTEGER FUNCTION seq_nxtarr ( rept, eqstr, dirstr, symarr, bytarr, begpos, endpos )
C
C 
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      INTEGER       rept
      CHARACTER*(*) eqstr
      CHARACTER*(*) dirstr
      LOGICAL*1     symarr(0:*) 
      BYTE          bytarr(*)
      INTEGER       begpos
      INTEGER       endpos
C
C Local variables
C
      CHARACTER*10 program_ID / 'SEQ_NXTARR' /
      INTEGER incr, ir
      CHARACTER*1 LF / 10 / 
C
C -----------------------------------------------------------------------------
C
      IF (dirstr.EQ.'>') THEN
         incr = 1
      ELSE IF (dirstr.EQ.'<') THEN
         incr = -1
      ELSE
         CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), program_ID // 
     -      'Error: Unknown direction operator "'//dirstr//'", '//LF//
     -      'program stopped.')
         STOP
      END IF

      ir = 0

      IF (eqstr.EQ.'.EQ.') THEN

         DO seq_nxtarr = begpos, endpos, incr
            IF (symarr(bytarr(seq_nxtarr))) ir = ir + 1
            IF (ir.GE.rept) RETURN
         END DO

      ELSE IF (eqstr.EQ.'.NE.') THEN

         DO seq_nxtarr = begpos, endpos, incr
            IF (.NOT.symarr(bytarr(seq_nxtarr))) ir = ir + 1
            IF (ir.GE.rept) RETURN
         END DO

      ELSE 
         CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), program_ID // 
     -      'Error: Unknown comparison operator "'//eqstr//'", '//LF//
     -      'program stopped.')
         STOP
      END IF

      RETURN
      END 


      INTEGER FUNCTION seq_nxtstr ( rept, eqstr, dirstr, symstr, bytarr, begpos, endpos )
C
C 
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      INTEGER       rept
      CHARACTER*(*) eqstr
      CHARACTER*(*) dirstr
      CHARACTER*(*) symstr
      BYTE          bytarr(*)
      INTEGER       begpos
      INTEGER       endpos
C
C Local variables
C
      CHARACTER*10 program_ID / 'SEQ_NXTSTR' /
      INTEGER incr, ir
      INTEGER INDEX
      CHARACTER*1 LF / 10 / 
C
C -----------------------------------------------------------------------------
C
      IF (dirstr.EQ.'>') THEN
         incr = 1
      ELSE IF (dirstr.EQ.'<') THEN
         incr = -1
      ELSE
         CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), program_ID // 
     -      'Error: Unknown direction operator "'//dirstr//'", '//LF//
     -      'program stopped.')
         STOP
      END IF

      ir = 0

      IF (eqstr.EQ.'.EQ.') THEN

         DO seq_nxtstr = begpos, endpos, incr
            IF (INDEX(symstr,CHAR(bytarr(seq_nxtstr))).GT.0) ir = ir + 1
            IF (ir.GE.rept) RETURN
         END DO

      ELSE IF (eqstr.EQ.'.NE.') THEN

         DO seq_nxtstr = begpos, endpos, incr
            IF (INDEX(symstr,CHAR(bytarr(seq_nxtstr))).EQ.0) ir = ir + 1
            IF (ir.GE.rept) RETURN
         END DO

      ELSE 
         CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), program_ID // 
     -      'Error: Unknown comparison operator "'//eqstr//'", '//LF//
     -      'program stopped.')
         STOP
      END IF

      RETURN
      END 

      SUBROUTINE get_gbk (inlun,isbase,seqdim,minseqlen,
     -                    seqsarr,sbegsarr,sendsarr,
     -                    alisarr,abegsarr,aendsarr,
     -                    sidsarr,lidsarr)
C
C Extracts short-ID's, sequences and aligned sequences from a file with
C multiple Genbank formatted entries. 
C 
      IMPLICIT none

      INTEGER       inlun                ! in
      LOGICAL*1     isbase (0:*)         ! in
      INTEGER       seqdim               ! in
      INTEGER       minseqlen            ! in 
      BYTE          seqsarr(*)           ! out
      INTEGER       sbegsarr(seqdim)     ! out
      INTEGER       sendsarr(seqdim)     ! out
      BYTE          alisarr(*)           ! out
      INTEGER       abegsarr(seqdim)     ! out
      INTEGER       aendsarr(seqdim)     ! out
      CHARACTER*(*) sidsarr(seqdim)      ! out
      CHARACTER*(*) lidsarr(seqdim)      ! out
C
C Local variables
C
      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      CHARACTER*80  inline
c      CHARACTER*80  locstr, scrstr
      CHARACTER*80  sidline, lidline
      CHARACTER*7   fun_ID  / 'GET_GBK' /
      CHARACTER*1   LF      / 10 /

      BYTE          bytech

c      INTEGER       str_beg, str_end
      INTEGER       iloc, itrm, ipos
      INTEGER       iseqs, ivals, ichrs
      INTEGER       linend
      INTEGER       i,j

      LOGICAL       dfnflg, orgflg
C
C -------------------------------------------------------------------------
C
      iseqs = 0   ! sequences
      ivals = 0   ! valid residues
      ichrs = 0   ! non-blank characters

      iloc = 0   ! LOCUS line count

      dfnflg = .FALSE.  ! true after definition, false after sequence read
      orgflg = .FALSE.  ! true after organism, false after sequence read

      DO WHILE (.TRUE.)

         READ (inlun,'(q,a)', END = 98, ERR = 99) linend, inline

         IF (inline(1:5).EQ.'LOCUS') THEN 

            iloc = iloc + 1
C            print*,'get_gbk; iloc = ',iloc
            sidline = inline (13:22)

         ELSE IF (inline(1:10).EQ.'DEFINITION') THEN 

            dfnflg = .TRUE.
            lidline = inline (13:linend)

         ELSE IF (inline(3:10).EQ.'ORGANISM') THEN 

            orgflg = .TRUE.
            lidline = inline(13:linend)

         ELSE IF (inline(1:6).EQ.'ORIGIN') THEN

            READ (inlun,'(q,a)', END = 98, ERR = 99) linend, inline
            
            i = 0
            j = 0
            
            DO WHILE (inline(1:2).EQ.'  ')

               DO ipos = 11, linend
                  bytech = ICHAR ( inline(ipos:ipos) )
                  IF ( bytech.NE.32 ) THEN
                     i = i + 1
                     alisarr(ichrs+i) = bytech
                     IF ( isbase(bytech) ) THEN
                        j = j + 1 
                        seqsarr(ivals+j) = bytech
                     END IF
                  END IF
               END DO
C               print*,'inner loop; inline(1:2) = ',inline(1:2)

               READ (inlun,'(q,a)', ERR = 99) linend, inline
            END DO
            
            IF (inline(1:2).EQ.'//') THEN

               itrm = itrm + 1

c$$$               IF (iloc.GT.itrm) THEN
c$$$                  CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
c$$$     -                fun_ID // ': ' //
c$$$     -                'Error: Entry previous to ' // locstr //
c$$$     -                ' is not properly terminated')
c$$$                  STOP
c$$$               ELSE IF (iloc.LT.itrm) THEN
c$$$                  IF (iloc.EQ.0) THEN
c$$$                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
c$$$     -                    'Error: First entry is missing a LOCUS line' // LF // 
c$$$     -                    'Program stop')
c$$$                  ELSE
c$$$                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
c$$$     -                    'Error: Entry following ' // locstr //
c$$$     -                    ' is missing a LOCUS line' // LF // 
c$$$     -                    'Program stop')
c$$$                  END IF
c$$$                  STOP
c$$$               END IF

            END IF

            IF ( j.GE.minseqlen ) THEN
               
               iseqs = iseqs + 1
               
               abegsarr(iseqs) = ichrs + 1 
               aendsarr(iseqs) = ichrs + i
               ichrs = ichrs + i

               sbegsarr(iseqs) = ivals + 1 
               sendsarr(iseqs) = ivals + j
               ivals = ivals + j
               
               sidsarr(iseqs) = sidline

               IF (dfnflg.OR.orgflg) THEN
                  lidsarr(iseqs) = lidline
               ELSE 
                  lidsarr(iseqs) = ' '
               END IF
 
               dfnflg = .FALSE.
               orgflg = .FALSE.
            
            ELSE
c$$$               WRITE (scrstr,*) minseqlen
c$$$               CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
c$$$     -              'Warning: Entry ' // sidline(str_beg(sidline):str_end(sidline)) //
c$$$     -              ' shorter than ' // scrstr(str_beg(scrstr):str_end(scrstr)) // 
c$$$     -              ' residues' // LF // '(entry ignored)' )
            END IF
            
         END IF
C         print*,' outer loop..'
      END DO

 98   RETURN

 99   CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' // 
     -    'Error: Unexpected end of file reached' // LF // 
     -    'Program stop')
      STOP

      END 

      SUBROUTINE get_keys (lstlun, maxndx, sidarr, begndx, endndx, fldnum, fldsep)

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

      SUBROUTINE get_memory (memptr,bytes,arrstr,substr)
C
C Allocates 'bytes' bytes of memory, starting at the memory adress memptr 
C     
      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      INTEGER bytes, malloc, memptr

      CHARACTER*(*) arrstr
      CHARACTER*(*) substr
C
C -------------------------------------------------------------------------------
C
      memptr = MALLOC (bytes)

      IF (memptr.EQ.0) THEN
         CALL write_message ( IO_luns(ndx_err),IO_fnms(ndx_err),
     -      'Error: Not enough memory for array ' // arrstr //
     -      ' in procedure ' // substr )
         STOP
      END IF

      RETURN
      END
      SUBROUTINE get_seqlst (lun, isbase, minlen, 
     -                       seqdim, resdim, 
     -                       seqs, sbegs, sends,
     -                       alis, abegs, aends )
      
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      INTEGER    lun
      LOGICAL*1  isbase (0:*)
      INTEGER    minlen
      INTEGER    seqdim
      INTEGER    resdim
      BYTE       seqs (resdim)
      INTEGER    sbegs (seqdim)
      INTEGER    sends (seqdim)
      BYTE       alis (resdim)
      INTEGER    abegs (seqdim)
      INTEGER    aends (seqdim)
C
C Local variables
C
      CHARACTER*8   fun_ID  / 'GET_SEQLST' /

      CHARACTER*10000 inline            ! REMOVE THIS LIMIT
      CHARACTER*20 intstr1, intstr2
      CHARACTER*1  LF / 10 /

      INTEGER ires, iseq, i, j, linend, chrs, ipos
      INTEGER str_beg, str_end

      BYTE bytech
C
C ------------------------------------------------------------------------------
C
      iseq = 0
      ires = 0 
      chrs = 0

      DO WHILE (.TRUE.)

         READ (lun,'(q,a)', ERR=99) linend, inline(1:linend)

         i = 0
         j = 0
      
         DO ipos = 1, linend
            bytech = ICHAR (inline(ipos:ipos))
            i = i + 1 
            alis(chrs+i) = bytech
            IF ( isbase(bytech) ) THEN
               j = j + 1
               seqs (ires+j) = bytech
            END IF
         END DO

         IF ( j.GE.minlen ) THEN

            iseq = iseq + 1

            abegs (iseq) = chrs + 1
            aends (iseq) = chrs + i

            chrs = chrs + i
            
            sbegs (iseq) = ires + 1
            sends (iseq) = ires + j
            ires = ires + j
            
         ELSE
            WRITE (intstr1,*) minlen
            WRITE (intstr2,*) iseq + 1
            CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
     -           fun_ID // ': ' // 'Error: Entry number ' // 
     -           intstr2(str_beg(intstr2):str_end(intstr2)) // 
     -           ' is shorter than ' //
     -           intstr1(str_beg(intstr1):str_end(intstr1)) // 
     -           ' residues' // LF // 'Program stopped' )
            STOP
         END IF

      END DO

 99   RETURN
      END

      SUBROUTINE get_tbl (lun, isbase, minlen, 
     -                         seqdim, resdim, 
     -                         seqs, sbegs, sends) 
      
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      INTEGER    lun
      LOGICAL*1  isbase (0:*)
      INTEGER    minlen
      INTEGER    seqdim
      INTEGER    resdim
      BYTE       seqs (resdim)
      INTEGER    sbegs (seqdim)
      INTEGER    sends (seqdim)
C
C Local variables
C
      CHARACTER*8   fun_ID  / 'GET_TBL' /

      CHARACTER*10000 inline            ! REMOVE THIS LIMIT
      CHARACTER*20 intstr1, intstr2
      CHARACTER*1  LF / 10 /

      INTEGER ires, iseq, j, linend, ipos
      INTEGER str_beg, str_end

      BYTE bytech
C
C ------------------------------------------------------------------------------
C
      iseq = 0
      ires = 0 

      DO WHILE (.TRUE.)

         READ (lun,'(q,a)', ERR=99) linend, inline(1:linend)
C         WRITE (6,'(a)') inline(1:linend)

         j = 0
         DO ipos = 1, linend
            bytech = ICHAR (inline(ipos:ipos))
            IF ( isbase(bytech) ) THEN
               j = j + 1
               seqs (ires+j) = bytech
C               write (6,'(a,$)') char(bytech)
            END IF
         END DO

         IF ( j.GE.minlen ) THEN

            iseq = iseq + 1

            sbegs (iseq) = ires + 1
            sends (iseq) = ires + j
            ires = ires + j
            
         ELSE
            WRITE (intstr1,*) minlen
            WRITE (intstr2,*) iseq + 1
            CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
     -           fun_ID // ': ' // 'Error: Entry number ' // 
     -           intstr2(str_beg(intstr2):str_end(intstr2)) // 
     -           ' is shorter than ' //
     -           intstr1(str_beg(intstr1):str_end(intstr1)) // 
     -           ' residues' // LF // 'Program stopped' )
            STOP
         END IF
C         print*,' ires = ',ires

      END DO

 99   RETURN
      END

      SUBROUTINE get_time (date,year,month,day,hour,minute,second)

      IMPLICIT none

      CHARACTER*(*) date
      CHARACTER*(*) year
      CHARACTER*(*) month
      CHARACTER*(*) day
      CHARACTER*(*) hour
      CHARACTER*(*) minute
      CHARACTER*(*) second
C
C Local variables
C
      CHARACTER*24 string
C
C -----------------------------------------------------------------------------
C
      CALL fdate (string) 
C
C Expand day
C
      IF (string(1:3).EQ.'Sun') THEN
         day = 'Sunday'
      ELSE IF (string(1:3).EQ.'Mon') THEN
         day = 'Monday'
      ELSE IF (string(1:3).EQ.'Tue') THEN
         day = 'Tuesday'
      ELSE IF (string(1:3).EQ.'Wed') THEN
         day = 'Wednesday'
      ELSE IF (string(1:3).EQ.'Thu') THEN
         day = 'Thursday'
      ELSE IF (string(1:3).EQ.'Fri') THEN
         day = 'Friday'
      ELSE IF (string(1:3).EQ.'Sat') THEN
         day = 'Saturday'
      ELSE 
         day = ' '
      END IF
C
C Expand month
C
      IF (string(5:7).EQ.'Jan') THEN
         month = 'January'
      ELSE IF (string(5:7).EQ.'Feb') THEN
         month = 'February'
      ELSE IF (string(5:7).EQ.'Mar') THEN
         month = 'March'
      ELSE IF (string(5:7).EQ.'Apr') THEN
         month = 'April'
      ELSE IF (string(5:7).EQ.'May') THEN
         month = 'May'
      ELSE IF (string(5:7).EQ.'Jun') THEN
         month = 'June'
      ELSE IF (string(5:7).EQ.'Jul') THEN
         month = 'July'
      ELSE IF (string(5:7).EQ.'Aug') THEN
         month = 'August'
      ELSE IF (string(5:7).EQ.'Sep') THEN
         month = 'September'
      ELSE IF (string(5:7).EQ.'Oct') THEN
         month = 'October'
      ELSE IF (string(5:7).EQ.'Nov') THEN
         month = 'November'
      ELSE IF (string(5:7).EQ.'Dec') THEN
         month = 'December'
      ELSE
         month = ' '
      END IF

      date = string (9:10)
      hour = string (12:13)
      minute = string (15:16)
      second = string (18:19)
      year = string (21:24)

      RETURN
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
      SUBROUTINE lnk_lst_diag (min_dia_len,wrd_len,
     -            x_iol_seq,y_iol_seq,
     -            x_lnk_bgs,x_lnk_lst,
     -            x_low_bnd,x_hig_bnd,
     -            y_low_bnd,y_hig_bnd,
     -            x_bst_beg,x_bst_end,
     -            y_bst_beg,y_bst_end,
     -            dia_flg)
C
C Linked list search - improve!
C
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_settings.inc'
      INCLUDE 'pal_res_eqv.inc'
      INCLUDE 'pal_cmdlin.inc'
C
C Input
C
      INTEGER min_dia_len             ! in
      INTEGER wrd_len                 ! in
      INTEGER x_iol_seq (*)           ! in
      INTEGER y_iol_seq (*)           ! in
      INTEGER x_lnk_bgs(0:*)          ! in
      INTEGER x_lnk_lst (*)           ! in
      INTEGER x_low_bnd,x_hig_bnd     ! in
      INTEGER y_low_bnd,y_hig_bnd     ! in
      INTEGER x_bst_beg,x_bst_end     ! out
      INTEGER y_bst_beg,y_bst_end     ! out
      LOGICAL dia_flg                 ! out
C
C Local variables
C
      CHARACTER*200 errstr
      INTEGER x_max_oli_pos,y_max_oli_pos
      INTEGER x_oli_pos,y_oli_pos
      INTEGER incr,max_incr
      LOGICAL incr_flg
C
C ---------------------------------------------------------------------------
C
      dia_flg = .FALSE.
C
C Return if min_dia_len < word length
C
      IF (min_dia_len.LT.wrd_len) THEN
         dia_flg = .FALSE.
         errstr = 'LINK_LST_DIAG: Error: min_dia_len < wrd_len'
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),errstr)
         RETURN
      END IF

      x_max_oli_pos = x_hig_bnd - wrd_len + 1
      y_max_oli_pos = y_hig_bnd - wrd_len + 1
C
C For each y_seq oligo position, look down the chain of positions in x_seq
C for one within x_low_bnd and x_max_oli_pos. See if the match can be extended 
C to min_dia_len or better, and return if so.
C
      DO y_oli_pos = y_low_bnd, y_max_oli_pos
         
         x_oli_pos = x_lnk_bgs (y_iol_seq(y_oli_pos))

         DO WHILE ( x_oli_pos .LE. x_max_oli_pos  .AND. 
     -              x_oli_pos .GT. 0 )
            
            IF (x_oli_pos.GE.x_low_bnd) THEN
               
               max_incr = MIN (x_max_oli_pos - x_oli_pos,
     -                         y_max_oli_pos - y_oli_pos)
C
C Is there a better way to avoid an array-out-of-bounds
C
               incr = 0            
               incr_flg = incr + 1 .LE. max_incr

               DO WHILE (incr_flg) 

                  IF (x_iol_seq (x_oli_pos + incr + 1) .EQ. 
     -                y_iol_seq (y_oli_pos + incr + 1)) THEN
                     incr = incr + 1
                  ELSE
                     incr_flg = .FALSE.
                  END IF

                  incr_flg = incr_flg  .AND.  incr + 1 .LE. max_incr

               END DO

               IF (incr.GE.min_dia_len-wrd_len) THEN
                  x_bst_beg = x_oli_pos
                  x_bst_end = x_oli_pos + wrd_len - 1 + incr
                  y_bst_beg = y_oli_pos 
                  y_bst_end = y_oli_pos + wrd_len - 1 + incr
                  dia_flg = .TRUE.
                  RETURN
               END IF

            END IF 
               
            x_oli_pos = x_lnk_lst(x_oli_pos)

         END DO
         
      END DO

      RETURN
      END
      SUBROUTINE lo_test ( x_seqbeg, x_seqend, x_seq, x_links,
     -                     y_seqbeg, y_seqend, y_seq, y_links, 
     -                       alibeg,   aliend, x_ali, y_ali, a_mrk )

      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
      INCLUDE 'pal_settings.inc'

      INTEGER x_seqbeg
      INTEGER x_seqend
      BYTE    x_seq   (x_seqbeg:x_seqend)
      BYTE    x_links (x_seqbeg:x_seqend)
      INTEGER y_seqbeg
      INTEGER y_seqend
      BYTE    y_seq   (y_seqbeg:y_seqend)
      BYTE    y_links (y_seqbeg:y_seqend)
      INTEGER alibeg
      INTEGER aliend
      BYTE    x_ali (alibeg:aliend)
      BYTE    y_ali (alibeg:aliend)
      BYTE    a_mrk (alibeg:aliend)
C
C Local variables
C
      CHARACTER*11 fun_ID  / 'LOAD_OUTALI' /
      CHARACTER*1      LF  /            10 /

      INTEGER x_seqpos, y_seqpos
      INTEGER x_first, y_first
      INTEGER alipos, diff 
C
C --------------------------------------------------------------------------
C
      WRITE (6,'(a,1x)') 'end5_ch = ' // char(end5_ch)
      WRITE (6,'(a,1x)') 'end3_ch = ' // char(end3_ch)
      WRITE (6,'(a,1x)') 'ambx_ch = ' // char(ambx_ch)
      WRITE (6,'(a,1x)') 'amby_ch = ' // char(amby_ch)
      WRITE (6,'(a,1x)') 'crtn_ch = ' // char(crtn_ch)
      WRITE (6,'(a,1x)') 'unal_ch = ' // char(unal_ch)
C
C Are there matching segments on x sequence (db sequence)?  In that
C case x_seqpos will be .LE. x_seqend, x_seqend+1 otherwise. 
C
      DO x_first = x_seqbeg, x_seqend
         IF ( x_links(x_first).GT.0 ) GO TO 10
      END DO

 10   CONTINUE
      print*,' x_first = ',x_first
C
C Are there matching segments on y sequence (new sequence)?  In that
C case y_first will be .LE. y_seqend, y_seqend+1 otherwise. 
C
      DO y_first = y_seqbeg, y_seqend
         IF ( y_links(y_first).GT.0 ) GO TO 20
      END DO

 20   CONTINUE 
      print*,' y_first = ',y_first
C
C If there are matches in one seq and not the other, alert and return,
C
      IF ( x_first.LE.x_seqend .AND. y_first.GT.y_seqend ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: Matches only marked in x_seq'      // LF // 
     -        'This is most likely a bug; program stopped. Contact' // LF // 
     -         author_ID//', '//email_ID ) 
         STOP
      END IF
      
      IF ( y_first.LE.y_seqend .AND. x_first.GT.x_seqend ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: Matches only marked in y_seq'      // LF // 
     -        'This is most likely a bug; program stopped. Contact' // LF // 
     -         author_ID//', '//email_ID ) 
         STOP
      END IF
C
C No fragments aligned with certainty
C -----------------------------------
C Simply left-justify the sequences and pad the 3-ends.  Fill the ruler 
C with special character (default '?') and return.
C      
      IF ( x_first.GT.x_seqend .AND. y_first.GT.y_seqend ) THEN

         IF (x_seqend.LE.y_seqend) THEN
            DO alipos = 1, x_seqend
               x_ali (alipos) = x_seq (alipos)
               y_ali (alipos) = y_seq (alipos)
            END DO
            DO alipos = x_seqend + 1, y_seqend
               x_ali (alipos) = x_gap3
               y_ali (alipos) = y_seq (alipos)
            END DO
         ELSE
            DO alipos = 1, y_seqend
               x_ali (alipos) = x_seq (alipos)
               y_ali (alipos) = y_seq (alipos)
            END DO
            DO alipos = y_seqend + 1, x_seqend
               x_ali (alipos) = x_seq (alipos)
               y_ali (alipos) = y_gap3
            END DO
         END IF

         DO alipos = 1, MAX (x_seqend,y_seqend)
            a_mrk (alipos) = unal_ch
         END DO

         RETURN

      END IF
C
C At least one matching fragment
C ------------------------------
C 
C Copy the parts up to the first matching fragment. 
C
      diff = ABS (y_first - x_first) + 1

      IF (x_first.GE.y_first) THEN
         y_seqpos = 1
         DO alipos = x_seqbeg, x_first - 1
            x_ali(alipos) = x_seq(alipos)
            a_mrk(alipos) = end5_ch
            IF (alipos.GE.diff) THEN
               y_ali(alipos) = y_seq(y_seqpos)
               y_seqpos = y_seqpos + 1
            ELSE
               y_ali(alipos) = y_gap3
            END IF
         END DO
         x_seqpos = alipos - 1
      ELSE
         x_seqpos = 1
         DO alipos = y_seqbeg, y_first - 1
            y_ali(alipos) = y_seq(alipos)
            a_mrk(alipos) = end5_ch
            IF (alipos.GE.diff) THEN
               x_ali(alipos) = x_seq(x_seqpos)
               x_seqpos = x_seqpos + 1
            ELSE
               x_ali(alipos) = x_gap3
            END IF
         END DO
         y_seqpos = alipos 
      END IF
      
      alipos = alipos - 1     ! because it was used as loop variable

      print*,' x_seqpos, y_seqpos, alipos = ', x_seqpos, y_seqpos, alipos
               
c$$$      y_seqpos = y_seqpos - x_seqpos + 1 
c$$$      x_seqpos = x_seqbeg
C     
C Alignment middle
C
      DO WHILE (x_seqpos.LE.x_seqend   .AND.
     -          y_seqpos.LE.y_seqend)

         alipos = alipos + 1

         IF ( x_links(x_seqpos).EQ.0 .AND. y_links(y_seqpos).EQ.0 ) THEN
            x_ali(alipos) = x_seq(x_seqpos)
            y_ali(alipos) = y_seq(y_seqpos)
            a_mrk(alipos) = amby_ch
            x_seqpos = x_seqpos + 1
            y_seqpos = y_seqpos + 1 
         ELSE IF ( x_links(x_seqpos).EQ.1 .AND. y_links(y_seqpos).EQ.1 ) THEN
            x_ali(alipos) = x_seq(x_seqpos)
            y_ali(alipos) = y_seq(y_seqpos)
            a_mrk(alipos) = crtn_ch
            x_seqpos = x_seqpos + 1
            y_seqpos = y_seqpos + 1 
         ELSE IF ( x_links(x_seqpos).EQ.0 .AND. y_links(y_seqpos).EQ.1 ) THEN
            x_ali(alipos) = x_seq(x_seqpos)
            y_ali(alipos) = y_gap4
            a_mrk(alipos) = amby_ch
            x_seqpos = x_seqpos + 1
         ELSE IF ( x_links(x_seqpos).EQ.1 .AND. y_links(y_seqpos).EQ.0 ) THEN
c$$$            ipos = alipos - 1
c$$$            DO WHILE (ipos.GE.1 .AND. a_mrk(ipos).EQ.amby_ch)
c$$$               a_mrk(ipos) = ambx_ch
c$$$               ipos = ipos - 1
c$$$            END DO
            x_ali(alipos) = x_gap4
            y_ali(alipos) = y_seq(y_seqpos)
            a_mrk(alipos) = ambx_ch
            y_seqpos = y_seqpos + 1 
         ELSE
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -          'Error: Alignment tracing went wrong, there is a bug.' //LF//
     -          'Contact '//author_ID//', '//email_ID)
            STOP
         END IF

      END DO
C
C Dangling 3' end if any
C
      DO WHILE (x_seqpos.LE.x_seqend)
         alipos = alipos + 1
         x_ali(alipos) = x_seq(x_seqpos)
         y_ali(alipos) = y_gap3
         a_mrk(alipos) = end3_ch
         x_seqpos = x_seqpos + 1 
      END DO

      DO WHILE (y_seqpos.LE.y_seqend)
         alipos = alipos + 1
         x_ali(alipos) = x_gap3
         y_ali(alipos) = y_seq(y_seqpos)
         a_mrk(alipos) = end3_ch
         y_seqpos = y_seqpos + 1 
      END DO

      IF ( alipos.NE.aliend ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: alipos <> alilen'      // LF // 
     -        'This is a bug, program stopped. Contact' // LF // 
     -        author_ID//', '//email_ID ) 
         STOP
      END IF

      RETURN
      END
      SUBROUTINE load_gbtxt ( lindim, gbtext, isbase, minlen, seqdim, resdim,
     -                        seqs, begs, ends, abgs, ands, sids, lids )
C
C UPDATE
C It looks through one or more Genbank formatted entries and returns 
C number of sequences (iseqs) and number of characters (ibyts) that 
C are flagged by isbase.
C
C Program is stopped if terminator lines ('//') dont always follow 
C LOCUS lines and vice versa
C 
      IMPLICIT none

      INTEGER       lindim
      CHARACTER*(*) gbtext (lindim)
      LOGICAL*1     isbase (0:*)
      INTEGER       minlen
      INTEGER       seqdim
      INTEGER       resdim
      BYTE          seqs (resdim)
      INTEGER       begs (seqdim)
      INTEGER       ends (seqdim)
      INTEGER       abgs (seqdim)
      INTEGER       ands (seqdim)
      CHARACTER*(*) sids (seqdim)
      CHARACTER*(*) lids (seqdim)
C
C Local variables
C
      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'

      CHARACTER*10  scrstr
      CHARACTER*8   fun_ID  / 'LOAD_GBK' /
      CHARACTER*1   LF      / 10 / 

      BYTE        bytchr

      INTEGER     str_beg, str_end
      INTEGER     iloc, itrm, iseq, ipos, ires, ilin
      INTEGER     locptr, dfnptr, orgptr, oriptr
      LOGICAL     dfnflg, orgflg
      INTEGER     i
C   
C -------------------------------------------------------------------------
C
      iloc = 0
      itrm = 0
      iseq = 0
      ires = 0

      ilin = 1

      dfnflg = .FALSE.
      orgflg = .FALSE.

      DO WHILE ( ilin.LE.lindim )

         IF ( gbtext(ilin)(1:5).EQ.'LOCUS' ) THEN 

            locptr = ilin
            iloc = iloc + 1

         ELSE IF ( gbtext(ilin)(1:10).EQ.'DEFINITION' ) THEN 

            dfnflg = .TRUE.
            dfnptr = ilin

         ELSE IF ( gbtext(ilin)(3:10).EQ.'ORGANISM' ) THEN 

            orgflg = .TRUE.
            orgptr = ilin

         ELSE IF ( gbtext(ilin)(1:6).EQ.'ORIGIN' ) THEN

            oriptr = ilin
            ilin = ilin + 1 
            i = 0
            
            DO WHILE ( gbtext(ilin)(1:2).EQ.'  ' )

               DO ipos = 11, 80
                  bytchr = ICHAR ( gbtext(ilin)(ipos:ipos) )
                  IF ( isbase(bytchr) ) THEN
                     i = i + 1
                     seqs (ires+i) = bytchr
                  END IF
               END DO

               ilin = ilin + 1

            END DO

            IF ( gbtext(ilin)(1:2).EQ.'//' ) THEN

               itrm = itrm + 1

               IF (iloc.GT.itrm) THEN
                  CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
     -                fun_ID // ': ' //
     -                'Error: Entry previous to ' // gbtext(locptr)(13:22) //
     -                ' is not properly terminated')
                  STOP
               ELSE IF (iloc.LT.itrm) THEN
                  IF (iloc.EQ.0) THEN
                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
     -                    'Error: First entry is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  ELSE
                     CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), fun_ID // ': ' //
     -                    'Error: Entry following ' // gbtext(locptr)(13:22) //
     -                    ' is missing a LOCUS line' // LF // 
     -                    'Program stop')
                  END IF
                  STOP
               END IF

            END IF

            IF ( i.GE.minlen ) THEN

               iseq = iseq + 1

               sids (iseq) = gbtext(locptr)(13:22)

               IF ( dfnflg ) THEN
                  lids (iseq) = gbtext (dfnptr)
               ELSE IF ( orgflg ) THEN
                  lids (iseq) = gbtext (orgptr)
               ELSE
                  lids (iseq) = ' '
               END IF

               abgs (iseq) = locptr
               ands (iseq) = oriptr

               begs (iseq) = ires + 1
               ends (iseq) = ires + i

               ires = ires + i

               dfnflg = .FALSE.
               orgflg = .FALSE.

            ELSE
               WRITE (scrstr,*) minlen
               CALL write_message (IO_luns(ndx_err), IO_fnms(ndx_err), 
     -              fun_ID // ': ' // 'Warning: Entry ' // 
     -              gbtext(locptr)(13:22) // ' shorter than ' //
     -              scrstr(str_beg(scrstr):str_end(scrstr)) // 
     -              ' residues' // LF // '(entry ignored)' )
            END IF

         END IF
         
         ilin = ilin + 1

      END DO
      
      RETURN
      END 
      SUBROUTINE load_outali ( x_seqbeg, x_seqend, x_seq, x_links,
     -                         y_seqbeg, y_seqend, y_seq, y_links, 
     -                           alibeg,   aliend, x_ali, y_ali, a_mrk )

      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
      INCLUDE 'pal_settings.inc'

      INTEGER x_seqbeg
      INTEGER x_seqend
      BYTE    x_seq   (x_seqbeg:x_seqend)
      BYTE    x_links (x_seqbeg:x_seqend)
      INTEGER y_seqbeg
      INTEGER y_seqend
      BYTE    y_seq   (y_seqbeg:y_seqend)
      BYTE    y_links (y_seqbeg:y_seqend)
      INTEGER alibeg
      INTEGER aliend
      BYTE    x_ali (alibeg:aliend)
      BYTE    y_ali (alibeg:aliend)
      BYTE    a_mrk (alibeg:aliend)
C
C Local variables
C
      CHARACTER*11 fun_ID  / 'LOAD_OUTALI' /
      CHARACTER*1      LF  /            10 /

      INTEGER xlnks, ylnks
      INTEGER x_seqpos, y_seqpos
      INTEGER alipos, seqpos
C
C --------------------------------------------------------------------------
C
c$$$      WRITE (6,'(a,1x)') 'end5_ch = ' // char(end5_ch)
c$$$      WRITE (6,'(a,1x)') 'end3_ch = ' // char(end3_ch)
c$$$      WRITE (6,'(a,1x)') 'ambx_ch = ' // char(ambx_ch)
c$$$      WRITE (6,'(a,1x)') 'amby_ch = ' // char(amby_ch)
c$$$      WRITE (6,'(a,1x)') 'crtn_ch = ' // char(crtn_ch)

      xlnks = 0
      DO seqpos = x_seqbeg, x_seqend
         IF ( x_links(seqpos).GT.0 ) xlnks = xlnks + 1 
      END DO

      ylnks = 0
      DO seqpos = y_seqbeg, y_seqend
         IF ( y_links(seqpos).GT.0 ) ylnks = ylnks + 1 
      END DO
C
C If there are matches in one seq and not the other, alert and return,
C
      IF ( xlnks.GT.0 .AND. ylnks.EQ.0 ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: Matches only marked in x_seq'      // LF // 
     -        'This is most likely a bug; program stopped. Contact' // LF // 
     -         author_ID//', '//email_ID ) 
         STOP
      END IF
      
      IF ( ylnks.GT.0 .AND. xlnks.EQ.0 ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: Matches only marked in y_seq'      // LF // 
     -        'This is most likely a bug; program stopped. Contact' // LF // 
     -         author_ID//', '//email_ID ) 
         STOP
      END IF
C
C No matches found, load input to output
C      
      IF ( xlnks.EQ.0 .AND. ylnks.EQ.0 ) THEN

         IF (x_seqend.LE.y_seqend) THEN
            DO alipos = 1, x_seqend
               x_ali (alipos) = x_seq (alipos)
               y_ali (alipos) = y_seq (alipos)
            END DO
            DO alipos = x_seqend + 1, y_seqend
               x_ali (alipos) = x_gap3
               y_ali (alipos) = y_seq (alipos)
            END DO
         ELSE
            DO alipos = 1, y_seqend
               x_ali (alipos) = x_seq (alipos)
               y_ali (alipos) = y_seq (alipos)
            END DO
            DO alipos = y_seqend + 1, x_seqend
               x_ali (alipos) = x_seq (alipos)
               y_ali (alipos) = y_gap3
            END DO
         END IF

         DO alipos = 1, MAX (x_seqend,y_seqend)
            a_mrk (alipos) = unal_ch
         END DO

         RETURN

      END IF
C
C This only works if there is a matching segment,
C
      x_seqpos = x_seqbeg      
      DO WHILE (x_links(x_seqpos).EQ.0) 
         x_seqpos = x_seqpos + 1 
      END DO

      y_seqpos = y_seqbeg    
      DO WHILE (y_links(y_seqpos).EQ.0) 
         y_seqpos = y_seqpos + 1 
      END DO

      alipos = 0
C
C Dangling x 5' end
C
      IF (x_seqpos.GT.y_seqpos) THEN
         x_seqpos = x_seqpos - y_seqpos + 1
         y_seqpos = y_seqbeg
         DO seqpos = x_seqbeg, x_seqpos-1
            alipos = alipos + 1 
            x_ali(alipos) = x_seq(seqpos)
            y_ali(alipos) = y_gap5
            a_mrk(alipos) = end5_ch
         END DO
C
C Dangling y 5' end
C
      ELSE IF (y_seqpos.GT.x_seqpos) THEN
         y_seqpos = y_seqpos - x_seqpos + 1 
         x_seqpos = x_seqbeg
         DO seqpos = y_seqbeg, y_seqpos-1
            alipos = alipos + 1
            y_ali(alipos) = y_seq(seqpos)
            x_ali(alipos) = x_gap5
            a_mrk(alipos) = end5_ch
         END DO
      END IF
C
C Alignment middle
C
      DO WHILE (x_seqpos.LE.x_seqend   .AND.
     -          y_seqpos.LE.y_seqend)

         alipos = alipos + 1

         IF ( x_links(x_seqpos).EQ.0 .AND. y_links(y_seqpos).EQ.0 ) THEN
            x_ali(alipos) = x_seq(x_seqpos)
            y_ali(alipos) = y_seq(y_seqpos)
            a_mrk(alipos) = amby_ch
            x_seqpos = x_seqpos + 1
            y_seqpos = y_seqpos + 1 
         ELSE IF ( x_links(x_seqpos).EQ.1 .AND. y_links(y_seqpos).EQ.1 ) THEN
            x_ali(alipos) = x_seq(x_seqpos)
            y_ali(alipos) = y_seq(y_seqpos)
            a_mrk(alipos) = crtn_ch
            x_seqpos = x_seqpos + 1
            y_seqpos = y_seqpos + 1 
         ELSE IF ( x_links(x_seqpos).EQ.0 .AND. y_links(y_seqpos).EQ.1 ) THEN
            x_ali(alipos) = x_seq(x_seqpos)
            y_ali(alipos) = y_gap4
            a_mrk(alipos) = amby_ch
            x_seqpos = x_seqpos + 1
         ELSE IF ( x_links(x_seqpos).EQ.1 .AND. y_links(y_seqpos).EQ.0 ) THEN
c$$$            ipos = alipos - 1
c$$$            DO WHILE (ipos.GE.1 .AND. a_mrk(ipos).EQ.amby_ch)
c$$$               a_mrk(ipos) = ambx_ch
c$$$               ipos = ipos - 1
c$$$            END DO
            x_ali(alipos) = x_gap4
            y_ali(alipos) = y_seq(y_seqpos)
            a_mrk(alipos) = ambx_ch
            y_seqpos = y_seqpos + 1 
         ELSE
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -          'Error: Alignment tracing went wrong, there is a bug.' //LF//
     -          'Contact '//author_ID//', '//email_ID)
            STOP
         END IF

      END DO
C
C Dangling 3' end if any
C
      DO WHILE (x_seqpos.LE.x_seqend)
         alipos = alipos + 1
         x_ali(alipos) = x_seq(x_seqpos)
         y_ali(alipos) = y_gap3
         a_mrk(alipos) = end3_ch
         x_seqpos = x_seqpos + 1 
      END DO

      DO WHILE (y_seqpos.LE.y_seqend)
         alipos = alipos + 1
         x_ali(alipos) = x_gap3
         y_ali(alipos) = y_seq(y_seqpos)
         a_mrk(alipos) = end3_ch
         y_seqpos = y_seqpos + 1 
      END DO

      IF ( alipos.NE.aliend ) THEN
         CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -        fun_ID // ':Error: alipos <> alilen'      // LF // 
     -        'This is a bug, program stopped. Contact' // LF // 
     -         author_ID//', '//email_ID) 
         STOP
      END IF

      RETURN
      END
      INTEGER FUNCTION lun_alloc (maxlun, errstat)
C
C Returns the lowest available logical unit (file handle), except
C 5 and 6 (stdin and stdout)
C 
      IMPLICIT none
      
      INTEGER maxlun
      INTEGER errstat
C
C Local variables
C
      LOGICAL lunopen
      INTEGER ilun
C
C ---------------------------------------------------------------------------------
C
      errstat = 0 

      DO ilun = 1, 4
         INQUIRE ( UNIT = ilun, 
     -           OPENED = lunopen )
         IF (.NOT.lunopen) THEN
            lun_alloc = ilun
            RETURN
         END IF
      END DO
C
C Skip 5 and 6,
C
      DO ilun = 7, maxlun
         INQUIRE ( UNIT = ilun, 
     -           OPENED = lunopen )
         IF (.NOT.lunopen) THEN
            lun_alloc = ilun
            RETURN
         END IF
      END DO
C
C If this point is reached, then no channels available, error, 
C
      errstat = -1

      RETURN
      END
      SUBROUTINE make_iol_seq (alp_siz, wrd_len, 
     -                         dim_beg, dim_end, bas_beg,bas_end,
     -                         ich_seq, iol_seq)
C
C Creates a sequence of unique INTEGERs from a sequence of BYTEs
C
      IMPLICIT none

      INTEGER alp_siz       ! in    alphabet size
      INTEGER wrd_len       ! in    word length
      INTEGER dim_beg       ! in    dimension min  (both input and output)
      INTEGER dim_end       ! in    dimension max  (both input and output)
      INTEGER bas_beg       ! in    translate from
      INTEGER bas_end       ! in    translate to
      BYTE    ich_seq (dim_beg:dim_end)   ! in    sequence of BYTE coded residues
      INTEGER iol_seq (dim_beg:dim_end)   ! out   sequence of unique INTEGERs
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
      SUBROUTINE make_lnk_lst (alp_siz, wrd_len, tup_dim, seq_dim, res_dim,
     -                         ich_sqs,
     -                         seq_min, seq_max, seq_bgs, seq_nds,
     -                         lnk_bgs, lnk_lst)
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
      INTEGER tup_dim
      INTEGER seq_dim
      INTEGER res_dim 
      BYTE    ich_sqs (res_dim)
      INTEGER seq_min
      INTEGER seq_max
      INTEGER seq_bgs (seq_dim)
      INTEGER seq_nds (seq_dim)
      INTEGER lnk_bgs ( 0 : tup_dim - 1 )
      INTEGER lnk_lst (res_dim)
C     
C Local variables
C
      INTEGER  bucket ( 0 : tup_dim - 1 )    
      POINTER (bucketptr,bucket)
      
      INTEGER iseq,ipos,ioli
      INTEGER bas_pos
C
C -------------------------------------------------------------------------------
C
      CALL get_memory (bucketptr, 4 * tup_dim, 'bucket', 'make_lnk_list')

      CALL i4_array_init ( 0, tup_dim - 1, bucket, 0, tup_dim - 1, 0 )
      CALL i4_array_init ( 0, tup_dim - 1, lnk_bgs, 0, tup_dim - 1, 0 )

      CALL i4_array_init ( 1, res_dim, lnk_lst, 
     -                     seq_bgs(seq_min), seq_nds(seq_max), 0 )

      DO iseq = seq_min,seq_max
         
         DO bas_pos = seq_bgs (iseq), seq_nds (iseq) - wrd_len + 1
            
            ioli = 0
            DO ipos = bas_pos,bas_pos + wrd_len - 1
               ioli = alp_siz * ioli + ich_sqs (ipos)
            END DO

            IF ( bucket(ioli).EQ.0 ) THEN
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

      SUBROUTINE make_oli_tables (tupdim,seqdim,resdim,tbldim,
     -                            ends,obegs,llst,
     -                            tblpos,tbl,tbegs,onums)

      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
      INCLUDE 'pal_settings.inc'
C
C Set up tbl, tbegs and onums. Tbl has runs of numbers which stand for the
C sequences where a given oligo occurs. The starting point of each run 
C is stored in tbegs, that uses the oligos as indices. Onums becomes the 
C number of oligos in each sequence.
C
C IMPROVE - DO ioli = 0,old_number-1  is probably dumb
C
      INTEGER   tupdim                    ! in 
      INTEGER   seqdim                    ! in
      INTEGER   resdim                    ! in
      INTEGER   tbldim                    ! in
      INTEGER   ends (seqdim)             ! in
      INTEGER   obegs (0:tupdim-1)        ! in
      INTEGER   llst (resdim)             ! in
      INTEGER   tblpos                    ! out
      INTEGER*2 tbl (tbldim)              ! out
      INTEGER   tbegs (0:tupdim-1)        ! out
      INTEGER   onums (seqdim)            ! out
C
C Local variables
C
      CHARACTER*15  fun_ID / 'MAKE_OLI_TABLES' /
      CHARACTER*1   LF     / 10 /
      INTEGER iseq, ioli
      INTEGER lstpos
C
C -------------------------------------------------------------------------------
C
      DO iseq = 1,seqdim
         onums(iseq) = 0 
      END DO
      
      tblpos = 0 
      
      DO ioli = 0, tupdim-1
         
         tblpos = tblpos + 1 
                  
         IF (tblpos.GT.tbldim) THEN
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -         fun_ID // ': Error: ' // 'old_tbl overflows limit..'// LF // 
     -         'Solution: Increase table size memory estimate, or' // LF //
     -         'Contact '//author_ID//', '//email_ID  // LF //
     -         'Program halted' )
            STOP
         END IF

         iseq = 1

         lstpos = obegs(ioli)
         tbegs(ioli) = tblpos
         
         DO WHILE ( lstpos.GT.0 .AND. iseq.LE.seqdim ) 

            DO WHILE (iseq.LE.seqdim .AND. ends(iseq).LT.lstpos)
               iseq = iseq + 1
            END DO

            tbl(tblpos) = iseq
            onums(iseq) = onums(iseq) + 1
            tblpos = tblpos + 1
            
            IF (tblpos.GT.tbldim) THEN
               CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err),
     -            fun_ID // ': Error: ' // 'old_tbl overflows limit..'// LF // 
     -            'Solution: Increase table size memory estimate, or' // LF //
     -            'Contact '//author_ID//', '//email_ID  // LF //
     -            'Program halted' )
               STOP
            END IF

            DO WHILE (lstpos.GT.0 .AND. ends(iseq).GE.lstpos)
               lstpos = llst(lstpos)
            END DO
            
            iseq = iseq + 1 
            
         END DO
         
         tbl (tblpos) = 0 
         
      END DO

      RETURN
      END 
      INTEGER FUNCTION member (string, list, sepch)

      IMPLICIT none

      CHARACTER*(*) string
      CHARACTER*(*) list
      CHARACTER*1   sepch
C
C -------------------------------------------------------------------------------
C
      INTEGER seplo, sephi
      INTEGER beg, end
      INTEGER listlen

      LOGICAL str_equal
C
C -------------------------------------------------------------------------------
C
      listlen = LEN (list)

      seplo = 0
      sephi = 1

      DO WHILE (seplo.LT.listlen)

         DO WHILE (sephi.LE.listlen .AND. list(sephi:sephi).NE.sepch)
            sephi = sephi + 1
         END DO
         
         beg = seplo + 1
         end = sephi - 1
         
         IF (end-beg.GE.0) THEN
            IF ( str_equal(string,list(beg:end)) ) THEN
               member = beg
               RETURN
            END IF
         END IF
         
         seplo = sephi
         sephi = seplo + 1
         
      END DO
      
      member = 0

      RETURN
      END

      SUBROUTINE parse_cmd_line (usestr, options)

      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
      INCLUDE 'pal_settings.inc'

      CHARACTER*(*) usestr
      CHARACTER*(*) options
C
C Local variables
C
      CHARACTER*200 argstr      ! arbitrary
      CHARACTER*50  optstr      ! arbitrary
      CHARACTER*1 LF / 10 /

      INTEGER errsum, member

      INTEGER str_beg, str_end
      INTEGER totarg, iarg, ndx
      INTEGER IARGC, ACCESS
      INTEGER lun_alloc, err_cod
C
C -------------------------------------------------------------------------------
C
      totarg = iargc()

      IF (totarg.EQ.0) THEN
         WRITE (6,'(a)') usestr
         STOP 
      END IF
C
C Set all IO flags to .FALSE., initialize all IO channels to stdout 
C (unit 6), and blank out all file names: 
C
      CALL by_array_init ( 1, ndx_max, IO_flags, 1, ndx_max, .FALSE. ) 

      CALL i4_array_init ( 1, ndx_max, IO_luns, 1, ndx_max, 6 )

      DO ndx = 1, ndx_max
         IO_fnms(ndx) = ' '
      END DO
C
C First find out if there is an error file specified on the command
C line; if so, direct all error messages to that file.  The command 
C line is parsed left -> right, so this must be done first to make 
C sure the errors go to the right place.
C
      DO iarg = 1, totarg-1 
         CALL getarg(iarg,optstr)
         CALL str_upc(optstr,str_beg(optstr),str_end(optstr))
         IF (optstr.EQ.'-ERRORS') THEN
            CALL getarg(iarg+1,argstr)
            IO_flags(ndx_err) = .TRUE.
            IO_fnms(ndx_err) = argstr
            IO_luns(ndx_err) = lun_alloc(ndx_max,err_cod)
         END IF
      END DO
C 
C Set errors and argument counts
C         
      errsum = 0
      iarg = 1
C
C Todo:  This long IF-THEN-ELSE monster could of course be avoided.  
C Baggage from earlier, sorry. 
C
      DO WHILE (iarg.LE.totarg)

         CALL getarg(iarg,optstr)
         CALL str_upc(optstr,str_beg(optstr),str_end(optstr))

         IF (member(optstr,options,' ').LE.0) THEN    !  if not option
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -          'Error: Unknown command line option ' //
     -           optstr(str_beg(optstr):str_end(optstr)) //LF//
     -          'See known options with the -help option') 
            STOP

         ELSE IF (optstr.EQ.'-H' .OR. optstr.EQ.'-HELP') THEN
            WRITE (6,'(a)') usestr
            STOP 

         ELSE IF (optstr.EQ.'-IN_GENBANK') THEN
            IO_flags(ndx_ingbk) = .TRUE.
            CALL getarg(iarg+1,argstr) 
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_ingbk) = argstr
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-IN_SIDS') THEN
            IO_flags(ndx_inkey) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_inkey) = argstr
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-IN_SEQS') THEN
            IO_flags(ndx_intbl) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_intbl) = argstr
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-DB_BINARY') THEN
            IO_flags(ndx_dbbin) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_dbbin) = argstr
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-DB_GENBANK') THEN
            IO_flags(ndx_dbgbk) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_dbgbk) = argstr
               iarg = iarg + 1
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-DB_SIDS') THEN
            IO_flags(ndx_dbkey) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_dbkey) = argstr
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-DB_SEQS') THEN
            IO_flags(ndx_dbtbl) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_dbtbl) = argstr
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-BP_MATRIX') THEN
            IO_flags(ndx_bpmat) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_bpmat) = argstr
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-OUT_GENBANK') THEN
            IO_flags(ndx_outgbk) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_outgbk) = argstr
               iarg = iarg + 1 
            END IF

         ELSE IF (optstr.EQ.'-OUT_SIDS') THEN
            IO_flags(ndx_outkey) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_outkey) = argstr
               iarg = iarg + 1
            END IF

         ELSE IF (optstr.EQ.'-OUT_SABS') THEN
            IO_flags(ndx_outsab) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_outsab) = argstr
               iarg = iarg + 1
            END IF

         ELSE IF (optstr.EQ.'-OUT_RULERS') THEN
            IO_flags(ndx_outrul) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_outrul) = argstr
               iarg = iarg + 1 
            END IF

         ELSE IF (optstr.EQ.'-OUT_SEQS') THEN
            IO_flags(ndx_outtbl) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_outtbl) = argstr
               iarg = iarg + 1 
            END IF

         ELSE IF (optstr.EQ.'-OUT_HUMAN') THEN
            IO_flags(ndx_outhum) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_outhum) = argstr
               iarg = iarg + 1 
            END IF

         ELSE IF (optstr.EQ.'-OUT_SERV') THEN
            IO_flags(ndx_outsrv) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_outsrv) = argstr
               iarg = iarg + 1 
            END IF

         ELSE IF (optstr.EQ.'-LOGS') THEN
            IO_flags(ndx_log) = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               IO_fnms(ndx_log) = argstr
               iarg = iarg + 1 
            END IF
C
C Just skip the errors, already done
C
         ELSE IF (optstr.EQ.'-ERRORS') THEN
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) iarg = iarg + 1 

         ELSE IF (optstr.EQ.'-TEMPLATES') THEN
            cl_tplflag = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               READ (argstr,*) cl_templates
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-OLIGO_LEN') THEN
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               READ (argstr,*) cl_olilen
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-COMPLETE_PCT') THEN
            cl_completeflag = .TRUE.
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               READ (argstr,*) cl_completepct
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-LISTWID') THEN
            CALL getarg(iarg+1,argstr)
            IF (member(argstr,options,' ').EQ.0) THEN
               READ (argstr,*) cl_listwid
               iarg = iarg + 1 
            ELSE
               errsum = errsum + 1
            END IF

         ELSE IF (optstr.EQ.'-LISTHTML') THEN
            cl_htmlflag = .TRUE.

         ELSE IF (optstr.EQ.'-NODBGAPS') THEN
            cl_dbgapsflag = .FALSE.

         ELSE IF (optstr.EQ.'-NOBINARY') THEN
            cl_outbinflag = .FALSE.

         ELSE IF (optstr.EQ.'-T2U') THEN
            cl_t2uflag = .TRUE.

         ELSE IF (optstr.EQ.'-U2T') THEN
            cl_u2tflag = .TRUE.

         ELSE IF (optstr.EQ.'-UPPER') THEN
            cl_upcaseflag = .TRUE.

         ELSE IF (optstr.EQ.'-LOWER') THEN
            cl_locaseflag = .TRUE.

         ELSE IF (optstr.EQ.'-DEBUG') THEN
            cl_debugflag = .TRUE.
            RETURN

         END IF  

         IF (errsum.GT.0) THEN
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -          'Error: Option ' //
     -           optstr(str_beg(optstr):str_end(optstr)) //
     -           ' requires a following filename or value' )
            STOP
         END IF

         iarg = iarg + 1

      END DO
C
C -------------------------------------------------------------------------------
C Validate command line input and stop program with message if files dont 
C exist, flags dont make sense in combination etc.
C -------------------------------------------------------------------------------
C
C Errors.
C Error if the error file already exists.  If you want all error messages
C collected in same file, direct them to stdout and use >>
C
      IF (IO_flags(ndx_err) .AND. str_end(IO_fnms(ndx_err)).GT.0) THEN

         IF (access(IO_fnms(ndx_err),' ').EQ.0) THEN    ! if it exists, then
            IO_luns(ndx_err) = 6
            CALL write_message ( IO_luns(ndx_err), IO_fnms(ndx_err), 
     -          'Error: Error file ' // IO_fnms(ndx_err)(str_beg(IO_fnms(ndx_err))
     -               :str_end(IO_fnms(ndx_err))) // ' already exists')
            errsum = errsum + 1 
         END IF

      END IF
C
C Log messages. (doesnt work yet)
C Error if the log file already exists.  If you want all log messages
C collected in same file, direct them to stdout and use >>
C
      IF (IO_flags(ndx_log) .AND. str_end(IO_fnms(ndx_log)).GT.0) THEN

         IF (access(IO_fnms(ndx_log),' ').EQ.0) THEN    
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_log),
     -           'Error: Log file ' // IO_fnms(ndx_log)(str_beg(IO_fnms(ndx_log))
     -              :str_end(IO_fnms(ndx_log))) // ' already exists')
            errsum = errsum + 1 
         END IF

      ELSE
         IO_luns(ndx_log) = 6
      END IF
C
C Unaligned Genbank sequences (-in_genbank). 
C Errors if 1) specified file doesnt exist as readable
C           2) no unaligned sequences given (no key/table as well)
C
      IF (IO_flags(ndx_ingbk)) THEN
                        
         IF (access(IO_fnms(ndx_ingbk),'r').NE.0) THEN    
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: New sequence file ' // 
     -            IO_fnms(ndx_ingbk)(str_beg(IO_fnms(ndx_ingbk)):
     -                               str_end(IO_fnms(ndx_ingbk))) // 
     -           ' does not exist as readable')
            errsum = errsum + 1 
         END IF

      ELSE IF (.NOT.IO_flags(ndx_inkey) .AND. .NOT.IO_flags(ndx_intbl)) THEN

         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -        'Error: New sequence file not specified')
         errsum = errsum + 1

      END IF
C
C Score matrix (-bp_matrix). 
C Errors:  If a specified file doesnt exist as readable
C
      IF (IO_flags(ndx_bpmat)) THEN
                        
         IF (access(IO_fnms(ndx_bpmat),'r').NE.0) THEN    
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: Score matrix file '                          // 
     -            IO_fnms(ndx_bpmat)(str_beg(IO_fnms(ndx_bpmat)):
     -                               str_end(IO_fnms(ndx_bpmat)))    // 
     -           ' does not exist as readable.'                       // LF // 
     -           'Solution: Find a blast formatted scoring matrix'    // LF //
     -           'meant for nucleic acids, place it where you want,'  // LF //
     -           'and make a new program alias. Or contact'           // LF // 
     -            author_ID//', '//email_ID )
            errsum = errsum + 1 
         END IF

      END IF
C
C If no binary file, genbank file, or table file given, stop with message,
C
      IF (.NOT.IO_flags(ndx_dbgbk) .AND. 
     -    .NOT.IO_flags(ndx_dbbin) .AND.
     -    .NOT.IO_flags(ndx_dbtbl)) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -        'Error: No database alignment specified')
         errsum = errsum + 1 
      END IF
C
C Binary output file must always be given with cl_outbinflag, 
C
      IF (cl_outbinflag .AND. .NOT.IO_flags(ndx_dbbin)) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -        'Error: Binary output file not specified')
         errsum = errsum + 1 
      END IF
C
C Write binary unless it has been switched off or none is specified or 
C one exists, 

      cl_outbinflag = cl_outbinflag .AND. IO_flags(ndx_dbbin) .AND. 
     -                access(IO_fnms(ndx_dbbin),'r').NE.0
C
C Use binary if readable. If it isnt, use alignment file,
C 
      IF (IO_flags(ndx_dbbin) .AND. access(IO_fnms(ndx_dbbin),'r').EQ.0) THEN

         IO_flags(ndx_dbgbk) = .FALSE.
         cl_outbinflag = .FALSE.

      ELSE IF (IO_flags(ndx_dbbin) .AND. access(IO_fnms(ndx_dbbin),'r').NE.0) THEN

         cl_outbinflag = .TRUE.

         IF (IO_flags(ndx_dbgbk)) THEN
            IF (access(IO_fnms(ndx_dbgbk),'r').NE.0) THEN
               CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -              'Error: Neither binary file ' // 
     -              IO_fnms(ndx_dbbin)(str_beg(IO_fnms(ndx_dbbin)):
     -                                 str_end(IO_fnms(ndx_dbbin))) // LF // 
     -              'nor alignment file ' // 
     -              IO_fnms(ndx_dbgbk)(str_beg(IO_fnms(ndx_dbgbk)):
     -                                 str_end(IO_fnms(ndx_dbgbk))) // 
     -              ' exist as readable')
               errsum = errsum + 1 
            ELSE
               IO_flags(ndx_dbbin) = .FALSE.
            END IF
         ELSE
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: Binary file ' // 
     -           IO_fnms(ndx_dbbin)(str_beg(IO_fnms(ndx_dbbin)):
     -                              str_end(IO_fnms(ndx_dbbin))) // 
     -           ' does not exist as readable')
            errsum = errsum + 1 
         END IF

      ELSE IF (IO_flags(ndx_dbgbk)) THEN

         IF (access(IO_fnms(ndx_dbgbk),'r').NE.0) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: Alignment file ' // 
     -           IO_fnms(ndx_dbgbk)(str_beg(IO_fnms(ndx_dbgbk)):
     -                              str_end(IO_fnms(ndx_dbgbk))) // 
     -           ' does not exist as readable')
            errsum = errsum + 1 
         ELSE
            IO_flags(ndx_dbbin) = .FALSE.
         END IF

      END IF
C
C -out_genbank / -in_genbank check,
C
      IF (IO_flags(ndx_outgbk)) THEN
         IF (.NOT.IO_flags(ndx_ingbk)) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: -out_genbank requires the -in_genbank option')
            errsum = errsum + 1 
         END IF
      END IF
C
C -in_keys / in_table check, 
C
      IF (IO_flags(ndx_inkey) .AND. .NOT.IO_flags(ndx_intbl)) THEN

         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -         'Error: -in_sids requires the -in_seqs option')
         errsum = errsum + 1 

      ELSE IF (IO_flags(ndx_intbl) .AND. .NOT.IO_flags(ndx_inkey)) THEN

         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -         'Error: -in_seqs requires the -in_sids option')
         errsum = errsum + 1 

      END IF
c$$$C
c$$$C -listwid doesnt make sense without -out_humane
c$$$C
c$$$      IF (cl_listwid.GT.0 .AND. .NOT.IO_flags(ndx_outhum)) THEN
c$$$         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
c$$$     -         'Error: -listwid requires the -out_human')
c$$$         errsum = errsum + 1 
c$$$      END IF
C
C -in_keys and -in_table must follow
C
      IF (IO_flags(ndx_inkey) .AND. .NOT. IO_flags(ndx_intbl)) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -         'Error: -in_sids requires the -in_seqs option')
         errsum = errsum + 1
      ELSE IF (IO_flags(ndx_intbl) .AND. .NOT. IO_flags(ndx_inkey)) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -        'Error: -in_seqs requires the -in_sids option')
         errsum = errsum + 1
      END IF
C
C TODO - handle inconsistencies:
C    -templates with no out_key, out_sab, or out_table

C
C u2t/t2u flag consistency check,
C
      IF (cl_t2uflag .AND. cl_u2tflag) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -        'Error: Both -t2u and -u2t flags specified')
         errsum = errsum + 1 
      END IF
C
C Is -listwid reasonable,
C
      IF (IO_flags(ndx_outhum)) THEN
         IF (cl_listwid.LE.0) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: Human listing width must be greater than 0')
            errsum = errsum + 1 
         END IF
      END IF
C
C Upper/lower flag consistency check,
C
      IF (cl_upcaseflag .AND. cl_locaseflag) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -        'Error: Both -upper and -lower flags specified')
         errsum = errsum + 1 
      END IF
C
C Is -complete percentage reasonable,
C
      IF (cl_completeflag) THEN
         IF ( cl_completepct.LE.0 ) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: Complete length percentage must be greater than 0')
            errsum = errsum + 1 
         ELSE IF ( cl_completepct.GT.100.0 ) THEN
            CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -           'Error: Complete length percentage must 100% or less')
            errsum = errsum + 1 
         END IF
      END IF

      IF ( errsum.GT.0 ) THEN
c         WRITE (6,'(a)') usestr
         STOP
      END IF

      RETURN
      END

      SUBROUTINE pop_stack (n_dim,m_dim,stack,stack_ptr,xy_arr)
C
C Copies from stack to xy_arr and decrements stack pointer
C
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'      
      INCLUDE 'pal_settings.inc'
      
      INTEGER n_dim,m_dim         ! in
      INTEGER stack (n_dim,m_dim) ! in
      INTEGER stack_ptr           ! in + out
      INTEGER xy_arr (n_dim)      ! out
C
C Local variables
C
      INTEGER i                 
      CHARACTER*1 LF / 10 /      
C
C ---------------------------------------------------------------------------
C
      IF (stack_ptr.LT.1) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -      'PUSH_STACK: Error: Stack underflow, most likely caused by a bug' //LF//
     -      '            Contact '//author_ID//', '//email_ID)
         STOP
      END IF

      DO i = 1,n_dim
         xy_arr(i) = stack (i,stack_ptr)
      END DO
      
      stack_ptr = stack_ptr - 1 

      RETURN
      END 
      SUBROUTINE push_stack (n_dim,m_dim,stack,stack_ptr,xy_arr)
C
C Increment pointer and copy from xy_arr to stack
C
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_cmdlin.inc'
      INCLUDE 'pal_settings.inc'

      INTEGER n_dim,m_dim
      INTEGER stack(n_dim,m_dim)
      INTEGER stack_ptr 
      INTEGER xy_arr(n_dim)
C
C Local variables
C
      INTEGER i
      CHARACTER*1 LF / 10 /
C
C ---------------------------------------------------------------------------
C
      stack_ptr = stack_ptr + 1 

      IF (stack_ptr.GT.m_dim) THEN
         CALL write_message (IO_luns(ndx_err),IO_fnms(ndx_err),
     -      'PUSH_STACK: Error: Stack overflow, most likely caused by a bug' //LF//
     -      '            Contact '//author_ID//', '//email_ID)
         STOP
      END IF

      DO i = 1,n_dim
         stack (i,stack_ptr) = xy_arr (i) 
      END DO

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
      SUBROUTINE r4_bin_io (iostr,lun,lodim,hidim,array)
C
C One dimensional REAL*4 binary READ or WRITE, determined by iostr
C 
      IMPLICIT none

      CHARACTER*(*) iostr
      INTEGER   lun
      INTEGER   lodim
      INTEGER   hidim
      REAL      array (lodim:hidim)
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
      SUBROUTINE read_lines ( lun, lines )

      IMPLICIT none

      INTEGER lun 
      INTEGER lines
C
C -----------------------------------------------------------------------------
C     
      lines = 0

      DO WHILE (.TRUE.)
         READ ( lun, '()', END = 99 )
         lines = lines + 1
      END DO

 99   RETURN
      END
      SUBROUTINE read_text ( lun, maxlin, text )

      IMPLICIT none

      INTEGER       lun 
      INTEGER       maxlin
      CHARACTER*(*) text (maxlin)
C
C Local variables
C
      INTEGER      lines
      CHARACTER*80 line
C
C -----------------------------------------------------------------------------
C     
      lines = 0

      DO WHILE (.TRUE.)
         READ ( lun, '(a)', END = 99 ) line
         lines = lines + 1
         text (lines) = line
      END DO

 99   RETURN
      END
      SUBROUTINE res_sco_dflt 
C
C Set the match / mismatch scores to what gives a good result with 
C DNA/RNA
C
      IMPLICIT none

      INCLUDE 'pal_res_eqv.inc'

      BYTE A / 65 / 
      BYTE G / 71 /
      BYTE C / 67 / 
      BYTE T / 84 / 
      BYTE U / 85 /

      BYTE sco
      INTEGER i, j
C
C --------------------------------------------------------------------------------
C
C Blank out with -2 everywhere
C 
      DO i = 0, 127
         DO j = 0, 127
            Res_Sco (i,j) = -2
         END DO
      END DO
C
C Perfect matches
C     
      Res_Sco (A,A) = 1
      Res_Sco (G,G) = 1
      Res_Sco (C,C) = 1
      Res_Sco (T,T) = 1
      Res_Sco (T,U) = 1
      Res_Sco (U,U) = 1
      Res_Sco (U,T) = 1
C
C Pu-pu, py-py
C
      Res_Sco (A,G) = -1
      Res_Sco (G,A) = -1
      Res_Sco (C,T) = -1
      Res_Sco (C,U) = -1
      Res_Sco (T,C) = -1
      Res_Sco (U,C) = -1
C
C Fold it so upper/lower case matches are covered,
C
      DO i = 65, 90
         DO j = 65, 90
            sco = Res_Sco(i,j)
            Res_Sco(i   ,j+32) = sco
            Res_Sco(i+32,j   ) = sco
            Res_Sco(i+32,j+32) = sco
         END DO
      END DO

      RETURN
      END
      SUBROUTINE Res_Sco_load (lun)
C
C Reads a Blast-style score matrix (such as a PAM matrix), and loads 
C symmetrically Res_Sco matrix, with integer ASCII code indices.
C
C  IMPROVE!
C
      IMPLICIT none

      INCLUDE 'pal_res_eqv.inc'

      INTEGER lun
C
C Local variables
C
      CHARACTER*100 line      ! arbitrary
      INTEGER       str_end
      BYTE          ichs(100) ! arbitrary
      INTEGER       maxich
      INTEGER       irow, icol, isco
      INTEGER       i, ich
C
C ------------------------------------------------------------------------------
C
C Skip over comment lines if any, they must start with a pound sign,
C
      READ (lun,'(a)') line
      DO WHILE (line(1:1).EQ.'#')
         READ (lun,'(a)') line
      END DO
C
C First line associates residues with columns
C
      maxich = 0 

      DO i = 1,str_end (line) 
         IF (line(i:i).NE.' ') THEN
            maxich = maxich + 1 
            ichs(maxich) = ICHAR (line(i:i))
         END IF
      END DO
C
C 
C 
      irow = 1
      READ (lun,'(a)') line

      DO WHILE (str_end(line).GT.0)
         READ (line,'(100i3)') (Res_Sco(ichs(irow),ichs(icol)),icol=1,maxich)
         DO ich = 1,maxich
            isco = Res_Sco(ichs(irow),ichs(ich))
            Res_Sco(ichs(irow),ichs(ich)+32) = isco
            Res_Sco(ichs(irow)+32,ichs(ich)) = isco
            Res_Sco(ichs(irow)+32,ichs(ich)+32) = isco
         END DO
         READ (lun,'(a)', END = 99 ) line
         irow = irow + 1 
      END DO

 99   RETURN
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

      SUBROUTINE safe_compare ( x_seqbeg, x_seqend, x_seq,
     -                          y_seqbeg, y_seqend, y_seq,
     -                          isbase, err_cod )
C
C Compares x_seq with y_seq. If they have an identical sequence of IUB
C base symbols, err_cod is 0, otherwise -1. Both sequences may contain
C invalid IUB symbols (such as indel characters), they are skipped. 
C
      IMPLICIT none

      INCLUDE 'pal_res_eqv.inc'

      INTEGER   x_seqbeg
      INTEGER   x_seqend
C      BYTE      x_seq (x_seqend-x_seqbeg+1)  ! x_seqbeg:x_seqend)
      BYTE      x_seq (*)
      INTEGER   y_seqbeg
      INTEGER   y_seqend
      BYTE      y_seq (*)  ! y_seqbeg:y_seqend)
C      BYTE      y_seq (y_seqend-y_seqbeg+1)  ! y_seqbeg:y_seqend)
      LOGICAL*1 isbase (0:*)
      INTEGER   err_cod
C
C local variables
C
C      BYTE x_scr(10000)
      BYTE     x_scr (x_seqend-x_seqbeg+1)
      POINTER (x_scr_ptr,x_scr)
      INTEGER  x_bases,x_indx

C      BYTE y_scr(10000)
      BYTE     y_scr (y_seqend-y_seqbeg+1)
      POINTER (y_scr_ptr,y_scr)
      INTEGER  y_bases,y_indx

      CHARACTER*12 fun_ID  / 'SAFE_COMPARE' /
C
C --------------------------------------------------------------------------
C
C      print*,'in safe compare:'
C      print*,' x_beg, x_end = ',x_seqbeg,x_seqend
C      print*,' y_beg, y_end = ',y_seqbeg,y_seqend

      err_cod = 0 

      IF (x_seqbeg.GT.x_seqend  .OR. y_seqbeg.GT.y_seqend) THEN
         err_cod = -1
         RETURN
      END IF

      CALL get_memory ( x_scr_ptr, x_seqend - x_seqbeg + 1, 'x_scr', fun_ID )

      x_bases = 0
      DO x_indx = x_seqbeg, x_seqend
         IF ( isbase(x_seq(x_indx)) ) THEN
            x_bases = x_bases + 1 
            x_scr(x_bases) = x_seq(x_indx)
         END IF
      END DO

      CALL get_memory ( y_scr_ptr, y_seqend - y_seqbeg + 1, 'y_scr', fun_ID )

      y_bases = 0
      DO y_indx = y_seqbeg, y_seqend
         IF ( isbase(y_seq(y_indx)) ) THEN
            y_bases = y_bases + 1 
            y_scr(y_bases) = y_seq(y_indx)
         END IF
      END DO

      IF (x_bases.NE.y_bases) THEN
         err_cod = -1
         GO TO 98
      END IF

      DO x_indx = 1, x_bases
         IF ( .NOT. bas_eqv(x_scr(x_indx),y_scr(x_indx)) ) THEN
            err_cod = -1
            GO TO 98
         END IF
      END DO

 98   CALL FREE (x_scr_ptr)
      CALL FREE (y_scr_ptr)
      
      RETURN
      END 

      SUBROUTINE seq_extract (aliarr,alibeg,aliend,seqarr,seqlen)

      IMPLICIT none

      INCLUDE 'pal_res_eqv.inc'

      BYTE    aliarr(*)
      INTEGER alibeg,aliend
      BYTE    seqarr(*)
      INTEGER seqlen
C
C Local variables
C
      INTEGER alipos
C
C --------------------------------------------------------------------------------
C
      seqlen = 0

      DO alipos = alibeg, aliend
         IF (IUB_base(aliarr(alipos))) THEN
            seqlen = seqlen + 1
            seqarr(seqlen) = aliarr(alipos)
         END IF
      END DO

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
      SUBROUTINE seq_match (  alp_siz,  wrd_len, wrd_dim,
     -                       x_seqbeg, x_seqend, x_seq, x_links, 
     -                       y_seqbeg, y_seqend, y_seq, y_links, 
     -                       hit_flag )
C
C Version 1.0  28 December, 1992
C Version 2.0  06 August, 1993 
C         Alignment tracing and loading taken out
C  
C Niels Larsen, niels@darwin.life.uiuc.edu
C
C EXPLAIN
C     
C ----------------------------------------------------------------------
C
      IMPLICIT none

      INCLUDE 'pal_params.inc'
      INCLUDE 'pal_settings.inc'
      INCLUDE 'pal_stacks.inc'
      INCLUDE 'pal_res_eqv.inc'
      INCLUDE 'pal_cmdlin.inc'
C
C Formal arguments
C
      INTEGER       alp_siz
      INTEGER       wrd_len
      INTEGER       wrd_dim
      INTEGER       x_seqbeg
      INTEGER       x_seqend
      BYTE          x_seq   (x_seqbeg:x_seqend)  ! Input unaligned template sequence
      BYTE          x_links (x_seqbeg:x_seqend)  ! Connects aligned pieces
      INTEGER       y_seqbeg
      INTEGER       y_seqend
      BYTE          y_seq   (y_seqbeg:y_seqend)  ! Input unaligned new sequence
      BYTE          y_links (y_seqbeg:y_seqend)  ! Connects aligned pieces
      LOGICAL       hit_flag                     ! false if no alignment found
C
C Local variables
C
      BYTE     s_debug(x_seqend)
      POINTER (s_debug_ptr,s_debug)

      BYTE     x_seq_4 (x_seqend)   
      POINTER (x_seq_4_ptr,x_seq_4)

      BYTE     y_seq_4 (y_seqend)
      POINTER (y_seq_4_ptr,y_seq_4)

      INTEGER  x_iol_seq (x_seqend)
      POINTER (x_iol_seq_ptr,x_iol_seq)

      INTEGER  y_iol_seq (y_seqend)
      POINTER (y_iol_seq_ptr,y_iol_seq)

      INTEGER  x_lnk_lst (x_seqend)   
      POINTER (x_lnk_lst_ptr,x_lnk_lst)

      INTEGER  x_lnk_bgs (0:wrd_dim-1)  
      POINTER (x_lnk_bgs_ptr,x_lnk_bgs)

      INTEGER x,y
      INTEGER x_dif,y_dif
c      INTEGER x_seqpos,y_seqpos
c      INTEGER alipos

      INTEGER n_left,m_left,left_ptr
      PARAMETER (n_left = 8, m_left = 1000)
      INTEGER left_stack(n_left,m_left)

      INTEGER n_done,m_done,done_ptr
      PARAMETER (n_done = 8, m_done = 1000)
      INTEGER done_stack(n_done,m_done)

      INTEGER n_work,m_work,work_ptr
      PARAMETER (n_work = 8, m_work = 1000)
      INTEGER work_stack(n_work,m_work)
      
      INTEGER xy_rect(n_left),xy_diag(n_left) ! ,xy_scr(n_left)

      REAL*8  dia_sco
      INTEGER min_dia_len,dia_len
      INTEGER dia_begs, err_cod
      INTEGER i_ptr
      LOGICAL dia_flg
      INTEGER cc_ich ! ms_ich,

      CHARACTER*9 fun_ID  / 'SEQ_MATCH' /
      CHARACTER*1     LF           / 10 /
C
C ---------------------------------------------------------------------------
C 
C  L I N K E D  L I S T  P A S S  
C
C  The purpose of the linked list approach is to quickly find the obvious
C  matches.  That leaves time to do more intensive and refined alignment in
C  later passes.
C
C ---------------------------------------------------------------------------
C
C For x_seq and y_seq, calculate a sequence of unique integers that 
C designate each wrd_len long oligo in the base sequence. Then for 
C x_seq, set up the linked list x_lnk_lst. 
C
      CALL get_memory ( x_seq_4_ptr, x_seqend, 'x_seq_4', fun_ID)
      CALL get_memory ( y_seq_4_ptr, y_seqend, 'y_seq_4', fun_ID)

      CALL To_Code_4 ( x_seq, 1, x_seqend, 1, x_seqend, x_seq_4 )
      CALL To_Code_4 ( y_seq, 1, y_seqend, 1, y_seqend, y_seq_4 )

      CALL get_memory ( x_iol_seq_ptr, x_seqend * 4, 'x_iol_seq', fun_ID )
      CALL get_memory ( y_iol_seq_ptr, y_seqend * 4, 'y_iol_seq', fun_ID)

      CALL make_iol_seq ( alp_siz, wrd_len, 1, x_seqend, 1, x_seqend,
     -                    x_seq_4, x_iol_seq )
      CALL make_iol_seq ( alp_siz, wrd_len, 1, y_seqend, 1, y_seqend,
     -                    y_seq_4, y_iol_seq )

      CALL FREE (y_seq_4_ptr)

      CALL get_memory (x_lnk_bgs_ptr,  wrd_dim * 4, 'x_lnk_bgs', fun_ID) 
      CALL get_memory (x_lnk_lst_ptr, x_seqend * 4, 'x_lnk_lst', fun_ID)

      CALL make_lnk_lst ( alp_siz, wrd_len, wrd_dim, 1, x_seqend, 
     -                    x_seq_4, 1, 1, x_seqbeg, x_seqend,
     -                    x_lnk_bgs, x_lnk_lst )

      CALL FREE (x_seq_4_ptr)
C
C Put the 'rectangles' spanned by the two sequences on work_stack, and
C calculate what part of the rectangle to search for matches. The parameters
C 'x_max_gap' and 'y_max_gap' determines this [ for rRNA there is not point
C searching beginning of x_seq against end of y_seq ].
C
      work_ptr = 1
      work_stack (xl,work_ptr) = 1
      work_stack (xh,work_ptr) = x_seqend
      work_stack (yl,work_ptr) = 1
      work_stack (yh,work_ptr) = y_seqend

      CALL xy_ini_lims (work_stack (xl,work_ptr),work_stack (xh,work_ptr),
     -                  work_stack (yl,work_ptr),work_stack (yh,work_ptr),
     -                  work_stack (xm,work_ptr),work_stack (ym,work_ptr) )
C
C Loop until work_stack is empty
C
      done_ptr = 0
      left_ptr = 0

      DO WHILE (work_ptr.GT.0)
C
C Get rectangle from stack
C
         CALL pop_stack (n_work,m_work,work_stack,work_ptr,xy_rect)
C
C Find minimum acceptable diagonal length - acceptable being consec_pen
C of the probability of a random match. Set parameter 'consec_pen' to
C some conservative low value, not to allow to build upon mistakes 
C later.
C
         min_dia_len = 10  ! arbitrary, but should be on the safe side
         dia_sco = consec_pen + 1.0
         err_cod = 0

         DO WHILE ( dia_sco.GT.consec_pen .AND. err_cod.EQ.0 )
            min_dia_len = min_dia_len + 1 
            dia_sco = dia_begs ( xy_rect(xl), xy_rect(xh),
     -                           xy_rect(yl), xy_rect(yh),
     -                           xy_rect(xm), xy_rect(ym),
     -                           min_dia_len, err_cod )
     -                 * p_mtc ** min_dia_len
c            print*,' min_dia_len, dia_sco = ',min_dia_len, dia_sco
         END DO

         IF ( err_cod.EQ.0 ) THEN
C
C Find best diagonal, return coordinates in xy_diag
C
            CALL lnk_lst_diag ( min_dia_len, wrd_len,
     -                          x_iol_seq, y_iol_seq,
     -                          x_lnk_bgs, x_lnk_lst,
     -                          xy_rect(xl), xy_rect(xh),
     -                          xy_rect(yl), xy_rect(yh),
     -                          xy_diag(xl), xy_diag(xh),
     -                          xy_diag(yl), xy_diag(yh),
     -                          dia_flg )

            IF (dia_flg) THEN
C 
C Diagonal found: 1) Push it to done_stack, 2) copy sub-rectangles to 
C work_stack, but only if found diagonal doesnt touch xy-limits.
C
               CALL update_stacks (n_work,m_work,work_stack,work_ptr,
     -                             n_done,m_done,done_stack,done_ptr,
     -                             n_left,xy_rect,xy_diag)
               done_stack(cc,done_ptr) = ICHAR('L')  ! debug only
               done_stack(ms,done_ptr) = 1 
            ELSE
C
C No diagonal found, copy to left_stack to handle later
C
               CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
            END IF

         ELSE 
            CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
         END IF
         
      END DO

      CALL FREE (x_iol_seq_ptr)
      CALL FREE (y_iol_seq_ptr)
C
C --------------------------------------------------------------------------
C
C  S C O R E   M A T R I X   P A S S  
C
C  A less crude similarity search, used on the linked list search leftovers 
C
C --------------------------------------------------------------------------
C
C Copy left_stack to work_stack; values of work_ptr (0) and left_ptr is
C inherited from previous pass. 
C
      DO WHILE (left_ptr.GT.0)
         CALL  pop_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
         CALL push_stack (n_work,m_work,work_stack,work_ptr,xy_rect)
      END DO
C
C Empty work_stack
C
      DO WHILE (work_ptr.GT.0)
C
C Get rectangle from stack
C
         CALL pop_stack (n_work,m_work,work_stack,work_ptr,xy_rect)
C
C Find best diagonal, return coordinates in xy_diag
C
         min_dia_len = 4

         IF (min_dia_len.LE.xy_rect(xh)-xy_rect(xl)+1 .AND.
     -       min_dia_len.LE.xy_rect(xh)-xy_rect(xl)+1) THEN 

            CALL best_hs_diag (x_seq,y_seq,min_dia_len,
     -              xy_rect(xl),xy_rect(xh),
     -              xy_rect(yl),xy_rect(yh),
     -              xy_rect(xm),xy_rect(ym),
     -              xy_diag(xl),xy_diag(xh),
     -              xy_diag(yl),xy_diag(yh),
     -              dia_len )

            IF ( dia_len.GE.min_dia_len ) THEN
C 
C Diagonal found: 1) Push it to done_stack, 2) copy sub-rectangles to 
C work_stack, but only if found diagonal doesnt touch xy-limits.
C
               CALL update_stacks (n_work,m_work,work_stack,work_ptr,
     -                             n_done,m_done,done_stack,done_ptr,
     -                             n_left,xy_rect,xy_diag)
               done_stack(cc,done_ptr) = ICHAR(':')  ! debug only
               done_stack(ms,done_ptr) = 1 ! ctnali_ich
            ELSE 
C
C No diagonal found, copy to left_stack to handle later
C
               CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
            END IF

         ELSE 
            CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
         END IF

      END DO
C
C --------------------------------------------------------------------------
C
C  A L I G N M E N T   D O N E 
C
C --------------------------------------------------------------------------
C
C  Move from left_stack to done_stack quadratic rectangles (improve)
C
      DO WHILE (left_ptr.GT.0)

         CALL pop_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
         
         x_dif = xy_rect(xh)-xy_rect(xl)
         y_dif = xy_rect(yh)-xy_rect(yl)

         IF ( x_dif.EQ.y_dif     .AND.           ! only quadratic
     -        xy_rect(xl).NE. 1  .AND.           ! not at beginning
     -        xy_rect(xh).NE.x_seqend ) THEN     ! or end
            CALL push_stack(n_done,m_done,done_stack,done_ptr,xy_rect)
            done_stack(cc,done_ptr) = ICHAR('~')  ! debug only
            done_stack(ms,done_ptr) = 1 
         ELSE
            CALL push_stack(n_work,m_work,work_stack,work_ptr,xy_rect)
         END IF

      END DO

      DO WHILE (work_ptr.GT.0)
         CALL  pop_stack (n_work,m_work,work_stack,work_ptr,xy_rect)
         CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
      END DO
C
C --------------------------------------------------------------------------
C  Move diagonals from done_stack to trace arrays
C --------------------------------------------------------------------------
C
      CALL by_array_init ( 1, x_seqend, x_links, 1, x_seqend, 0 )
      CALL by_array_init ( 1, y_seqend, y_links, 1, y_seqend, 0 )

      CALL get_memory (s_debug_ptr, x_seqend, 's_debug', fun_ID)
      CALL by_array_init ( 1, x_seqend, s_debug, 1, x_seqend, 32 ) ! 32=blank

      DO i_ptr = 1, done_ptr

         cc_ich = done_stack (cc,i_ptr)
              y = done_stack (yl,i_ptr)

         DO x = done_stack (xl,i_ptr),done_stack(xh,i_ptr)
            x_links(x) = 1
            y_links(y) = 1
            IF (bas_eqv(x_seq(x),y_seq(y))) s_debug(x) = cc_ich 
            y = y + 1 
         END DO

      END DO

      hit_flag = done_ptr .GT. 0 

      CALL FREE (s_debug_ptr)

      RETURN
      END 

      SUBROUTINE best_hs_diag (x_seq,y_seq,min_dia_len,
     -                             x_low_bnd,x_hig_bnd,
     -                             y_low_bnd,y_hig_bnd,
     -                             x_ini_lim,y_ini_lim,
     -                             x_bst_beg,x_bst_end,
     -                             y_bst_beg,y_bst_end,
     -                             bst_sco_sum )
C
C Finds longest diagonal of consecutive matches over a certain rectangle.
C The rectangle is searched between its two diagonals plus x_gap_max and
C y_gap_max to lower right and upper left respectively. Length and 
C coordinates for longest diagonal is returned.
C
      IMPLICIT none

      INCLUDE 'pal_settings.inc'
      INCLUDE 'pal_stacks.inc'
      INCLUDE 'pal_res_eqv.inc'
C
C Input
C
      BYTE    x_seq(*)                 ! first sequence 
      BYTE    y_seq(*)                 ! second sequence
      INTEGER min_dia_len              ! minimum diagonal length
      INTEGER x_low_bnd                ! low x of alignment rectangle
      INTEGER x_hig_bnd                ! high x     -         - 
      INTEGER y_low_bnd                ! low y of alignment rectangle
      INTEGER y_hig_bnd                ! high y     -         - 
      INTEGER x_ini_lim                ! 
      INTEGER y_ini_lim                ! in
      INTEGER x_bst_beg,x_bst_end                ! out
      INTEGER y_bst_beg,y_bst_end                ! out
      INTEGER bst_sco_sum                        ! out
C
C Local variables
C
      INTEGER min_bnd_dif
      INTEGER x_ini,y_ini
      INTEGER x_fin,y_fin
      INTEGER x,y

      INTEGER sco_sum

      INTEGER cum_scores(0:10000)  
      INTEGER dia_lst(3,1000),dia_ptr
      INTEGER i_ptr,j_ptr
      BYTE    x_ich,y_ich
      LOGICAL dia_flag

      REAL scr
      INTEGER tmp_sco,add_len,parea5,parea3,x_beg,x_end
C
C ---------------------------------------------------------------------------
C
c      write (6,'()')
c      write (6,'(a,2i6)') ' x_low_bnd,x_hig_bnd = ',x_low_bnd,x_hig_bnd
c      write (6,'(a,2i6)') ' y_low_bnd,y_hig_bnd = ',y_low_bnd,y_hig_bnd
c$$$      write (6,'(a,2i6)') ' x_ini_lim,y_ini_lim = ',x_ini_lim,y_ini_lim

      min_bnd_dif = MIN (x_hig_bnd-x_low_bnd,y_hig_bnd-y_low_bnd)
      bst_sco_sum = -100000  ! well..

      x_bst_beg = 0
      y_bst_beg = 0
      x_bst_end = 0
      y_bst_end = 0

c$$$      IF (dbgflg) THEN
c$$$         x_ini_lim = x_low_bnd
c$$$         y_ini_lim = y_low_bnd
c$$$      END IF

      x_ini = x_ini_lim
      y_ini = y_low_bnd
      x_fin = x_ini + MIN (x_hig_bnd-x_ini,y_hig_bnd-y_ini)
      y_fin = y_ini + MIN (x_hig_bnd-x_ini,y_hig_bnd-y_ini)
C
C Search for matches along each possible diagonal (high x --> 1 --> high y),
C in given rectangular space
C
      DO WHILE (y_ini.LE.y_ini_lim)

c         write (6,'(a,4i6)') ' x_ini,y_ini,x_fin,y_fin =',x_ini,y_ini,x_fin,y_fin

         dia_flag = .FALSE.
         dia_ptr = 0

         x = x_ini
         y = y_ini

         cum_scores (x-1) = 0
C
C Dia_lst is a table of diagonals:
C dia_lst(1,*)   - diagonal start x index
C dia_lst(2,*)   - diagonal end   x index
C
         DO WHILE (x.LE.x_hig_bnd .AND. y.LE.y_hig_bnd)

            x_ich = x_seq(x)
            y_ich = y_seq(y)

            cum_scores (x) = cum_scores (x-1) + res_sco (x_ich,y_ich)

            IF (res_sco(x_ich,y_ich).GT.0) THEN

               IF (.NOT.dia_flag) THEN
                  dia_ptr = dia_ptr + 1
                  dia_lst (1,dia_ptr) = x   ! x_low
                  dia_lst (2,dia_ptr) = x   ! x_high
                  dia_flag = .TRUE.
               ELSE
                  dia_lst (2,dia_ptr) = x
               END IF

            ELSE
               dia_flag = .FALSE.
            END IF

            x = x + 1 
            y = y + 1

         END DO
C
C horrible temporary fix
C
         j_ptr = dia_ptr
         dia_ptr = 0 
         DO i_ptr = 1,j_ptr
            IF (dia_lst(2,i_ptr)-dia_lst(1,i_ptr)+1.GE.min_dia_len) THEN
               dia_ptr = dia_ptr + 1 
               dia_lst(1,dia_ptr) = dia_lst(1,i_ptr)
               dia_lst(2,dia_ptr) = dia_lst(2,i_ptr)
               x_beg = dia_lst(1,dia_ptr)
               x_end = dia_lst(2,dia_ptr)
c               y_beg = y_ini + x_beg - x_ini
c               y_end = y_beg + x_end - x_beg
               parea5 = (x_beg-x_ini+1) * (MAX (x_ini-x_low_bnd,y_ini-y_low_bnd)+1)
               parea3 = (x_fin-x_end+1) * (MAX (x_hig_bnd-x_fin,y_hig_bnd-y_fin)+1)
               tmp_sco = MIN (parea5,parea3)
               add_len = 0
               scr = 1.0/p_mtc
               DO WHILE (scr ** add_len .LE. tmp_sco) 
                  add_len = add_len + 1 
               END DO
c               add_len = add_len + 1
               dia_lst(3,dia_ptr) = add_len
c               write (6,'(a,4i6)') '   dia_ptr,i_ptr,tmp_sco,add_len =',
c     -                                 dia_ptr,i_ptr,tmp_sco,add_len
            END IF
         END DO

C
         DO i_ptr = 1, dia_ptr

            DO j_ptr = i_ptr, dia_ptr

               sco_sum = cum_scores (dia_lst(2,j_ptr)) 
     -                 - cum_scores (dia_lst(1,i_ptr)-1)
     -                 - MIN (dia_lst(3,i_ptr),dia_lst(3,j_ptr))

               IF (sco_sum.GT.bst_sco_sum) THEN
                  bst_sco_sum = sco_sum
                  x_bst_beg = dia_lst (1,i_ptr)
                  y_bst_beg = y_ini + x_bst_beg - x_ini
                  x_bst_end = dia_lst (2,j_ptr)
                  y_bst_end = y_ini + x_bst_end - x_ini
               END IF

            END DO

         END DO

         IF (x_ini.GT.x_low_bnd) THEN
            x_ini = x_ini - 1
         ELSE
            y_ini = y_ini + 1
         END IF

         x_fin = x_ini + MIN (x_hig_bnd-x_ini,y_hig_bnd-y_ini)
         y_fin = y_ini + MIN (x_hig_bnd-x_ini,y_hig_bnd-y_ini)

      END DO

c         stop
      


c$$$      IF (x_low_bnd.EQ.346  .AND. y_low_bnd.EQ.346  .OR.
c$$$     -    x_low_bnd.EQ.384  .AND. y_low_bnd.EQ.384  .OR.
c$$$     -    x_hig_bnd.EQ.442  .AND. y_hig_bnd.EQ.448  .OR.
c$$$     -    x_low_bnd.EQ. 20  .AND. y_low_bnd.EQ. 20  .OR.
c$$$     -    x_low_bnd.EQ. 58  .AND. y_low_bnd.EQ. 58  .OR.
c$$$     -    x_hig_bnd.EQ. 66  .AND. y_hig_bnd.EQ. 122)     THEN
c$$$         write (6,'()')
c$$$         write (6,'(a,2i6)') ' x_low_bnd,x_hig_bnd = ',x_low_bnd,x_hig_bnd
c$$$         write (6,'(a,2i6)') ' y_low_bnd,y_hig_bnd = ',y_low_bnd,y_hig_bnd
c$$$         write (6,'(a,2i6)') ' x_ini_lim,y_ini_lim = ',x_ini_lim,y_ini_lim
c$$$         write (6,'(a,2i6)') ' x_bst_beg,x_bst_end = ',x_bst_beg,x_bst_end
c$$$         write (6,'(a,2i6)') ' y_bst_beg,y_bst_end = ',y_bst_beg,y_bst_end
c$$$         write (6,'(a,i6)') ' bst_sco_sum = ',bst_sco_sum
c$$$      END IF

      RETURN
      END


c$$$C
c$$$C ---------------------------------------------------------------------------
c$$$C PASS 1 - find diagonals of consecutive matches (crude quick similarity)
c$$$C ---------------------------------------------------------------------------
c$$$C
c$$$      work_ptr = 1
c$$$
c$$$      DO WHILE (work_ptr.GT.0)
c$$$C
c$$$C Get rectangle from stack
c$$$C
c$$$         CALL pop_stack (n_work,m_work,work_stack,work_ptr,xy_rect)
c$$$C
c$$$C Find minimum acceptable diagonal length; acceptable means being 
c$$$C consec_pen times less probable than the probability of a random 
c$$$C match
c$$$C 
c$$$         min_dia_len = 0
c$$$         err_cod = 0
c$$$         dia_sco = consec_pen + 1.0
c$$$
c$$$         DO WHILE (dia_sco.GT.consec_pen .AND. err_cod.EQ.0)
c$$$            min_dia_len = min_dia_len + 1 
c$$$            dia_sco = dia_begs (xy_rect(xl),xy_rect(xh),
c$$$     -                          xy_rect(yl),xy_rect(yh),
c$$$     -                          xy_rect(xm),xy_rect(ym),
c$$$     -                          min_dia_len,
c$$$     -                          err_cod,err_str)
c$$$     -                 * p_mtc ** min_dia_len
c$$$         END DO
c$$$
c$$$         IF (err_cod.EQ.0) THEN
c$$$C
c$$$C Find best diagonal, return coordinates in xy_diag
c$$$C
c$$$            CALL consec_diag (
c$$$     -            x_seq,y_seq,
c$$$     -            xy_rect(xl),xy_rect(xh),
c$$$     -            xy_rect(yl),xy_rect(yh),
c$$$     -            xy_rect(xm),xy_rect(ym),
c$$$     -            xy_diag(xl),xy_diag(xh),
c$$$     -            xy_diag(yl),xy_diag(yh),
c$$$     -            dia_len )
c$$$
c$$$            IF (dia_len.GE.min_dia_len) THEN
c$$$C 
c$$$C Diagonal found: 1) Push it to done_stack, 2) copy sub-rectangles to 
c$$$C work_stack, but only if found diagonal doesnt touch xy-limits.
c$$$C
c$$$               CALL update_stacks (n_work,m_work,work_stack,work_ptr,
c$$$     -                             n_done,m_done,done_stack,done_ptr,
c$$$     -                             n_left,xy_rect,xy_diag)
c$$$               done_stack(cc,done_ptr) = ICHAR('|')  ! debug only
c$$$            ELSE 
c$$$C
c$$$C No diagonal found, copy to left_stack to handle later
c$$$C
c$$$               xy_rect(ms) = min_dia_len
c$$$               CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
c$$$            END IF
c$$$
c$$$         ELSE 
c$$$            xy_rect(ms) = 0
c$$$            CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
c$$$         END IF
c$$$         
c$$$      END DO
c$$$      SUBROUTINE best_permut_diag (x_seq,y_seq,
c$$$     -                             x_low_bnd,x_hig_bnd,
c$$$     -                             y_low_bnd,y_hig_bnd,
c$$$     -                             x_ini_lim,y_ini_lim,
c$$$     -                             x_bst_beg,x_bst_end,
c$$$     -                             y_bst_beg,y_bst_end,
c$$$     -                             bst_all_sco )
c$$$C
c$$$C Finds longest diagonal of consecutive matches over a certain rectangle.
c$$$C The rectangle is searched between its two diagonals plus x_gap_max and
c$$$C y_gap_max to lower right and upper left respectively. Length and 
c$$$C coordinates for longest diagonal is returned.
c$$$C
c$$$      IMPLICIT none
c$$$
c$$$      INCLUDE 'pal_settings.inc'
c$$$      INCLUDE 'pal_stacks.inc'
c$$$      INCLUDE 'pal_bas_eqv.inc'
c$$$      INCLUDE 'pal_permut.inc'
c$$$C
c$$$C Input
c$$$C
c$$$      BYTE    x_seq(*),y_seq(*)                  ! in
c$$$      INTEGER x_low_bnd,x_hig_bnd                ! in
c$$$      INTEGER y_low_bnd,y_hig_bnd                ! in
c$$$      INTEGER x_ini_lim,y_ini_lim                ! in
c$$$      INTEGER x_bst_beg,x_bst_end                ! out
c$$$      INTEGER y_bst_beg,y_bst_end                ! out
c$$$      REAL*8  bst_all_sco                        ! out
c$$$C
c$$$C Local variables
c$$$C
c$$$      INTEGER x_ini,y_ini
c$$$c      INTEGER x_dia_beg,x_dia_end
c$$$c      INTEGER y_dia_beg,y_dia_end
c$$$      INTEGER x,y
c$$$
c$$$      REAL*8  bst_dia_sco
c$$$
c$$$      INTEGER dia_lst(5,1000),dia_ptr
c$$$      INTEGER prev_mtc,prev_mis
c$$$      INTEGER i_mtc,i_mis
c$$$      INTEGER i_ptr,j_ptr
c$$$      INTEGER i_bst_ptr,j_bst_ptr
c$$$      INTEGER dia_len
c$$$      LOGICAL dia_flag,fus_flag
c$$$      INTEGER len
c$$$C
c$$$C ---------------------------------------------------------------------------
c$$$C
c$$$c      write (6,'()')
c$$$c      write (6,'(a,2i6)') ' x_low_bnd,x_hig_bnd = ',x_low_bnd,x_hig_bnd
c$$$c      write (6,'(a,2i6)') ' y_low_bnd,y_hig_bnd = ',y_low_bnd,y_hig_bnd
c$$$c      write (6,'(a,2i6)') ' x_ini_lim,y_ini_lim = ',x_ini_lim,y_ini_lim
c$$$
c$$$      bst_all_sco = 9999.0
c$$$
c$$$      x_bst_beg = 0
c$$$      y_bst_beg = 0
c$$$      x_bst_end = 0
c$$$      y_bst_end = 0
c$$$
c$$$c$$$      IF (dbgflg) THEN
c$$$c$$$         x_ini_lim = x_low_bnd
c$$$c$$$         y_ini_lim = y_low_bnd
c$$$c$$$      END IF
c$$$
c$$$      x_ini = x_ini_lim
c$$$      y_ini = y_low_bnd
c$$$C
c$$$C Decrement x down to x_low_bnd, then increment y up to y_hig_bnd
c$$$C
c$$$c      print*,' y_ini,y_ini_lim = ',y_ini,y_ini_lim 
c$$$      DO WHILE (y_ini.LE.y_ini_lim)
c$$$
c$$$         dia_ptr = 0
c$$$
c$$$         x = x_ini
c$$$         y = y_ini
c$$$
c$$$         prev_mtc = 0
c$$$         prev_mis = 0
c$$$
c$$$         dia_flag = .FALSE.
c$$$         dia_len = 0
c$$$
c$$$         DO WHILE (x.LE.x_hig_bnd .AND. y.LE.y_hig_bnd) 
c$$$
c$$$            IF (bas_eqv(x_seq(x),y_seq(y))) THEN
c$$$               IF (.NOT.dia_flag) THEN
c$$$                  dia_ptr = dia_ptr + 1
c$$$                  dia_lst (1,dia_ptr) = x
c$$$                  dia_lst (2,dia_ptr) = y
c$$$                  dia_lst (3,dia_ptr) = prev_mtc
c$$$                  dia_lst (4,dia_ptr) = prev_mis
c$$$                  dia_flag = .TRUE.
c$$$               END IF
c$$$               prev_mtc = prev_mtc + 1
c$$$               dia_len = dia_len + 1 
c$$$            ELSE
c$$$               IF (dia_flag) THEN
c$$$                  dia_lst (5,dia_ptr) = dia_len
c$$$                  dia_len = 0
c$$$                  dia_flag = .FALSE.
c$$$               END IF
c$$$               prev_mis = prev_mis + 1
c$$$            END IF
c$$$
c$$$            x = x + 1 
c$$$            y = y + 1
c$$$
c$$$         END DO
c$$$
c$$$         IF (dia_flag) dia_lst (5,dia_ptr) = dia_len
c$$$C
c$$$C
c$$$c$$$         IF (dbgflg) THEN
c$$$c$$$            DO i_ptr = 1,dia_ptr
c$$$c$$$               WRITE (6,'(a,6(1x,i5))') ' i_ptr, dia_list : ',i_ptr,
c$$$c$$$     -            (dia_lst(j_ptr,i_ptr),j_ptr=1,5)
c$$$c$$$            END DO
c$$$c$$$         END IF
c$$$
c$$$         IF (dia_ptr.GT.0) THEN
c$$$
c$$$         i_bst_ptr = 1 
c$$$         j_bst_ptr = 1
c$$$         bst_dia_sco = permut_arr (dia_lst(5,1),0)
c$$$
c$$$         DO i_ptr = 1,dia_ptr
c$$$
c$$$c            print*,' i_ptr,bst_dia_sco = ',i_ptr,bst_dia_sco
c$$$            fus_flag = .TRUE.
c$$$            j_ptr = i_ptr + 1
c$$$          
c$$$            DO WHILE (fus_flag .AND. j_ptr.LE.dia_ptr)
c$$$
c$$$               i_mtc = dia_lst(5,j_ptr) 
c$$$     -               + dia_lst(3,j_ptr) - dia_lst(3,i_ptr)
c$$$               i_mis = dia_lst(4,j_ptr) - dia_lst(4,i_ptr)
c$$$
c$$$c               print*,'   j_ptr,i_mtc,i_mis = ', j_ptr,i_mtc,i_mis
c$$$
c$$$               IF (i_mtc.LE.per_win_max .AND. i_mis.LE.per_win_max) THEN
c$$$                  IF ( permut_arr (i_mtc,i_mis) .LT. bst_dia_sco ) THEN
c$$$                     bst_dia_sco = permut_arr (i_mtc,i_mis)
c$$$                     i_bst_ptr = i_ptr 
c$$$                     j_bst_ptr = j_ptr
c$$$c                     print*,'      i_bst_ptr,j_bst_ptr,bst_dia_sco = ',
c$$$c     -                     i_bst_ptr,j_bst_ptr,bst_dia_sco
c$$$                  END IF
c$$$               ELSE
c$$$                  fus_flag = .FALSE.
c$$$               END IF
c$$$
c$$$               j_ptr = j_ptr + 1
c$$$
c$$$            END DO
c$$$
c$$$         END DO
c$$$
c$$$         END IF
c$$$           
c$$$         IF (bst_dia_sco.LT.bst_all_sco) THEN
c$$$                  len = dia_lst(5,j_bst_ptr) 
c$$$     -                + dia_lst(3,j_bst_ptr) - dia_lst(3,i_bst_ptr)
c$$$     -                + dia_lst(4,j_bst_ptr) - dia_lst(4,i_bst_ptr)
c$$$            x_bst_beg = dia_lst (1,i_bst_ptr)
c$$$            y_bst_beg = dia_lst (2,i_bst_ptr)
c$$$            x_bst_end = x_bst_beg + len - 1
c$$$            y_bst_end = y_bst_beg + len - 1
c$$$            bst_all_sco = bst_dia_sco
c$$$         END IF
c$$$            
c$$$         IF (x_ini.GT.x_low_bnd) THEN
c$$$            x_ini = x_ini - 1
c$$$         ELSE
c$$$            y_ini = y_ini + 1
c$$$         END IF
c$$$
c$$$      END DO
c$$$
c$$$      IF (x_low_bnd.EQ.346  .AND. y_low_bnd.EQ.346  .OR.
c$$$     -    x_low_bnd.EQ.384  .AND. y_low_bnd.EQ.384  .OR.
c$$$     -    x_hig_bnd.EQ.442  .AND. y_hig_bnd.EQ.448  .OR.
c$$$     -    x_low_bnd.EQ. 20  .AND. y_low_bnd.EQ. 20  .OR.
c$$$     -    x_low_bnd.EQ. 58  .AND. y_low_bnd.EQ. 58  .OR.
c$$$     -    x_hig_bnd.EQ. 66  .AND. y_hig_bnd.EQ. 122)     THEN
c$$$         write (6,'()')
c$$$         write (6,'(a,2i6)') ' x_low_bnd,x_hig_bnd = ',x_low_bnd,x_hig_bnd
c$$$         write (6,'(a,2i6)') ' y_low_bnd,y_hig_bnd = ',y_low_bnd,y_hig_bnd
c$$$         write (6,'(a,2i6)') ' x_ini_lim,y_ini_lim = ',x_ini_lim,y_ini_lim
c$$$         write (6,'(a,2i6)') ' x_bst_beg,x_bst_end = ',x_bst_beg,x_bst_end
c$$$         write (6,'(a,2i6)') ' y_bst_beg,y_bst_end = ',y_bst_beg,y_bst_end
c$$$         write (6,'(a,f50.48)') ' bst_all_sco = ',bst_all_sco
c$$$      END IF
c$$$
c$$$      RETURN
c$$$      END
c$$$
c$$$      do ipos = 1,x_seqend
c$$$         write (6,'(i6)') x_lnk_lst(ipos)
c$$$      end do
c$$$      stop

c$$$      do ipos = 1,x_seqend
c$$$         write (6,'(i4,1x,a1,1x,i1,1x,i10)') ipos,char(x_seq(ipos)),
c$$$     -                   x_seq_4(ipos),x_iol_seq(ipos)
c$$$      end do
c$$$      print*,' after try'

c$$$c      open (unit=1,file='pal.garb',status='new')
c$$$      DO ioli = 0, (alp_siz**wrd_len)-1
c$$$
c$$$         ipos = x_lnk_bgs(ioli)
c$$$
c$$$         IF (ipos.GT.0) THEN
c$$$            print*,' ipos = ',ipos
c$$$
c$$$            DO WHILE (ipos.GT.0)
c$$$               WRITE (6,'(i5)') ipos
c$$$c$$$               WRITE (6,'(1x,i4,1x,<wrd_len>a1)') ipos, 
c$$$c$$$     -            (char(x_seq(i)),i=ipos,ipos+wrd_len-1)
c$$$               ipos = x_lnk_lst(ipos)
c$$$            END DO
c$$$
c$$$c            WRITE (1,'()')
c$$$         END IF
c$$$
c$$$      END DO
c$$$c      close(1)
c      stop

c$$$C
c$$$C --------------------------------------------------------------------------
c$$$C PASS 2 - find diagonals with low permutation (refined similarity)
c$$$C --------------------------------------------------------------------------
c$$$C
c$$$      left_ptr = 0       ! not really necessary, but..
c$$$
c$$$c$$$      DO i_ptr = 1,work_ptr
c$$$c$$$         write (6,'(4i4)') (work_stack(i,i_ptr),i=1,4)
c$$$c$$$      END DO
c$$$
c$$$      DO WHILE (work_ptr.GT.0)
c$$$c         print*,'work_ptr = ',work_ptr
c$$$C
c$$$C Get rectangle from stack
c$$$C
c$$$         CALL pop_stack (n_work,m_work,work_stack,work_ptr,xy_rect)
c$$$C
c$$$C 
c$$$         min_dia_len = 0
c$$$         err_cod = 0
c$$$         tmp_sco = permut_pen + 1.0
c$$$
c$$$         DO WHILE (tmp_sco.GT.permut_pen .AND. err_cod.EQ.0)
c$$$            min_dia_len = min_dia_len + 1 
c$$$            tmp_sco = dia_begs (xy_rect(xl),xy_rect(xh),
c$$$     -                          xy_rect(yl),xy_rect(yh),
c$$$     -                          xy_rect(xm),xy_rect(ym),
c$$$     -                          tmp_len,
c$$$     -                          err_cod,err_str)
c$$$     -                 * p_mtc ** min_dia_len
c$$$         END DO
c$$$         max_dia_sco = p_mtc ** min_dia_len
c$$$c         print*,' tmp_len,tmp_sco,max_dia_sco = ',tmp_len,tmp_sco,max_dia_sco 
c$$$C
c$$$C Find best diagonal, return coordinates in xy_diag
c$$$C
c$$$         IF ( err_cod.EQ.0 ) THEN 
c$$$c            write (6,'(a,4i5)') ' before p, xy_rect = ',(xy_rect(i),i=1,4)
c$$$            CALL best_permut_diag (x_seq,y_seq,
c$$$     -              xy_rect(xl),xy_rect(xh),
c$$$     -              xy_rect(yl),xy_rect(yh),
c$$$     -              xy_rect(xm),xy_rect(ym),
c$$$     -              xy_diag(xl),xy_diag(xh),
c$$$     -              xy_diag(yl),xy_diag(yh),
c$$$     -              dia_sco )
c$$$c            write (6,'(a,f7.6,4i5)') ' after p, max_dia_sco,xy_diag = ',
c$$$c     -              max_dia_sco,(xy_diag(i),i=1,4)
c$$$c            stop
c$$$            IF ( dia_sco.LE.max_dia_sco ) THEN
c$$$C 
c$$$C Diagonal found: 1) Push it to done_stack, 2) copy sub-rectangles to 
c$$$C work_stack, but only if found diagonal doesnt touch xy-limits.
c$$$C
c$$$               CALL update_stacks (n_work,m_work,work_stack,work_ptr,
c$$$     -                             n_done,m_done,done_stack,done_ptr,
c$$$     -                             n_left,xy_rect,xy_diag)
c$$$               done_stack(cc,done_ptr) = ICHAR(':')  ! debug only
c$$$            ELSE 
c$$$C
c$$$C No diagonal found, copy to left_stack to handle later
c$$$C
c$$$               xy_rect(ms) = min_dia_len
c$$$               CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
c$$$            END IF
c$$$
c$$$         ELSE 
c$$$            xy_rect(ms) = 0
c$$$            CALL push_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
c$$$         END IF
c$$$
c$$$      END DO

c$$$C
c$$$C Where matching segments, set flag
c$$$C
c$$$      s_fit_ptr = MALLOC ( x_seqend )
c$$$      DO i = 1,x_seqend
c$$$         s_fit(i) = 0 ! ambali_ich
c$$$      END DO
c$$$
c$$$      DO i_ptr = 1,done_ptr
c$$$         DO x = done_stack (xl,i_ptr),done_stack(xh,i_ptr)
c$$$            s_fit(x) = 1  ! ctnali_ich ! done_stack(ms,i_ptr)
c$$$         END DO
c$$$      END DO
c$$$
c      write (6,'(150a1)') (char(s_fit(i)),i=1,150) ! x_seqend)
c      write (6,'(<x_seqend>a1)') (char(s_fit(i)),i=1,x_seqend)
C
c$$$C --------------------------------------------------------------------------
c$$$C  Empty stack with still unresolved regions
c$$$C --------------------------------------------------------------------------
c$$$C
c$$$C From rectangles still left on left_stack, connect unaligned stretches
c$$$C by forming diagonals, that are copied to done_stack; everthing else will
c$$$C then be indels, and the alignment will be traced below with insertion of
c$$$C proper symbols at proper places. Always insert trailing indels except at
c$$$C the 5' end and only if at least one match was found. 
c$$$C
c$$$      DO WHILE (left_ptr.GT.0) 
c$$$
c$$$         CALL pop_stack (n_left,m_left,left_stack,left_ptr,xy_rect)
c$$$
c$$$         xy_dif = (xy_rect(xh)-xy_rect(xl)) - (xy_rect(yh)-xy_rect(yl))
c$$$C
c$$$C If there is at least one diagonal found, let gaps be leading,
c$$$C
c$$$         IF (xy_rect(xl).EQ.1 .AND. done_ptr.GT.1) THEN 
c$$$
c$$$            IF (xy_dif.GT.0) THEN
c$$$               xy_diag(xl) = xy_rect(xh) - xy_rect(yh) + xy_rect(yl)
c$$$               xy_diag(yl) = xy_rect(yl)
c$$$            ELSE
c$$$               xy_diag(xl) = xy_rect(xl)
c$$$               xy_diag(yl) = xy_rect(yh) - xy_rect(xh) + xy_rect(xl)
c$$$            END IF
c$$$            
c$$$            xy_scr(xl) = xy_diag(xl)
c$$$            xy_scr(xh) = xy_rect(xh)
c$$$            xy_scr(yl) = xy_diag(yl)
c$$$            xy_scr(yh) = xy_rect(yh)
c$$$C
c$$$C otherwise, insert trailing gaps,
c$$$C
c$$$         ELSE                 
c$$$
c$$$            IF (xy_dif.GT.0) THEN
c$$$               xy_diag(xh) = xy_rect(xl) + xy_rect(yh) - xy_rect(yl)
c$$$               xy_diag(yh) = xy_rect(yh)
c$$$            ELSE
c$$$               xy_diag(xh) = xy_rect(xh)
c$$$               xy_diag(yh) = xy_rect(yl) + xy_rect(xh) - xy_rect(xl)
c$$$            END IF
c$$$
c$$$            xy_scr(xl) = xy_rect(xl)
c$$$            xy_scr(xh) = xy_diag(xh)
c$$$            xy_scr(yl) = xy_rect(yl)
c$$$            xy_scr(yh) = xy_diag(yh)
c$$$
c$$$         END IF
c$$$
c$$$         CALL push_stack (n_done,m_done,done_stack,done_ptr,xy_scr)
c$$$         done_stack(cc,done_ptr) = ICHAR(' ')
c$$$
c$$$      END DO

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
      LOGICAL FUNCTION str_equal (str1,str2)
C
C Only likes characters in the 'printable' range 
C
      IMPLICIT none
 
      CHARACTER*(*) str1,str2

      INTEGER       str_beg,str_end
      INTEGER       beg1,end1
      INTEGER       beg2,end2
      INTEGER       i,j
C
C----------------------------------------------------------------------
C
      beg1 = str_beg(str1)
      end1 = str_end(str1)

      beg2 = str_beg(str2)
      end2 = str_end(str2)

      IF (end1.LT.beg1 .OR.
     -    end2.LT.beg2 .OR.
     -    end2-beg2.NE.end1-beg1) THEN
         str_equal = .FALSE.
         RETURN
      END IF

      j = beg2

      DO i = beg1,end1
         IF (str1(i:i).NE.str2(j:j) .AND.
     -       CHAR(ICHAR(str1(i:i))+32).NE.str2(j:j) .AND.
     -       str1(i:i).NE.CHAR(ICHAR(str2(j:j))+32)) THEN
            str_equal = .FALSE.
            RETURN
         END IF
         j = j+1
      END DO

      str_equal = .TRUE.

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
 
      SUBROUTINE str_upc (str,beg,end)
C
C Changes lower case characters to upper case from beg to end
C
      IMPLICIT none

      CHARACTER*1 str(*)

      INTEGER beg,end
      INTEGER i 
C
C ------------------------------------------------------------------------
C 
      DO i = beg,end
         IF (ICHAR(str(i)).GE.97 .AND. ICHAR(str(i)).LE.122) 
     -       str(i) = CHAR (ICHAR (str(i)) - 32)
      END DO

      RETURN
      END 
      SUBROUTINE To_Code_4 (inbytarr,begdim,enddim,from,to,outbytarr)
C
C The incoming ASCII values (inbytarr(beg) through inbytarr(end)) 
C are translated and returned in outbytarr, according to the 
C following code:
C
C     ASCII    Meaning   Code 
C    ------   --------   -----
C         A =  A           0        (Adenine)
C       U/T =        U     1        (Uracil/Thymine)
C         G =      G       2        (Guanine)
C         C =    C         3        (Cytosine)
C
      IMPLICIT none
      
      INTEGER begdim
      INTEGER enddim
      BYTE    inbytarr (begdim:enddim)
      INTEGER from
      INTEGER to
      BYTE    outbytarr (begdim:enddim)
C
C Local variables
C
      BYTE    Table(0:127)
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
      DO i = from, to
         OutBytArr (i) = Table ( InBytArr(i) )
      END DO

      RETURN
      END
      SUBROUTINE update_stacks (n_work,m_work,work_stack,work_ptr,
     -                          n_done,m_done,done_stack,done_ptr,
     -                          n_left,xy_rect,xy_diag)
C
C 
      INCLUDE 'pal_settings.inc'
      INCLUDE 'pal_stacks.inc'

      INTEGER n_work,m_work
      INTEGER work_stack(n_work,m_work),work_ptr
      INTEGER n_done,m_done
      INTEGER done_stack(n_done,m_done),done_ptr
      INTEGER n_left
      INTEGER xy_rect(n_left),xy_diag(n_left)
C
C Local variables
C
      INTEGER xy_scr(8)  ! improve
C
C --------------------------------------------------------------------------
C
C Push diagonal to done_stack
C
      CALL push_stack (n_done,m_done,done_stack,done_ptr,xy_diag)
C
C Lower left unresolved rectangle, spanned by lower left diagonal end
C and lower left x,y boundaries
C     
      IF (xy_diag(xl).GT.xy_rect(xl) .AND.   ! diagonal doesnt
     -    xy_diag(yl).GT.xy_rect(yl)) THEN   ! touch lower left corner

         xy_scr(xl) = xy_rect(xl)
         xy_scr(xh) = xy_diag(xl) - 1
         xy_scr(yl) = xy_rect(yl)
         xy_scr(yh) = xy_diag(yl) - 1
         CALL xy_ini_lims ( xy_scr(xl), xy_scr(xh),
     -                      xy_scr(yl), xy_scr(yh),
     -                      xy_scr(xm), xy_scr(ym) )
         CALL push_stack (n_work,m_work,work_stack,work_ptr,xy_scr)

      END IF
C
C Upper right unresolved rectangle, spanned by upper right diagonal end
C and upper right x,y boundaries
C     
      IF (xy_diag(xh).LT.xy_rect(xh) .AND.   !  diagonal doesnt
     -    xy_diag(yh).LT.xy_rect(yh)) THEN   !  touch upper right corner

         xy_scr(xl) = xy_diag(xh) + 1
         xy_scr(xh) = xy_rect(xh)
         xy_scr(yl) = xy_diag(yh) + 1
         xy_scr(yh) = xy_rect(yh)
         CALL xy_ini_lims ( xy_scr(xl), xy_scr(xh),
     -                      xy_scr(yl), xy_scr(yh),
     -                      xy_scr(xm), xy_scr(ym) )
         CALL push_stack (n_work,m_work,work_stack,work_ptr,xy_scr)

      END IF

      RETURN
      END
      SUBROUTINE write_gbk_ent ( lun, 
     -                           txtdim, txtbeg, txtend, txt,
     -                           alidim, alibeg, aliend, ali )
C
C explain
C
      IMPLICIT none

      INTEGER        lun 
      INTEGER        txtdim
      INTEGER        txtbeg
      INTEGER        txtend
      CHARACTER*(*)  txt (txtdim)
      INTEGER        alidim
      INTEGER        alibeg
      INTEGER        aliend
      BYTE           ali (alidim)
C
C Local variables
C
      CHARACTER*70 outlin

      INTEGER str_end

      INTEGER ilin, istr, ires
      INTEGER beg, end
      INTEGER i
C
C -------------------------------------------------------------------------------
C
C Write annotation
C
      DO ilin = txtbeg, txtend
         WRITE (lun,'(a)') txt (ilin) (1:str_end(txt(ilin)))
      END DO
C
C Write sequence
C
      beg = alibeg
      end = alibeg + 59 

      DO WHILE ( beg.LE.aliend )

         istr = 0
         ires = 0

         DO i = beg, MIN (end,aliend)
            istr = istr + 1
            ires = ires + 1 
            IF ( MOD(ires,10).EQ.1 ) THEN
               outlin(istr:istr) = ' ' 
               istr = istr + 1 
            END IF
            outlin(istr:istr) = CHAR( ali(i) )
         END DO

         WRITE (lun,'(i9,a)') beg,outlin(1:istr)

         beg = end + 1 
         end = end + 60

      END DO

      WRITE (lun,'(''//'')') 

      RETURN
      END

      SUBROUTINE write_humane (outlun, outwid,
     -                         alilen, x_ali, y_ali, a_fit,
     -                         x_sid, y_sid )
      IMPLICIT none

      INCLUDE 'pal_res_eqv.inc'
C
C Simple listing readable by humans
C
      INTEGER       outlun
      INTEGER       outwid
      INTEGER       alilen
      BYTE          x_ali(*)
      BYTE          y_ali(*)
      BYTE          a_fit(*)
      CHARACTER*(*) x_sid
      CHARACTER*(*) y_sid
c      BYTE          debug(*)
C
C Local variables
C
      CHARACTER*20000 ticmrk      ! silly
      CHARACTER*20000 outstr      ! silly 

      INTEGER outbeg,outend
      INTEGER beg,end
      INTEGER xnum,ynum
      INTEGER i,j,itic

C      CHARACTER*2000 garbstr
C      INTEGER str_end

      CHARACTER*12 program_ID  / 'write_humane' /
C
C ---------------------------------------------------------------------------
C
      IF (outwid.GT.alilen) outwid = alilen

C      print*,'in out_humane'
C      DO i = 1,alilen
C         garbstr(i:i) = CHAR(x_ali(i))
C      END DO
C      WRITE (6,'(a)') garbstr(1:str_end(garbstr))

      outbeg = 1
      outend = outwid

      xnum = 0
      ynum = 0

      DO WHILE (outbeg.LE.alilen)

         beg = outbeg
         end = MIN (outend,alilen)

         itic = 0 

         DO i = 1,outwid
            ticmrk(i:i) = ' ' 
         END DO

         DO i = beg,end
            itic = itic + 1
            IF (IUB_base(x_ali(i))) THEN
               xnum = xnum + 1
               IF (MOD(xnum,50).EQ.0) THEN
                  ticmrk(itic:itic) = '|'
               ELSE IF (MOD(xnum,10).EQ.0) THEN
                  ticmrk(itic:itic) = ','
               END IF
            END IF
         END DO

         WRITE (outlun,'(11x,a)') ticmrk(1:itic)
         
         j = 0
         DO i = beg, end
            j = j + 1 
            outstr(j:j) = CHAR(x_ali(i))
         END DO

         WRITE (outlun,'(a,i5)') x_sid // ' ' // outstr(1:j) // ' ', xnum

         j = 0
         DO i = beg, end
            j = j + 1 
            outstr(j:j) = CHAR(a_fit(i))
         END DO

         WRITE (outlun,'(11x,a)') outstr(1:j)

         itic = 0 

         DO i = 1,outwid
            ticmrk(i:i) = ' ' 
         END DO

         DO i = beg,end
            itic = itic + 1
            IF (IUB_base(y_ali(i))) THEN
               ynum = ynum + 1
               IF (MOD(ynum,50).EQ.0) THEN
                  ticmrk(itic:itic) = '|'
               ELSE IF (MOD(ynum,10).EQ.0) THEN
                  ticmrk(itic:itic) = '`'
               END IF
            END IF
         END DO

         j = 0
         DO i = beg, end
            j = j + 1 
            outstr(j:j) = CHAR(y_ali(i))
         END DO

         WRITE (outlun,'(a,i5)') y_sid // ' ' // outstr(1:j) // ' ', ynum

         WRITE (outlun,'(11x,a)') ticmrk(1:itic)

         outbeg = outend + 1 
         outend = outend + outwid

      END DO

      RETURN
      END

      SUBROUTINE write_message (lun,fnm,str) 
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
         WRITE (6,'(a)') ' ******** ' //LF// 
     -                   ' ** PAL: WRITE_MESSAGE: Error: Cannot open' //
     -                   ' new file ' // fnm(str_beg(fnm):str_end(fnm)) //LF//
     -                   ' **      it may already exist, check also' //
     -                   ' permissions' //LF//
     -                   ' ******** '
         STOP
      END IF

      iout = 20        !1234567890      1     234567890
      outstr(1:iout) = ' ******** ' // LF // ' ** PAL: '

      DO i = 1,str_end(str)
         iout = iout + 1
         outstr(iout:iout) = str(i:i)
         IF (str(i:i).EQ.LF) THEN
            outstr(iout+1:iout+9) = ' **      '
            iout = iout + 9
         END IF
      END DO

      outstr(iout+1:iout+11) = LF // ' ******** '
      iout = iout + 11

      WRITE (lun,'(a)' ) outstr(1:iout)

      RETURN
      END
      SUBROUTINE xy_ini_lims (x_low_bnd,x_hig_bnd,y_low_bnd,y_hig_bnd,
     -                        x_ini_lim,y_ini_lim)
      IMPLICIT none

      INCLUDE 'pal_settings.inc'

      INTEGER x_low_bnd,x_hig_bnd
      INTEGER y_low_bnd,y_hig_bnd
      INTEGER x_ini_lim,y_ini_lim
      INTEGER x_len,y_len
C
C ---------------------------------------------------------------------------
C
      x_len = x_hig_bnd - x_low_bnd 
      y_len = y_hig_bnd - y_low_bnd 

      IF (x_len.GE.y_len) THEN
         x_ini_lim = x_hig_bnd - y_len * (1.0 - x_max_gap)
         y_ini_lim = y_hig_bnd - y_len * (1.0 - y_max_gap)
      ELSE
         x_ini_lim = x_hig_bnd - x_len * (1.0 - x_max_gap)
         y_ini_lim = y_hig_bnd - x_len * (1.0 - y_max_gap)
      END IF

      RETURN
      END
