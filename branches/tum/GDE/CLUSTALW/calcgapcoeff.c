#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "clustalw.h"


/*
 *   Prototypes
 */
void calc_p_penalties(char **aln, sint n, sint fs, sint ls, sint *weight);
void calc_h_penalties(char **aln, sint n, sint fs, sint ls, sint *weight);
sint local_penalty(sint penalty, sint n, sint *pweight, sint *hweight);
/*
 *   Global variables
 */

extern sint gap_dist;
extern sint max_aa;
extern sint debug;
extern Boolean dnaflag;
extern Boolean use_endgaps;
extern Boolean no_hyd_penalties, no_pref_penalties;
extern char hyd_residues[];
extern char *amino_acid_codes;

char   pr[] =     {'A' , 'C', 'D', 'E', 'F', 'G', 'H', 'K', 'I', 'L',
                   'M' , 'N', 'P', 'Q', 'R', 'S', 'T', 'V', 'Y', 'W'};
sint    pas_op[] = { 87, 87,104, 69, 80,139,100,104, 68, 79,
                    71,137,126, 93,128,124,111, 75,100, 77};
sint    pas_op2[] ={ 88, 57,111, 98, 75,126, 95, 97, 70, 90,
                    60,122,110,107, 91,125,124, 81,106, 88};
sint    pal_op[] = { 84, 69,128, 78, 88,176, 53, 95, 55, 49,
                    52,148,147,100, 91,129,105, 51,128, 88};

float reduced_gap = 0.3;
Boolean nhyd_pen,npref_pen; /* local copies of ho_hyd_penalties, no_pref_penalties */
sint gdist;                  /* local copy of gap_dist */

void calc_gap_coeff(char **alignment, sint *gaps, sint **profile, Boolean struct_penalties,
         char *gap_penalty_mask, sint first_seq, sint last_seq,
         sint prf_length, sint gapcoef, sint lencoef)
{

   char c;
   sint i, j;
   sint is, ie;
   static sint numseq,val;
   static sint *gap_pos;
   static sint *p_weight, *h_weight;
   static float scale;
   
   numseq = last_seq - first_seq;

   for (j=0; j<prf_length; j++)
        gaps[j] = 0;
/*
   Check for a gap penalty mask
*/
   if (struct_penalties != NONE)
     {
        nhyd_pen = npref_pen = TRUE;
        gdist = 0;
     }
   else
     {
        nhyd_pen = no_hyd_penalties;
        npref_pen = no_pref_penalties;
        gdist = gap_dist;
     }                  
     
   for (i=first_seq; i<last_seq; i++)
     {
/*
   Include end gaps as gaps ?
*/
        is = 0;
        ie = prf_length;
        if (use_endgaps == FALSE)
        {
          for (j=0; j<prf_length; j++)
            {
              c = alignment[i][j];
              if ((c < 0) || (c > max_aa))
                 is++;
              else
                 break;
            }
          for (j=prf_length-1; j>=0; j--)
            {
              c = alignment[i][j];
              if ((c < 0) || (c > max_aa))
                 ie--;
              else
                 break;
            }
        }

        for (j=is; j<ie; j++)
          {
              if ((alignment[i][j] < 0) || (alignment[i][j] > max_aa))
                 gaps[j]++;
          }
     }

   if ((!dnaflag) && (npref_pen == FALSE))
     {
        p_weight = (sint *) ckalloc( (prf_length+2) * sizeof (sint) );
        calc_p_penalties(alignment, prf_length, first_seq, last_seq, p_weight);
     }

   if ((!dnaflag) && (nhyd_pen == FALSE))
     {
        h_weight = (sint *) ckalloc( (prf_length+2) * sizeof (sint) );
        calc_h_penalties(alignment, prf_length, first_seq, last_seq, h_weight);
     }

   gap_pos = (sint *) ckalloc( (prf_length+2) * sizeof (sint) );
/*
    mark the residues close to an existing gap (set gaps[i] = -ve)
*/
   if (dnaflag || (gdist <= 0))
     {
       for (i=0;i<prf_length;i++) gap_pos[i] = gaps[i];
     }
   else
     {
       i=0;
       while (i<prf_length)
         {
            if (gaps[i] <= 0)
              {
                 gap_pos[i] = gaps[i];
                 i++;
              }
            else 
              {
                 for (j = -gdist+1; j<0; j++)
                  {
                   if ((i+j>=0) && (i+j<prf_length) &&
                       ((gaps[i+j] == 0) || (gaps[i+j] < j))) gap_pos[i+j] = j;
                  }
                 while (gaps[i] > 0)
                    {
                       if (i>=prf_length) break;
                       gap_pos[i] = gaps[i];
                       i++;
                    }
                 for (j = 0; j<gdist; j++)
                  {
                   if (gaps[i+j] > 0) break;
                   if ((i+j>=0) && (i+j<prf_length) && 
                       ((gaps[i+j] == 0) || (gaps[i+j] < -j))) gap_pos[i+j] = -j-1;
                  }
                 i += j;
              }
         }
     }

if (debug>1)
{
fprintf(stdout,"gap open %d gap ext %d\n",(pint)gapcoef,(pint)lencoef);
fprintf(stdout,"gaps:\n");
  for(i=0;i<prf_length;i++) fprintf(stdout,"%d ", (pint)gaps[i]);
  fprintf(stdout,"\n");
fprintf(stdout,"gap_pos:\n");
  for(i=0;i<prf_length;i++) fprintf(stdout,"%d ", (pint)gap_pos[i]);
  fprintf(stdout,"\n");
}

   for (j=0;j<prf_length; j++)
     {
          
        if (gap_pos[j] <= 0)
          {
/*
    apply residue-specific and hydrophilic gap penalties.
*/
	     	if (!dnaflag) {
              	profile[j+1][GAPCOL] = local_penalty(gapcoef, j,
                                                   p_weight, h_weight);
              	profile[j+1][LENCOL] = lencoef;
	     	}
	     	else {
              	profile[j+1][GAPCOL] = gapcoef;
              	profile[j+1][LENCOL] = lencoef;
	     	}

/*
    increase gap penalty near to existing gaps.
*/
             if (gap_pos[j] < 0)
                {
                    profile[j+1][GAPCOL] *= 2.0+2.0*(gdist+gap_pos[j])/gdist;
                }


          }
        else
          {
             scale = ((float)(numseq-gaps[j])/(float)numseq) * reduced_gap;
             profile[j+1][GAPCOL] = scale*gapcoef;
             profile[j+1][LENCOL] = 0.5 * lencoef;
          }
/*
    apply the gap penalty mask
*/
        if (struct_penalties != NONE)
          {
            val = gap_penalty_mask[j]-'0';
            if (val > 0 && val < 10)
              {
                profile[j+1][GAPCOL] *= val;
                profile[j+1][LENCOL] *= val;
              }
          }
/*
   make sure no penalty is zero - even for all-gap positions
*/
        if (profile[j+1][GAPCOL] <= 0) profile[j+1][GAPCOL] = 1;
        if (profile[j+1][LENCOL] <= 0) profile[j+1][LENCOL] = 1;
     }

   profile[0][GAPCOL] = 0;
   profile[0][LENCOL] = 0;

   profile[prf_length][GAPCOL] = 0;
   profile[prf_length][LENCOL] = 0;

if (debug>1)
{
  fprintf(stdout,"Opening penalties:\n");
  for(i=0;i<prf_length;i++) fprintf(stdout," %d:%d ",i, (pint)profile[i+1][GAPCOL]);
  fprintf(stdout,"\n");
}
if (debug>1)
{
  fprintf(stdout,"Extension penalties:\n");
  for(i=0;i<prf_length;i++) fprintf(stdout,"%d:%d ",i, (pint)profile[i+1][LENCOL]);
  fprintf(stdout,"\n");
}
   if ((!dnaflag) && (npref_pen == FALSE))
        p_weight=ckfree((void *)p_weight);

   if ((!dnaflag) && (nhyd_pen == FALSE))
        h_weight=ckfree((void *)h_weight);


   gap_pos=ckfree((void *)gap_pos);
}              
            
void calc_p_penalties(char **aln, sint n, sint fs, sint ls, sint *weight)
{
  char ix;
  sint j,k,numseq;
  sint i;

  numseq = ls - fs;
  for (i=0;i<n;i++)
    {
      weight[i] = 0;
      for (k=fs;k<ls;k++)
        {
           for (j=0;j<22;j++)
             {
                ix = aln[k][i];
                if ((ix < 0) || (ix > max_aa)) continue;
                if (amino_acid_codes[ix] == pr[j])
                  {
                    weight[i] += (200-pas_op[j]);
                    break;
                  }
             }
        }
      weight[i] /= numseq;
    }

}
            
void calc_h_penalties(char **aln, sint n, sint fs, sint ls, sint *weight)
{

/*
   weight[] is the length of the hydrophilic run of residues.
*/
  char ix;
  sint nh,j,k;
  sint i,e,s;
  sint *hyd;
  float scale;

  hyd = (sint *)ckalloc((n+2) * sizeof(sint));
  nh = (sint)strlen(hyd_residues);
  for (i=0;i<n;i++)
     weight[i] = 0;

  for (k=fs;k<ls;k++)
    {
       for (i=0;i<n;i++)
         {
             hyd[i] = 0;
             for (j=0;j<nh;j++)
                {
                   ix = aln[k][i];
                   if ((ix < 0) || (ix > max_aa)) continue;
                   if (amino_acid_codes[ix] == hyd_residues[j])
                      {
                         hyd[i] = 1;
                         break;
                      }
                }
          }
       i = 0;
       while (i < n)
         {
            if (hyd[i] == 0) i++;
            else
              {
                 s = i;
                 while ((hyd[i] != 0) && (i<n)) i++;
                 e = i;
                 if (e-s > 3)
                    for (j=s; j<e; j++) weight[j] += 100;
              }
         }
    }

  scale = ls - fs;
  for (i=0;i<n;i++)
     weight[i] /= scale;

  hyd=ckfree((void *)hyd);

if (debug>1)
{
  for(i=0;i<n;i++) fprintf(stdout,"%d ", (pint)weight[i]);
  fprintf(stdout,"\n");
}

}
            
sint local_penalty(sint penalty, sint n, sint *pweight, sint *hweight)
{

  Boolean h = FALSE;
  float gw;

  if (dnaflag) return(1);

  gw = 1.0;
  if (nhyd_pen == FALSE)
    {
        if (hweight[n] > 0)
         {
           gw *= reduced_gap;
           h = TRUE;
         }
    }
  if ((npref_pen == FALSE) && (h==FALSE))
    {
       gw *= ((float)pweight[n]/100.0);
    }

  gw *= penalty;
  return((sint)gw);

}

