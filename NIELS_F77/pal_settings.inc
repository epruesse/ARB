C
C Settings common block
C
      REAL       x_max_gap,y_max_gap   ! Restrict rectangle searches
      REAL       consec_pen            ! Strictness requirement
      REAL       p_mtc,p_mis           ! probability of match & mismatch
      BYTE       x_gap5,y_gap5         ! Leading gap symbols
      BYTE       x_gap4,y_gap4         ! Internal gap symbols
      BYTE       x_gap3,y_gap3         ! Trailing gap symbols
      BYTE       end5_ch, end3_ch      
      BYTE       ambx_ch, amby_ch
      BYTE       crtn_ch, unal_ch

      LOGICAL*1  dbgflg 

      COMMON / SETTINGS / y_max_gap, x_max_gap, consec_pen, p_mtc, p_mis,
     -                    x_gap5, y_gap5, x_gap4, y_gap4, x_gap3, y_gap3,
     -                    end5_ch, end3_ch, ambx_ch, amby_ch, crtn_ch, unal_ch,
     -                    dbgflg
C
C ID stamps
C
      CHARACTER*12 author_ID 
      CHARACTER*23 email_ID  
      CHARACTER*3 program_ID
      CHARACTER*3 version_ID
      CHARACTER*15 revdate_ID
            
      COMMON / IDSTAMPS / author_ID, email_ID, program_ID, 
     -                    version_ID, revdate_ID
