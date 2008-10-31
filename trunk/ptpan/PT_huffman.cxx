#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"

/* /// "BuildHuffmanCodeRec()" */
void BuildHuffmanCodeRec(struct HuffCode *hcbase, 
                         struct HuffCodeInternal *hc,
                         ULONG len, ULONG rootidx,
                         ULONG codelen, ULONG code)
{
  ULONG idx;
  idx = hc[rootidx].hc_Left;
  if(idx < len) // left is leaf?
  {
    hcbase[hc[idx].hc_ID].hc_CodeLength = codelen;
    hcbase[hc[idx].hc_ID].hc_Codec = code;
  } else {
    BuildHuffmanCodeRec(hcbase, hc, len, idx, codelen + 1, code << 1);
  }
  code ^= 1;
  idx = hc[rootidx].hc_Right;
  if(idx < len) // right is leaf?
  {
    hcbase[hc[idx].hc_ID].hc_CodeLength = codelen;
    hcbase[hc[idx].hc_ID].hc_Codec = code;
  } else {
    BuildHuffmanCodeRec(hcbase, hc, len, idx, codelen + 1, code << 1);
  }
}
/* \\\ */

/* /// "BuildHuffmanCode()" */
BOOL BuildHuffmanCode(struct HuffCode *hcbase, ULONG len, LONG threshold)
{
  ULONG cnt;
  ULONG w;
  ULONG min0idx, min0val;
  ULONG min1idx, min1val;
  struct HuffCodeInternal *hc;
  ULONG newlen;
  ULONG xtrlen;
  ULONG rootidx;
  ULONG total = 0;
  BOOL take;

  /* generate huffman tree. I know this is not the fastest
     routine as it doesn't sort the array prior to building
     the tree, but as we are speaking of very small trees
     this should not be a problem */

  /* calculate total weight */
  newlen = 0;
  for(cnt = 0; cnt < len; cnt++)
  {
    if((w = hcbase[cnt].hc_Weight))
    {
      if(((LONG) w) >= threshold) /* check, if we've got a threshold and need to skip it */
      {
        newlen++;
        total += w;
      }
    }
  }
  if(!newlen)
  {
    return(FALSE);
  }
  hc = (struct HuffCodeInternal *) calloc(newlen << 1, sizeof(struct HuffCodeInternal));
  if(!hc)
  {
    printf("ARGHGHH! No temporary memory for huffman tree!\n");
    return(FALSE);
  }
  rootidx = xtrlen = 0;
  for(cnt = 0; cnt < len; cnt++)
  {
    if((w = hcbase[cnt].hc_Weight))
    {
      hc[xtrlen].hc_Weight = w;
      take = TRUE;
      if(threshold)
      {
        if(threshold < 0) /* automatic threshold calculation */
        {
        if(w*3 <= (total / newlen)) /* make less popular codes uniformly */
        {
        hc[xtrlen].hc_Weight = 1; /* reduce weight, but keep it */
        }
        }
        else if(w < (ULONG) threshold) /* hard threshold -- don't generate code for this weight */
        {
          take = FALSE;
        }
      }
      if(take)
      {
        hc[xtrlen++].hc_ID = cnt;
      }
    }
  }
  do
  {
    /* now choose the two items with the smallest weight != 0 */
    min0idx = min0val = 0xffffffff;
    min1idx = min1val = 0xffffffff;
    for(cnt = 0; cnt < xtrlen; cnt++)
    {
      w = hc[cnt].hc_Weight;
      if(w)
      {
        if(w < min0val)
        {
          min1val = min0val;
          min1idx = min0idx;
          min0val = w;
          min0idx = cnt;
        } 
        else if(w < min1val)
        {
          min1val = w;
          min1idx = cnt;
        }
      }
    }
    if(min1idx == 0xffffffff)
    {
      break;
    }
    /* merge these nodes */
    hc[xtrlen].hc_Weight = min0val + min1val;
    hc[xtrlen].hc_Left = min0idx;
    hc[xtrlen].hc_Right = min1idx;
    hc[min0idx].hc_Weight = 0;
    hc[min1idx].hc_Weight = 0;
    rootidx = xtrlen++;
  } while(TRUE);

  //printf("Codespace: %ld, codes generated: %ld\n", len, newlen);
  /* now generate codes */
  BuildHuffmanCodeRec(hcbase, hc, newlen, rootidx, 1, 0);

  /* generate average code length for debugging */
#if 0
  {
    float clen = 0;
    for(cnt = 0; cnt < len; cnt++)
    {
      clen += hcbase[cnt].hc_Weight * hcbase[cnt].hc_CodeLength;
    }
    printf("Average code length: %f\n", clen / ((float) total));
  }
#endif
  free(hc);
  return(TRUE);
}
/* \\\ */

/* /// "WriteHuffmanTree()" */
void WriteHuffmanTree(struct HuffCode *hc, ULONG size, FILE *fh)
{
  ULONG cnt;

  fwrite(&size, sizeof(size), 1, fh);
  for(cnt = 0; cnt < size; cnt++)
  {
    if(hc[cnt].hc_CodeLength)
    {
      fwrite(&cnt, sizeof(cnt), 1, fh);
      fwrite(&hc[cnt].hc_CodeLength, sizeof(hc[cnt].hc_CodeLength), 1, fh);
      fwrite(&hc[cnt].hc_Codec, sizeof(hc[cnt].hc_Codec), 1, fh);
    }
  }
  cnt = ~0UL;
  fwrite(&cnt, sizeof(cnt), 1, fh);
}
/* \\\ */

/* /// "ReadHuffmanTree()" */
struct HuffTree * ReadHuffmanTree(FILE *fh)
{
  struct HuffTree *ht;
  struct HuffTree *root;
  ULONG maxid;
  ULONG cnt;
  UWORD codelen;
  ULONG codec;
  UWORD depth;
  UWORD leafbit;

  root = (struct HuffTree *) calloc(sizeof(struct HuffTree), 1);
  if(!root)
  {
    return(NULL); /* out of memory */
  }
  /* read length first (not used) */
  fread(&maxid, sizeof(maxid), 1, fh);
  do
  {
    fread(&cnt, sizeof(cnt), 1, fh);
    if(cnt == ~0UL)
    {
      break;
    }

    fread(&codelen, sizeof(codelen), 1, fh);
    fread(&codec, sizeof(codec), 1, fh);

    /* build leaf from the root going down */
    ht = root;
    depth = 0;
    while(depth++ < codelen)
    {
      leafbit = (codec >> (codelen - depth)) & 1;
      if(!ht->ht_Child[leafbit])
      {
        if(!(ht->ht_Child[leafbit] = (struct HuffTree *) calloc(sizeof(struct HuffTree), 1)))
        {
        return(NULL); /* out of memory */
        }
      }
      ht = ht->ht_Child[leafbit];
    }
    /* got to the leaf */
    ht->ht_ID = cnt;
    /* these are not really needed, but codelength is used to check if this is a leaf */
    ht->ht_Codec = codec;
    ht->ht_CodeLength = codelen;
    if(ht->ht_Child[0] || ht->ht_Child[1]) /* debugging purposes */
    {
      printf("Huffman tree does not comply to the fano condition (%ld: %08lx, %d)!\n", 
         cnt, codec, codelen);
    }
  } while(TRUE);
  return(root);
}
/* \\\ */

/* /// "BuildHuffmanTreeFromTable()" */
struct HuffTree * BuildHuffmanTreeFromTable(struct HuffCode *hc, ULONG maxid)
{
  struct HuffTree *ht;
  struct HuffTree *root;
  ULONG cnt;
  UWORD codelen;
  ULONG codec;
  UWORD depth;
  UWORD leafbit;

  root = (struct HuffTree *) calloc(sizeof(struct HuffTree), 1);
  if(!root)
  {
    return(NULL); /* out of memory */
  }

  for(cnt = 0; cnt < maxid; cnt++)
  {
    if((codelen = hc[cnt].hc_CodeLength))
    {
      codec = hc[cnt].hc_Codec;
      
      /* build leaf from the root going down */
      ht = root;
      depth = 0;
      while(depth++ < codelen)
      {
        leafbit = (codec >> (codelen - depth)) & 1;
        if(!ht->ht_Child[leafbit])
        {
        if(!(ht->ht_Child[leafbit] = (struct HuffTree *) calloc(sizeof(struct HuffTree), 1)))
        {
        return(NULL); /* out of memory */
        }
        }
        ht = ht->ht_Child[leafbit];
      }
      /* got to the leaf */
      ht->ht_ID = cnt;
      /* these are not really needed, but codelength is used to check if this is a leaf */
      ht->ht_Codec = codec;
      ht->ht_CodeLength = codelen;
      if(ht->ht_Child[0] || ht->ht_Child[1]) /* debugging purposes */
      {
        printf("Huffman tree does not comply to the fano condition (%ld: %08lx, %d)!\n", 
        cnt, codec, codelen);
      }
    }
  }
  return(root);
}
/* \\\ */

/* /// "FreeHuffmanTree()" */
void FreeHuffmanTree(struct HuffTree *root)
{
  if(!root)
  {
    return;
  }
  FreeHuffmanTree(root->ht_Child[0]);
  FreeHuffmanTree(root->ht_Child[1]);
  free(root);
}
/* \\\ */

/* /// "FindHuffTreeID()" */
struct HuffTree * FindHuffTreeID(struct HuffTree *ht, UBYTE *adr, ULONG bitpos)
{
  adr += bitpos >> 3;
  bitpos &= 7;
  while(!ht->ht_CodeLength)
  {
    ht = ht->ht_Child[(*adr >> (7 - bitpos)) & 1];
    if(++bitpos > 7)
    {
      adr++;
      bitpos = 0;
    }   
  }
  return(ht);
}
/* \\\ */
