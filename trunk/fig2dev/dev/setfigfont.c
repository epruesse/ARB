/* Add a (La)TeX macro, if TeX fonts are used. */
/* This macro is called with three arguments:
 *    #1   fontsize (without `pt')
 *    #2   baselineskip (without `pt')
 *    #3   font  (without escape character)
 */

#include <stdio.h>

define_setfigfont(tfp)
     FILE *tfp;
{
	fprintf(tfp, "%%\n\
\\begingroup\\makeatletter\n\
%% extract first six characters in \\fmtname\n\
\\def\\x#1#2#3#4#5#6#7\\relax{\\def\\x{#1#2#3#4#5#6}}%%\n\
\\expandafter\\x\\fmtname xxxxxx\\relax \\def\\y{splain}%%\n\
\\ifx\\x\\y   %% LaTeX or SliTeX?\n\
\\gdef\\SetFigFont#1#2#3{%%\n\
  \\ifnum #1<17\\tiny\\else \\ifnum #1<20\\small\\else\n\
  \\ifnum #1<24\\normalsize\\else \\ifnum #1<29\\large\\else\n\
  \\ifnum #1<34\\Large\\else \\ifnum #1<41\\LARGE\\else\n\
     \\huge\\fi\\fi\\fi\\fi\\fi\\fi\n\
  \\csname #3\\endcsname}%%\n\
\\else\n\
\\gdef\\SetFigFont#1#2#3{\\begingroup\n\
  \\count@#1\\relax \\ifnum 25<\\count@\\count@25\\fi\n\
  \\def\\x{\\endgroup\\@setsize\\SetFigFont{#2pt}}%%\n\
  \\expandafter\\x\n\
    \\csname \\romannumeral\\the\\count@ pt\\expandafter\\endcsname\n\
    \\csname @\\romannumeral\\the\\count@ pt\\endcsname\n\
  \\csname #3\\endcsname}%%\n\
\\fi\n\
\\endgroup\n");
}
