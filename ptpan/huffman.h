/*!
 * \file huffman.h
 *
 * \date 04.02.2011
 * \author Chris Hodges
 * \author Tilo Eissler
 */

#ifndef PTPAN_HUFFMAN_H_
#define PTPAN_HUFFMAN_H_

#include <stdio.h>

#include "types.h"

/* huffman tree node */
struct HuffTree {
 struct HuffTree *ht_Child[2]; /* children */
 ULONG ht_ID; /* ID to return */
 ULONG ht_Codec; /* complete codec */
 UWORD ht_CodeLength; /* code length */
};

/* huffman table entry */
struct HuffCode {
 ULONG hc_Weight; /* weight of item */
 ULONG hc_Codec; /* actual bits (in lsb) */
 UWORD hc_CodeLength; /* lengths in bits */
};

/* temporary internal node type */
struct HuffCodeInternal {
 ULONG hc_ID; /* code id */
 ULONG hc_Weight; /* weight of item */
 UWORD hc_Left; /* ID of left parent */
 UWORD hc_Right; /* ID of right parent */
};

void BuildHuffmanCodeRec(struct HuffCode *hcbase, struct HuffCodeInternal *hc,
  ULONG len, ULONG rootidx, ULONG codelen, ULONG code);
BOOL BuildHuffmanCode(struct HuffCode *hcbase, ULONG len, LONG threshold);
void WriteHuffmanTree(struct HuffCode *hc, ULONG size, FILE *fh);
struct HuffTree * ReadHuffmanTree(FILE *fh);
struct HuffTree * BuildHuffmanTreeFromTable(struct HuffCode *hc, ULONG maxid);
void FreeHuffmanTree(struct HuffTree *root);
struct HuffTree * FindHuffTreeID(struct HuffTree *ht, UBYTE *adr, ULONG bitpos);

#endif /* PTPAN_HUFFMAN_H_ */
