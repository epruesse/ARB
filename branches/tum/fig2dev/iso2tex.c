/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation. The authors make no representations about the suitability 
 * of this software for any purpose.  It is provided "as is" without express 
 * or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* map ISO-Font Symbols to appropriate sequences in TeX */
/* Herbert Bauer 22.11.1991 */

/* B.Raichle 12.10.92, changed some of the definitions */


/* B.Raichle 12.10.92, changed some of the definitions */


char *ISOtoTeX[] =   /* starts at octal 240 */
{
  "{}",
  "{!`}",	/* inverse ! */
  "{}",		/* cent sign (?) */
  "\\pounds{}",
  "{}",		/* circle with x mark */
  "{}",		/* Yen */
  "{}",		/* some sort of space - doen't work under mwm */
  "\\S{}",	/* paragraph sign */
  "\\\"{}",		/* diaresis points */
  "\\copyright{}",
  "\\b{a}",
  "\\mbox{$\\ll$}",		/* << */
  "{--}", 	/* longer dash - doesn't work with mwm */
  "{-}",		/* short dash */
  "{}",		/* trademark */
  "{}",		/* overscore */
/* 0xb0 */
  "{\\lower.2ex\\hbox{\\char\\'27}}",		/* degree */
  "\\mbox{$\\pm$}",	/* plus minus - math mode */
  "\\mbox{$\\mathsurround 0pt{}^2$}",		/* squared  - math mode */
  "\\mbox{$\\mathsurround 0pt{}^3$}",		/* cubed  - math mode */
  "\\'{}",		/* accent egue */
  "\\mbox{$\\mu$}",	/* greek letter mu - math mode */
  "\\P{}",	/* paragraph */
  "\\mbox{$\\cdot$}",	/* centered dot  - math mode */
  "",
  "\\mbox{$\\mathsurround 0pt{}^1$}",		/* superscript 1  - math mode */
  "\\b{o}",
  "\\mbox{$\\gg$}",		/* >> */
  "\\mbox{$1\\over 4$}",	/* 1/4 - math mode */
  "\\mbox{$1\\over 2$}",	/* 1/2 - math mode */
  "\\mbox{$3\\over 4$}",	/* 3/4 - math mode */
  "{?`}",		/* inverse ? */
/* 0xc0 */
  "\\`A",
  "\\'A",
  "\\^A",
  "\\~A",
  "\\\"A",
  "\\AA{}",
  "\\AE{}",
  "\\c C",
  "\\`E",
  "\\'E",
  "\\^E",
  "\\\"E",
  "\\`I",
  "\\'I",
  "\\^I",
  "\\\"I",
/* 0xd0 */
  "{\\rlap{\\raise.3ex\\hbox{--}}D}", /* Eth */
  "\\~N",
  "\\`O",
  "\\'O",
  "\\^O",
  "\\~O",
  "\\\"O",
  "\\mbox{$\\times$}",	/* math mode */
  "\\O{}",
  "\\`U",
  "\\'U",
  "\\^U",
  "\\\"U",
  "\\'Y",
  "{}",		/* letter P wide-spaced */
  "\\ss{}",
/* 0xe0 */
  "\\`a",
  "\\'a",
  "\\^a",
  "\\~a",
  "\\\"a",
  "\\aa{}",
  "\\ae{}",
  "\\c c",
  "\\`e",
  "\\'e",
  "\\^e",
  "\\\"e",
  "\\`\\i{}",
  "\\'\\i{}",
  "\\^\\i{}",
  "\\\"\\i{}",
/* 0xf0 */
  "\\mbox{$\\partial$}",	/* correct?  - math mode */
  "\\~n",
  "\\`o",
  "\\'o",
  "\\^o",
  "\\~o",
  "\\\"o",
  "\\mbox{$\\div$}",	/* math mode */
  "\\o{}",
  "\\`u",
  "\\'u",
  "\\^u",
  "\\\"u",
  "\\'y",
  "{}",		/* letter p wide-spaced */
  "\\\"y"
};

