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

static char		*texfontnames[] = {
			"rm", "rm",		/* default */
			"rm",			/* roman */
			"bf",			/* bold */
			"it",			/* italic */
			"sf", 			/* sans serif */
			"tt"			/* typewriter */
		};

/* The selection of font names may be site dependent.
 * Not all fonts are preloaded at all sizes.
 */

static char		*texfontsizes[] = {
 			"Elv", "elv",		/* default */
 			"Fiv", "Fiv", "Fiv", "Fiv", 	/* small fonts */
 			"Fiv",			/* five point font */
 			"Six", "Sev", "Egt",	/* etc */
 			"Nin", "Ten", "Elv",
 			"Twl", "Twl", "Frtn",	
 			"Frtn", "Frtn", "Svtn",
 			"Svtn", "Svtn", "Twty",
 			"Twty", "Twty", "Twty", "Twty", "Twfv",
                        "Twfv", "Twfv", "twfv", "Twentynine",
                        "Twentynine", "Twentynine", "Twentynine", "Twentynine",
                        "Thirtyfour", "Thirtyfour", "Thirtyfour", "Thirtyfour",
                        "Thirtyfour", "Thirtyfour", "Thirtyfour", "Fortyone",
                        "Fortyone", "Fortyone"
  			};

#define MAXFONTSIZE 42

#define TEXFONT(F)	(texfontnames[((F) <= MAX_FONT) ? (F)+1 : MAX_FONT])
#define TEXFONTSIZE(S)	(texfontsizes[((S) <= MAXFONTSIZE) ? round(S)+1\
				      				: MAXFONTSIZE])
#define TEXFONTMAG(T)	TEXFONTSIZE(T->size*(rigid_text(T) ? 1.0 : mag))

