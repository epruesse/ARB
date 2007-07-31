      CHARACTER*80 cl_alifnm  ! -a  ( ASCII alignment file name )
      CHARACTER*80 cl_binfnm  ! -b  ( binary file name )
      CHARACTER*80 cl_outfnm  ! -o  ( output file name )
      CHARACTER*80 cl_errfnm  ! -e  ( error file name ) 
      CHARACTER*80 cl_newfnm  ! -i  ( new sequence file name )
      CHARACTER*80 cl_logfnm  ! -l  ( log file name )
      CHARACTER*80 cl_namfnm  ! -n  ( namelist file name )
      CHARACTER*80 cl_addfnm  ! -u  ( user appended file name )

      INTEGER      cl_alilun, cl_binlun, cl_outlun, 
     -             cl_errlun, cl_newlun, cl_loglun,
     -             cl_namlun, cl_addlun

      LOGICAL      cl_aliflag, cl_binflag, cl_outflag, 
     -             cl_errflag, cl_newflag, cl_logflag,
     -             cl_namflag, cl_addflag, cl_urlflag,
     -             cl_lstflag

      LOGICAL      cl_listflag
      LOGICAL      cl_widflag
      LOGICAL      cl_stepflag
      LOGICAL      cl_chimflag   ! -chimera  ( chimera option given )
      LOGICAL      cl_simpflag 
      LOGICAL      cl_htmlflag   ! html output
      LOGICAL      cl_partflag
      LOGICAL      cl_fragflag
      LOGICAL      cl_tupflag

      INTEGER      cl_listlen    ! 
      INTEGER      cl_fraglen
      INTEGER      cl_partlen
      INTEGER      cl_tuplen
      INTEGER      cl_outwid
      INTEGER      cl_steplen

      CHARACTER*80 cl_urlpre  ! prefix url to RDP sequences
      CHARACTER*80 cl_lststr  ! Tells what the RDP data were

      COMMON / IO / cl_alifnm,  cl_binfnm,  cl_outfnm, cl_namfnm,
     -              cl_errfnm,  cl_newfnm,  cl_logfnm, cl_addfnm,
     -              cl_alilun,  cl_binlun,  cl_outlun, cl_namlun,
     -              cl_errlun,  cl_newlun,  cl_loglun, cl_addlun,
     -              cl_aliflag, cl_binflag, cl_outflag, cl_namflag,
     -              cl_errflag, cl_newflag, cl_logflag, cl_addflag,
     -              cl_listflag, cl_listlen, cl_chimflag, 
     -              cl_fragflag, cl_fraglen, cl_tuplen, 
     -              cl_tupflag, cl_outwid, cl_widflag,
     -              cl_steplen, cl_stepflag, cl_partlen,
     -              cl_partflag, cl_simpflag, cl_htmlflag,
     -              cl_urlpre, cl_urlflag, cl_lststr, cl_lstflag

