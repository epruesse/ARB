#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "ptpan.h"
#include "pt_prototypes.h"

/* /// "AllocHashArray()" */
struct HashArray * AllocHashArray(ULONG size)
{
  struct HashArray *ha;
  ha = (struct HashArray *) calloc(1, sizeof(struct HashArray));
  if(ha)
  {
    ha->ha_Array = (struct HashEntry *) calloc(size, sizeof(struct HashEntry));
    if(ha->ha_Array)
    {
      ha->ha_InitialSize = ha->ha_Size = size;
      return(ha);
    }
    freenull(ha);
  }
  return(ha);
}
/* \\\ */

/* /// "FreeHashArray()" */
void FreeHashArray(struct HashArray *ha)
{
  if(!ha)
  {
    return;
  }
  free(ha->ha_Array);
  free(ha);
}
/* \\\ */

/* /// "ClearHashArray()" */
void ClearHashArray(struct HashArray *ha)
{
  memset(ha->ha_Array, 0, ha->ha_Size * sizeof(struct HashEntry));
  ha->ha_InitialSize = ha->ha_Size;
  ha->ha_Used = 0;
}
/* \\\ */

/* /// "GetHashEntry()" */
struct HashEntry * GetHashEntry(struct HashArray *ha, ULONG hashkey)
{
  struct HashEntry *he;
  ULONG tmpkey = hashkey;
  ULONG size = ha->ha_Size;
  hashkey++; /* avoid zero */
  do
  {
    he = &ha->ha_Array[tmpkey++ % size];
    if(!he->he_Key)
    {
      if(size > ha->ha_InitialSize)
      {
        size /= 2;
        tmpkey = hashkey-1;
      } else {
        return(NULL);
      }
    }
    if(he->he_Key == hashkey)
    {
      return(he);
    }
  } while(TRUE);
  return(NULL);
}
/* \\\ */

/* /// "EnlargeHashArray() */
BOOL EnlargeHashArray(struct HashArray *ha)
{
  struct HashEntry *newarray;
  newarray = (struct HashEntry *) realloc(ha->ha_Array, ((ha->ha_Size<<1)+1) * sizeof(struct HashEntry));
  printf("doubling size!\n");
  if(newarray)
  {
    memset(&newarray[ha->ha_Size], 0, (ha->ha_Size + 1) * sizeof(struct HashEntry));
    ha->ha_Size += ha->ha_Size + 1;
    ha->ha_Array = newarray;
    return(TRUE);
  }
  return(FALSE);
}
/* \\\ */

/* /// "InsertHashEntry()" */
BOOL InsertHashEntry(struct HashArray *ha, ULONG hashkey, ULONG data)
{
  ULONG size = ha->ha_Size;
  ULONG maxnext = (ULONG) sqrt((double) size);
  struct HashEntry *he;
  ULONG tmpkey = hashkey;

  if(ha->ha_Used > (size >> 1)) /* if hash table > 50% full, resize */
  {
    EnlargeHashArray(ha);
    size = ha->ha_Size;
  }
  hashkey++; /* avoid zero */
  do
  {
    he = &ha->ha_Array[tmpkey++ % size];
    if(!he->he_Key)
    {
      he->he_Key = hashkey;
      he->he_Data = data;
      ha->ha_Used++;
      return(TRUE);
    }
  } while(--maxnext);
  /* okay, this is a very bad case of collisions in a row */
  if(ha->ha_InitialSize < ha->ha_Size)
  {
    /* try to clean things up first */
    struct HashEntry *oldarray = ha->ha_Array;
    ULONG cnt;
    
    ha->ha_Array  = (struct HashEntry *) calloc(ha->ha_Size, sizeof(struct HashEntry));
    if(ha->ha_Array)
    {
      ha->ha_InitialSize = ha->ha_Size; /* avoid running into this part a second time */
      /* insert stuff from scratch */
      for(cnt = 0; cnt < ha->ha_Size; cnt++)
      {
        if(oldarray[cnt].he_Key)
        {
        InsertHashEntry(ha, oldarray[cnt].he_Key - 1, oldarray[cnt].he_Data);
        }
      }
      free(oldarray);
      return(InsertHashEntry(ha, hashkey - 1, data));
    }
  } else {
    /* bad. increase size as last resort. */
    if(EnlargeHashArray(ha))
    {
      return(InsertHashEntry(ha, hashkey - 1, data));
    } else {
      /* okay, we tried everything */
      return(FALSE);
    }
  }
  return(FALSE);
}
/* \\\ */
