#include <cstdio>
#include "PRD_design.hxx"

using namespace std;

const char *test_seq = "..........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................AUUCUGGUU-----GA--U-CC-U-G-CCAG-AG-GU-U-AC-U-GC--U-AU-C--G-GU-GUUC--GC---C-U--AAGCCA-U-GC-G-AGU-CAU-A-UGU---------A--------------------------------------------------------------------------------------------------GCA-A--------------------------------------------------------------------------------------------------------------------U--A--CA-U-GG-C-GU-A--C-------------UG-----------C-UCAGU-A---AC-A-C-G-U-G-GA---UAA--C-CU-G--C-C-CUU--GG-G--------------------------------------------------------------------A-CC----GGC-AU-AA-CCC-------------------------C-G-G-----------------------GA-A-A---CUG-GGG-AUAA-UU---CC-G--G-AU-A---------------------------------A--C-G-C--A--U-------------------AU-UUGC-U---------------------------------------------------------------------------------------------------------------------------------G-GA-A--------------------------------------------------------------------------------------------------------------------------------------U-G-CU-U-U---------------A--U-G-C-G-U-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------CAAA-AG-G-A------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------UUC-G-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------U--C---U-G--------------C----C-C---A-AG-G---AU---G-G-----G-UCU-GCG----G-CCU--A------UC-----A--G-GU-A----G---UAGU-G-G-GU-G-U----AAU-GU-A-C-C-UACU-----A-GC-C-A--G-CG-A------------CGG-G-U--------------AC-GG-G-U-UGU-G-AG-------------A--GC-AA--G-AG-C-CC-GGA-G-A-UGGAU--U-C-UG--A-GA-C-AU-G-A-AUCCAG-GCCC-UAC------G--G-G-G-C-GC-A-GC-A-G-GC---GC-G-A-AAA-CUUUA-C-AA-U-GC--GG-GA-A----A-C-CG-U-GA-UA-AG-GGG---------A-CACC-G-AG-U---------G-C-C--A--G----------C-A-U--------------------CA-U---------------------A-U-G--------C--U-GGC--------------------UG-UC-C-G--G-AUG----U------------------------------G--U--AA-A---A----U----------------------------------A-C-AU-C-CG-U-------U-AG-----------CAAGG-GCCGGG-C--AA---G-AC-CGGU-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------GCCA--G-C---C--G-CCG---C-GG--UA-AC--AC---CG-GC-GGC-CCG-A-G-UG-GUGA-U-CGU-GA-UU-A--U-U--GGGU-CUA-----AA-GGGU-CC--G-UA-G-C-C-G---------------G--U-UU-G-G-U-CA----G-U-C-C---U-CCG-GG-A-AA-UC--UG-AUA-G--------------------------------------------------------------------CU-C-AA-------------------------------------------------------------------------CU-A-U-UA-GG-CU---U-U-C-G-G-G--------G--GA-U-A-C-U-G-CCA--G-A-C--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------U-U-G-G-A-A-C-----C-GG--GA-G-A------------G-GU-A-AG-A----GG--U--ACU-ACA-G-GG--GU-A-G-GA-GUG--AAA-UC-UUG-UAAU-C-CC-U-GUG------GG-A-CC-A-CC-UG--U--G--GC-GAA-G--G-C---G--------U--C-U-UACCA---------G-AA-CG----------------------------------------------------------------------------------------------------------------------------------------------------------------------------GG--------U-U-C--GA--CG-----GU-GA-GG--G-A-CGA--AA-G-C--------------U-GGGG-GCA-C-G-AACC--GG-AUUA-G-AUA-C-----CC-G-G-GUA-G-U---------------C-CC--A-G-CCG-U-AAA--C-GAUG-CU--CG-CU---------A-GG--U--G-U-CA-G-GC-A--U--GG------------------------------------------------------------------------------------CGC-GA-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------CC---G-U-G-UC--U-G-G-U-GC-C------GC--A----GG-GAA--GC-C-G-U--G--AA-GC----G----A-GCC-ACC-U-G-GG-AAG-UA---CGG-----C-C--G-C-A-A-GGC-U--GAA-ACUU-AAA---------GGAA-UU-G-GCGGG-G-G-AGCA----CAA--C-A-A-CGG-GU-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------G--G--AG-CC-U--GC-GGU-UU-AAUU-G-G-ACU-CAAC-G-CC-G-GA-C-A-AC-UC-A-CC-GGGGG------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------CGACAGC--AAUAUGUAGGCCAAGCUGAAGACUUUGCCU-GA-AUCGCUGAG-A-G-G-A-GGUG-CA-UGG-CC--GUC-GCC-A-GU-UC---G-UA-CU-G--U---GA-AG-CAU-C-CU-G-UU-AA-GU-CAGGC--------AA--------C-GAG-CGA-G-----ACC-C-G-UG--CC--C-ACUG--U-U-A-C-C---AG-C-A--U--GU-CC---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------UCCG--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------GG---A---CG---A----U-G------------G----G---U-A--CU---------------C-U-G-U-G-GG-G--AC-C-G-CCG--A-U------------------------------------G-U---UAA-------------------------------------A-U-C-G--G-A-GG-A--AGG-U--GCGG-G-CCUC-GGU--A--GGU---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------C---AGU-A-U-G-C-C-C-CGA----AU-C--UC-C-C-GG-GC-UA-CAC-GCGGG-C--UA--CAAUG---G-AUGG-G-A--C-AAU-GG-GU--------------------------------------------------------------------------------------------------C-C-C-U-C--C-CCUG-A--A---------------------------------------A-AG-G-G-----------C--U-G-GU---A----------A--UCU-C------A-C-AAACC-CA-U-U-C-G-UAG-UUC--------GGA-U-CGAGG-GC--U-GUAA-CU-C-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------G-C----CCUC-G-U-G-AA-G-CU-GGAAU-CC-G-UA--G-UA-AU-C-G-C----GUU-UC-A-A--U------A-U--AGC-GC-G-GU-G-AAU-ACGU-C-CCUGCUCCU-UGCA----CACACCG-CCC-GUC-----A---AA--CCA-CC-CG-A--G---UGA-G-GU-AU-GGG--U-GA-------G--G-GCA-C--G-G-A-C----------------------------------------------------------------------------------------------------------------------U-UC----------------------------------------------------------------------------------------------------------------------------------------------------------------GUGC--C---GU-GUU--CG--AAC-C----U-GUG-CU-UUG------------------------CA--AGG-GUGG-UU-AAG-UCGUAACAA-GG-UAG-CCGU-AGGGGAA-UCUG-CGGC-UGGAUCACCUCCU.............................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................";

int main(void) {
printf("Hello world\n");


}


