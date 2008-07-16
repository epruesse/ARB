/* ring.h --- macros for manipulating doubly-linked rings
   Jim Blandy <jimb@gnu.ai.mit.edu> --- October 1994 */

/* Rings are circular doubly-linked lists.  Usually you have one
   pseudo-node in the chain that separates the first true node and the
   last true node, called the "head"; inserting nodes before or after
   the head thus effectively inserts them at the end and beginning of
   the list.

   The macros all take an argument LINK, which is the name prefix of
   the links the macros should manipulate.  If you pass the identifier
   foo as the macro's LINK argument, the macro will manipulate the
   foo_next and foo_prev members of its arguments.  */


/* Make NODE into a singleton ring.  */
#define RING_SINGLETON(node, link)              \
    ((node)->link ## _prev = (node),            \
     (node)->link ## _next = (node))

/* Return non-zero iff NODE forms a singleton ring.  */
#define RING_IS_SINGLETON(node, link)           \
    ((node)->link ## _prev == (node))

/* Insert NEW after NODE.  */
#define RING_INSERT_AFTER(new, node, link)              \
    ((new)->link ## _prev = (node),                     \
     (new)->link ## _next = (node)->link ## _next,      \
     (node)->link ## _next->link ## _prev = (new),      \
     (node)->link ## _next = (new))

/* Insert NEW before NODE.  */
#define RING_INSERT_BEFORE(new, node, link)             \
    ((new)->link ## _next = (node),                     \
     (new)->link ## _prev = (node)->link ## _prev,      \
     (node)->link ## _prev->link ## _next = (new),      \
     (node)->link ## _prev = (new))

/* Remove NODE from whatever ring it's in.  This affects NODE's
   immediate neighbors; NODE itself is left unchanged.  */
#define RING_REMOVE(node, link)                                         \
    ((node)->link ## _prev->link ## _next = (node)->link ## _next,      \
     (node)->link ## _next->link ## _prev = (node)->link ## _prev)
