
# WARNING : this file is created automatically 
# edit ARB_GDEmenus . source and MENUS / xxx . menu instead 
# and run make in this directory 

# To get more information read GDE2 .2 _manual_text 
# I added two new argtypes : 
# argtype : tree A list of trees in the database 
# argtype : weights A list of conservation profiles in the db 

lmenu : export 
item : Export Sequences only to Foreign Format ( using readseq : slow ) ... 

itemmethod : arb_readseq in1 - pipe - all - form = $ FORMAT > $ OUTPUTFILE 
itemhelp : readseq . help 

arg : FORMAT 
argtype : choice_menu 
argchoice : GenBank : genbank 
argchoice : IG / Stanford : ig 
argchoice : NBRF : nbrf 
argchoice : EMBL : embl 
argchoice : GCG : gcg 
argchoice : DNA Strider : strider 
argchoice : Fitch : fitch 
argchoice : Pearson / Fasta : pearson 
argchoice : Zuker : zuker 
argchoice : Olsen : olsen 
argchoice : Phylip : phylip 
argchoice : Plain text : raw 
argchoice : ASN .1 : asn 
argchoice : PIR : pir 
argchoice : MSF : msf 
argchoice : PAUP : paup 
argchoice : Pretty : pretty - nametop - nameleft = 3 - numright - nameright - numtop 

arg : OUTPUTFILE 
argtype : text 
arglabel : Save as ? 

in : in1 
informat : genbank 

lmenu : pretty_print 
item : Pretty Print Sequences ( slow ) ... 

itemmethod : arb_readseq in1 - p - a - f = pretty $ NAMELEFT $ NAMERIGHT $ NUMTOP $ NUMBOT $ NUMLEFT $ NUMRIGHT - col = $ COLS - width = $ WIDTH $ MATCH $ GAPC > in1 . pretty ; ( arb_textprint in1 . pretty ; / bin / rm - f in1 in1 . pretty ) & 
itemhelp : readseq . help 
in : in1 
informat : genbank 

arg : NAMETOP 
argtype : chooser 
arglabel : Names at top ? 
argchoice : No : 
argchoice : Yes : - nametop 

arg : NAMELEFT 
argtype : chooser 
arglabel : Names at left ? 
argchoice : No : 
argchoice : Yes : - nameleft 

arg : NAMERIGHT 
argtype : chooser 
arglabel : Names at right ? 
argchoice : Yes : - nameright 
argchoice : No : 

arg : NUMTOP 
argtype : chooser 
arglabel : Numbers at top ? 
argchoice : Yes : - numtop 
argchoice : No : 

arg : NUMBOT 
argtype : chooser 
arglabel : Numbers at tail ? 
argchoice : No : 
argchoice : Yes : - numbot 

arg : NUMLEFT 
argtype : chooser 
arglabel : Numbers at left ? 
argchoice : Yes : - numleft 
argchoice : No : 

arg : NUMRIGHT 
argtype : chooser 
arglabel : Numbers at right ? 
argchoice : Yes : - numright 
argchoice : No : 

arg : MATCH 
argtype : chooser 
arglabel : Use match '.' for 2. . n species ? 
argchoice : No : 
argchoice : Yes : - match 

arg : GAPC 
argtype : chooser 
arglabel : Count gap symbols ? 
argchoice : No : 
argchoice : Yes : - gap 

arg : WIDTH 
argtype : slider 
arglabel : Sequence width ? 
argmin : 10 
argmax : 200 
argvalue : 100 

arg : COLS 
argtype : slider 
arglabel : Column spacers ? 
argmin : 0 
argmax : 50 
argvalue : 10 

lmenu : import 
item : Import Sequences in Foreign Format ( using readseq : slow ! ! ) ... 

itemmethod : arb_readseq $ INPUTFILE - a - f2 > OUTPUTFILE 
itemhelp : readseq . help 

arg : INPUTFILE 
argtype : text 
arglabel : Name of foreign file ? 

out : OUTPUTFILE 
outformat : genbank 



item : Test arb_jim ... 


itemmethod : arb_jim 
itemhelp : jim . help 


out : OUTPUTFILE 
outformat : genbank 

menu : align 

arb_item : Clustalv Alignment ( DNA ) 
itemmethod : ( tr '"%//' '>' < in1 > clus_in ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f clus_in * in1 ) 

itemhelp : clustal_help 

arg : KTUP 
argtype : slider 
arglabel : K - tuple size for fast pairwise search 
argmin : 1 
argmax : 10 
argvalue : 2 

arg : WIN 
argtype : slider 
arglabel : Window size 
argmin : 1 
argmax : 10 
argvalue : 4 

arg : Trans 
argtype : chooser 
arglabel : Transitions weighted ? 
argchoice : Yes : / TRANSIT 
argchoice : No : 

arg : FIXED 
argtype : slider 
arglabel : Fixed gap penalty 
argmin : 1 
argmax : 100 
argvalue : 10 

arg : FLOAT 
arglabel : Floating gap penalty 
argtype : slider 
argmin : 1 
argmax : 100 
argvalue : 10 

in : in1 
informat : flat 
insave : 

out : out1 
outformat : flat 


item : Clustalw DNA Alignment 
itemmethod : ( tr '"%//' '>' < in1 > clus_in ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f clus_in * in1 ) 

itemhelp : clustal . help 

arg : KTUP 
argtype : slider 
arglabel : K - tuple size for fast pairwise search 
argmin : 1 
argmax : 10 
argvalue : 2 

arg : WIN 
argtype : slider 
arglabel : Window size for fast pairwise search 
argmin : 1 
argmax : 10 
argvalue : 4 

arg : MATRIX_ 
argtype : chooser 
arglabel : Weighting matrix 
argchoice : BLOSUM : BLOSUM 
argchoice : PAM : PAM 
argchoice : GONNET : GONNET 
argchoice : ID : ID 

arg : FIXED_ 
argtype : slider 
arglabel : Gap Open Panelty 
argmin : 1 
argmax : 100 
argvalue : 8.7 

arg : FLOAT_ 
arglabel : Gap Extend Panelty 
argtype : slider 
argmin : .001 
argmax : 1 
argvalue : .1 


in : in1 
informat : flat 
insave : 

out : out1 
outformat : flat 


item : Clustalw Protein Alignment 
itemmethod : ( tr '"%//' '>' < in1 > clus_in ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; \ / bin / rm - f clus_in * in1 ) 


itemhelp : clustalw . help 

arg : KTUP_ 
argtype : slider 
arglabel : K - tuple size for fast pairwise search 
argmin : 1 
argmax : 10 
argvalue : 2 

arg : WIN_ 
argtype : slider 
arglabel : Window size and top diagonals for fast alignment 
argmin : 1 
argmax : 10 
argvalue : 4 

arg : MATRIX_ 
argtype : chooser 
arglabel : Weighting matrix 
argchoice : BLOSUM : BLOSUM 
argchoice : PAM : PAM 
argchoice : GONNET : GONNET 
argchoice : ID : ID 

arg : FIXED_ 
argtype : slider 
arglabel : Gap Open Panelty 
argmin : 1 
argmax : 100 
argvalue : 8.7 

arg : FLOAT_ 
arglabel : Gap Extend Panelty 
argtype : slider 
argmin : .001 
argmax : 1 
argvalue : .1 



in : in1 
informat : flat 
insave : 

out : out1 
outformat : flat 

item : Assemble Contigs 
itemmethod : ( tr '"%//' '>' < in1 > in1 . tmp ; CAP2 in1 . tmp $ OVERLAP $ PMATCH > out1 ; / bin / rm - f in1 . tmp ) 
itemhelp : CAP2 . help 

arg : OVERLAP 
argtype : slider 
arglabel : Minimum overlap ? 
argmin : 5 
argmax : 100 
argvalue : 20 

arg : PMATCH 
argtype : slider 
arglabel : Percent match required within overlap 
argmin : 25 
argmax : 100 
argvalue : 90 

in : in1 
informat : flat 

out : out1 
outformat : gde 

menu : user 

item : Start a slave arb on a foreign host ... 

itemmethod : $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' & 

arg : dis 
argtype : text 
arglabel : hostname of slave host ? 

menu : DNA / RNA 

item : Translate ... 
itemmethod : Translate - tbl $ TBL - frame $ FRAME - min_frame $ MNFRM $ LTRCODE $ NOSEP in1 > out1 

arg : FRAME 
argtype : chooser 
arglabel : Which reading frame ? 
argchoice : First : 1 
argchoice : Second : 2 
argchoice : Third : 3 
argchoice : All six : 6 

arg : MNFRM 
arglabel : Minimum length of NT sequence to translate ? 
argtype : slider 
argmin : 0 
argmax : 500 
argvalue : 20 

arg : LTRCODE 
argtype : chooser 
arglabel : Translate to : 
argchoice : Single letter codes : 
argchoice : Triple letter codes : - 3 

arg : NOSEP 
argtype : chooser 
arglabel : Seperate ? 
argchoice : No : 
argchoice : Yes : - sep 

arg : TBL 
arglabel : Codon table ? 
argtype : chooser 
argchoice : mycoplasma : 2 
argchoice : universal : 1 
argchoice : yeast : 3 
argchoice : Vert . mito . : 4 

in : in1 
informat : gde 

out : out1 
outformat : gde 

item : Dot plot 
itemmethod : ( DotPlotTool in1 ; / bin / rm - f in1 ) & 
itemhelp : DotPlotTool . help 

in : in1 
informat : gde 
insave : 


item : Clustal alignment 
itemmethod : ( tr '"%//' '>' < in1 > clus_in ; cp in1 test ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f clus_in * in1 ) 

itemhelp : clustal_help 

arg : KTUP 
argtype : slider 
arglabel : K - tuple size for pairwise search 
argmin : 1 
argmax : 10 
argvalue : 2 

arg : WIN 
argtype : slider 
arglabel : Window size 
argmin : 1 
argmax : 10 
argvalue : 4 

arg : Trans 
argtype : chooser 
arglabel : Transitions weighted ? 
argchoice : Yes : / TRANSIT 
argchoice : No : 

arg : FIXED 
argtype : slider 
arglabel : Fixed gap penalty 
argmin : 1 
argmax : 100 
argvalue : 10 

arg : FLOAT 
arglabel : Floating gap penalty 
argtype : slider 
argmin : 1 
argmax : 100 
argvalue : 10 


in : in1 
informat : flat 
insave : 

out : out1 
outformat : flat 



item : Variable Positions 
itemmethod : varpos $ REV < in1 > out1 

arg : REV 
argtype : chooser 
arglabel : Highlight ( darken ) 
argchoice : Conserved positions : 
argchoice : variable positions : - rev 

in : in1 
informat : flat 

out : out1 
outformat : colormask 




item : MFOLD 
itemmethod : ( tr 'a-z' 'A-Z' < seqGB > . GDE . tmp . caps ; ZUKERGDE . sh . GDE . tmp . caps $ CT $ ARBHOME / GDEHELP / ZUKER / > out1 && $ METHOD < out1 ; Zuk_to_gen < $ CT > file . gen ; gde file . gen & $ { ARB_TEXTEDIT : - textedit } RegionTable ) & 
itemhelp : MFOLD . help 

in : seqGB 
informat : genbank 
insave : 

arg : METHOD 
argtype : chooser 
arglabel : RNA type 
argchoice : Fold Linear RNA : lrna 
argchoice : Fold Circular RNA : crna 

arg : CT 
argtype : text 
arglabel : Pairing ( ct ) File Name 
argtext : mfold_out 


item : Draw Secondary structure 
itemmethod : ( LoopTool $ TEMPLATE in1 ; / bin / rm - f in1 ) & 
itemhelp : LoopTool . help 

arg : TEMPLATE 
argtype : chooser 
arglabel : Use template file . / loop . temp ? 
argchoice : No : 
argchoice : Yes : - t loop . temp 

in : in1 
informat : genbank 
insave : 

item : blastn 
itemmethod : ( sed "s/[//%]/>/" < in1 > in1 . f ; blastn $ BLASTDB in1 . f W = $ WORDLEN M = $ MATCH N = $ MMSCORE > in1 . tmp ; $ { ARB_TEXTEDIT : - textedit } in1 . tmp ; rm in1 * ) & 
itemhelp : blast . help 

in : in1 
informat : flat 
insave : 

arg : BLASTDB 
argtype : choice_menu 
arglabel : Which Database 
argchoice : genbank : $ ARBHOME / GDEHELP / BLAST / genbank 
argchoice : genbank update : $ ARBHOME / GDEHELP / BLAST / genupdate 

arg : WORDLEN 
argtype : slider 
arglabel : Word Size 
argmin : 4 
argmax : 18 
argvalue : 12 

arg : MATCH 
argtype : slider 
arglabel : Match Score 
argmin : 1 
argmax : 10 
argvalue : 5 

arg : MMSCORE 
argtype : slider 
arglabel : Mismatch Score 
argmin : - 10 
argmax : - 1 
argvalue : - 5 

item : blastx 
itemmethod : ( sed "s/[//%]/>/" < in1 > in1 . f ; cp $ ARBHOME / GDEHELP / BLAST / PAM ? ? ? . ; blastx $ BLASTDB in1 . f W = $ WORDLEN M = $ Matrix C = $ CODE > in1 . tmp ; $ { ARB_TEXTEDIT : - textedit } in1 . tmp ; rm in1 * PAM ? ] & 
itemhelp : blast . help 

in : in1 
informat : flat 
insave : 

arg : BLASTDB 
argtype : choice_menu 
arglabel : Which Database 
argchoice : pir1 : $ ARBHOME / GDEHELP / BLAST / pir 
argchoice : genpept : $ ARBHOME / GDEHELP / BLAST / genpept 

arg : WORDLEN 
argtype : slider 
arglabel : Word Size 
argmin : 1 
argmax : 5 
argvalue : 3 

arg : Matrix 
arglabel : Substitution Matrix : 
argtype : choice_menu 
argchoice : PAM120 : PAM120 
argchoice : PAM250 : PAM250 

arg : CODE 
argtype : choice_menu 
arglabel : Genetic Code 
argchoice : Standard or Universal : 0 
argchoice : Vertebrate Mitochondrial : 1 
argchoice : Yeast Mitochondrial : 2 
argchoice : Mold Mitochondrial and Mycoplasma : 3 
argchoice : Invertebrate Mitochondrial : 4 
argchoice : Ciliate Macronuclear : 5 
argchoice : Protozoan Mitochondrial : 6 
argchoice : Plant Mitochondrial : 7 
argchoice : Echinodermate Mitochondrial : 8 


item : FASTA ( DNA / RNA ) 
itemmethod : ( sed "s/[//%]/>/" < in1 > in1 . fasta ; fasta - Q - d $ NUMOFALN $ MATRIX in1 . fasta $ DBASE > in1 . out ; $ { ARB_TEXTEDIT : - textedit } in1 . out ; \ rm in1 * ) & 
itemhelp : FASTA . help 

in : in1 
informat : flat 

arg : DBASE 
argtype : choice_menu 
arglabel : Database 
argchoice : GenBank Primate : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbpri . seq \ 1 
argchoice : GenBank Rodent : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbrod . seq \ 1 
argchoice : GenBank all Mammal : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbmam . seq \ 1 
argchoice : GenBank verteBrates : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbvrt . seq \ 1 
argchoice : GenBank Inverts : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbinv . seq \ 1 
argchoice : GenBank pLants : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbpln . seq \ 1 
argchoice : GenBank Struct RNA : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbrna . seq \ 1 
argchoice : GenBank euk . Organelles : $ ARBHOME / GDEHELP / FASTA / GENBANK / gborg . seq \ 1 
argchoice : GenBank phaGe : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbphg . seq \ 1 
argchoice : GenBank bacTeria : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbbct . seq \ 1 
argchoice : GenBank sYnthetic : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbsyn . seq \ 1 
argchoice : GenBank Viral : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbvrl . seq \ 1 
argchoice : GenBank Unannotated : $ ARBHOME / GDEHELP / FASTA / GENBANK / gbuna . seq \ 1 

arg : NUMOFALN 
argtype : slider 
arglabel : Number of Alignment to Report 
argmin : 1 
argmax : 100 
argvalue : 20 

arg : MATRIX 
arglabel : Which SMATRIX 
argtype : choice_menu 
argchoice : Default : 
argchoice : altdiag . mat : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / altdiag . mat 
argchoice : altprot . mat : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / altprot . mat 
argchoice : dna . mat : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / dna . mat 
argchoice : prot . mat : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / prot . mat 

menu : Protein 

item : Clustal Protein Alignment 
itemmethod : ( tr '"%//' '>' < in1 > clus_in ; cp in1 test ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f clus_in * in1 ) 


itemhelp : clustal_help 

arg : KTUP 
argtype : slider 
arglabel : K - tuple size for pairwise search 
argmin : 1 
argmax : 10 
argvalue : 2 

arg : WIN 
argtype : slider 
arglabel : Window size 
argmin : 1 
argmax : 10 
argvalue : 4 

arg : Matrx 
argtype : chooser 
arglabel : Weighting matrix 
argchoice : PAM 250 : PAM250 
argchoice : PAM 100 : PAM100 
argchoice : Identity : ID 

arg : FIXED 
argtype : slider 
arglabel : Fixed gap penalty 
argmin : 1 
argmax : 100 
argvalue : 10 

arg : FLOAT 
arglabel : Floating gap penalty 
argtype : slider 
argmin : 1 
argmax : 100 
argvalue : 10 



in : in1 
informat : flat 
insave : 

out : out1 
outformat : flat 

item : blastp 
itemmethod : ( sed "s/[//%]/>/" < in1 > in1 . f ; cp $ ARBHOME / GDEHELP / BLAST / PAM ? ? ? . ; blastp $ BLASTDB in1 . f W = $ WORDLEN M = $ Matrix > in1 . tmp ; $ { ARB_TEXTEDIT : - textedit } in1 . tmp ; rm in1 * PAM ? ] & 
itemhelp : blast . help 

in : in1 
informat : flat 
insave : 

arg : BLASTDB 
argtype : choice_menu 
arglabel : Which Database 
argchoice : pir : $ ARBHOME / GDEHELP / BLAST / pir 
argchoice : genpept : $ ARBHOME / GDEHELP / BLAST / genpept 
argchoice : local : $ ARBHOME / GDEHELP / BLAST / local_db 

arg : Matrix 
arglabel : Substitution Matrix : 
argtype : choice_menu 
argchoice : PAM120 : PAM120 
argchoice : PAM250 : PAM250 

arg : WORDLEN 
argtype : slider 
arglabel : Word Size 
argmin : 1 
argmax : 5 
argvalue : 3 

item : tblastn 
itemmethod : ( sed "s/[//%]/>/" < in1 > in1 . f ; cp $ ARBHOME / GDEHELP / BLAST / PAM ? ? ? . ; tblastn $ BLASTDB in1 . f W = $ WORDLEN M = $ Matrix C = $ CODE > in1 . tmp ; $ { ARB_TEXTEDIT : - textedit } in1 . tmp ; rm in1 * PAM ? ] & 
itemhelp : blast . help 

in : in1 
informat : flat 
insave : 

arg : BLASTDB 
argtype : choice_menu 
arglabel : Which Database 
argchoice : genbank : $ ARBHOME / GDEHELP / BLAST / genbank 
argchoice : genbank update : $ ARBHOME / GDEHELP / BLAST / genupdate 

arg : Matrix 
arglabel : Substitution Matrix : 
argtype : choice_menu 
argchoice : PAM120 : PAM120 
argchoice : PAM250 : PAM250 

arg : WORDLEN 
argtype : slider 
arglabel : Word Size 
argmin : 4 
argmax : 18 
argvalue : 12 

arg : CODE 
argtype : choice_menu 
arglabel : Genetic Code 
argchoice : Standard or Universal : 0 
argchoice : Vertebrate Mitochondrial : 1 
argchoice : Yeast Mitochondrial : 2 
argchoice : Mold Mitochondrial and Mycoplasma : 3 
argchoice : Invertebrate Mitochondrial : 4 
argchoice : Ciliate Macronuclear : 5 
argchoice : Protozoan Mitochondrial : 6 
argchoice : Plant Mitochondrial : 7 
argchoice : Echinodermate Mitochondrial : 8 

item : blast3 
itemmethod : ( sed "s/[//%]/>/" < in1 > in1 . f ; cp $ ARBHOME / GDEHELP / BLAST / PAM ? ? ? . ; blast3 $ BLASTDB in1 . f W = $ WORDLEN M = $ Matrix > in1 . tmp ; $ { ARB_TEXTEDIT : - textedit } in1 . tmp ; rm in1 * PAM ? ] & 
itemhelp : blast3 . help 

in : in1 
informat : flat 
insave : 

arg : BLASTDB 
argtype : choice_menu 
arglabel : Which Database 
argchoice : pir1 : $ ARBHOME / GDEHELP / BLAST / pir 
argchoice : genpept : $ ARBHOME / GDEHELP / BLAST / genpept 

arg : Matrix 
arglabel : Substitution Matrix : 
argtype : choice_menu 
argchoice : PAM120 : PAM120 
argchoice : PAM250 : PAM250 

arg : WORDLEN 
argtype : slider 
arglabel : Word Size 
argmin : 1 
argmax : 5 
argvalue : 3 

item : FASTA ( Protein ) 
itemmethod : ( sed "s/[//%]/>/" < in1 > in1 . fasta ; fasta - Q - d $ NUMOFALN $ MATRIX in1 . fasta $ DBASE > in1 . out ; $ { ARB_TEXTEDIT : - textedit } in1 . out ; \ rm in1 * ) & 
itemhelp : FASTA . help 

in : in1 
informat : flat 

arg : DBASE 
argtype : choice_menu 
arglabel : Database 
argchoice : NBRF PIR1 : $ ARBHOME / GDEHELP / FASTA / PIR / pir1 . dat \ 2 
argchoice : NBRF PIR2 : $ ARBHOME / GDEHELP / FASTA / PIR / pir2 . dat \ 2 
argchoice : NBRF PIR3 : $ ARBHOME / GDEHELP / FASTA / PIR / pir3 . dat \ 2 


arg : NUMOFALN 
argtype : slider 
arglabel : Number of Alignment to Report 
argmin : 1 
argmax : 100 
argvalue : 20 

arg : MATRIX 
arglabel : Which SMATRIX 
argtype : choice_menu 
argchoice : Default : 
argchoice : Minimum mutation matrix : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / codaa . mat 
argchoice : Identity matrix : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / idnaa . mat 
argchoice : Identity matrix for mismatches : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / idpaa . mat 
argchoice : PAM250 : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / pam250 . mat 
argchoice : PAM120 : - s $ ARBHOME / GDEHELP / FASTA / MATRIX / pam120 . mat 

menu : Seq management 

item : Assemble Contigs 
itemmethod : ( sed "s////>/" < in1 > in1 . tmp ; CAP2 in1 . tmp $ OVERLAP $ PMATCH > out1 ; / bin / rm - f in1 . tmp ) 
itemhelp : CAP2 . help 

arg : OVERLAP 
argtype : slider 
arglabel : Minimum overlap ? 
argmin : 5 
argmax : 100 
argvalue : 20 

arg : PMATCH 
argtype : slider 
arglabel : Percent match required within overlap 
argmin : 25 
argmax : 100 
argvalue : 90 

in : in1 
informat : flat 

out : out1 
outformat : gde 
outoverwrite : 

item : Map View 
itemmethod : ( mapview in1 - pbl $ PBL - npp $ NPP ; / bin / rm - f in1 ) & 
itemhelp : mapview . help 

in : in1 
informat : gde 
insave : 

arg : PBL 
arglabel : Pixel Between Lines 
argtype : slider 
argvalue : 10 
argmin : 1 
argmax : 15 

arg : NPP 
arglabel : Nucleotides Per Pixel 
argtype : slider 
argvalue : 1 
argmin : 1 
argmax : 20 

arg : LWIDTH 
arglabel : Line Thickness 
argtype : slider 
argvalue : 2 
argmin : 1 
argmax : 5 


item : Restriction sites 
itemmethod : ( cp $ ENZ in1 . tmp ; $ PRE_EDIT Restriction in1 . tmp in1 > out1 ; rm in1 . tmp ; cp out1 tmp ) ; 
itemhelp : Restriction . help 

arg : ENZ 
argtype : text 
arglabel : Enzyme file 
argtext : $ ARBHOME / GDEHELP / DATA_FILES / enzymes 

arg : PRE_EDIT 
argtype : chooser 
arglabel : Edit enzyme file first ? 
argchoice : Yes : $ { ARB_TEXTEDIT : - textedit } in1 . tmp ; 
argchoice : No : 

in : in1 
informat : flat 

out : out1 
outformat : colormask 

menu : SAI 

item : Positional Variability by Olson 's dnamlrates (max 5000 Species, 8000 seqlength) ...' itemmethod : ( / bin / rm - f outfile infile treefile ; ( echo C F M T U L ; echo C 35 ; echo M 10 ; echo T 2.0 ) | arb_convert_aln - GenBank in1 - phylip2 in1 . ph ; echo 1 >> in1 . ph ; arb_export_tree $ TREE >> in1 . ph ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; echo done ) & 
itemhelp : phylip . help 

arg : TREE 
argtype : tree 
arglabel : Base Tree 
argchoice : tree_main 

in : in1 
informat : genbank 
inmask : 
insave : 

menu : Incremental_Phylogeny 

item : FastDnaMl ( max 500 Species ) ... 
itemmethod : ( / bin / rm - f outfile infile treefile ; ( echo Y F Q G T R "`arb_export_rates $RATES`" ; echo G 0 0 ; echo T 2.0 ) | arb_convert_aln - GenBank in1 - phylip2 infile ; arb_export_tree $ TREE >> infile ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; rm in1 ) & 
itemhelp : phylip . help 

arg : RATES 
argtype : weights 
arglabel : Select a Weighting Mask 
argchoice : POS_VAR_BY_PARS 

arg : TREE 
argtype : tree 
arglabel : Base Tree 
argchoice : tree_main 

arg : DISPLAY_FUNC 
argtype : chooser 
arglabel : What to do with the tree : ? 
argchoice : ARB ( 'tree_fml_//' ) : arb_read_tree tree_fml_ $ $ treefile "PRG=FASTDNAML FILTER=$FILTER PKG=olsen/felsenstein RATE=$RATES" 
argchoice : Treetool : treetool treefile 


in : in1 
informat : genbank 
inmask : 
insave : 

menu : Phylogeny 

item : DeSoete Tree fit 
itemmethod : ( $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f in1 * ) & 
itemhelp : lsadt . help 

in : in1 
informat : genbank 
insave : 
inmask : 

arg : CORR 
arglabel : Dist . correction ? 
argtype : chooser 
argchoice : Olsen : - c = olsen 
argchoice : Jukes / Cantor : - c = jukes 
argchoice : None : - c = none 

arg : INIT 
arglabel : Parameter estimate 
argtype : choice_menu 
argchoice : uniformly distributed random numbers : 1 
argchoice : error - perturbed data : 2 
argchoice : original distance data from input matrix : 3 

arg : SEED 
argtype : slider 
arglabel : Random nr . seed 
argmin : 0 
argmax : 65535 
argvalue : 12345 

arg : DISPLAY_FUNC 
argtype : chooser 
arglabel : View tree using 
argchoice : ARB ( 'tree_desoete' ) : arb_read_tree tree_desoete_ $ $ in1 . out "PRG=DeSoete CORR=$CORR FILTER=$FILTER" 
argchoice : TextEdit : $ { ARB_TEXTEDIT : - textedit } in1 . out 
argchoice : Treetool : treetool in1 . out 

item : Phylip Distance Methods ( Original Phylip , Interactiv ) 
itemmethod : ( / bin / rm - f treefile infile outfile ; arb_convert_aln - GenBank in1 - phylip infile ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f in1 infile outfile treefile ) & 

arg : DISPLAY_FUNC 
argtype : chooser 
arglabel : What to do with the tree : ? 
argchoice : ARB ( 'tree_ph_xxx' ) : ( arb_read_tree tree_ph_ $ $ treefile "FILTER=$FILTER PKG=phylip" ) 
argchoice : TextEdit : $ { ARB_TEXTEDIT : - textedit } outfile 
argchoice : Treetool : treetool treefile 

arg : PROGRAM 
arglabel : Which method ? 
argtype : chooser 
argchoice : DNADIST : mv - f infile outfile 
argchoice : Fitch : $ { ARB_XCMD : - cmdtool } fitch 
argchoice : Kitsch : $ { ARB_XCMD : - cmdtool } kitsch 
argchoice : Neighbor : $ { ARB_XCMD : - cmdtool } neighbor 

arg : DPGM 
arglabel : Treat data as .. 
argtype : chooser 
argchoice : DNA : dnadist 
argchoice : RNA : dnadist 
argchoice : AA : protdist 


in : in1 
informat : genbank 
inmask : sdfgdfg 
insave : 

item : Phylip Distance Methods ( Simple GUI Based Interface ) 
itemmethod : ( / bin / rm - f treefile infile outfile ; arb_convert_aln - GenBank in1 - phylip infile ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f in1 infile outfile treefile ) & 
itemhelp : phylip . help 

arg : DISPLAY_FUNC 
argtype : chooser 
arglabel : What to do with the tree : ? 
argchoice : ARB ( 'tree_ph_xxx' ) : ( $ CORRECTION ; echo || $ DPGM ; arb_read_tree tree_ph_ $ $ treefile "PRG=$PROGRAM CORR=$TEXT FILTER=$FILTER PKG=phylip" ) 
argchoice : TextEdit : $ { ARB_TEXTEDIT : - textedit } outfile 
argchoice : Treetool : treetool treefile 

arg : PROGRAM 
arglabel : Which method ? 
argtype : chooser 
argchoice : Neighbor : neighbor 
argchoice : Fitch : fitch 
argchoice : Kitsch : kitsch 

arg : DPGM 
arglabel : Treat data as .. 
argtype : chooser 
argchoice : DNA / RNA : ( arb_echo $ DNAFLAGS | dnadist ) ; TEXT = $ DNATEXT 
argchoice : AA : ( arb_echo $ PROFLAGS | protdist ) ; TEXT = $ PROTEXT 

arg : CORRECTION 
arglabel : Correction DNA / PROT 
argtype : chooser 
argchoice : Kimura / Dayhoff : DNATEXT = kimura ; DNAFLAGS = "y" ; PROTEXT = dayhoff ; PROFLAGS = "y" 
argchoice : Jin_Nei / Kimura : DNATTEXT = jinnai ; DNAFLAGS = "d y" ; PROTEXT = kimura ; PROFLAGS = "p y" 
argchoice : Max . Lik . / ( George / Hunt / Barker ) : DNATEXT = maxlik ; DNAFLAGS = "d d y" ; PROTEXT = ghb ; PROFLAGS = "p p y" 
argchoice : JukesC . / Chemical : DNATEXT = jc ; DNAFLAGS = "d d d y" ; PROTEXT = chemical ; PROFLAGS = "p p a c y" 
argchoice : JukesC . / Hall : DNATEXT = jc ; DNAFLAGS = "d d d y" ; PROTEXT = hall ; PROFLAGS = "p p a h y" 
argchoice : USER / USER : DNATEXT = user ; DNAFLAGS = "" ; PROTEXT = user ; PROFLAGS = "" 



in : in1 
informat : genbank 
inmask : 
insave : 


item : Phylip Distance Matrix 
itemmethod : ( / bin / rm - f treefile infile outfile ; arb_convert_aln - GenBank in1 - phylip infile ; $ PREEDIT $ { ARB_XCMD : - cmdtool } $ DPGM ; $ { ARB_TEXTEDIT : - textedit } outfile ; / bin / rm - f in1 infile outfile ) & 
itemhelp : phylip . help 

arg : DPGM 
arglabel : Treat data as .. 
argtype : chooser 
argchoice : DNA / RNA : dnadist 
argchoice : AA : protdist 

arg : PREEDIT 
argtype : chooser 
arglabel : Edit input before running ? 
argchoice : No : 
argchoice : Yes : $ { ARB_TEXTEDIT : - textedit } infile ; 

in : in1 
informat : genbank 
inmask : 
insave : 

item : Phylip Parsimony / M . L . 

itemmethod : ( / bin / rm - f treefile infile outfile ; arb_convert_aln - GenBank in1 - phylip infile ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f in1 infile outfile treefile ) & 
itemhelp : phylip . help 

arg : DISPLAY_FUNC 
argtype : chooser 
arglabel : What to do with the tree : ? 
argchoice : ARB ( 'tree_phylip' ) : arb_read_tree $ CONSFLAG tree_ph_ $ $ treefile "PRG=$PROGRAM FILTER=$FILTER PKG=phylip" 
argchoice : TextEdit : arb_textedit outfile 
argchoice : Treetool : treetool treefile 

arg : BOOTSTRP 
arglabel : How many bootstraps ? 
argtype : chooser 
argchoice : Do not bootstrap : CONSFLAG = ; arb_echo y | $ PROGRAM 
argchoice : 10 : arb_echo 1 r 10 y | seqboot ; / bin / mv - f outfile infile ; arb_echo m 10 y | $ PROGRAM ; / bin / mv - f treefile infile ; arb_echo y | consense ; arb_textedit outfile 
argchoice : 100 : arb_echo 1 r 100 y | seqboot ; / bin / mv - f outfile infile ; arb_echo m 100 y | $ PROGRAM ; / bin / mv - f treefile infile ; arb_echo y | consense ; arb_textedit outfile 
argchoice : 1000 : arb_echo 1 r 1000 y | seqboot ; / bin / mv - f outfile infile ; arb_echo m 1000 y | $ PROGRAM ; / bin / mv - f treefile infile ; arb_echo y | consense ; arb_textedit outfile 

arg : PROGRAM 
argtype : chooser 
arglabel : Which program to run ? 
argchoice : DNAPARS : dnapars 


argchoice : DNACOMP : dnacomp 

argchoice : DNAINVAR : dnainvar 
argchoice : PROTPARS : protpars 




in : in1 
informat : genbank 
inmask : 
insave : 

item : DNA_ML ( FastDnaMl ) ( max 150 Species ) 
itemmethod : ( / bin / rm - f outfile infile treefile ; echo Y F "`arb_export_rates $RATES`" | arb_convert_aln - GenBank in1 - phylip2 infile ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; rm in1 ) & 
itemhelp : phylip . help 

arg : RATES 
argtype : weights 
arglabel : Select a Weighting Mask 
argchoice : POS_VAR_BY_PARS 

arg : DISPLAY_FUNC 
argtype : chooser 
arglabel : What to do with the tree : ? 
argchoice : ARB ( 'tree_fml_//' ) : arb_read_tree tree_fml_ $ $ treefile "PRG=FASTDNAML FILTER=$FILTER PKG=olsen/felsenstein RATE=$RATES" 
argchoice : Treetool : treetool treefile 

in : in1 
informat : genbank 
inmask : 
insave : 

item : Protein_ML ( molphy max 40 species ) 

itemmethod : ( / bin / rm - f treefile infile outfile ; arb_convert_aln - GenBank in1 - phylip infile ; cat infile | tr '?' '-' > in1 . in ; $ { ARB_XCMD : - cmdtool } sh - c 'LD_LIBRARY_PATH="${ARB_LIBRARY_PATH:-/usr/arb}";export LD_LIBRARY_PATH;command;arb_wait' ; / bin / rm - f in1 infile outfile treefile ) & 
itemhelp : phylip . help 

arg : DISPLAY_FUNC 
argtype : chooser 
arglabel : What to do with the tree : ? 
argchoice : ARB ( 'tree_phylip' ) : arb_read_tree $ CONSFLAG tree_ph_ $ $ protml . tre "PRG=protml PARMETERS=$MODEL$SEARCH FILTER=$FILTER PKG=molphy" 
argchoice : TextEdit : arb_textedit outfile 
argchoice : Treetool : treetool treefile 

arg : BOOTSTRP 
arglabel : How many bootstraps ? 
argtype : chooser 
argchoice : Do not bootstrap : CONSFLAG = ; protml $ MODEL $ SEARCH - v - I in1 . in 
argchoice : 10 : arb_echo 1 r 10 y | seqboot ; / bin / mv - f outfile infile ; arb_echo m 10 y | $ PROGRAM ; / bin / mv - f treefile infile ; arb_echo y | consense ; arb_textedit outfile 
argchoice : 100 : arb_echo 1 r 100 y | seqboot ; / bin / mv - f outfile infile ; arb_echo m 100 y | $ PROGRAM ; / bin / mv - f treefile infile ; arb_echo y | consense ; arb_textedit outfile 
argchoice : 1000 : arb_echo 1 r 1000 y | seqboot ; / bin / mv - f outfile infile ; arb_echo m 1000 y | $ PROGRAM ; / bin / mv - f treefile infile ; arb_echo y | consense ; arb_textedit outfile 

arg : MODEL 
argtype : chooser 
arglabel : Which model to use ? 
argchoice : JTT : - j 
argchoice : Dayhoff : - d 
argchoice : mtREV24 : - m 
argchoice : Poisson : - p 
argchoice : Jones Tayler & Thornton : - jf 
argchoice : Dayhoff - F : - df 
argchoice : Adachi & Hasegawa ( 1995 ) : - mf 

arg : SEARCH 
argtype : chooser 
arglabel : Search Strategie 
argchoice : star decomposition : - s 
argchoice : exhaustive : - e 
argchoice : quickadd : - q 

in : in1 
informat : genbank 
inmask : 
insave : 
