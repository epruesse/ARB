C
C True if an IUB base, false otherwise
C
      BYTE                IUB_base (0:127) 
      COMMON / IUB_base / IUB_base
C
C True if a WC base, false otherwise
C
      BYTE               WC_base (0:127) 
      COMMON / WC_base / WC_base
C
C Tells whether two ICHAR'ed byte values could represent the same 
C nucleic acid
C
      BYTE              Bas_Eqv (0:127,0:127)
      COMMON / parmat / Bas_Eqv
C
C Weights matches between residues
C
      REAL              res_frq (0:127,0:127)
      BYTE              res_sco (0:127,0:127)

      COMMON / RESSCO / res_frq, res_sco
