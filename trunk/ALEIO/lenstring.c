/* lenstring.c --- implementation of lenstring functions
   Jim Blandy <jimb@gnu.ai.mit.edu> --- September 1994 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lenstring.h"
#include "xmalloc.h"

/* The name of the program we're running, for use in error messages.
   It isn't terribly modular of us to use this, but hey, every program
   should have a progname variable.  */
extern char *progname;

/* Most entries are smaller than this, so this value avoids a few
   calls to realloc.  */
#define INITIAL_BUF_LEN 4096


/* Write STRING to STREAM.
   Return value same as for stdio's fwrite.  */
size_t
write_lenstring (lenstring *string,
		 FILE *stream)
{
  return fwrite (string->text, sizeof (*string->text), string->len, stream);
}


/* Write STRING to STREAM, left-justified in a field of FIELD spaces.
   If STRING is longer than FIELD, truncate it.  */

void
display_clipped_lenstring (lenstring *string, int field, FILE *stream)
{
  if (string->len >= field)
    fwrite (string->text, sizeof (*string->text), field, stream);
  else
    {
      write_lenstring (string, stream);

      {
	int pad = field - string->len;

	while (pad-- > 0)
	  putc (' ', stream);
      }
    }
}


/* Read text from SOURCE until we find DELIMITER, or hit EOF.
   Set *STRING to the text we read; the delimiting string or EOF is
   not included in STRING.

   If the text was terminated by EOF, and is empty (i.e. its length is
   zero), set STRING->text to zero and return EOF.  In this case, the
   caller should not free STRING->text.

   Otherwise, the text is stored in memory obtained via malloc, and
   should be freed by the caller.

   If the string was terminated by DELIMITER, return 0.
   If the string was non-empty and terminated by EOF, return 1.
   If an error occurred reading the string, print an error message
   and exit.  */
int read_delimited_lenstring (lenstring *string, const char *delimiter, FILE *stream)
{
  size_t delimiter_len = strlen (delimiter);
  char delimiter_last_char;

  size_t buf_len = INITIAL_BUF_LEN;
  char *buf = (char *) xmalloc (buf_len);

  size_t i = 0;
  int c;

  if (delimiter_len == 0)
    {
      /* If there are other reasonable ways to handle this case,
         perhaps we should just abort here.  */
      string->len = 0;
      string->text = buf;
      return 0;
    }

  delimiter_last_char = delimiter[delimiter_len - 1];

  while ((c = getc (stream)) != EOF)
    {
      /* Do we need to enlarge the buffer?  */
      if (i >= buf_len-1)
        {
          buf_len *= 2;
          buf = (char *) xrealloc (buf, buf_len);
        }

      buf[i++] = c;

      /* Have we read the delimiter?  We check to see if we just
         stored delim_last_char; this is a quick, false-positive test.
         Then we check for the whole string; this is a slow but
         correct test.  */
      if (c == delimiter_last_char
          && i >= delimiter_len
          && ! memcmp (&buf[i - delimiter_len], delimiter, delimiter_len))
	break;
    }

  if (ferror (stream))
    {
      perror (progname);
      exit (2);
    }

  if (c == EOF)
    {
      /* Special case, as documented.  */
      if (i == 0)
	{
	  free (buf);
	  string->text = 0;
	  string->len = 0;
	  return EOF;
	}
      else
	{
	  string->text = buf;
	  string->len = i;
	  return 1;
	}
    }
  else
    {
      string->text = buf;
      string->len = i - delimiter_len;
      return 0;
    }
}


/* Search STRING for an occurrence of SUBSTRING starting not before
   START, and return its starting position, or -1 if SUBSTRING is not
   a substring of STRING.  */
size_t
search_lenstring (lenstring *string,
		  const char *substring,
		  size_t start)
{
  /* Based on the memmem implementation in the GNU C library.
     This implementation's copyright is:

     Copyright (C) 1991, 1992 Free Software Foundation, Inc.
     This file is part of the GNU C Library.

     The GNU C Library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public License as
     published by the Free Software Foundation; either version 2 of the
     License, or (at your option) any later version.

     The GNU C Library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.

     You should have received a copy of the GNU Library General Public
     License along with the GNU C Library; see the file COPYING.LIB.  If
     not, write to the Free Software Foundation, Inc., 675 Mass Ave,
     Cambridge, MA 02139, USA.  */

  const void *haystack = string->text + start;
  const size_t haystack_len = string->len - start;
  const void *needle = substring;
  const size_t needle_len = strlen (substring);

  const char *begin;
  const char *last_possible
    = (const char *) haystack + haystack_len - needle_len;

  if (needle_len == 0)
    return start;

  for (begin = (const char *) haystack;
       begin <= last_possible;
       ++begin)
    if (!memcmp ((void *) begin, needle, needle_len))
      return (begin - (const char *) haystack) + start;

  return -1;
}


/* Strip newlines from IN, leaving the result in OUT.
   OUT points to the same text as IN.  */
void
strip_newlines (lenstring *out, lenstring *in)
{
  char *p = in->text;
  char *p_end = in->text + in->len;

  while (p < p_end && *p == '\n')
    p++;

  out->text = p;
  out->len  = p_end - p;
}


/* Append STR2 to the end of STR1.  Assume STR1 has sufficient space
   allocated.  */
void
append_lenstring (lenstring *str1, lenstring *str2)
{
  memcpy (str1->text + str1->len, str2->text, str2->len);
  str1->len += str2->len;
}
