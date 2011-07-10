#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "ptpan.h"
#include "pt_prototypes.h"
#include <struct_man.h>

/* *** FIXME *** do we need this function? */
/* called by:
   PROBE/PT_match.cxx:      pt_export_error(locs,"error: probe too short!!\n");
   PROBE/PT_new_design.cxx: pt_export_error(locs,"No Species selected");
*/

/* /// "SetARBErrorMsg()" */
void SetARBErrorMsg(PT_local *locs, const STRPTR error)
{
  if(locs->ls_error)
  {
    free(locs->ls_error);
  }
  locs->ls_error = strdup(error);
}
/* \\\ */

/* get the name with a virtual function */
/* /// "virt_name()" */
extern "C" STRPTR virt_name(PT_probematch *ml)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  return(pg->pg_SpeciesMap[ml->name]->ps_Name);
}
/* \\\ */

/* /// "virt_fullname()" */
extern "C" STRPTR virt_fullname(PT_probematch *ml)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  return(pg->pg_SpeciesMap[ml->name]->ps_FullName);
}
/* \\\ */

/* called by:
   AISC
*/
extern "C" bytestring *PT_unknown_names(PT_pdc *pdc)
{
  struct PTPanGlobal *pg = PTPanGlobalPtr;
  STRPTR specnames;
  STRPTR namestart;
  UBYTE namechr;
  ULONG allocsize = 2;
  ULONG pos = 0;
  STRPTR tarptr;

  printf("EXTERN: PT_unknown_names\n");
  /* free old string and allocate minimum buffer */
  freeset(pg->pg_UnknownSpecies.data, (STRPTR) malloc(allocsize));
  *pg->pg_UnknownSpecies.data = 0;
  namestart = specnames = pdc->names.data;
  do
  {
    namechr = *specnames;
    /* if we encounter a hash or a nullbyte, we're at the end of a species string */
    if((namechr == '#') || (!namechr))
    {
      if(namestart < specnames) /* don't try to find an empty string */
      {
    /* temporarily terminate the string */
    *specnames = 0;
    if(!GBS_read_hash(pg->pg_SpeciesNameHash, namestart))
    {
    allocsize += (specnames - namestart) + 1;
    pg->pg_UnknownSpecies.data = (STRPTR) realloc(pg->pg_UnknownSpecies.data, allocsize);
    tarptr = &pg->pg_UnknownSpecies.data[pos];
    if(pos)
    {
        *tarptr++ = '#';
    }
    /* copy name */
    while((*tarptr++ = *namestart++))
    {
        pos++;
    }
    }
    /* restore character */
    *specnames++ = namechr;
    namestart = specnames;
      } else {
    namestart = ++specnames;
      }
    } else {
      specnames++;
    }
  } while(namechr);
  pg->pg_UnknownSpecies.size = pos + 1;
  return(&pg->pg_UnknownSpecies);
}
