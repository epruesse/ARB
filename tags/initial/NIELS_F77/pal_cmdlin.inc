C
C Indices to IO arrays with flags, logical units and file names
C
      INTEGER ndx_ingbk, ndx_inkey, ndx_intbl
      INTEGER ndx_dbbin, ndx_dbgbk, ndx_dbkey, ndx_dbtbl
      INTEGER ndx_bpmat
      INTEGER ndx_outgbk, ndx_outkey, ndx_outsab, ndx_outtbl,
     -        ndx_outhum, ndx_outrul, ndx_outsrv
      INTEGER ndx_log, ndx_err

      COMMON / NDX /  ndx_ingbk, ndx_inkey, ndx_intbl,
     -                ndx_dbbin, ndx_dbgbk, ndx_dbkey, ndx_dbtbl,
     -                ndx_bpmat,
     -                ndx_outgbk, ndx_outkey, ndx_outsab, ndx_outtbl,
     -                ndx_outhum, ndx_outrul, ndx_outsrv,
     -                ndx_log, ndx_err
C
C Arrays that store flags, logical units and file names,
C
      LOGICAL   IO_flags (ndx_max) 
      COMMON  / IO_FLAGS / IO_flags

      INTEGER  IO_luns (ndx_max)
      COMMON / IO_LUNS / IO_luns

      CHARACTER*80 IO_fnms (ndx_max)
      COMMON / IO_FNMS / IO_fnms
C
C Options
C
      INTEGER      cl_templates
      REAL         cl_completepct
      INTEGER      cl_listwid
      INTEGER      cl_olilen

      LOGICAL      cl_completeflag, cl_tplflag,  cl_outbinflag,
     -             cl_dbgapsflag,   cl_t2uflag,  cl_u2tflag,
     -             cl_upcaseflag, cl_locaseflag,  cl_debugflag,
     -             cl_htmlflag
C
C Common block for command line options, 
C
      COMMON / IO_OPTS / 
     -    cl_templates,  cl_completepct,  cl_listwid,  cl_olilen,
     -    cl_completeflag, cl_tplflag, cl_outbinflag,
     -    cl_dbgapsflag, cl_t2uflag,      cl_u2tflag,  
     -    cl_upcaseflag, cl_locaseflag,   cl_debugflag,
     -    cl_htmlflag



C
C New unaligned sequences and names
C
C      CHARACTER*80 IO_fnms(ndx_ingbk),  IO_fnms(ndx_inkey),  IO_fnms(ndx_intbl) 
C      INTEGER      IO_luns(ndx_ingbk),  IO_luns(ndx_inkey),  IO_luns(ndx_intbl) 
C      LOGICAL      IO_flags(ndx_ingbk), IO_flags(ndx_inkey), IO_flags(ndx_intbl)
CC
CC Database of aligned sequences
CC
C      CHARACTER*80 IO_fnms(ndx_dbgbk),  cl_dbkeyfnm,  cl_dbtblfnm,  IO_fnms(ndx_dbbin)
C      INTEGER      IO_luns(ndx_dbgbk),  cl_dbkeylun,  cl_dbtbllun,  IO_luns(ndx_dbbin)
C      LOGICAL      IO_flags(ndx_dbgbk), cl_dbkeyflag, cl_dbtblflag, IO_flags(ndx_dbbin)
CC
CC Matrices
CC
C      CHARACTER*80 IO_fnms(ndx_bpmat)
C      INTEGER      IO_luns(ndx_bpmat)
C      LOGICAL      IO_flags(ndx_bpmat)
CC
CC
CC Output of aligned sequences and names
CC
C      CHARACTER*80 IO_fnms(ndx_outgbk),  cl_outkeyfnm,  cl_outrulfnm, 
C     -             cl_outtblfnm,  cl_outhumfnm,  cl_outsabfnm
C      INTEGER      IO_luns(ndx_outgbk),  IO_luns(ndx_outkey),  IO_luns(ndx_outrul),
C     -             IO_luns(ndx_outtbl),  cl_outhumlun,  IO_luns(ndx_outsab)
C      LOGICAL      IO_flags(ndx_outgbk), IO_flags(ndx_outkey), IO_flags(ndx_outrul), 
C     -             IO_flags(ndx_outtbl), cl_outhumflag, cl_outsabflag
CC
CC Log and error messages
CC
C      CHARACTER*80 IO_fnms(ndx_err),  IO_fnms(ndx_log)
C      INTEGER      IO_luns(ndx_err),  IO_luns(ndx_log)
C      INTEGER      IO_flags(ndx_err), IO_flags(ndx_log)
CC
CC Options
CC
C      INTEGER      cl_templates
C      REAL         cl_completepct
C      INTEGER      cl_listwid
C      INTEGER      cl_olilen
C
C      LOGICAL      cl_completeflag, cl_tplflag,  IO_flags(ndx_outbin),
C     -             cl_dbgapsflag,   cl_t2uflag,  cl_u2tflag,
C     -             cl_upcaseflag, cl_locaseflag,  cl_debugflag
CC
CC Common block for everything that relates to command line
CC
C      COMMON / IO / 
C     -    IO_fnms(ndx_ingbk),   IO_fnms(ndx_inkey),   IO_fnms(ndx_intbl),
C     -    IO_luns(ndx_ingbk),   IO_luns(ndx_inkey),   IO_luns(ndx_intbl),
C     -    IO_flags(ndx_ingbk),  IO_flags(ndx_inkey),  IO_flags(ndx_intbl),
C     -    IO_fnms(ndx_dbgbk),   cl_dbkeyfnm,   cl_dbtblfnm,  IO_fnms(ndx_dbbin),
C     -    IO_luns(ndx_dbgbk),   cl_dbkeylun,   cl_dbtbllun,  IO_luns(ndx_dbbin),
C     -    IO_flags(ndx_dbgbk),  cl_dbkeyflag,  cl_dbtblflag, IO_flags(ndx_dbbin),
C     -    IO_fnms(ndx_bpmat),   IO_luns(ndx_bpmat),   IO_flags(ndx_bpmat),
C     -    IO_fnms(ndx_outgbk),  cl_outkeyfnm,  cl_outrulfnm,  
C     -    cl_outtblfnm,  cl_outhumfnm,  cl_outsabfnm,
C     -    IO_luns(ndx_outgbk),  IO_luns(ndx_outkey),  IO_luns(ndx_outrul), 
C     -    IO_luns(ndx_outtbl),  cl_outhumlun,  IO_luns(ndx_outsab),
C     -    IO_flags(ndx_outgbk), IO_flags(ndx_outkey), IO_flags(ndx_outrul), 
C     -    IO_flags(ndx_outtbl), cl_outhumflag, cl_outsabflag,
C     -    IO_fnms(ndx_err),     IO_fnms(ndx_log),
C     -    IO_luns(ndx_err),     IO_luns(ndx_log),
C     -    IO_flags(ndx_err),    IO_flags(ndx_log),
C     -    cl_templates,  cl_completepct,  cl_listwid,  cl_olilen,
C     -    cl_completeflag, cl_tplflag, IO_flags(ndx_outbin), 
C     -    cl_dbgapsflag, cl_t2uflag,      cl_u2tflag,  
C     -    cl_upcaseflag, cl_locaseflag,   cl_debugflag
C
