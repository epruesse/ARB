C
C Initialize indices into the arrays that store flags, logical units 
C and file names,
C
      ndx_ingbk  = 1
      ndx_inkey  = 2
      ndx_intbl  = 3
      ndx_dbbin  = 4
      ndx_dbgbk  = 5
      ndx_bpmat  = 6
      ndx_outgbk = 7
      ndx_outkey = 8 
      ndx_outsab = 9
      ndx_outtbl = 10 
      ndx_outhum = 11
      ndx_outrul = 12
      ndx_outsrv = 15
      ndx_log    = 13
      ndx_err    = 14
C
C Initialize flags and default values, 
C
      cl_dbgapsflag = .TRUE.
      cl_t2uflag    = .FALSE.
      cl_u2tflag    = .FALSE.
      cl_upcaseflag = .FALSE.
      cl_locaseflag = .FALSE.
      cl_completeflag = .FALSE.
      cl_tplflag    = .FALSE.
      cl_debugflag  = .FALSE.

      cl_listwid = 60
      cl_completepct = 90.0
      cl_olilen = 7
      cl_templates = 0

C      IO_fnms(ndx_dbgbk) = ' '
C      IO_fnms(ndx_dbbin) = ' '
C      IO_fnms(ndx_outgbk) = ' '
C      IO_fnms(ndx_err) = ' '
C      IO_fnms(ndx_ingbk) = ' ' 
C      IO_fnms(ndx_log) = ' '
C      IO_fnms(ndx_bpmat) = ' '
CC
CC Its of course not a good idea to permanently assign logical unit
CC numbers; they should be assigned as needed by a function.  
CC
C      IO_luns(ndx_ingbk) = 2
C      IO_luns(ndx_inkey) = 10
C      IO_luns(ndx_intbl) = 11
C      IO_luns(ndx_dbbin) = 3
C      IO_luns(ndx_dbgbk) = 1
C      IO_luns(ndx_bpmat) = 8
C      IO_luns(ndx_outgbk) = 7
C      IO_luns(ndx_outkey) = 12
C      IO_luns(ndx_outsab) = 16
C      IO_luns(ndx_outtbl) = 13
C      IO_luns(ndx_outhum) = 14
C      IO_luns(ndx_outrul) = 15
C      IO_luns(ndx_log) = 4
C      IO_luns(ndx_err) = 9
C
C      IO_flags(ndx_ingbk) = .FALSE.
C      IO_flags(ndx_inkey) = .FALSE.
C      IO_flags(ndx_intbl) = .FALSE.
C      IO_flags(ndx_dbbin) = .FALSE.
C      IO_flags(ndx_dbgbk) = .FALSE.
C      IO_flags(ndx_bpmat) = .FALSE.
C      IO_flags(ndx_outgbk) = .FALSE.
C      IO_flags(ndx_outkey) = .FALSE.
C      IO_flags(ndx_outtbl) = .FALSE. 
C      IO_flags(ndx_outhum) = .FALSE.
C      IO_flags(ndx_outrul) = .FALSE.
C      IO_flags(ndx_outbin) = .TRUE.
C      IO_flags(ndx_err) = .FALSE.
C      IO_flags(ndx_log) = .FALSE.
C
C      cl_dbgapsflag = .TRUE.
C      cl_t2uflag    = .FALSE.
C      cl_u2tflag    = .FALSE.
C      cl_upcaseflag = .FALSE.
C      cl_locaseflag = .FALSE.
C      cl_completeflag = .FALSE.
C      cl_tplflag    = .FALSE.
C      cl_debugflag  = .FALSE.
C
C      cl_listwid = 60
C      cl_completepct = 90.0
C      cl_olilen = 7
