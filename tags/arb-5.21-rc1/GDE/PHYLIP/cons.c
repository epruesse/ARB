#include "phylip.h"
#include "cons.h"

int tree_pairing;

Char outfilename[FNMLNGTH], intreename[FNMLNGTH], intree2name[FNMLNGTH], outtreename[FNMLNGTH];
node *root;

long numopts, outgrno, col, setsz;
long maxgrp;               /* max. no. of groups in all trees found  */

boolean trout, firsttree, noroot, outgropt, didreroot, prntsets,
               progress, treeprint, goteof, strict, mr=false, mre=false,
               ml=false; /* initialized all false for Treedist */
pointarray nodep;
pointarray treenode;
group_type **grouping, **grping2, **group2;/* to store groups found  */
long **order, **order2, lasti;
group_type *fullset;
node *grbg;
long tipy;

double **timesseen, **tmseen2, **times2 ;
double trweight, ntrees, mlfrac;

/* prototypes */
void censor(void);
boolean compatible(long, long);
void elimboth(long);
void enternohash(group_type*, long*);
void enterpartition (group_type*, long*);


void initconsnode(node **p, node **grbg, node *q, long len, long nodei,
                        long *ntips, long *parens, initops whichinit,
                        pointarray treenode, pointarray nodep, Char *str,
                        Char *ch, FILE *intree)
{
  /* initializes a node */
  long i;
  char c;
  boolean minusread;
  double valyew, divisor, fracchange;

  switch (whichinit) {
  case bottom:
    gnu(grbg, p);
    (*p)->index = nodei;
    (*p)->tip = false;
    for (i=0; i<MAXNCH; i++)
      (*p)->nayme[i] = '\0';
    nodep[(*p)->index - 1] = (*p);
    break;
  case nonbottom:
    gnu(grbg, p);
    (*p)->index = nodei;
    break;
  case tip:
    (*ntips)++;
    gnu(grbg, p);
    nodep[(*ntips) - 1] = *p;
    setupnode(*p, *ntips);
    (*p)->tip = true;
    strncpy ((*p)->nayme, str, MAXNCH);
    if (firsttree && prntsets) {
      fprintf(outfile, "  %ld. ", *ntips);
      for (i = 0; i < len; i++)
        putc(str[i], outfile);
      putc('\n', outfile);
      if ((*ntips > 0) && (((*ntips) % 10) == 0))
        putc('\n', outfile);
    }
    break;
  case length:
    processlength(&valyew, &divisor, ch, &minusread, intree, parens);
    fracchange = 1.0;
    (*p)->v = valyew / divisor / fracchange;
    break;
  case treewt:
    if (!eoln(intree)) {
      fscanf(intree, "%lf", &trweight);
      getch(ch, parens, intree);
      if (*ch != ']') {
        printf("\n\nERROR: Missing right square bracket\n\n");
        exxit(-1);
      } else {
        getch(ch, parens, intree);
        if (*ch != ';') {
          printf("\n\nERROR: Missing semicolon after square brackets\n\n");
          exxit(-1);
        }
      }
    }
    break;
  case unittrwt:
    /* This comes not only when seting trweight but also at the end of
     * any tree. The following code saves the current position in a 
     * file and reads to a new line. If there is a new line then we're 
     * at the end of tree, otherwise warn the user. This function should
     * really leave the file alone, so once we're done with 'intree' 
     * we seek the position back so that it doesn't look like we did 
     * anything */
    trweight = 1.0 ;
    i = ftell (intree);
    c = ' ';
    while (c == ' ')  {
        if (eoff(intree)) {
                fseek(intree,i,SEEK_SET);
                return;
        }
        c = gettc(intree);
    }
    fseek(intree,i,SEEK_SET);
    if ( c != '\n')
      printf("WARNING: Tree weight set to 1.0\n");
    break;
  default:                /* cases hslength, iter, hsnolength      */ 
    break;                /* should there be an error message here?*/
  }
} /* initconsnode */


void censor(void)
{
  /* delete groups that are too rare to be in the consensus tree */
  long i;

  i = 1;
  do {
    if (timesseen[i-1])
      if (!(mre || (mr && (2*(*timesseen[i-1]) > ntrees))
                || (ml && ((*timesseen[i-1]) > mlfrac*ntrees))
                || (strict && ((*timesseen[i-1]) == ntrees)))) {
        free(grouping[i - 1]);
        free(timesseen[i - 1]);
        grouping[i - 1] = NULL;
        timesseen[i - 1] = NULL;
    }
    i++;
  } while (i < maxgrp);
} /* censor */


void compress(long *n)
{
  /* push all the nonempty subsets to the front end of their array */
  long i, j;

  i = 1;
  j = 1;
  do {
    while (grouping[i - 1] != NULL)
      i++;
    if (j <= i)
      j = i + 1;
    while ((grouping[j - 1] == NULL) && (j < maxgrp))
      j++;
    if (j < maxgrp) {
      grouping[i - 1] = (group_type *)Malloc(setsz * sizeof(group_type));
      timesseen[i - 1] = (double *)Malloc(sizeof(double));
      memcpy(grouping[i - 1], grouping[j - 1], setsz * sizeof(group_type));
      *timesseen[i - 1] = *timesseen[j - 1];
      free(grouping[j - 1]);
      free(timesseen[j - 1]);
      grouping[j - 1] = NULL;
      timesseen[j - 1] = NULL;
    }
  } while (j != maxgrp);
  (*n) = i - 1;
}  /* compress */


void sort(long n)
{
  /* Shell sort keeping grouping, timesseen in same order */
  long gap, i, j;
  group_type *stemp;
  double rtemp;

  gap = n / 2;
  stemp = (group_type *)Malloc(setsz * sizeof(group_type));
  while (gap > 0) {
    for (i = gap + 1; i <= n; i++) {
      j = i - gap;
      while (j > 0) {
        if (*timesseen[j - 1] < *timesseen[j + gap - 1]) {
          memcpy(stemp, grouping[j - 1], setsz * sizeof(group_type));
          memcpy(grouping[j - 1], grouping[j + gap - 1], setsz * sizeof(group_type));
          memcpy(grouping[j + gap - 1], stemp, setsz * sizeof(group_type));
          rtemp = *timesseen[j - 1];
          *timesseen[j - 1] = *timesseen[j + gap - 1];
          *timesseen[j + gap - 1] = rtemp;
        }
        j -= gap;
      }
    }
    gap /= 2;
  }
  free(stemp);
}  /* sort */


boolean compatible(long i, long j)
{
  /* are groups i and j compatible? */
  boolean comp;
  long k;

  comp = true;
  for (k = 0; k < setsz; k++)
    if ((grouping[i][k] & grouping[j][k]) != 0)
      comp = false;
  if (!comp) {
    comp = true;
    for (k = 0; k < setsz; k++)
      if ((grouping[i][k] & ~grouping[j][k]) != 0)
        comp = false;
    if (!comp) {
      comp = true;
      for (k = 0; k < setsz; k++)
        if ((grouping[j][k] & ~grouping[i][k]) != 0)
          comp = false;
      if (!comp) {
        comp = noroot;
        if (comp) {
          for (k = 0; k < setsz; k++)
            if ((fullset[k] & ~grouping[i][k] & ~grouping[j][k]) != 0)
              comp = false;
        }
      }
    }
  }
  return comp;
} /* compatible */


void eliminate(long *n, long *n2)
{
  /* eliminate groups incompatible with preceding ones */
  long i, j, k;
  boolean comp;

  for (i = 2; i <= (*n); i++) {
    comp = true;
    for (j = 0; comp && (j <= i - 2); j++) {
      if ((timesseen[j] != NULL) && *timesseen[j] > 0) {
        comp = compatible(i-1,j);
        if (!comp) {
          (*n2)++;
          times2[(*n2) - 1] = (double *)Malloc(sizeof(double));
          group2[(*n2) - 1] = (group_type *)Malloc(setsz * sizeof(group_type));
          *times2[(*n2) - 1] = *timesseen[i - 1];
          memcpy(group2[(*n2) - 1], grouping[i - 1], setsz * sizeof(group_type));
          *timesseen[i - 1] = 0.0;
          for (k = 0; k < setsz; k++)
            grouping[i - 1][k] = 0;
        }
      }
    }
    if (*timesseen[i - 1] == 0.0) {
      free(grouping[i - 1]);
      free(timesseen[i -  1]);
      timesseen[i - 1] = NULL;
      grouping[i - 1] = NULL;
    }
  }
}  /* eliminate */


void printset(long n)
{
  /* print out the n sets of species */
  long i, j, k, size;
  boolean noneprinted;

  fprintf(outfile, "\nSet (species in order)   ");
  for (i = 1; i <= spp - 25; i++)
    putc(' ', outfile);
  fprintf(outfile, "  How many times out of %7.2f\n\n", ntrees);
  noneprinted = true;
  for (i = 0; i < n; i++) {
    if ((timesseen[i] != NULL) && (*timesseen[i] > 0)) {
      size = 0;
      k = 0;
      for (j = 1; j <= spp; j++) {
        if (j == ((k+1)*SETBITS+1)) k++;
        if (((1L << (j - 1 - k*SETBITS)) & grouping[i][k]) != 0)
          size++;
      }
      if (size != 1 && !(noroot && size >= (spp-1))) {
        noneprinted = false;
        k = 0;
        for (j = 1; j <= spp; j++) {
          if (j == ((k+1)*SETBITS+1)) k++;
          if (((1L << (j - 1 - k*SETBITS)) & grouping[i][k]) != 0)
            putc('*', outfile);
          else
            putc('.', outfile);
          if (j % 10 == 0)
            putc(' ', outfile);
        }
        for (j = 1; j <= 23 - spp; j++)
          putc(' ', outfile);
        fprintf(outfile, "    %5.2f\n", *timesseen[i]);
      }
    }
  }
  if (noneprinted)
    fprintf(outfile, " NONE\n");
}  /* printset */


void bigsubset(group_type *st, long n)
{
  /* Find a maximal subset of st among the n groupings,
     to be the set at the base of the tree.  */
  long i, j;
  group_type *su;
  boolean max, same;

  su = (group_type *)Malloc(setsz * sizeof(group_type));
  for (i = 0; i < setsz; i++)
    su[i] = 0;
  for (i = 0; i < n; i++) {
    max = true;
    for (j = 0; j < setsz; j++)
      if ((grouping[i][j] & ~st[j]) != 0)
        max = false;
    if (max) {
      same = true;
      for (j = 0; j < setsz; j++)
        if (grouping[i][j] != st[j])
          same = false;
      max = !same;
    }
    if (max) {
      for (j = 0; j < setsz; j ++)
        if ((su[j] & ~grouping[i][j]) != 0)
          max = false;
      if (max) {
        same = true;
        for (j = 0; j < setsz; j ++)
          if (su[j] != grouping[i][j])
            same = false;
        max = !same;
      }
      if (max)
        memcpy(su, grouping[i], setsz * sizeof(group_type));
    }
  }
  memcpy(st, su, setsz * sizeof(group_type));
  free(su);
}  /* bigsubset */


void recontraverse(node **p, group_type *st, long n, long *nextnode)
{
  /* traverse to add next node to consensus tree */
  long i, j = 0, k = 0, l = 0;

  boolean found, same = 0, zero, zero2;
  group_type *tempset, *st2;
  node *q, *r;

  for (i = 1; i <= spp; i++) {  /* count species in set */
    if (i == ((l+1)*SETBITS+1)) l++;
    if (((1L << (i - 1 - l*SETBITS)) & st[l]) != 0) {
      k++;               /* k  is the number of species in the set */
      j = i;             /* j  is set to last species in the set */
    }
  }
  if (k == 1) {           /* if only 1, set up that tip */
    *p = nodep[j - 1];
    (*p)->tip = true;
    (*p)->index = j;
    return;
  }
  gnu(&grbg, p);          /* otherwise make interior node */
  (*p)->tip = false;
  (*p)->index = *nextnode;
  nodep[*nextnode - 1] = *p;
  (*nextnode)++;
  (*p)->deltav = 0.0;
  for (i = 0; i < n; i++) { /* go through all sets */
    same = true;            /* to find one which is this one */
    for (j = 0; j < setsz; j++)
      if (grouping[i][j] != st[j])
        same = false;
    if (same)
      (*p)->deltav = *timesseen[i];
  }
  tempset = (group_type *)Malloc(setsz * sizeof(group_type));
  memcpy(tempset, st, setsz * sizeof(group_type));
  q = *p;
  st2 = (group_type *)Malloc(setsz * sizeof(group_type));
  memcpy(st2, st, setsz * sizeof(group_type));
  zero = true;      /* having made two copies of the set ... */
  for (j = 0; j < setsz; j++)       /* see if they are empty */
    if (tempset[j] != 0)
      zero = false;
  if (!zero)
    bigsubset(tempset, n);        /* find biggest set within it */
  zero = zero2 = false;           /* ... tempset is that subset */
  while (!zero && !zero2) {
    zero = zero2 = true;
    for (j = 0; j < setsz; j++) {
      if (st2[j] != 0)
        zero = false;
      if (tempset[j] != 0)
        zero2 = false;
    }
    if (!zero && !zero2) {
      gnu(&grbg, &q->next);
      q->next->index = q->index;
      q = q->next;
      q->tip = false;
      r = *p;
      recontraverse(&q->back, tempset, n, nextnode); /* put it on tree */
      *p = r;
      q->back->back = q;
      for (j = 0; j < setsz; j++)
        st2[j] &= ~tempset[j];     /* remove that subset from the set */
      memcpy(tempset, st2, setsz * sizeof(group_type));  /* that becomes set */
      found = false;
      i = 1;
      while (!found && i <= n) {
        if (grouping[i - 1] != 0) {
          same = true;
          for (j = 0; j < setsz; j++)
            if (grouping[i - 1][j] != tempset[j])
              same = false;
        }
        if ((grouping[i - 1] != 0) && same)
          found = true;
        else
          i++;
      }
      zero = true;
      for (j = 0; j < setsz; j++)
        if (tempset[j] != 0)
          zero = false;
      if (!zero && !found)
        bigsubset(tempset, n);
    }
  }
  q->next = *p;
  free(tempset);
  free(st2);
}  /* recontraverse */


void reconstruct(long n)
{
  /* reconstruct tree from the subsets */
  long nextnode;
  group_type *s;

  nextnode = spp + 1;
  s = (group_type *)Malloc(setsz * sizeof(group_type));
  memcpy(s, fullset, setsz * sizeof(group_type));
  recontraverse(&root, s, n, &nextnode);
  free(s);
}  /* reconstruct */


void coordinates(node *p, long *tipy)
{
  /* establishes coordinates of nodes */
  node *q, *first, *last;
  long maxx;

  if (p->tip) {
    p->xcoord = 0;
    p->ycoord = *tipy;
    p->ymin = *tipy;
    p->ymax = *tipy;
    (*tipy) += down;
    return;
  }
  q = p->next;
  maxx = 0;
  while (q != p) {
    coordinates(q->back, tipy);
    if (!q->back->tip) {
      if (q->back->xcoord > maxx)
        maxx = q->back->xcoord;
    }
    q = q->next;
  }
  first = p->next->back;
  q = p;
  while (q->next != p)
    q = q->next;
  last = q->back;
  p->xcoord = maxx + OVER;
  p->ycoord = (long)((first->ycoord + last->ycoord) / 2);
  p->ymin = first->ymin;
  p->ymax = last->ymax;
}  /* coordinates */


void drawline(long i)
{
  /* draws one row of the tree diagram by moving up tree */
  node *p, *q;
  long n, j;
  boolean extra, done, trif;
  node *r,  *first = NULL, *last = NULL;
  boolean found;

  p = root;
  q = root;
  fprintf(outfile, "  ");
  extra = false;
  trif = false;
  do {
    if (!p->tip) {
      found = false;
      r = p->next;
      while (r != p && !found) {
        if (i >= r->back->ymin && i <= r->back->ymax) {
          q = r->back;
          found = true;
        } else
          r = r->next;
      }
      first = p->next->back;
      r = p;
      while (r->next != p)
        r = r->next;
      last = r->back;
    }
    done = (p->tip || p == q);
    n = p->xcoord - q->xcoord;
    if (extra) {
      n--;
      extra = false;
    }
    if (q->ycoord == i && !done) {
      if (trif)
        putc('-', outfile);
      else
        putc('+', outfile);
      trif = false;
      if (!q->tip) {
        for (j = 1; j <= n - 7; j++)
          putc('-', outfile);
        if (noroot && (root->next->next->next == root) &&
            (((root->next->back == q) && root->next->next->back->tip)
             || ((root->next->next->back == q) && root->next->back->tip)))
          fprintf(outfile, "------|");
        else {
          if (!strict) {   /* write number of times seen */
            if (q->deltav >= 100)
              fprintf(outfile, "%5.1f-|", (double)q->deltav);
            else if (q->deltav >= 10)
              fprintf(outfile, "-%4.1f-|", (double)q->deltav);
            else
              fprintf(outfile, "--%3.1f-|", (double)q->deltav);
          } else
            fprintf(outfile, "------|");
        }
        extra = true;
        trif = true;
      } else {
        for (j = 1; j < n; j++)
          putc('-', outfile);
      }
    } else if (!p->tip && last->ycoord > i && first->ycoord < i &&
               (i != p->ycoord || p == root)) {
      putc('|', outfile);
      for (j = 1; j < n; j++)
        putc(' ', outfile);
    } else {
      for (j = 1; j <= n; j++)
        putc(' ', outfile);
      if (trif)
        trif = false;
    }
    if (q != p)
      p = q;
  } while (!done);
  if (p->ycoord == i && p->tip) {
    for (j = 0; (j < MAXNCH) && (p->nayme[j] != '\0'); j++)
      putc(p->nayme[j], outfile);
  }
  putc('\n', outfile);
}  /* drawline */


void printree()
{
  /* prints out diagram of the tree */
  long i;
  long tipy;

  if (treeprint) {
    fprintf(outfile, "\nCONSENSUS TREE:\n");
    if (mr || mre || ml) {
      if (noroot) {
        fprintf(outfile, "the numbers on the branches indicate the number\n");
 fprintf(outfile, "of times the partition of the species into the two sets\n");
        fprintf(outfile, "which are separated by that branch occurred\n");
      } else {
        fprintf(outfile, "the numbers forks indicate the number\n");
        fprintf(outfile, "of times the group consisting of the species\n");
        fprintf(outfile, "which are to the right of that fork occurred\n");
      }
      fprintf(outfile, "among the trees, out of %6.2f trees\n", ntrees);
      if (ntrees <= 1.001)
        fprintf(outfile, "(trees had fractional weights)\n");
    }
    tipy = 1;
    coordinates(root, &tipy);
    putc('\n', outfile);
    for (i = 1; i <= tipy - down; i++)
      drawline(i);
    putc('\n', outfile);
  }
  if (noroot) {
    fprintf(outfile, "\n  remember:");
    if (didreroot)
      fprintf(outfile, " (though rerooted by outgroup)");
    fprintf(outfile, " this is an unrooted tree!\n");
  }
  putc('\n', outfile);
}  /* printree */


void enternohash(group_type *s, long *n)
{
  /* if set s is already there, enter it into groupings in the next slot
     (without hash-coding).  n is number of sets stored there and is updated */
  long i, j;
  boolean found;

  found = false;
  for (i = 0; i < (*n); i++) {  /* go through looking whether it is there */
    found = true;
    for (j = 0; j < setsz; j++) {  /* check both parts of partition */
      found = found && (grouping[i][j] == s[j]);
      found = found && (group2[i][j] == (fullset[j] & (~s[j])));
    }
    if (found)
      break;
  }
  if (!found) {    /* if not, add it to the slot after the end,
                      which must be empty */
    grouping[i] = (group_type *)Malloc(setsz * sizeof(group_type));
    timesseen[i] = (double *)Malloc(sizeof(double));
    group2[i] = (group_type *)Malloc(setsz * sizeof(group_type));
    for (j = 0; j < setsz; j++)
      grouping[i][j] = s[j];
    *timesseen[i] = 1;
    (*n)++;
  }
} /* enternohash */


void enterpartition (group_type *s1, long *n)
{
  /* try to put this partition in list of partitions.  If implied by others,
     don't bother.  If others implied by it, replace them.  If this one
     vacuous because only one element in s1, forget it */
  long i, j;
  boolean found;

/* this stuff all to be rewritten but left here so pieces can be used */
  found = false;
  for (i = 0; i < (*n); i++) {  /* go through looking whether it is there */
    found = true;
    for (j = 0; j < setsz; j++) {  /* check both parts of partition */
      found = found && (grouping[i][j] == s1[j]);
      found = found && (group2[i][j] == (fullset[j] & (~s1[j])));
    }
    if (found)
      break;
  }
  if (!found) {    /* if not, add it to the slot after the end,
                      which must be empty */
    grouping[i] = (group_type *)Malloc(setsz * sizeof(group_type));
    timesseen[i] = (double *)Malloc(sizeof(double));
    group2[i] = (group_type *)Malloc(setsz * sizeof(group_type));
    for (j = 0; j < setsz; j++)
      grouping[i][j] = s1[j];
    *timesseen[i] = 1;
    (*n)++;
  }
} /* enterpartition */


void elimboth(long n)
{
  /* for Adams case: eliminate pairs of groups incompatible with each other */
  long i, j;
  boolean comp;

  for (i = 0; i < n-1; i++) {
    for (j = i+1; j < n; j++) {
      comp = compatible(i,j);
      if (!comp) {
        *timesseen[i] = 0.0;
        *timesseen[j] = 0.0;
      }
    }
    if (*timesseen[i] == 0.0) {
      free(grouping[i]);
      free(timesseen[i]);
      timesseen[i] = NULL;
      grouping[i] = NULL;
    }
  }
  if (*timesseen[n-1] == 0.0) {
    free(grouping[n-1]);
    free(timesseen[n-1]);
    timesseen[n-1] = NULL;
    grouping[n-1] = NULL;
  }
}  /* elimboth */


void consensus(pattern_elm ***pattern_array, long trees_in)
{
  long i, n, n2, tipy;

  group2 = (group_type **)  Malloc(maxgrp*sizeof(group_type *));
  for (i = 0; i < maxgrp; i++)
    group2[i] = NULL;
  times2 = (double **)Malloc(maxgrp*sizeof(double *));
  for (i = 0; i < maxgrp; i++)
    times2[i] = NULL;
  n2 = 0;
  censor();                /* drop groups that are too rare */
  compress(&n);            /* push everybody to front of array */
  if (!strict) {           /* drop those incompatible, if any */
    sort(n);
    eliminate(&n, &n2);
    compress(&n);
    }
  reconstruct(n);
  tipy = 1;
  coordinates(root, &tipy);
  if (prntsets) {
    fprintf(outfile, "\nSets included in the consensus tree\n");
    printset(n);
    for (i = 0; i < n2; i++) {
      if (!grouping[i]) {
        grouping[i] = (group_type *)Malloc(setsz * sizeof(group_type));
        timesseen[i] = (double *)Malloc(sizeof(double));
        }
      memcpy(grouping[i], group2[i], setsz * sizeof(group_type));
      *timesseen[i] = *times2[i];
    }
    n = n2;
    fprintf(outfile, "\n\nSets NOT included in consensus tree:");
    if (n2 == 0)
      fprintf(outfile, " NONE\n");
    else {
      putc('\n', outfile);
      printset(n);
    }
  }
  putc('\n', outfile);
  if (strict)
    fprintf(outfile, "\nStrict consensus tree\n");
  if (mre)
    fprintf(outfile, "\nExtended majority rule consensus tree\n");
  if (ml) {
    fprintf(outfile, "\nM  consensus tree (l = %4.2f)\n", mlfrac);
    fprintf(outfile, " l\n");
  }
  if (mr)
    fprintf(outfile, "\nMajority rule consensus tree\n");
  printree();
  free(nayme);  
  for (i = 0; i < maxgrp; i++)
    free(grouping[i]);
  free(grouping);
  for (i = 0; i < maxgrp; i++)
    free(order[i]);
  free(order);
  for (i = 0; i < maxgrp; i++)
    if (timesseen[i] != NULL)
      free(timesseen[i]);
  free(timesseen);
}  /* consensus */


void rehash()
{
  group_type *s;
  long i, j, k;
  double temp, ss, smult;
  boolean done;

  smult = (sqrt(5.0) - 1) / 2;
  s = (group_type *)Malloc(setsz * sizeof(group_type));
  for (i = 0; i < maxgrp/2; i++) {
    k = *order[i];
    memcpy(s, grouping[k], setsz * sizeof(group_type));
    ss = 0.0;
    for (j = 0; j < setsz; j++)
      ss += s[j] /* pow(2, SETBITS*j)*/;
    temp = ss * smult;
    j = (long)(maxgrp * (temp - floor(temp)));
    done = false;
    while (!done) {
      if (!grping2[j]) {
        grping2[j] = (group_type *)Malloc(setsz * sizeof(group_type));
        order2[i] = (long *)Malloc(sizeof(long));
        tmseen2[j] = (double *)Malloc(sizeof(double));
        memcpy(grping2[j], grouping[k], setsz * sizeof(group_type));
        *tmseen2[j] = *timesseen[k];
        *order2[i] = j;
        grouping[k] = NULL;
        timesseen[k] = NULL;
        order[i] = NULL;
        done = true;
      } else {
        j++;
        if (j >= maxgrp) j -= maxgrp;
      }
    }
  }
  free(s);
}  /* rehash */


void enternodeset(node *r)
{ /* enter a the set of species from a node into the hash table */
  group_type *s;

  s = (group_type *)Malloc(setsz * sizeof(group_type));
  memcpy(s, r->nodeset, setsz * sizeof(group_type));
  enterset(s);
  free(s);
} /* enternodeset */


void enterset(group_type *s)
{ /* enter a set of species into the hash table */
  long i, j, start;
  double ss, n;
  boolean done, same;
  double times ;

  same = true;
  for (i = 0; i < setsz; i++)
    if (s[i] != fullset[i])
      same = false;
  if (same) 
    return;
  times = trweight;
  ss = 0.0;                        /* compute the hashcode for the set */
  n = ((sqrt(5.0) - 1.0) / 2.0);   /* use an irrational multiplier */
  for (i = 0; i < setsz; i++)
    ss += s[i] * n;
  i = (long)(maxgrp * (ss - floor(ss))) + 1; /* use fractional part of code */
  start = i;
  done = false;                   /* go through seeing if it is there */
  while (!done) {
    if (grouping[i - 1]) {        /* ... i.e. if group is absent, or  */
      same = false;               /* (will be false if timesseen = 0) */
      if (!(timesseen[i-1] == 0)) {  /* ... if timesseen = 0 */
        same = true;
        for (j = 0; j < setsz; j++) {
          if (s[j] != grouping[i - 1][j])
            same = false;
        }
      }
    }
    if (grouping[i - 1] && same) {  /* if it is there, increment timesseen */
      *timesseen[i - 1] += times;
      done = true;
    } else if (!grouping[i - 1]) {  /* if not there and slot empty ... */
      grouping[i - 1] = (group_type *)Malloc(setsz * sizeof(group_type));
      lasti++;
      order[lasti] = (long *)Malloc(sizeof(long));
      timesseen[i - 1] = (double *)Malloc(sizeof(double));
      memcpy(grouping[i - 1], s, setsz * sizeof(group_type));
      *timesseen[i - 1] = times;
      *order[lasti] = i - 1;
      done = true;
    } else {  /* otherwise look to put it in next slot ... */
      i++;
      if (i > maxgrp) i -= maxgrp;
    }
    if (!done && i == start) {  /* if no place to put it, expand hash table */
      maxgrp = maxgrp*2;
      tmseen2 = (double **)Malloc(maxgrp*sizeof(double *));
      for (j = 0; j < maxgrp; j++)
        tmseen2[j] = NULL;
      grping2 = (group_type **)Malloc(maxgrp*sizeof(group_type *));
      for (j = 0; j < maxgrp; j++)
        grping2[j] = NULL;
      order2 = (long **)Malloc(maxgrp*sizeof(long *));
      for (j = 0; j < maxgrp; j++)
        order2[j] = NULL;
      rehash();
      free(timesseen);
      free(grouping);
      free(order);
      timesseen = tmseen2;
      grouping = grping2;
      order = order2;
      done = true;
      lasti = maxgrp/2 - 1;
      enterset(s);
    }
  }
}  /* enterset */


void accumulate(node *r_)
{
  node *r;
  node *q;
  long i;

  r = r_;
  if (r->tip) {
    if (!r->nodeset)
      r->nodeset = (group_type *)Malloc(setsz * sizeof(group_type));
    for (i = 0; i < setsz; i++)
      r->nodeset[i] = 0L;
    i = (r->index-1) / (long)SETBITS;
    r->nodeset[i] = 1L << (r->index - 1 - i*SETBITS);
  }
  else {
    q = r->next;
    while (q != r) {
      accumulate(q->back);
      q = q->next;
    }
    q = r->next;
    if (!r->nodeset)
      r->nodeset = (group_type *)Malloc(setsz * sizeof(group_type));
    for (i = 0; i < setsz; i++)
      r->nodeset[i] = 0;
    while (q != r) {
      for (i = 0; i < setsz; i++)
        r->nodeset[i] |= q->back->nodeset[i];
      q = q->next;
    }
  }
  if ((!r->tip && (r->next->next != r)) || r->tip)
    enternodeset(r);
}  /* accumulate */


void dupname2(Char *name, node *p, node *this)
{
  /* search for a duplicate name recursively */
  node *q;

  if (p->tip) {
    if (p != this) {
      
      if (strcmp(name,p->nayme) == 0) {
        printf("\n\nERROR in user tree: duplicate name found: ");
        puts(p->nayme);
        printf("\n\n");
        exxit(-1);
      }
    }
  } else {
    q = p;
    while (p->next != q) {
      dupname2(name, p->next->back, this);
      p = p->next;
    }
  }
}  /* dupname2 */


void dupname(node *p)
{
  /* search for a duplicate name in tree */
  node *q;

  if (p->tip)
    dupname2(p->nayme, root, p);
  else {
    q = p;
    while (p->next != q) {
      dupname(p->next->back);
      p = p->next;
    }
  }
}  /* dupname */


void gdispose(node *p)
{
  /* go through tree throwing away nodes */
  node *q, *r;

  if (p->tip) {
    chuck(&grbg, p);
    return;
  }
  q = p->next;
  while (q != p) {
    gdispose(q->back);
    r = q;
    q = q->next;
    chuck(&grbg, r);
  }
  chuck(&grbg, q);
}  /* gdispose */


void initreenode(node *p)
{
  /* traverse tree and assign species names to tip nodes */
  node *q;

  if (p->tip) {
    memcpy(nayme[p->index - 1], p->nayme, MAXNCH);
  } else {
    q = p->next;
    while (q && q != p) {
      initreenode(q->back);
      q = q->next;
    }
  }
} /* initreenode */


void reroot(node *outgroup, long *nextnode)
{
  /* reorients tree, putting outgroup in desired position. */
  long i;
  boolean nroot;
  node *p, *q;

  nroot = false;
  p = root->next;
  while (p != root) {
    if ((outgroup->back == p) && (root->next->next->next == root)) {
      nroot = true;
      p = root;
    } else
      p = p->next;
  }
  if (nroot)
    return;
  p = root;
  i = 0;
  while (p->next != root) {
    p = p->next;
    i++;
  }
  if (i == 2) {
    root->next->back->back = p->back;
    p->back->back = root->next->back;
    q = root->next;
  } else {
    p->next = root->next;
    nodep[root->index-1] = root->next;
    gnu(&grbg, &root->next);
    q = root->next;
    gnu(&grbg, &q->next);
    p = q->next;
    p->next = root;
    q->tip = false;
    p->tip = false;
    nodep[*nextnode] = root;
    (*nextnode)++;
    root->index = *nextnode;
    root->next->index = root->index;
    root->next->next->index = root->index;
  }
  q->back = outgroup;
  p->back = outgroup->back;
  outgroup->back->back = p;
  outgroup->back = q;
}  /* reroot */


void store_pattern (pattern_elm ***pattern_array,
                        double *timesseen_changes, int trees_in_file)
{ 
  /* put a tree's groups into a pattern array.
     Don't forget that when not Adams, grouping[] is not compressed. . . */
  long i, total_groups=0, j=0, k;

  /* First, find out how many groups exist in the given tree. */
  for (i = 0 ; i < maxgrp ; i++)
    if ((grouping[i] != NULL) &&
       (*timesseen[i] > timesseen_changes[i]))
      /* If this is group exists and is present in the current tree, */
      total_groups++ ;

  /* Then allocate a space to store the bit patterns. . . */
  for (i = 0 ; i < setsz ; i++) {
    pattern_array[i][trees_in_file]
      = (pattern_elm *) Malloc(sizeof(pattern_elm)) ;
    pattern_array[i][trees_in_file]->apattern = 
      (group_type *) Malloc (total_groups * sizeof (group_type)) ;
    pattern_array[i][trees_in_file]->patternsize = (long *)Malloc(sizeof(long));
  }
  /* Then go through groupings again, and copy in each element
     appropriately. */
  for (i = 0 ; i < maxgrp ; i++)
    if (grouping[i] != NULL)
      if (*timesseen[i] > timesseen_changes[i]) {
        for (k = 0 ; k < setsz ; k++)
          pattern_array[k][trees_in_file]->apattern[j] = grouping[i][k] ;  
        j++ ;
        timesseen_changes[i] = *timesseen[i] ;
      }
  *pattern_array[0][trees_in_file]->patternsize = total_groups;
}  /* store_pattern */


boolean samename(naym name1, plotstring name2)
{
  return !(strncmp(name1, name2, MAXNCH)); 
}  /* samename */


void reordertips()
{
  /* matchs tip nodes to species names first read in */
  long i, j;
  boolean done;
  node *p, *q, *r;
  for (i = 0;  i < spp; i++) {
    j = 0;
    done = false;
    do {
      if (samename(nayme[i], nodep[j]->nayme)) {
        done =  true;
        if (i != j) {
          p = nodep[i];
          q = nodep[j];
          r = p->back;
          p->back->back = q;
          q->back->back = p;
          p->back =  q->back;
          q->back = r;
          memcpy(q->nayme, p->nayme, MAXNCH);
          memcpy(p->nayme, nayme[i], MAXNCH);
        }
      }
      j++;
    } while (j < spp && !done);
  }
}  /* reordertips */


void read_groups (pattern_elm ****pattern_array,double *timesseen_changes,
                        long *trees_in_1, FILE *intree)
{
  /* read the trees.  Accumulate sets. */
  int i, j, k;
  boolean haslengths, initial;
  long nextnode;

  /* set up the groupings array and the timesseen array */
  grouping  = (group_type **)  Malloc(maxgrp*sizeof(group_type *));
  for (i = 0; i < maxgrp; i++)
    grouping[i] = NULL;
  order     = (long **) Malloc(maxgrp*sizeof(long *));
  for (i = 0; i < maxgrp; i++)
    order[i] = NULL;
  timesseen = (double **)Malloc(maxgrp*sizeof(double *));
  for (i = 0; i < maxgrp; i++)
    timesseen[i] = NULL;

  firsttree = true;
  grbg = NULL;
  initial = true;
  while (!eoff(intree)) {          /* go till end of input tree file */
    goteof = false;
    nextnode = 0;
    haslengths = false;
    allocate_nodep(&nodep, &intree, &spp);
    if (firsttree)
      nayme = (naym *)Malloc(spp*sizeof(naym));
    treeread(intree, &root, treenode, &goteof, &firsttree, nodep, 
              &nextnode, &haslengths, &grbg, initconsnode);
    if (!initial) { 
      reordertips();
    } else {
      initial = false;
      dupname(root);
      initreenode(root);
      setsz = (long)ceil((double)spp/(double)SETBITS);
      if (tree_pairing != NO_PAIRING)
        {
          /* Now that we know setsz, we can malloc pattern_array
          and pattern_array[n] accordingly. */
          (*pattern_array) =
            (pattern_elm ***)Malloc(setsz * sizeof(pattern_elm **));

          /* For this assignment, let's assume that there will be no
             more than maxtrees. */
          for (j = 0 ; j < setsz ; j++)
            (*pattern_array)[j] = 
              (pattern_elm **)Malloc(maxtrees * sizeof(pattern_elm *)) ;
        }

      fullset = (group_type *)Malloc(setsz * sizeof(group_type));
      for (j = 0; j < setsz; j++)
        fullset[j] = 0L;
      k = 0;
      for (j = 1; j <= spp; j++) {
        if (j == ((k+1)*SETBITS+1)) k++;
        fullset[k] |= 1L << (j - k*SETBITS - 1);
      }
    }
    if (goteof)
      continue;
    ntrees += trweight;
    if (noroot) {
      reroot(nodep[outgrno - 1], &nextnode);
      didreroot = outgropt;
    }
    accumulate(root);
    gdispose(root);
    for (j = 0; j < 2*(1+spp); j++)
      nodep[j] = NULL;
    free(nodep);
    /* Added by Dan F. */
    if (tree_pairing != NO_PAIRING) {
        /* If we're computing pairing or need separate tree sets, store the
           current pattern as an element of it's trees array. */
        store_pattern ((*pattern_array), timesseen_changes, (*trees_in_1)) ;
        (*trees_in_1)++ ;
    }
  }
} /* read_groups */


