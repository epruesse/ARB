/* gdbmgenes.h --- The format in which we store genetic data in GDBM files
   Jim Blandy <jimb@gnu.ai.mit.edu> --- June 1994  */

/* We put one of these characters at the front of the key to say what
   sort of entry it is associated with.  */

/* Key is associated with a struct tabl_entry.  */
#define GDBM_TABL_ENTRY 't'

/* Key is associated with an index, whose format is determined by the rest
   of the key name.  */
#define GDBM_INDEX_ENTRY 'i'


struct tabl_entry {

  /* The length of the annotation in bytes.
     Sure, this is byte-order dependent, but GDBM is too, at version
     1.7.1.  */
  size_t ann_len, seq_len;

  /* The annotation and sequence text.
     data[0..ann_len-1] is the annotation data, in whatever format you
     want.  data[ann_len..ann_len + seq_len - 1] is the sequence data,
     in tabl format.  */
  char data[1];
};
