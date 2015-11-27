/*
 *  MrBayes 3
 *
 *  (c) 2002-2010
 *
 *  John P. Huelsenbeck
 *  Dept. Integrative Biology
 *  University of California, Berkeley
 *  Berkeley, CA 94720-3140
 *  johnh@berkeley.edu
 *
 *  Fredrik Ronquist
 *  Swedish Museum of Natural History
 *  Box 50007
 *  SE-10405 Stockholm, SWEDEN
 *  fredrik.ronquist@nrm.se
 *
 *  With important contributions by
 *
 *  Paul van der Mark (paulvdm@sc.fsu.edu)
 *  Maxim Teslenko (maxim.teslenko@nrm.se)
 *
 *  and by many users (run 'acknowledgements' to see more info)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details (www.gnu.org).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "mb.h"
#include "globals.h"
#include "bayes.h"
#include "best.h"
#include "model.h"
#include "command.h"
#include "mcmc.h"
#include "mbmath.h"
#include "tree.h"
#include "utils.h"

const char* const svnRevisionModelC="$Rev: 513 $";   /* Revision keyword which is expended/updated by svn on each commit/update*/

#if defined(__MWERKS__)
#include "SIOUX.h"
#endif

#ifndef NDEBUG
#include <assert.h>
#endif

#undef	DEBUG_ADDDUMMYCHARS
#undef  DEBUG_CONSTRAINTS
#undef	DEBUG_COMPRESSDATA

/* local prototypes */
int       AddDummyChars (void);
void      AllocateCppEvents (Param *p);
MCMCMove *AllocateMove (MoveType *moveType, Param *param);
int		  AllocateNormalParams (void);
int		  AllocateTreeParams (void);
void      CheckCharCodingType (Matrix *m, CharInfo *ci);
int       CheckExpandedModels (void);
int       CompressData (void);
int		  InitializeChainTrees (Param *p, int from, int to, int isRooted);
int       FillBrlensSubParams (Param *param, int chn, int state);
void      FreeCppEvents (Param *p);
void	  FreeMove (MCMCMove *mv);
void      GetPossibleAAs (int aaCode, int aa[]);
void      GetPossibleNucs (int nucCode, int nuc[]);
void      GetPossibleRestrictionSites (int resSiteCode, int *sites);
int       GetUserTreeFromName (int *index, char *treeName);
void	  InitializeMcmcTrees (Param *p);
int       IsApplicable (Param *param);
int       IsApplicableTreeAgeMove (Param *param);
int		  IsModelSame (int whichParam, int part1, int part2, int *isApplic1, int *isApplic2);
int		  NumActiveParts (void);
int       NumNonExcludedChar (void);
int		  NumStates (int part);
int	      PrintCompMatrix (void);
int	      PrintMatrix (void);
int       ProcessStdChars (SafeLong *seed);
int		  SetModelParams (void);
int       SetPopSizeParam (Param *param, int chn, int state, PolyTree *pt);
int       SetRelaxedClockParam (Param *param, int chn, int state, PolyTree *pt);
int		  SetUpLinkTable (void);
int		  ShowMoves (int used);
int		  ShowParameters (int showStartVals, int showMoves, int showAllAvailable);
int 	  UpdateCppEvolLength (int *nEvents, MrBFlt **pos, MrBFlt **rateMult, MrBFlt *evolLength, TreeNode *p, MrBFlt baseRate);



/* globals */
int				*activeParams[NUM_LINKED];              /* a table holding the parameter link status        */
int				localOutGroup;							/* outgroup for non-excluded taxa				    */
Calibration		**localTaxonCalibration = NULL;			/* stores local taxon calibrations (ages)           */
char			**localTaxonNames = NULL;				/* points to names of non-excluded taxa             */
Model			*modelParams;							/* holds model params								*/
ModelInfo		*modelSettings;							/* stores important info on model params			*/
MCMCMove		**moves;								/* vector of pointers to applicable moves			*/
int				numApplicableMoves;						/* number of moves applicable to model parameters	*/
int				numCurrentDivisions;          			/* number of partitions of data                     */
int				numGlobalChains;						/* number of global chains							*/
int				numLocalTaxa;						    /* number of non-excluded taxa						*/
int				numLocalChar;							/* number of non-excluded characters				*/
int				numParams;								/* number of parameters in model			    	*/
int				numTopologies;						    /* number of topologies for one chain and state	    */
int				numTrees;						        /* number of trees for one chain and state			*/
Param			*params;								/* vector of parameters						 	    */
Param			*printParams;						    /* vector of subst model parameters	to print        */
ShowmovesParams	showmovesParams;						/* holds parameters for Showmoves command           */
Param			*treePrintparams;						/* vector of tree parameters to print               */
int             setUpAnalysisSuccess;                   /* Set to YES if analysis is set without error      */

/* globals used to describe and change the current model; allocated in AllocCharacters and SetPartition */
int         *numVars;                                   /* number of variables in setting arrays         */
int         *activeParts;                               /* partitions changes should apply to            */
int         *linkTable[NUM_LINKED];                     /* how parameters are linked across parts        */
int         *tempLinkUnlink[NUM_LINKED];                /* for changing parameter linkage                */
int         *tempLinkUnlinkVec;                         /* for changing parameter linkage                */
MrBFlt      *tempNum;                                   /* vector of numbers used for setting arrays     */

/* Aamodel parameters */
MrBFlt			aaJones[20][20];	         /* rates for Jones model                        */
MrBFlt			aaDayhoff[20][20];           /* rates for Dayhoff model                      */
MrBFlt			aaMtrev24[20][20];	         /* rates for mtrev24 model                      */
MrBFlt			aaMtmam[20][20];	         /* rates for mtmam model                        */
MrBFlt			aartREV[20][20];             /* rates for rtREV model                        */
MrBFlt			aaWAG[20][20];               /* rates for WAG model                          */
MrBFlt			aacpREV[20][20];             /* rates for aacpREV model                      */
MrBFlt			aaVt[20][20];                /* rates for VT model                           */
MrBFlt			aaBlosum[20][20];            /* rates for Blosum62 model                     */
MrBFlt			jonesPi[20];                 /* stationary frequencies for Jones model       */
MrBFlt			dayhoffPi[20];               /* stationary frequencies for Dayhoff model     */
MrBFlt			mtrev24Pi[20];               /* stationary frequencies for mtrev24 model     */
MrBFlt			mtmamPi[20];                 /* stationary frequencies for mtmam model       */
MrBFlt			rtrevPi[20];                 /* stationary frequencies for rtREV model       */
MrBFlt			wagPi[20];                   /* stationary frequencies for WAG model         */
MrBFlt			cprevPi[20];                 /* stationary frequencies for aacpREV model     */
MrBFlt			vtPi[20];                    /* stationary frequencies for VT model          */
MrBFlt			blosPi[20];                  /* stationary frequencies for Blosum62 model    */


/* parser flags and variables */
int         fromI, toJ, foundDash, foundComma, foundEqual, foundBeta,
            foundAaSetting, foundExp, modelIsFixed, linkNum, foundLeftPar,
            tempNumStates, isNegative;
MrBFlt      tempStateFreqs[200], tempAaModelPrs[10];
char		colonPr[100], clockPr[30];

/* other local variables (this file) */
MrBFlt			empiricalFreqs[200];         /* emprical base frequencies for partition                 */
int				intValsRowSize = 0;	         /* row size of intValues matrix				            */
int			    *intValues = NULL;           /* stores int values of chain parameters                   */
Tree			**mcmcTree;                  /* pointers to trees for mcmc                              */
int				paramValsRowSize = 0;	     /* row size of paramValues matrix				            */
MrBFlt			*paramValues = NULL;         /* stores actual values and subvalues of chain parameters  */
int				*relevantParts = NULL;       /* partitions that are affected by this move               */
Param			**subParamPtrs;		         /* pointer to subparams for topology params                */
int				*stateSize;			         /* # states for each compressed char			            */



/*-----------------------------------------------------------------------
|
|	AddDummyChars: Add dummy characters to relevant partitions
|
------------------------------------------------------------------------*/
int AddDummyChars (void)
{

	int			i, j, k, d, numIncompatible, numDeleted, numStdChars, oldRowSize,
				newRowSize, numDummyChars, newColumn, newChar, oldColumn, oldChar, 
				isCompat, *tempChar, numIncompatibleChars;
	SafeLong	*tempMatrix;
	CLFlt		*tempSitesOfPat;
	ModelInfo	*m;
	ModelParams	*mp;
	CharInfo	cinfo;
	Matrix		matrix;

	extern int	NBits(int x);

	/* set pointers to NULL */
	tempMatrix = NULL;
	tempSitesOfPat = NULL;
	tempChar = NULL;

	/* check how many dummy characters needed in total */
	numDummyChars = 0;
	numStdChars = 0;
	for (d=0; d<numCurrentDivisions; d++)
		{
		m = &modelSettings[d];
		mp = &modelParams[d];

		m->numDummyChars = 0;

		if (mp->dataType == RESTRICTION && !strcmp(mp->parsModel,"No"))
			{
			if (!strcmp(mp->coding, "Variable"))
				m->numDummyChars = 2;
			else if (!strcmp(mp->coding, "Noabsencesites") || !strcmp(mp->coding, "Nopresencesites"))
				m->numDummyChars = 1;
			else if (!strcmp(mp->coding, "Informative"))
				m->numDummyChars = 2 + 2 * numLocalTaxa;
			}

		if (mp->dataType == STANDARD && !strcmp(mp->parsModel,"No"))
			{
			if (!strcmp(mp->coding, "Variable"))
				m->numDummyChars = 2;
			else if (!strcmp(mp->coding, "Informative"))
				m->numDummyChars = 2 + 2 * numLocalTaxa;
			numStdChars += (m->numChars + m->numDummyChars);
			}

		numDummyChars += m->numDummyChars;
		m->numChars += m->numDummyChars;

		}

	/* exit if dummy characters not needed */
	if (numDummyChars == 0)
		return NO_ERROR;

	/* print original compressed matrix */
#	if	0
	MrBayesPrint ("Compressed matrix before adding dummy characters...\n");
	PrintCompMatrix();
#	endif		

	/* set row sizes for old and new matrices */
	oldRowSize = compMatrixRowSize;
	compMatrixRowSize += numDummyChars;
	newRowSize = compMatrixRowSize;
	numCompressedChars += numDummyChars;

	/* allocate space for new data */
	tempMatrix = (SafeLong *) SafeCalloc (numLocalTaxa * newRowSize, sizeof(SafeLong));
	tempSitesOfPat = (CLFlt *) SafeCalloc (numCompressedChars, sizeof(CLFlt));
	tempChar = (int *) SafeCalloc (compMatrixRowSize, sizeof(int));
	if (!tempMatrix || !tempSitesOfPat || !tempChar)
		{
		MrBayesPrint ("%s   Problem allocating temporary variables in AddDummyChars\n", spacer);
		goto errorExit;
		}

	/* initialize indices */
	oldChar = newChar = newColumn = numDeleted = 0;

	/* set up matrix struct */
	matrix.origin = compMatrix;
	matrix.nRows = numLocalTaxa;
	matrix.rowSize = oldRowSize;

	/* loop over divisions */
	for (d=0; d<numCurrentDivisions; d++)
		{
		m = &modelSettings[d];
		mp = &modelParams[d];

		/* insert the dummy characters first for each division */
		if (m->numDummyChars > 0)
			{
			MrBayesPrint("%s   Adding dummy characters (unobserved site patterns) for division %d\n", spacer, d+1);

			if (!strcmp(mp->coding, "Variable") || !strcmp(mp->coding, "Informative"))
				{
				for (k=0; k<2; k++)
					{
					for (i=0; i<numLocalTaxa; i++)
						tempMatrix[pos(i,newColumn,newRowSize)] = (1<<k);
					tempSitesOfPat[newChar] = 0;
					tempChar[newColumn] = -1;
					newChar++;
					newColumn++;
					}
				}

			if (!strcmp(mp->coding, "Informative"))
				{
				for (k=0; k<2; k++)
					{
					for (i=0; i< numLocalTaxa; i++)
						{
						for (j=0; j<numLocalTaxa; j++)
							{
							if(j == i)
								tempMatrix[pos(j,newColumn,newRowSize)] = (1 << k) ^ 3;
							else
								tempMatrix[pos(j,newColumn,newRowSize)] = 1 << k;
							}
						tempSitesOfPat[newChar] = 0;
						tempChar[newColumn] = -1;
						newChar++;
						newColumn++;
						}
					}
				}

			if (!strcmp(mp->coding, "Noabsencesites"))
				{
				for (i=0; i<numLocalTaxa; i++)
					tempMatrix[pos(i,newColumn,newRowSize)] = 1;
				tempSitesOfPat[newChar] = 0;
				tempChar[newColumn] = -1;
				newChar++;
				newColumn++;
				}

			if (!strcmp(mp->coding, "Nopresencesites"))
				{
				for (i=0; i<numLocalTaxa; i++)
					tempMatrix[pos(i,newColumn,newRowSize)] = 2;
				tempSitesOfPat[newChar] = 0;
				tempChar[newColumn] = -1;
				newChar++;
				newColumn++;
				}
			}

		/* add the normal characters */
		numIncompatible = numIncompatibleChars = 0;
		for (oldColumn=m->compMatrixStart; oldColumn<m->compMatrixStop; oldColumn++)
			{
			isCompat = YES;
			/* first check if the character is supposed to be present */
			if (m->numDummyChars > 0)
				{
				/* set up matrix struct */
				matrix.column = oldColumn;
				/* set up charinfo struct */
				cinfo.dType = mp->dataType;
				cinfo.cType = charInfo[origChar[oldChar]].ctype;
				cinfo.nStates = charInfo[origChar[oldChar]].numStates;
				CheckCharCodingType(&matrix, &cinfo);

				if (!strcmp(mp->coding, "Variable") && cinfo.variable == NO)
					isCompat = NO;
				else if (!strcmp(mp->coding, "Informative") && cinfo.informative == NO)
					isCompat = NO;
				else if (!strcmp(mp->coding, "Noabsencesites") && cinfo.constant[0] == YES)
					isCompat = NO;
				else if (!strcmp(mp->coding, "Nopresencesites") && cinfo.constant[1] == YES)
					isCompat = NO;
				}

			if (isCompat == NO)
				{
				numIncompatible++;
				numIncompatibleChars += (int) numSitesOfPat[oldChar];
				oldChar++;
				}
			else
				{
				/* add character */
				for (i=0; i<numLocalTaxa; i++)
					tempMatrix[pos(i,newColumn,newRowSize)] = compMatrix[pos(i,oldColumn,oldRowSize)];
				/* set indices */
				compCharPos[origChar[oldColumn]] = newChar;
				compColPos[origChar[oldColumn]] = newColumn;
				tempSitesOfPat[newChar] = numSitesOfPat[oldChar];
				tempChar[newColumn] = origChar[oldColumn];
				newColumn++;
				if ((oldColumn-m->compMatrixStart+1) % m->nCharsPerSite == 0)
					{
					newChar++;
					oldChar++;
					}
				}
			}

		/* print a warning if there are incompatible characters */
		if (numIncompatible > 0)
			{
			m->numChars -= numIncompatible;
			m->numUncompressedChars -= numIncompatibleChars;
			numDeleted += numIncompatible;
			if (numIncompatibleChars > 1)
				{
				MrBayesPrint ("%s   WARNING: There are %d characters incompatible with the specified\n", spacer, numIncompatibleChars);
				MrBayesPrint ("%s            coding bias. These characters will be excluded.\n", spacer);
				}
			else
				{
				MrBayesPrint ("%s   WARNING: There is one character incompatible with the specified\n", spacer);
				MrBayesPrint ("%s            coding bias. This character will be excluded.\n", spacer);
				}
			}

		/* update division comp matrix and comp char pointers */
		m->compCharStop = newChar;
		m->compMatrixStop = newColumn;
		m->compCharStart = newChar - m->numChars;
		m->compMatrixStart = newColumn - m->nCharsPerSite * m->numChars;

		}	/* next division */

	/* compress matrix if necessary */
	if (numDeleted > 0)
		{
		for (i=k=0; i<numLocalTaxa; i++)
			{
			for (j=0; j<newRowSize-numDeleted; j++)
				{
				tempMatrix[k++] = tempMatrix[j+i*newRowSize];
				}
			}
		numCompressedChars -= numDeleted;
		compMatrixRowSize -= numDeleted;
		}


	/* free old data, set pointers to new data */
	free (compMatrix);
	free (numSitesOfPat);
	free (origChar);
	
	compMatrix = tempMatrix;
	numSitesOfPat = tempSitesOfPat;
	origChar = tempChar;
	
	tempMatrix = NULL;
	tempSitesOfPat = NULL;
	tempChar = NULL;
	
	/* print new compressed matrix */
#	if	defined (DEBUG_ADDDUMMYCHARS)
	MrBayesPrint ("After adding dummy characters...\n");
	PrintCompMatrix();
#	endif		

	return NO_ERROR;

	errorExit:
		if (tempMatrix)
			free (tempMatrix);
		if (tempSitesOfPat)
			free (tempSitesOfPat);
		if (tempChar)
			free (tempChar);

		return ERROR;	
}





/* Allocate space for cpp events */
void AllocateCppEvents (Param *p)
{
    int     i;

    p->nEvents = (int **) SafeCalloc (2*numGlobalChains, sizeof (int *));
	p->nEvents[0] = (int *) SafeCalloc (2*numGlobalChains*(2*numLocalTaxa), sizeof (int));
	for (i=1; i<2*numGlobalChains; i++)
		p->nEvents[i] = p->nEvents[i-1] + (2*numLocalTaxa);
	p->position = (MrBFlt ***) SafeCalloc (2*numGlobalChains, sizeof (MrBFlt **));
	p->position[0] = (MrBFlt **) SafeCalloc (2*numGlobalChains*(2*numLocalTaxa), sizeof (MrBFlt *));
	for (i=1; i<2*numGlobalChains; i++)
		p->position[i] = p->position[i-1] + (2*numLocalTaxa);
	p->rateMult = (MrBFlt ***) SafeCalloc (2*numGlobalChains, sizeof (MrBFlt **));
	p->rateMult[0] = (MrBFlt **) SafeCalloc (2*numGlobalChains*(2*numLocalTaxa), sizeof (MrBFlt *));
	for (i=1; i<2*numGlobalChains; i++)
		p->rateMult[i] = p->rateMult[i-1] + (2*numLocalTaxa);

}





/*----------------------------------------------------------------------------
|
|   AllocateMove: Allocate space for and initialize one applicable move
|
-----------------------------------------------------------------------------*/

MCMCMove *AllocateMove (MoveType *moveType, Param *param)

{
	int			i, j, nameLength;
	char		*partitionDescriptor = "";
	MCMCMove	*temp;

	if ((temp = (MCMCMove *) SafeCalloc (1, sizeof (MCMCMove))) == NULL)
		return (NULL);

	/* calculate length */
    if (strcmp (moveType->paramName, "") == 0)
		nameLength = (int) (strlen (moveType->shortName) + strlen (param->name)) + 10;
	else
		{
		partitionDescriptor = param->name;
		while (*partitionDescriptor != '\0')
			{
			if (*partitionDescriptor == '{')
				break;
			partitionDescriptor++;
			}
		nameLength = (int) (strlen (moveType->shortName) + strlen (moveType->paramName) + strlen (partitionDescriptor)) + 10;
		}
    /* add length of names of subparams */
	if (moveType->subParams == YES)
		{
        for (i=0; i<param->nSubParams; i++)
            nameLength += (int)(strlen(param->subParams[i]->name)) + 1;
		}

    if ((temp->name = (char *) SafeCalloc (nameLength, sizeof (char))) == NULL)
		{
		free (temp);
		return NULL;
		}

	if ((temp->nAccepted = (int *) SafeCalloc (5*numGlobalChains, sizeof (int))) == NULL)
		{
		free (temp->name);
		free (temp);
		return NULL;
		}
	temp->nTried       = temp->nAccepted + numGlobalChains;
	temp->nBatches     = temp->nAccepted + 2*numGlobalChains;
    temp->nTotAccepted = temp->nAccepted + 3*numGlobalChains;
    temp->nTotTried    = temp->nAccepted + 4*numGlobalChains; 
	
	if ((temp->relProposalProb = (MrBFlt *) SafeCalloc (4*numGlobalChains, sizeof (MrBFlt))) == NULL)
		{
		free (temp->nAccepted);
		free (temp->name);
		free (temp);
		return NULL;
		}
	temp->cumProposalProb = temp->relProposalProb + numGlobalChains;
	temp->targetRate = temp->relProposalProb + 2*numGlobalChains;
	temp->lastAcceptanceRate = temp->relProposalProb + 3*numGlobalChains;

	if ((temp->tuningParam = (MrBFlt **) SafeCalloc (numGlobalChains, sizeof (MrBFlt *))) == NULL)
		{
		free (temp->relProposalProb);
		free (temp->nAccepted);
		free (temp->name);
		free (temp);
		return NULL;
		}
	if ((temp->tuningParam[0] = (MrBFlt *) SafeCalloc (moveType->numTuningParams*numGlobalChains, sizeof (MrBFlt))) == NULL)
		{
		free (temp->tuningParam);
		free (temp->relProposalProb);
		free (temp->nAccepted);
		free (temp->name);
		free (temp);
		return NULL;
		}
	for (i=1; i<numGlobalChains; i++)
		temp->tuningParam[i] = temp->tuningParam[0] + (i * moveType->numTuningParams);

	/* set default values */
	if (strcmp(moveType->paramName, "") != 0)
		sprintf (temp->name, "%s(%s%s)", moveType->shortName, moveType->paramName, partitionDescriptor);
    else
        {
        sprintf (temp->name, "%s(%s", moveType->shortName, param->name);
        if (moveType->subParams == YES)
            {
            for (i=0; i<param->nSubParams; i++)
                {
                strcat(temp->name,",");
                strcat(temp->name,param->subParams[i]->name);
                }
            }
        strcat (temp->name,")");
        }
        
	temp->moveType = moveType;
	temp->moveFxn = moveType->moveFxn;
	for (i=0; i<numGlobalChains; i++)
		{
		temp->relProposalProb[i] = moveType->relProposalProb;
		temp->cumProposalProb[i] = 0.0;
		temp->nAccepted[i] = 0;
		temp->nTried[i] = 0;
        temp->nBatches[i] = 0;
        temp->nTotAccepted[i] = 0;
        temp->nTotTried[i] = 0;
        temp->targetRate[i] = moveType->targetRate;
        temp->lastAcceptanceRate[i] = 0.0;
		for (j=0; j<moveType->numTuningParams; j++)
			temp->tuningParam[i][j] = moveType->tuningParam[j];
		}
	return (temp);
}





/*----------------------------------------------------------------------
|
|   AllocateNormalParams: Allocate space for normal parameters
|
-----------------------------------------------------------------------*/
int AllocateNormalParams (void)

{
	int			i, k, nOfParams, nOfIntParams;
	Param		*p;
	
	/* Count the number of param values and subvalues */
	nOfParams = 0;
    nOfIntParams = 0;
	for (k=0; k<numParams; k++)
		{
		nOfParams += params[k].nValues;
		nOfParams += params[k].nSubValues;
        nOfIntParams += params[k].nIntValues;
		}

	/* Set row size and find total number of values */
	paramValsRowSize = nOfParams;
    intValsRowSize = nOfIntParams;
	nOfParams *= (2 * numGlobalChains);
    nOfIntParams *= (2 * numGlobalChains);

	if (memAllocs[ALLOC_PARAMVALUES] == YES)
		{
		paramValues = (MrBFlt *) SafeRealloc ((void *) paramValues, nOfParams * sizeof (MrBFlt));
		for (i=0; i<nOfParams; i++)
			paramValues[i] = 0.0;
        if (nOfIntParams > 0)
            intValues = (int *) SafeRealloc ((void *) intValues, nOfIntParams * sizeof(int));
		}
	else
        {
		paramValues = (MrBFlt *) SafeCalloc (nOfParams, sizeof(MrBFlt));
        if (nOfIntParams > 0)
            intValues = (int *) SafeCalloc (nOfIntParams, sizeof(int));
        else
            intValues = NULL;
        }
	if (!paramValues || (nOfIntParams > 0 && !intValues))
		{
		MrBayesPrint ("%s   Problem allocating paramValues\n", spacer);
        if (paramValues)
            free (paramValues);
        if (intValues)
            free (intValues);
		return ERROR;
		}
	else
		memAllocs[ALLOC_PARAMVALUES] = YES;

	/* set pointers to values for chain 1 state 0            */
	/* this scheme keeps the chain and state values together */
	nOfParams = 0;
    nOfIntParams = 0;
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
        if (p->nValues > 0)
    		p->values = paramValues + nOfParams;
        else
            p->values = NULL;
		nOfParams += p->nValues;
        if (p->nSubValues > 0)
    		p->subValues = paramValues + nOfParams;
        else
            p->subValues = NULL;
		nOfParams += p->nSubValues;
        if (p->nIntValues > 0)
            p->intValues = intValues + nOfIntParams;
        else
            p->intValues = NULL;
        nOfIntParams += p->nIntValues;
		}
	
	/* allocate space for cpp events */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
        if (p->paramType == P_CPPEVENTS)
           AllocateCppEvents(p);
		}

    return (NO_ERROR);
}





/*----------------------------------------------------------------------
|
|   AllocateTreeParams: Allocate space for tree parameters
|
-----------------------------------------------------------------------*/
int AllocateTreeParams (void)

{
	int			i, j, k, n, nOfParams, nOfTrees, isRooted, numSubParamPtrs;
	Param		*p, *q;

	/* Count the number of trees and dated trees */
	/* based on branch length parameters */
	/*	One tree is needed for each brlen parameter.
		A topology may apply to several trees; a topology parameter
		contains pointers to all trees it applies to */
	numTrees = 0;
	numTopologies = 0;
	for (k=0; k<numParams; k++)
		{
		if (params[k].paramType == P_BRLENS)
			numTrees++;
		else if (params[k].paramType == P_TOPOLOGY)
			numTopologies++;
		}

	/* We need to add the trees that do not have any branch lengths */
	/* that is, the pure parsimony model trees, and the species trees */
	for (k=0; k<numParams; k++)
		{
		if (params[k].paramType == P_TOPOLOGY)
			{
			if (!strcmp(modelParams[params[k].relParts[0]].parsModel, "Yes"))
				numTrees++;
			}
        else if (params[k].paramType == P_SPECIESTREE)
            {
            numTopologies++;
            numTrees++;
            }
		}

	/* Finally add subparam pointers for relaxed clock parameters and species trees */
	numSubParamPtrs = 0;
	for (k=0; k<numParams; k++)
		{
        if (params[k].paramType == P_TOPOLOGY && params[k].paramId == TOPOLOGY_SPECIESTREE)
            numSubParamPtrs += 1;
        else if (params[k].paramType == P_BRLENS)
            numSubParamPtrs += 1;
        else if (params[k].paramType == P_CPPEVENTS)
			numSubParamPtrs += 3;
		else if (params[k].paramType == P_TK02BRANCHRATES)
			numSubParamPtrs += 2;
		else if (params[k].paramType == P_IGRBRANCHLENS)
			numSubParamPtrs += 2;
		}
		
	/* Allocate space for trees and subparam pointers */
	if (memAllocs[ALLOC_MCMCTREES] == YES)
		{
		free (subParamPtrs);
        free (mcmcTree);
        subParamPtrs = NULL;
        mcmcTree = NULL;
        memAllocs[ALLOC_MCMCTREES] = NO;
		}
	subParamPtrs = (Param **) SafeCalloc (numSubParamPtrs, sizeof (Param *));
	mcmcTree = (Tree **) SafeCalloc (numTrees * 2 * numGlobalChains, sizeof (Tree *));
	if (!subParamPtrs || !mcmcTree)
		{
		if (subParamPtrs) free (subParamPtrs);
		if (mcmcTree) free (mcmcTree);
        subParamPtrs = NULL;
        mcmcTree = NULL;
		MrBayesPrint ("%s   Problem allocating mcmc trees\n", spacer);
		return (ERROR);
		}
	else
		memAllocs[ALLOC_MCMCTREES] = YES;

	/* Initialize number of subparams, just in case */
	for (k=0; k<numParams; k++)
		params[k].nSubParams = 0;
	
	/* Count number of trees (brlens) for each topology or species tree */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (params[k].paramType == P_BRLENS)
			{
			q = modelSettings[p->relParts[0]].topology;
			q->nSubParams++;
			if (p->printParam == YES)
				q->nPrintSubParams++;
			}
        else if (params[k].paramType == P_TOPOLOGY)
            {
            q = modelSettings[p->relParts[0]].speciesTree;
            if (q != NULL)
                q->nSubParams++;
            }
		}

	/* Make sure there is also one subparam for a parsimony tree */
	for (k=0; k<numParams; k++)
		if (params[k].paramType == P_TOPOLOGY)
			{
			p = &params[k];
			if (p->nSubParams == 0)
				p->nSubParams = 1;
			}

	/* Count subparams for relaxed clock parameters */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType == P_CPPEVENTS)
			{
			q = modelSettings[p->relParts[0]].cppRate;
			q->nSubParams++;
            q = modelSettings[p->relParts[0]].cppMultDev;
			q->nSubParams++;
			q = modelSettings[p->relParts[0]].brlens;
			q->nSubParams++;
			}
		else if (p->paramType == P_TK02BRANCHRATES)
			{
			q = modelSettings[p->relParts[0]].tk02var;
			q->nSubParams++;
			q = modelSettings[p->relParts[0]].brlens;
			q->nSubParams++;
			}
		else if (p->paramType == P_IGRBRANCHLENS)
			{
			q = modelSettings[p->relParts[0]].igrvar;
			q->nSubParams++;
			q = modelSettings[p->relParts[0]].brlens;
			q->nSubParams++;
			}
		}

	/* set pointers to subparams */
	nOfParams = 0;
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->nSubParams > 0)
			{
			p->subParams = subParamPtrs + nOfParams;
			nOfParams += p->nSubParams;
			}
		}
    assert (nOfParams == numSubParamPtrs);

	/* Set brlens param pointers and tree values */
	/* the scheme below keeps trees for the same state and chain together */
	nOfTrees = 0;
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if ((p->paramType == P_BRLENS) ||
			(p->paramType == P_TOPOLOGY && p->paramId == TOPOLOGY_PARSIMONY_UNIFORM) ||
			(p->paramType == P_TOPOLOGY && p->paramId == TOPOLOGY_PARSIMONY_CONSTRAINED) ||
            (p->paramType == P_SPECIESTREE))
			{
			/* allocate space for trees and initialize trees */
			p->treeIndex = nOfTrees;
			p->tree = mcmcTree + nOfTrees;
			nOfTrees++;
			}
		}

	/* Set topology params and associated brlen subparams */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType == P_TOPOLOGY)
			{
			if (p->paramId == TOPOLOGY_PARSIMONY_UNIFORM ||
				p->paramId == TOPOLOGY_PARSIMONY_CONSTRAINED)
				/* pure parsimony topology case */
				{
				/* there is no brlen subparam */
				/* so let subparam point to the param itself */
				q = p->subParams[0] = p;
				/* p->tree and p->treeIndex have been set above */
				}
			else
				{
				/* first set brlens pointers for any parsimony partitions */
				for (i=j=0; i<p->nRelParts; i++)
					{
					if (modelSettings[p->relParts[i]].parsModelId == YES)
						{
						modelSettings[p->relParts[i]].brlens = p;
						}
					}

				/* now proceed with pointer assignment */
				q = modelSettings[p->relParts[0]].brlens;
				n = 0;	/* number of stored subParams */
				i = 0;	/* relevant partition number  */
				while (i < p->nRelParts)
					{
					for (j=0; j<n; j++)
						if (q == p->subParams[j])
							break;
					
					if (j == n && q != p)	/* a new tree (brlens) for this topology */
						{
						p->subParams[n++] = q;
						}
					q = modelSettings[p->relParts[++i]].brlens;
					}
				
				p->tree = p->subParams[0]->tree;
				p->treeIndex = p->subParams[0]->treeIndex;
				}
			}
		else if (p->paramType == P_SPECIESTREE)
			{
			/* now proceed with pointer assignment */
			q = modelSettings[p->relParts[0]].topology;
			n = 0;	/* number of stored subParams */
			i = 0;	/* relevant partition number  */
			while (i < p->nRelParts)
				{
				for (j=0; j<n; j++)
					if (q == p->subParams[j])
						break;

				if (j == n && q != p)	/* a new topology for this species tree */
					{
					p->subParams[n++] = q;
					}
				q = modelSettings[p->relParts[++i]].topology;
				}
			}
		}

	/* Check for constraints */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType == P_TOPOLOGY)
			{
			if (!strcmp(modelParams[p->relParts[0]].topologyPr, "Constraints"))
				{
				for (i=0; i<p->nSubParams; i++)
					{
					q = p->subParams[i];
					q->checkConstraints = YES;
					}
				}
			else
				{
				for (i=0; i<p->nSubParams; i++)
					{
					q = p->subParams[i];
					q->checkConstraints = NO;
					}
				}
			}
		}

	/* update paramId */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType == P_TOPOLOGY)
			{
			if (p->nSubParams > 1)
				{
				if (p->paramId == TOPOLOGY_NCL_UNIFORM_HOMO)
				    p->paramId = TOPOLOGY_NCL_UNIFORM_HETERO;
				else if	(p->paramId == TOPOLOGY_NCL_CONSTRAINED_HOMO)
				    p->paramId = TOPOLOGY_NCL_CONSTRAINED_HETERO;
				else if	(p->paramId == TOPOLOGY_NCL_FIXED_HOMO)
				    p->paramId = TOPOLOGY_NCL_FIXED_HETERO;
                else
                    {
                    MrBayesPrint ("%s   A clock tree cannot have more than one set of branch lengths\n", spacer);
		    printf("nparam:%d paramid:%d",p->nSubParams,p->paramId);
                    return (ERROR);
                    }
				}
			}
		}

	/* finally initialize trees */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType == P_BRLENS)
			{
			/* find type of tree */
			if (!strcmp(modelParams[p->relParts[0]].brlensPr,"Clock"))
				isRooted = YES;
			else
				isRooted = NO;

			if (InitializeChainTrees (p, 0, numGlobalChains, isRooted) == ERROR)
				return (ERROR);
			}
        else if (p->paramType == P_SPECIESTREE)
            {
            if (InitializeChainTrees (p, 0, numGlobalChains, YES) == ERROR)
                return (ERROR);
            }
		}

	/* now initialize subparam pointers for relaxed clock models */
	/* use nSubParams to point to the next available subParam by first
	   resetting all nSubParams to 0 */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType == P_CPPRATE ||
			p->paramType == P_CPPMULTDEV ||
			p->paramType == P_BRLENS ||
            p->paramType == P_TK02VAR ||
            p->paramType == P_IGRVAR)
			p->nSubParams = 0;
		}
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType == P_CPPEVENTS)
			{
			q = modelSettings[p->relParts[0]].cppRate;
			q->subParams[q->nSubParams++] = p;
			q = modelSettings[p->relParts[0]].cppMultDev;
			q->subParams[q->nSubParams++] = p;
			q = modelSettings[p->relParts[0]].brlens;
			q->subParams[q->nSubParams++] = p;
			p->treeIndex = q->treeIndex;
			p->tree = q->tree;
			if (p->printParam == YES)
				q->nPrintSubParams++;
			}
		else if (p->paramType == P_TK02BRANCHRATES)
			{
			q = modelSettings[p->relParts[0]].tk02var;
			q->subParams[q->nSubParams++] = p;
			q = modelSettings[p->relParts[0]].brlens;
			q->subParams[q->nSubParams++] = p;
			p->treeIndex = q->treeIndex;
			p->tree = q->tree;
			if (p->printParam == YES)
				q->nPrintSubParams++;
			}
		else if (p->paramType == P_IGRBRANCHLENS)
			{
			q = modelSettings[p->relParts[0]].igrvar;
			q->subParams[q->nSubParams++] = p;
			q = modelSettings[p->relParts[0]].brlens;
			q->subParams[q->nSubParams++] = p;
			p->treeIndex = q->treeIndex;
			p->tree = q->tree;
			if (p->printParam == YES)
				q->nPrintSubParams++;
			}
		}

    return (NO_ERROR);
}





int AreDoublesEqual (MrBFlt x, MrBFlt y, MrBFlt tol)

{

	if ((x - y) < -tol || (x - y) > tol)
		return (NO);
	else
		return (YES);
	
}





int ChangeNumChains (int from, int to)

{
	int			i, i1, j, k, m, st, nRuns, fromIndex, toIndex, run, chn, *tempIntVals, nCppEventParams, *toEvents, *fromEvents;
	MCMCMove	**tempMoves, *fromMove, *toMove;
	Tree		**tempTrees;
	MrBFlt		*tempVals, **toRateMult, **toPosition, **fromRateMult, **fromPosition, *stdStateFreqsOld;
    Param       *p, *q, *cppEventParams = NULL;
	Tree		**oldMcmcTree, *tree;

    if(from == to)
        return (NO_ERROR);

	/* set new number of chains */
	chainParams.numChains = to;
	nRuns = chainParams.numRuns;
	numGlobalChains = chainParams.numRuns * chainParams.numChains;

	/* Do the normal parameters */	
	/* first save old values */
	tempVals = paramValues;
	paramValues = NULL;
    tempIntVals = intValues;
    intValues = NULL;
	memAllocs[ALLOC_PARAMS] = NO;
    /* .. and old cpp events parameters */
    nCppEventParams = 0;
    for (i=0; i<numParams; i++)
        {
        p = &params[i];
        if (p->paramType == P_CPPEVENTS)
            nCppEventParams++;
        }
    cppEventParams = (Param *) SafeCalloc (nCppEventParams, sizeof(Param));
    for (i=0; i<nCppEventParams; i++)
        {
        cppEventParams[i].paramType = P_CPPEVENTS;
        AllocateCppEvents (&cppEventParams[i]);
        }
    for (i=j=0; i<numParams; i++)
        {
        p = &params[i];
        if (p->paramType == P_CPPEVENTS)
            {
            cppEventParams[j].nEvents = p->nEvents;
            p->nEvents = NULL;
            cppEventParams[j].position = p->position;
            p->position = NULL;
            cppEventParams[j].rateMult = p->rateMult;
            p->rateMult = NULL;
            j++;
            }
        }
	if (AllocateNormalParams () == ERROR)
		return (ERROR);

    /* then fill all params */
	FillNormalParams (&globalSeed, 0, numGlobalChains);
	
	/* finally overwrite with old values if present */
	for (run=0; run<nRuns; run++)
		{
		for (chn=0; chn<from; chn++)
			{
			if (chn < to)
				{
				fromIndex = (run*from + chn)*2*paramValsRowSize;
				toIndex = (run*to + chn)*2*paramValsRowSize;
				for (i=0; i<2*paramValsRowSize; i++)
					paramValues[toIndex++] = tempVals[fromIndex++];
				fromIndex = (run*from + chn)*2*intValsRowSize;
				toIndex = (run*to + chn)*2*intValsRowSize;
				for (i=0; i<2*intValsRowSize; i++)
					intValues[toIndex++] = tempIntVals[fromIndex++];
                for (i=i1=0; i<numParams; i++)
                    {
                    p = &params[i];
                    if (p->paramType == P_CPPEVENTS)
                        {
                        fromIndex = 2*(run*from + chn);
                        toIndex = 2*(run*to + chn);
                        fromEvents = cppEventParams[i1].nEvents[fromIndex];
                        toEvents = p->nEvents[toIndex];
                        fromPosition = cppEventParams[i1].position[fromIndex];
                        toPosition = p->position[toIndex];
                        fromRateMult = cppEventParams[i1].rateMult[fromIndex];
                        toRateMult = p->rateMult[toIndex];
                        for (j=0; j<2*numLocalTaxa; j++)
                            {
                            toEvents[j] = fromEvents[j];
                            toPosition[j] = (MrBFlt *) SafeRealloc ((void *)toPosition[j], toEvents[j]*sizeof(MrBFlt));
                            toRateMult[j] = (MrBFlt *) SafeRealloc ((void *)toRateMult[j], toEvents[j]*sizeof(MrBFlt));
                            for (k=0; k<toEvents[j]; k++)
                                {
                                toPosition[j][k] = fromPosition[j][k];
                                toRateMult[j][k] = fromRateMult[j][k];
                                }
                            }
                        i1++;
                        }
                    }
                assert( nCppEventParams==i1 );
				}
			}
		}
	
	/* and free up space */
	free (tempVals);
    if (intValsRowSize > 0)
        free (tempIntVals);
    for (i=0; i<nCppEventParams; i++)
        {
        numGlobalChains = chainParams.numRuns * from; /* Revert to the old value to clean old Cpp events in FreeCppEvents() */
        FreeCppEvents(&cppEventParams[i]);
        numGlobalChains = chainParams.numRuns * chainParams.numChains; /*Set to proper value again*/
        }
    if (nCppEventParams > 0)
        free (cppEventParams);

	/* then do the trees (they cannot be done before the parameters because otherwise FillTreeParams will overwrite
       relaxed clock parameters that need to be saved) */

    /* reallocate trees */
	tempTrees = (Tree **) SafeCalloc (2*nRuns*from*numTrees, sizeof(Tree *));
    for (i=0; i<2*nRuns*from*numTrees; i++)
        tempTrees[i] = mcmcTree[i];
	oldMcmcTree = mcmcTree;
	mcmcTree = (Tree **) SafeRealloc ((void *)(mcmcTree), (size_t)(2*numGlobalChains*numTrees*sizeof(Tree*)));
    for (i=0; i<2*nRuns*to*numTrees; i++)
        mcmcTree[i] = NULL;

    /* move the old trees over */
    for (run=0; run<nRuns; run++)
        {
        for (chn=0; chn<from; chn++)
            {
            if (chn >= to)
                continue;
            /*Here we move only one tree per chain/state?! Should not we move numTrees??*/
            fromIndex = 2*(run*from + chn)  * numTrees;
            toIndex   = 2*(run*to   + chn)  * numTrees;
            for(k=0;k<2*numTrees;k++)
                {
                mcmcTree[toIndex+k]    = tempTrees[fromIndex+k];
                tempTrees[fromIndex+k] = NULL;
                }
            }
        }

    /* remove any remaining old trees */
    for (i=0; i<2*nRuns*from*numTrees; i++)
        if (tempTrees[i] != NULL)
			FreeTree (tempTrees[i]);
    free (tempTrees);

	/* now fill in the tree parameters */
    for (i=0; i<numParams; i++)
		{
		p = &params[i];
		if (p->paramType == P_TOPOLOGY)
			{
			p->tree += (mcmcTree - oldMcmcTree);	/* calculate new address */
			for (j=0; j<p->nSubParams; j++)
				{
				q = p->subParams[j];
                assert( q->paramType==P_BRLENS );
                //assert( q->paramType != P_CPPEVENTS && q->paramType != TK02BRANCHRATES);
				q->tree += (mcmcTree - oldMcmcTree);	/* calculate new address */
				if (to > from)
					for (run=0; run<nRuns; run++)
					    {
                        /*rename old trees because each run has more chains*/
                        for (m=0; m<from; m++)
		                    {
		                    for (st=0; st<2; st++)
			                    {
			                    tree = GetTree (q,run*to + m, st);
			                    if (numTrees > 1)
				                    sprintf (tree->name, "mcmc.tree%d_%d", p->treeIndex+1, run*to + m +1);
			                    else /* if (numTrees == 1) */
				                    sprintf (tree->name, "mcmc.tree_%d", run*to + m +1);
                                }
                            }
						InitializeChainTrees (q, run*to + from, run*to + to , GetTree (q, 0, 0)->isRooted);
						}
				}
			}
        else if (p->paramType == P_CPPEVENTS || p->paramType == P_TK02BRANCHRATES || p->paramType == P_IGRBRANCHLENS)
            p->tree += (mcmcTree - oldMcmcTree);
        else
            assert( p->paramType==P_BRLENS || p->tree==NULL );
		}

    
	/* fill new tree parameters */
	if (to > from)
        {
	    for (run=0; run<nRuns; run++)
            {
            for (chn=from; chn<to; chn++)
                {
                toIndex = run*to + chn;
                FillTreeParams (&globalSeed, toIndex, toIndex+1);
                }
            }
        }


    	/* fix stationary frequencies for standard data */
	if(   stdStateFreqsRowSize > 0 )
		{
		assert(memAllocs[ALLOC_STDSTATEFREQS] == YES);
		stdStateFreqsOld=stdStateFreqs;
		stdStateFreqs = (MrBFlt *) SafeMalloc ((size_t)stdStateFreqsRowSize * 2 * numGlobalChains * sizeof (MrBFlt));
		if (!stdStateFreqs)
			{
			MrBayesPrint ("%s   Problem reallocating stdStateFreqs\n", spacer);
			return (ERROR);
			}

        /* set pointers */
		for (k=0; k<numParams; k++)
			{
			p = &params[k];
			if (p->paramType != P_PI || modelParams[p->relParts[0]].dataType != STANDARD)
				continue;
			p->stdStateFreqs += stdStateFreqs-stdStateFreqsOld;
			}
        
        for (run=0; run<nRuns; run++)
            {
            /* copy old chains values*/
            for (chn=0; chn<from; chn++)
                {
                if (chn >= to)
                    break;

                fromIndex = 2*(run*from + chn)*stdStateFreqsRowSize;
                toIndex = 2*(run*to + chn)*stdStateFreqsRowSize;
                for(k=0;k<2*stdStateFreqsRowSize;k++)
                    {
                    stdStateFreqs[toIndex+k]=stdStateFreqsOld[fromIndex+k];
                    }
                }
            /* set new chains */
		    FillStdStateFreqs( run*to+from, run*to+to, &globalSeed);
            }
        free(stdStateFreqsOld);
	}
    

	/* Do the moves */
	/* first allocate space and set up default moves */
	tempMoves = moves;
	moves = NULL;
	memAllocs[ALLOC_MOVES] = NO;
	SetMoves ();
	
	/* then overwrite with old values if present */
	for (i=0; i<numApplicableMoves; i++)
		{
		toMove = moves[i];
		fromMove = tempMoves[i];
		for (run=0; run<nRuns; run++)
			{
			for (chn=0; chn<from; chn++)
				{
				if (chn < to)
					{
					fromIndex = run*from + chn;
					toIndex = run*to + chn;
					toMove->relProposalProb[toIndex] = fromMove->relProposalProb[fromIndex];
					for (j=0; j<toMove->moveType->numTuningParams; j++)
						{
						toMove->tuningParam[toIndex][j] = fromMove->tuningParam[fromIndex][j];
						}
					}
				}
			}
		}
	
	/* and free up space */
	for (i=0; i<numApplicableMoves; i++)
		FreeMove (tempMoves[i]);
	free (tempMoves);
	
	return (NO_ERROR);
}





int ChangeNumRuns (int from, int to)

{
	int			i, i1, j, k, n, nChains;
	Param		*p, *q;
	MoveType	*mvt;
	Tree		**oldMcmcTree;
	MrBFlt		*oldParamValues;
	MrBFlt		*stdStateFreqsOld;
    int         *oldintValues;

    if(from == to)
        return (NO_ERROR);

#if 0
    for (i=0; i<numParams; i++)
        {
        p = &params[i];
        if (p->paramType == P_CPPEVENTS)
            {
            printf ("Trees before changing number of runs\n");
            for (j=0; j<numGlobalChains; j++)
                {
                printf ("Event tree for chain %d\n", j+1);
                for (k=0; k<2*numLocalTaxa; k++)
                    {
                    printf ("%d -- %d:", k, p->nEvents[2*j][k]);
                    for (i1=0; i1<p->nEvents[2*j][k]; i1++)
                        {
                        if (i1 == 0)
                            printf ("( %lf %lf,", p->position[2*j][k], p->rateMult[2*j][k]);
                        else if (i1 == p->nEvents[2*j][k]-1)
                            printf (" %lf %lf)", p->position[2*j][k], p->rateMult[2*j][k]);
                        else
                            printf (" %lf %lf,", p->position[2*j][k], p->rateMult[2*j][k]);
                        }
                    printf("\n");
                    }
                for (k=0; k<2*numLocalTaxa; k++)
                    {
                    printf ("%d -- %d:", k, p->nEvents[2*j][k]);
                    for (i1=0; i1<p->nEvents[2*j+1][k]; i1++)
                        {
                        if (i1 == 0)
                            printf ("( %lf %lf,", p->position[2*j+1][k], p->rateMult[2*j+1][k]);
                        else if (i1 == p->nEvents[2*j][k]-1)
                            printf (" %lf %lf)", p->position[2*j+1][k], p->rateMult[2*j+1][k]);
                        else
                            printf (" %lf %lf,", p->position[2*j+1][k], p->rateMult[2*j+1][k]);
                        }
                    printf("\n");
                    }
                }
            }
        }
#endif

	/* set new number of runs */
	chainParams.numRuns = to;
	nChains = chainParams.numChains;
	numGlobalChains = chainParams.numRuns * chainParams.numChains;

	/* do the trees, free tree's memory if we reduce number of trees. */
	for (i=to*2*nChains*numTrees; i<from*2*nChains*numTrees; i++)
		{
		FreeTree (mcmcTree[i]);
		}
	oldMcmcTree = mcmcTree;
	mcmcTree = (Tree **) SafeRealloc ((void *) mcmcTree, numTrees * 2 * numGlobalChains * sizeof (Tree *));
	if (mcmcTree == NULL)
		{
		memAllocs[ALLOC_MCMCTREES] = NO;
		MrBayesPrint ("%s   Problem reallocating mcmcTree\n", spacer);
		return (ERROR);
		}

  	for (i=from*2*nChains*numTrees; i<to*2*nChains*numTrees; i++)
		{
		mcmcTree[i]=NULL;
		}  
    /* then the cppevents parameters */
    for (i1=0; i1<numParams; i1++)
        {
        p = &params[i1];
        if (p->paramType == P_CPPEVENTS)
            {
            p->nEvents = (int **) SafeRealloc ((void *)p->nEvents, 2*numGlobalChains*sizeof (int *));
	        p->nEvents[0] = (int *) SafeRealloc ((void *)p->nEvents[0], 2*numGlobalChains*(2*numLocalTaxa)*sizeof (int));
	        for (i=1; i<2*numGlobalChains; i++)
		        p->nEvents[i] = p->nEvents[i-1] + (2*numLocalTaxa);
            if (from > to)
                {
                for (j=numGlobalChains; j<from*nChains; j++)
                    {
                    for (k=0; k<2*numLocalTaxa; k++)
                        {
                        free (p->position[2*j+0][k]);
                        p->position[2*j+0][k] = NULL;
                        free (p->rateMult[2*j+0][k]);
                        p->rateMult[2*j+0][k] = NULL;
                        }
                    }
                }
        	p->position = (MrBFlt ***) SafeRealloc ((void *)p->position, 2*numGlobalChains*sizeof (MrBFlt **));
            p->position[0] = (MrBFlt **) SafeRealloc ((void *)p->position[0], 2*numGlobalChains*(2*numLocalTaxa)*sizeof (MrBFlt *));
        	for (i=1; i<2*numGlobalChains; i++)
		        p->position[i] = p->position[i-1] + (2*numLocalTaxa);
        	p->rateMult = (MrBFlt ***) SafeRealloc ((void *)p->rateMult, 2*numGlobalChains*sizeof (MrBFlt **));
        	p->rateMult[0] = (MrBFlt **) SafeRealloc ((void *)p->rateMult[0], 2*numGlobalChains*(2*numLocalTaxa)*sizeof (MrBFlt *));
        	for (i=1; i<2*numGlobalChains; i++)
		        p->rateMult[i] = p->rateMult[i-1] + (2*numLocalTaxa);
            if (to > from)
                {
                for (j=from*nChains; j<numGlobalChains; j++)
                    {
                    for (k=0; k<2*numLocalTaxa; k++)
                        {
                        p->nEvents[2*j+0][k] = 0;
                        p->position[2*j+0][k] = NULL;
                        p->rateMult[2*j+0][k] = NULL;
                        p->nEvents[2*j+1][k] = 0;
                        p->position[2*j+1][k] = NULL;
                        p->rateMult[2*j+1][k] = NULL;
                        }
                    }
                }
            }
        }
	/* and finally the normal parameters */
	oldParamValues = paramValues;
	paramValues = (MrBFlt *) SafeRealloc ((void *) paramValues, paramValsRowSize * 2 * numGlobalChains * sizeof (MrBFlt));
    oldintValues = intValues;
    intValues = (int *) SafeRealloc ((void *) intValues, intValsRowSize * 2 * numGlobalChains * sizeof (int));
	if (paramValues == NULL)
		{
		memAllocs[ALLOC_PARAMVALUES] = NO;
		MrBayesPrint ("%s   Problem reallocating paramValues\n", spacer);
		return (ERROR);
		}
	for (i=0; i<numParams; i++)
		{
		params[i].values += (paramValues - oldParamValues);
		params[i].subValues += (paramValues - oldParamValues);
        params[i].intValues += (intValues - oldintValues);
		}

    /* fill new chains paramiters with appropriate values */
    if (to > from)
		FillNormalParams (&globalSeed, from*nChains, to*nChains);

	/* now fill in the tree parameters */
    for (i=0; i<numParams; i++)
		{
		p = &params[i];
		if (p->paramType == P_TOPOLOGY)
			{
			p->tree += (mcmcTree - oldMcmcTree);	/* calculate new address */
			for (j=0; j<p->nSubParams; j++)
				{
				q = p->subParams[j];
                assert( q->paramType==P_BRLENS );
				q->tree += (mcmcTree - oldMcmcTree);	/* calculate new address */
				InitializeChainTrees (q, from*nChains, to*nChains, GetTree (q, 0, 0)->isRooted);
				}
			}
        else if (p->paramType == P_CPPEVENTS || p->paramType == P_TK02BRANCHRATES || p->paramType == P_IGRBRANCHLENS)
            p->tree += (mcmcTree - oldMcmcTree);
		}

	FillTreeParams (&globalSeed, from*nChains, to*nChains);


	/* fix stationary frequencies for standard data */
	if(   stdStateFreqsRowSize > 0 )
		{
		assert(memAllocs[ALLOC_STDSTATEFREQS] == YES);
		stdStateFreqsOld=stdStateFreqs;
		stdStateFreqs = (MrBFlt *) SafeRealloc ((void *) stdStateFreqs, stdStateFreqsRowSize * 2 * numGlobalChains * sizeof (MrBFlt));
		if (!stdStateFreqs)
			{
			MrBayesPrint ("%s   Problem reallocating stdStateFreqs\n", spacer);
			return (ERROR);
			}
		
		/* set pointers */
		for (k=n=0; k<numParams; k++)
			{
			p = &params[k];
			if (p->paramType != P_PI || modelParams[p->relParts[0]].dataType != STANDARD)
				continue;
			p->stdStateFreqs += stdStateFreqs-stdStateFreqsOld;
			}
		
		FillStdStateFreqs( from*nChains, to*nChains, &globalSeed);
	}


	/* do the moves */
	for (i=0; i<numApplicableMoves; i++)
		{
		mvt = moves[i]->moveType;
		moves[i]->tuningParam = (MrBFlt **) SafeRealloc ((void *) moves[i]->tuningParam, (size_t) (numGlobalChains * sizeof (MrBFlt *)));
		moves[i]->tuningParam[0] = (MrBFlt *) SafeRealloc ((void *) moves[i]->tuningParam[0], (size_t) (numGlobalChains * mvt->numTuningParams * sizeof (MrBFlt)));
		for (j=1; j<numGlobalChains; j++)
			moves[i]->tuningParam[j] = moves[i]->tuningParam[0] + j * mvt->numTuningParams;
		moves[i]->relProposalProb = (MrBFlt *) SafeRealloc ((void *) moves[i]->relProposalProb, (size_t) (4 * numGlobalChains * sizeof (MrBFlt)));
		moves[i]->cumProposalProb = moves[i]->relProposalProb + numGlobalChains;
        moves[i]->targetRate = moves[i]->relProposalProb + 2*numGlobalChains;
        moves[i]->lastAcceptanceRate = moves[i]->relProposalProb + 3*numGlobalChains;
		moves[i]->nAccepted = (int *) SafeRealloc ((void *) moves[i]->nAccepted, (size_t) (5 * numGlobalChains * sizeof (int)));
		moves[i]->nTried = moves[i]->nAccepted + numGlobalChains;
		moves[i]->nBatches = moves[i]->nAccepted + 2*numGlobalChains;
        moves[i]->nTotAccepted = moves[i]->nAccepted + 3*numGlobalChains;
        moves[i]->nTotTried    = moves[i]->nAccepted + 4*numGlobalChains;
		/* initialize all values to default */
		for (j=0; j<numGlobalChains; j++)
			{
			moves[i]->nAccepted[j] = 0;
			moves[i]->nTried[j] = 0;
            moves[i]->nBatches[j] = 0;
            moves[i]->nTotAccepted[j] = 0;
            moves[i]->nTotTried[j] = 0;
			moves[i]->relProposalProb[j] = mvt->relProposalProb;
			moves[i]->cumProposalProb[j] = 0.0;
            moves[i]->lastAcceptanceRate[j] = 0.0;
			for (k=0; k<mvt->numTuningParams; k++)
				moves[i]->tuningParam[j][k] = mvt->tuningParam[k];
            moves[i]->targetRate[j] = mvt->targetRate;
			}
		}


#if 0
    for (i=0; i<numParams; i++)
        {
        p = &params[i];
        if (p->paramType == P_CPPEVENTS)
            {
            printf ("Trees after changing number of runs\n");
            for (j=0; j<numGlobalChains; j++)
                {
                printf ("Event tree for chain %d\n", j+1);
                for (k=0; k<2*numLocalTaxa; k++)
                    {
                    printf ("%d -- %d:", k, p->nEvents[2*j][k]);
                    assert (p->nEvents[2*j] >= 0);
                    for (i1=0; i1<p->nEvents[2*j][k]; i1++)
                        {
                        if (i1 == 0)
                            printf ("( %lf %lf,", p->position[2*j][k], p->rateMult[2*j][k]);
                        else if (i1 == p->nEvents[2*j][k]-1)
                            printf (" %lf %lf)", p->position[2*j][k], p->rateMult[2*j][k]);
                        else
                            printf (" %lf %lf,", p->position[2*j][k], p->rateMult[2*j][k]);
                        }
                    printf("\n");
                    }
                for (k=0; k<2*numLocalTaxa; k++)
                    {
                    printf ("%d -- %d:", k, p->nEvents[2*j+1][k]);
                    assert (p->nEvents[2*j+1] >= 0);
                    for (i1=0; i1<p->nEvents[2*j+1][k]; i1++)
                        {
                        if (i1 == 0)
                            printf ("( %lf %lf,", p->position[2*j+1][k], p->rateMult[2*j+1][k]);
                        else if (i1 == p->nEvents[2*j][k]-1)
                            printf (" %lf %lf)", p->position[2*j+1][k], p->rateMult[2*j+1][k]);
                        else
                            printf (" %lf %lf,", p->position[2*j+1][k], p->rateMult[2*j+1][k]);
                        }
                    printf ("\n");
                    }
                }
            }
        }
#endif

    return (NO_ERROR);
}





/*-----------------------------------------------------------
|
|	CheckCharCodingType: check if character is parsimony-
|		informative, variable, or constant
|
-----------------------------------------------------------*/
void CheckCharCodingType (Matrix *m, CharInfo *ci)

{
	int		i, j, k, x, n1[10], n2[10], largest, smallest, numPartAmbig,
			numConsidered, numInformative, lastInformative=0, uniqueBits,
			newPoss, oldPoss, combinations[2048], *newComb, *oldComb, *tempComb;

	extern int NBits (int x);

	/* set up comb pointers */
	oldComb = combinations;
	newComb = oldComb + 1024;

	/* set counters to 0 */
	numPartAmbig = numConsidered = 0;

	/* set variable and informative to yes */
	ci->variable = ci->informative = YES;

	/* set constant to no and state counters to 0 for all states */
	for (i=0; i<10; i++)
		{
		ci->constant[i] = NO;
		n1[i] = n2[i] = 0;
		}

	for (i=0; i<m->nRows; i++)
		{
		/* retrieve character */
		x = (int) m->origin[m->column + i*m->rowSize];

		/* add it to counters if not all ambiguous */
		if (NBits(x) < ci->nStates)
			{
			numConsidered++;
			if (NBits(x) > 1)
				numPartAmbig++;
			for (j=0; j<10; j++)
				{
				if (((1<<j) & x) != 0)
					{	
					n1[j]++;
					if (NBits(x) == 1)
						n2[j]++;
					}
				}
			}
		}

	/* if the ambig counter for any state is equal to the number of considered
	   states, then set constant for that state and set variable and informative to no */
	for (i=0; i<10; i++)
		{
		if (n1[i] == numConsidered)
			{
			ci->constant[i] = YES;
			ci->variable = ci->informative = NO;
			}
		}

	/* return if variable is no or if a restriction site char */
	if (ci->variable == NO || ci->dType == RESTRICTION)
		return;

	/* the character is either (variable and uninformative) or informative */
	
	/* first consider unambiguous characters */
	/* find smallest and largest unambiguous state for this character */
	smallest = 9;
	largest = 0;
	for (i=0; i<10; i++)
		{
		if (n2[i] > 0)
			{
			if (i < smallest)
				smallest = i;
			if (i > largest)
				largest = i;
			}
		}
		
	/* count the number of informative states in the unambiguous codings */
	for (i=numInformative=0; i<10; i++)
		{
		if (ci->cType == ORD && n2[i] > 0 && i != smallest && i != largest)
			{	
			numInformative++;
			lastInformative = i;
			}
		else if (n2[i] > 1)
			{
			numInformative++;
			lastInformative = i;
			}
		}

	/* set informative */
	if (numInformative > 1)
		ci->informative = YES;
	else
		ci->informative = NO;

	
	/* we can return now unless informative is no and numPartAmbig is not 0 */
	if (!(numPartAmbig > 0 && ci->informative == NO))
		return;

	/* check if partially ambiguous observations make this character informative
	   after all */
	
	/* first set the bits for the taken states */
	x = 0;
	for (i=0; i<10; i++)
		{
		if (n2[i] > 0 && i != lastInformative)
			x |= (1<<i);
		}
	oldPoss = 1;
	oldComb[0] = x;

	/* now go through all partambig chars and see if we can add them without
	   making the character informative */
	for (i=0; i<m->nRows; i++)
		{
		x = (int) m->origin[m->column + i*m->rowSize];
		/* if partambig */ 
		if (NBits(x) > 1 && NBits(x) < ci->nStates)
			{
			/* remove lastInformative */
			x &= !(1<<lastInformative);
			/* reset newPoss */
			newPoss = 0;
			/* see if we can add it, store all possible combinations */
			for (j=0; j<oldPoss; j++)
				{
				uniqueBits = x & (!oldComb[j]);
				for (k=0; k<10; k++)
					{
					if (((1<<k) & uniqueBits) != 0)
						newComb[newPoss++] = oldComb[j] | (1<<k);
					}
				}
			/* break out if we could not add it */
			if (newPoss == 0)
				break;
			
			/* prepare for next partAmbig */
			oldPoss = newPoss;
			tempComb = oldComb;
			oldComb = newComb;
			newComb = tempComb;
			}
		}

	if (i < m->nRows)
		ci->informative = YES;

	return;
	
}





/*-----------------------------------------------------------
|
|   CheckModel: check model and warn user if strange things
|      are discovered.
|
-------------------------------------------------------------*/
int CheckModel (void)
{
    int         i, j, k, answer;
    Tree        *t;
    TreeNode    *p;
    
    /* there should only be one calibrated tree */
    for (i=0; i<numTrees; i++)
        {
        t = GetTreeFromIndex(i,0,0);
        if (t->isCalibrated == YES)
            break;
        }
    
    if (i < numTrees)
        {
        if (!strcmp(modelParams[t->relParts[0]].clockRatePr, "Fixed") && AreDoublesEqual(modelParams[t->relParts[0]].clockRateFix, 1.0, 1E-6) == YES)
            {
            MrBayesPrint("%s   WARNING: You have calibrated the tree but the clock rate is fixed to 1.0.\n", spacer);
            MrBayesPrint("%s      This means that time is measured in expected changes per time unit. If\n", spacer);
            MrBayesPrint("%s      the calibrations use a different time scale, you need to modify the model\n", spacer);
            MrBayesPrint("%s      by introducing a prior for the clock rate ('prset clockratepr').\n", spacer);

            if (noWarn == NO)
                {
                answer = WantTo("Do you want to continue with the run regardless");
                if (answer == YES)
                    {
                    MrBayesPrint("%s   Continuing with the run...\n\n", spacer);
                    }
                else
                    {
                    MrBayesPrint("%s   Stopping the run...\n\n", spacer);
                    return (ERROR);
                    }
                }
            }
        }

    /* check coalescence model */
    for (i=0; i<numTrees; i++)
        {
        t = GetTreeFromIndex(i, 0, 0);
        if ((!strcmp(modelParams[t->relParts[0]].clockPr,"Coalescence") || !strcmp(modelParams[t->relParts[0]].clockPr,"Speciestreecoalescence"))
            && AreDoublesEqual(modelParams[t->relParts[0]].clockRateFix, 1.0, 1E-6) == YES)
            {
            MrBayesPrint("%s   WARNING: You are using a coalescent model but the clock rate is fixed to 1.0.\n", spacer);
            MrBayesPrint("%s      This is likely to be incorrect unless you have set the population size prior\n", spacer);
            MrBayesPrint("%s      ('prset popsizepr') to reflect an appropriate prior on theta. \n", spacer);

            if (noWarn == NO)
                {
                answer = WantTo("Do you want to continue with the run regardless");
                if (answer == YES)
                    {
                    MrBayesPrint("%s   Continuing with the run...\n\n", spacer);
                    }
                else
                    {
                    MrBayesPrint("%s   Stopping the run...\n\n", spacer);
                    return (ERROR);
                    }
                }
            }
        }


    /* Check consistency of best model. First we guarantee that if one topology has
       a species tree prior, then all topologies have the same prior. Then we make
       sure that all clock trees have a coalescence prior. */

    j = 0;
    for (i=0; i<numCurrentDivisions; i++)
        {
        if (!strcmp(modelParams[i].topologyPr, "Speciestree"))
            j++;
        }

    if (j > 0)
        {
        if (j != numCurrentDivisions)
            {
            MrBayesPrint("%s   ERROR: If one gene tree has a speciestree prior then all\n", spacer);
            MrBayesPrint("%s          gene trees must have the same prior.\n", spacer);
            return (ERROR);
            }
        for (i=0; i<numTrees-1; i++)
            {
            t = GetTreeFromIndex(i,0,0);
            if (strcmp(modelParams[t->relParts[0]].clockPr,"Speciestreecoalescence") != 0)
                {
                MrBayesPrint("%s   ERROR: All gene trees must have a speciestreecoalescence prior\n", spacer);
                MrBayesPrint("%s          if they fold into a species tree.\n", spacer);
                return (ERROR);
                }
            if (t->isCalibrated == YES)
                {
                for (k=0; k<t->nNodes-1; k++)
                    {
                    p = t->allDownPass[k];
                    if (p->calibration != NULL)
                        {
                        MrBayesPrint("%s   ERROR: Gene trees cannot be individually calibrated\n", spacer);
                        MrBayesPrint("%s          if they fold into a species tree.\n", spacer);
                        return (ERROR);
                        }
                    }
                }
            }
        }
    else
        {
        for (i=0; i<numTrees; i++)
            {
            t = GetTreeFromIndex(i,0,0);
            if (t->isCalibrated == YES && strcmp(modelParams[t->relParts[0]].clockPr, "Uniform") != 0)
                {
                for (k=0; k<t->nNodes-1; k++)
                    {
                    p = t->allDownPass[k];
                    if (p->calibration != NULL)
                        {
                        MrBayesPrint("%s   ERROR: MrBayes does not yet suppport calibration of tips or\n", spacer);
                        MrBayesPrint("%s          interior nodes under the %s clock prior.\n", spacer, modelParams[t->relParts[0]].clockPr);
                        return (ERROR);
                        }
                    }
                }
            }
        }

    return NO_ERROR;
}





/*-----------------------------------------------------------
|
|   CheckExpandedModels: check data partitions that have
|   the codon or doublet model specified
|
-------------------------------------------------------------*/
int CheckExpandedModels (void)

{

	int				c, d, i, t, s, s1, s2, s3, whichNuc, uniqueId, numCharsInPart, 
					firstChar, lastChar, contiguousPart, badBreak, badExclusion,
					nGone, nuc1, nuc2, nuc3, foundStopCodon, posNucs1[4], posNucs2[4], posNucs3[4],
					oneGoodCodon, foundUnpaired, nPair, allCheckedOut;
	char			*tempStr;
	int             tempStrSize=100;
	ModelParams		*mp;
	
	tempStr = (char *) SafeMalloc((size_t) (tempStrSize * sizeof(char)));
	if (!tempStr)
		{
		MrBayesPrint ("%s   Problem allocating tempString (%d)\n", spacer, tempStrSize * sizeof(char));
		return (ERROR);
		}

	/* first, set charId to 0 for all characters */
	for (i=0; i<numChar; i++)
		charInfo[i].charId = 0;
	
	/* loop over partitions */
	allCheckedOut = 0;
	uniqueId = 1;
	for (d=0; d<numCurrentDivisions; d++)
		{
		mp = &modelParams[d];
		
		if (mp->dataType == DNA || mp->dataType == RNA)
			{
			if (!strcmp(mp->nucModel,"Codon") || !strcmp(mp->nucModel,"Protein"))
				{
				/* start check that the codon model is appropriate for this partition */
				
				/* find first character in this partition */
				for (c=0; c<numChar; c++)
					{
					if (partitionId[c][partitionNum] == d+1)
						break;
					}
				firstChar = c;
				/*printf ("   first character = %d\n", firstChar);*/
				
				/* find last character in this partition */
				for (c=numChar-1; c>=0; c--)
					{
					if (partitionId[c][partitionNum] == d+1)
						break;
					}
				lastChar = c;
				/*printf ("   last character = %d\n", lastChar);*/
				
				/* check that the number of characters in partition is divisible by 3 */
				numCharsInPart = 0;
				for (c=firstChar; c<=lastChar; c++)
					{
					if (charInfo[c].isExcluded == YES || partitionId[c][partitionNum] != d+1)
						continue;
					numCharsInPart++;
					}
				/*printf ("   numCharsInPart = %d\n", numCharsInPart);*/
				if (numCharsInPart % 3 != 0)
					{
					if (numCurrentDivisions == 1)
						{
						MrBayesPrint ("%s   The number of characters is not divisible by three.\n", spacer);
						MrBayesPrint ("%s   You specified a %s model which requires triplets\n", spacer, mp->nucModel);
						MrBayesPrint ("%s   However, you only have %d characters.\n", spacer, numCharsInPart);
						}
					else
						{
						MrBayesPrint ("%s   The number of characters in partition %d is not\n", spacer, d+1);
						MrBayesPrint ("%s   divisible by three. You specified a %s model\n", spacer, mp->nucModel);
						MrBayesPrint ("%s   which requires triplets. \n", spacer);
						MrBayesPrint ("%s   However, you only have %d characters in this  \n", spacer, numCharsInPart);
						MrBayesPrint ("%s   partition \n", spacer);
						}
					free (tempStr);
					return (ERROR);
					}
				
				/* check that all of the characters in the partition are contiguous */
				contiguousPart = YES;
				for (c=firstChar; c<=lastChar; c++)
					{
					if (partitionId[c][partitionNum] != d+1)
						contiguousPart = NO;
					}
				if (contiguousPart == NO)
					{
					MrBayesPrint ("%s   Partition %d is not contiguous. You specified that\n", spacer, d+1);
					MrBayesPrint ("%s   a %s model be used for this partition. However, there\n", spacer, mp->nucModel);
					MrBayesPrint ("%s   is another partition that is between some of the characters\n", spacer);
					MrBayesPrint ("%s   in this partition. \n", spacer);
					free (tempStr);
					return (ERROR);
					}
					
				/* check that there is not a break inside a triplet of characters */
				badBreak = NO;
				whichNuc = 0;
				for (c=firstChar; c<=lastChar; c++)
					{
					whichNuc++;
					if (charInfo[c].bigBreakAfter == YES && whichNuc != 3)
						badBreak = YES;
					if (whichNuc == 3)
						whichNuc = 0;
					}
				if (badBreak == YES)
					{
					MrBayesPrint ("%s   You specified a databreak inside of a coding triplet.\n", spacer);
					MrBayesPrint ("%s   This is a problem, as you imply that part of the codon\n", spacer);
					MrBayesPrint ("%s   lies in one gene and the remainder in another gene. \n", spacer);
					free (tempStr);
					return (ERROR);
					}

				/* make certain excluded characters are in triplets */
				badExclusion = NO;
				whichNuc = nGone = 0;
				for (c=firstChar; c<=lastChar; c++)
					{
					whichNuc++;
					if (charInfo[c].isExcluded == YES)
						nGone++;
					if (whichNuc == 3)
						{
						if (nGone == 1 || nGone == 2)
							badExclusion = YES;
						whichNuc = nGone = 0;
						}
					}
				if (badExclusion == YES)
					{
					MrBayesPrint ("%s   In excluding characters, you failed to remove all of the\n", spacer);
					MrBayesPrint ("%s   sites of at least one codon. If you exclude sites, make \n", spacer);
					MrBayesPrint ("%s   certain to exclude all of the sites in the codon(s). \n", spacer);
					free (tempStr);
					return (ERROR);
					}
				
				/* check that there are no stop codons */
				foundStopCodon = NO;
				for (c=firstChar; c<=lastChar; c+=3)
					{
					if (charInfo[c].isExcluded == NO)
						{
						for (t=0; t<numTaxa; t++)
							{
							if (taxaInfo[t].isDeleted == YES)
								continue;
							nuc1 = matrix[pos(t,c+0,numChar)];
							nuc2 = matrix[pos(t,c+1,numChar)];
							nuc3 = matrix[pos(t,c+2,numChar)];
                            /*nucX is in range 0-15 to represent any possible set of states that nucleatide could be in*/
							GetPossibleNucs (nuc1, posNucs1);
							GetPossibleNucs (nuc2, posNucs2);
							GetPossibleNucs (nuc3, posNucs3);
							
							oneGoodCodon = NO;
							s = 0;
							for (s1=0; s1<4; s1++)
								{
								for (s2=0; s2<4; s2++)
									{
									for (s3=0; s3<4; s3++)
										{
										if (posNucs1[s1] == 1 && posNucs2[s2] == 1 && posNucs3[s3] == 1)
											{
											if (mp->codon[s1*16 + s2*4 + s3] != 21)
												oneGoodCodon = YES;
											}
										s++;
										}
									}
								}
							if (oneGoodCodon == NO)
								{
								foundStopCodon = YES;
								MrBayesPrint ("%s   Stop codon: taxon %s, sites %d to %d (%c%c%c, %s code)\n", spacer, 
									taxaNames[t], c+1, c+3, WhichNuc (nuc1), WhichNuc (nuc2), WhichNuc (nuc3), mp->geneticCode);
								}
							}
						}
					}				
				if (foundStopCodon == YES)
					{
					MrBayesPrint ("%s   At least one stop codon was found. Stop codons are not\n", spacer);
					MrBayesPrint ("%s   allowed under the codon models.  \n", spacer);
					free (tempStr);
					return (ERROR);
					}
				
				/* everything checks out. Now we can initialize charId */
				whichNuc = 0;
				for (c=firstChar; c<=lastChar; c++)
					{
					whichNuc++;
					charInfo[c].charId = uniqueId;
					if (whichNuc == 3)
						{
						whichNuc = 0;
						uniqueId++;
						}
					}
				
				allCheckedOut++;
				/* end check that the codon model is appropriate for this partition */
				}
			else if (!strcmp(mp->nucModel,"Doublet"))
				{
				/* start check that the doublet model is appropriate for this partition */
				
				/* Check that pairsId does not equal 0 for any of the characters in
				   the partition. If it does, then this means that at least one 
				   site was not appropriately paired. Remember, that pairsId is
				   initialized 1, 2, 3, ... for the first pair, second pair, etc. 
				   Also, check that every pair is only represented two times. */
				foundUnpaired = NO;
				for (c=0; c<numChar; c++)
					{
					if (partitionId[c][partitionNum] == d+1 && charInfo[c].pairsId == 0 && charInfo[c].isExcluded == NO)
						foundUnpaired = YES;
					}
					
				for (c=0; c<numChar; c++)
					{
					if (partitionId[c][partitionNum] == d+1 && charInfo[c].isExcluded == NO)
						{
						nPair = 1;
						for (i=0; i<numChar; i++)
							{
							if (i != c && partitionId[i][partitionNum] == d+1 && charInfo[i].isExcluded == NO && charInfo[c].pairsId == charInfo[i].pairsId)
								nPair++;
							}
						if (nPair != 2)
							foundUnpaired = YES;
						}
					}
				if (foundUnpaired == YES)
					{
					if (numCurrentDivisions == 1)
						{
						MrBayesPrint ("%s   Found unpaired nucleotide sites. The doublet model\n", spacer);
						MrBayesPrint ("%s   requires that all sites are paired. \n", spacer);
						}
					else
						{
						MrBayesPrint ("%s   Found unpaired nucleotide sites in partition %d.\n", spacer, d+1);
						MrBayesPrint ("%s   The doublet model requires that all sites are paired. \n", spacer);
						}
					free (tempStr);
					return (ERROR);
					}

				/* everything checks out. Now we can initialize charId */
				for (c=0; c<numChar; c++)
					{
					nuc1 = nuc2 = -1;
					if (partitionId[c][partitionNum] == d+1 && charInfo[c].charId == 0)
						{
						nuc1 = c;
						for (i=0; i<numChar; i++)
							{
							if (i != c && charInfo[i].charId == 0 && charInfo[c].pairsId == charInfo[i].pairsId)
								nuc2 = i;
							}
						if (nuc1 >= 0 && nuc2 >= 0)
							{
							charInfo[nuc1].charId = charInfo[nuc2].charId = uniqueId;
							uniqueId++;
							}
						else
							{
							MrBayesPrint ("%s   Weird doublet problem in partition %d.\n", spacer, d+1);
							free (tempStr);
							return (ERROR);
							}
						}
					}
					
				allCheckedOut++;
				/* end check that the doublet model is appropriate for this partition */
				}
			}
		}
		
	/*
    if (allCheckedOut > 0)
		MrBayesPrint ("%s   Codon/Doublet models successfully checked\n", spacer);
    */
		
#	if 0
	for (c=0; c<numChar; c++)
			printf (" %d", charId[c]);
	printf ("\n");
#	endif
	free (tempStr);
	return (NO_ERROR);
	
}





/*----------------------------------------------------------------------
|
|   InitializeChainTrees: 'Constructor' for chain trees
|
-----------------------------------------------------------------------*/
int InitializeChainTrees (Param *p, int from, int to, int isRooted)

{

	int     i, st, isCalibrated, isClock, nTaxa, numActiveHardConstraints=0;
	Tree	*tree, **treeHandle;
	Model	*mp;
	
	mp = &modelParams[p->relParts[0]];

    if (p->paramType == P_SPECIESTREE)
        nTaxa = numSpecies;
    else
        nTaxa = numLocalTaxa;

	/* figure out whether the trees are clock */
	if (!strcmp(mp->brlensPr,"Clock"))
		isClock = YES;
	else
		isClock = NO;

	/* figure out whether the trees are calibrated */
	if (!strcmp(mp->brlensPr,"Clock") && (strcmp(mp->nodeAgePr,"Calibrated") == 0 || strcmp(mp->clockRatePr,"Fixed") != 0 ||
        (strcmp(mp->clockRatePr, "Fixed") == 0 && AreDoublesEqual(mp->clockRateFix, 1.0, 1E-6) == NO)))
		isCalibrated = YES;
	else
		isCalibrated = NO;

    if (p->checkConstraints == YES)
		{

        for (i=0; i<numDefinedConstraints; i++)
		    {
		    if (mp->activeConstraints[i] == YES && definedConstraintsType[i] == HARD )
			    numActiveHardConstraints++;
            }
		}

	/* allocate space for and construct the trees */
    /* NOTE: The memory allocation scheme used here must match GetTree and GetTreeFromIndex */
	for (i=from; i<to; i++)
		{
		treeHandle = mcmcTree + p->treeIndex + 2*i*numTrees;
		if (*treeHandle)
            free(*treeHandle);
		if ((*treeHandle = AllocateTree (nTaxa)) == NULL)
			{
			MrBayesPrint ("%s   Problem allocating mcmc trees\n", spacer);
			return (ERROR);
			}
		treeHandle = mcmcTree + p->treeIndex + (2*i + 1)*numTrees;
        if (*treeHandle)
            free(*treeHandle);
		if ((*treeHandle = AllocateTree (nTaxa)) == NULL)
			{
			MrBayesPrint ("%s   Problem allocating mcmc trees\n", spacer);
			return (ERROR);
			}
		}

	/* initialize the trees */
	for (i=from; i<to; i++)
		{
		for (st=0; st<2; st++)
			{
			tree = GetTree (p, i, st);
			if (numTrees > 1)
				sprintf (tree->name, "mcmc.tree%d_%d", p->treeIndex+1, i+1);
			else /* if (numTrees == 1) */
				sprintf (tree->name, "mcmc.tree_%d", i+1);
            tree->nRelParts = p->nRelParts;
			tree->relParts = p->relParts;
            tree->isRooted = isRooted;
			tree->isClock = isClock;
			tree->isCalibrated = isCalibrated;
            if (p->paramType == P_SPECIESTREE)
                {
                tree->nNodes = 2*numSpecies;
                tree->nIntNodes = numSpecies - 1;
                }
            else if (tree->isRooted == YES)
                {
                tree->nNodes = 2*numLocalTaxa;
                tree->nIntNodes = numLocalTaxa - 1;
                }
            else /* if (tree->isRooted == NO) */
                {
                tree->nNodes = 2*numLocalTaxa - 2;
                tree->nIntNodes = numLocalTaxa - 2;
                }
			if (p->checkConstraints == YES)
				{
				tree->checkConstraints = YES;
				tree->nConstraints = mp->numActiveConstraints;
				tree->nLocks = numActiveHardConstraints;
				tree->constraints = mp->activeConstraints;
				}
			else
				{
				tree->checkConstraints = NO;
				tree->nConstraints = tree->nLocks = 0;
				tree->constraints = NULL;
				}
			}
		}

	return (NO_ERROR);
}





/*-----------------------------------------------------------
|
|   CompressData: compress original data matrix
|
-------------------------------------------------------------*/
int CompressData (void)

{

	int				a, c, d, i, j, k, t, col[3], isSame, newRow, newColumn,
					*isTaken, *tempSitesOfPat, *tempChar;
	SafeLong        *tempMatrix;
	ModelInfo		*m;
	ModelParams		*mp;

#	if defined DEBUG_COMPRESSDATA
	if (PrintMatrix() == ERROR)
		goto errorExit;
	getchar();
#	endif

	/* set all pointers that will be allocated locally to NULL */
	isTaken = NULL;
	tempMatrix = NULL;
	tempSitesOfPat = NULL;
	tempChar = NULL;

	/* allocate indices pointing from original to compressed matrix */
	if (memAllocs[ALLOC_COMPCOLPOS] == YES)
        {
	    free (compColPos);
        compColPos = NULL;
        memAllocs[ALLOC_COMPCOLPOS] = NO;
        }
	compColPos = (int *)SafeMalloc((size_t) (numChar * sizeof(int)));
	if (!compColPos)
		{
		MrBayesPrint ("%s   Problem allocating compColPos (%d)\n", spacer, numChar * sizeof(int));
		goto errorExit;
		}
	for (i=0; i<numChar; i++)
		compColPos[i] = 0;
	memAllocs[ALLOC_COMPCOLPOS] = YES;

	if (memAllocs[ALLOC_COMPCHARPOS] == YES)
        {
	    free (compCharPos);
        compCharPos = NULL;
        memAllocs[ALLOC_COMPCHARPOS] = NO;
        }
	compCharPos = (int *)SafeMalloc((size_t) (numChar * sizeof(int)));
	if (!compCharPos)
		{
		MrBayesPrint ("%s   Problem allocating compCharPos (%d)\n", spacer, numChar * sizeof(int));
		goto errorExit;
		}
	for (i=0; i<numChar; i++)
		compCharPos[i] = 0;
	memAllocs[ALLOC_COMPCHARPOS] = YES;

	/* allocate space for temporary matrix, tempSitesOfPat,             */
	/* vector keeping track of whether a character has been compressed, */
	/* and vector indexing first original char for each compressed char */
	tempMatrix = (SafeLong *) SafeCalloc (numLocalTaxa * numLocalChar, sizeof(SafeLong));
	tempSitesOfPat = (int *) SafeCalloc (numLocalChar, sizeof(int));
	isTaken = (int *) SafeCalloc (numChar, sizeof(int));
	tempChar = (int *) SafeCalloc (numLocalChar, sizeof(int));
	if (!tempMatrix || !tempSitesOfPat || !isTaken || !tempChar)
		{
		MrBayesPrint ("%s   Problem allocating temporary variables in CompressData\n", spacer);
		goto errorExit;
		}

    /* initialize isTaken */
    for(c=0; c<numChar; c++)
        isTaken[c] = NO;

	/* set index to first empty column in temporary matrix */
	newColumn = 0;

	/* initialize number of compressed characters */
	numCompressedChars = 0;

	/* sort and compress data */
	for (d=0; d<numCurrentDivisions; d++)
		{
		/* set pointers to the model params and settings for this division */
		m = &modelSettings[d];
		mp = &modelParams[d];

		/* set column offset for this division in compressed matrix */
		m->compMatrixStart = newColumn;

		/* set compressed character offset for this division */
		m->compCharStart = numCompressedChars;

		/* set number of compressed characters to 0 for this division */
		m->numChars = 0;

		/* find the number of original characters per model site */
		m->nCharsPerSite = 1;
		if (mp->dataType == DNA || mp->dataType == RNA)
			{	
			if (!strcmp(mp->nucModel, "Doublet"))
				m->nCharsPerSite = 2;
			if (!strcmp(mp->nucModel, "Codon") || !strcmp(mp->nucModel, "Protein"))
				m->nCharsPerSite = 3;
			}
		
		/* sort and compress the characters for this division */
		for (c=0; c<numChar; c++)
			{
			if (charInfo[c].isExcluded == YES || partitionId[c][partitionNum] != d+1 || isTaken[c] == YES)
				continue;

			col[0] = c;
			isTaken[c] = YES;
			
			/* find additional columns if more than one character per model site      */
			/* return error if the number of matching characters is smaller or larger */
			/* than the actual number of characters per model site                    */
			if (m->nCharsPerSite > 1)
				{
				j = 1;
				if (charInfo[c].charId == 0)
					{
					MrBayesPrint("%s   Character %d is not properly defined\n", spacer, c+1);
					goto errorExit;
					}
				for (i=c+1; i<numChar; i++)
					{
					if (charInfo[i].charId == charInfo[c].charId)
						{
						if (j >= m->nCharsPerSite)
							{
							MrBayesPrint("%s   Too many matches in charId (division %d char %d)\n", spacer, d, numCompressedChars);
							goto errorExit;
							}
						else
							{
							col[j++] = i;
							isTaken[i] = YES;
							}
						}
					}
				if (j != m->nCharsPerSite)
					{
					MrBayesPrint ("%s   Too few matches in charId (division %d char %d)\n", spacer, d, numCompressedChars);
					goto errorExit;
					}
				}
			
			/* add character to temporary matrix in column(s) at newColumn */
			for (t=newRow=0; t<numTaxa; t++)
				{
				if (taxaInfo[t].isDeleted == YES)
					continue;

				for (k=0; k<m->nCharsPerSite; k++)
					{
					tempMatrix[pos(newRow,newColumn+k,numLocalChar)] = matrix[pos(t,col[k],numChar)];
					}
				newRow++;
				}
			
			/* is it unique? */
			isSame = NO;
			if (mp->dataType != CONTINUOUS)
				{
				for (i=m->compMatrixStart; i<newColumn; i+=m->nCharsPerSite)
					{
					isSame = YES;
					for (j=0; j<numLocalTaxa; j++)
                        {
						for (k=0; k<m->nCharsPerSite; k++)
							if (tempMatrix[pos(j,newColumn+k,numLocalChar)] != tempMatrix[pos(j,i+k,numLocalChar)])
								{
								isSame = NO;
								break;
								}
                        if (isSame == NO)
                            break;
                        }
					if (isSame == YES)
						break;
					}
				}

			/* if subject to data augmentation, it is always unique */
			if (!strcmp(mp->augmentData, "Yes"))
				{
				for (k=0; k<m->nCharsPerSite; k++)
					{
					if (charInfo[col[k]].isMissAmbig == YES)
						isSame = NO;
					}
				}

			if (isSame == NO)
				{
				/* if it is unique then it should be added */
				tempSitesOfPat[numCompressedChars] = 1;
				for (k=0; k<m->nCharsPerSite; k++)
					{
					compColPos[col[k]] = newColumn + k;
					compCharPos[col[k]] = numCompressedChars;
					tempChar[newColumn + k] = col[k];
					}
				newColumn+=m->nCharsPerSite;
				m->numChars++;
				numCompressedChars++;
				}
			else
				{
				/* if it is not unique then simply update tempSitesOfPat     */
				/* calculate compressed character position and put it into a */
				/* (i points to compressed column position)                  */
				a = m->compCharStart + ((i - m->compMatrixStart) / m->nCharsPerSite);
				tempSitesOfPat[a]++;
				for (k=0; k<m->nCharsPerSite; k++)
					{
					compColPos[col[k]] = i;
					compCharPos[col[k]] = a;
					/* tempChar (pointing from compressed to uncompresed) */
					/* can only be set for first pattern */
					}
				}
			}	/* next character */
			
		/* check that the partition has at least a single character */
		if (m->numChars <= 0)
			{
			MrBayesPrint ("%s   You must have at least one site in a partition. Partition %d\n", spacer, d+1);
			MrBayesPrint ("%s   has %d site patterns.\n", spacer, m->numChars);
			goto errorExit;
			}

		m->compCharStop = m->compCharStart + m->numChars;
		m->compMatrixStop = newColumn;

		} /* next division */

	compMatrixRowSize = newColumn;

	/* now we know the size, so we can allocate space for the compressed matrix ... */
	if (memAllocs[ALLOC_COMPMATRIX] == YES)
		{
		free (compMatrix);
        compMatrix = NULL;
        memAllocs[ALLOC_COMPMATRIX] = NO;
		}
	compMatrix = (SafeLong *) SafeCalloc (compMatrixRowSize * numLocalTaxa, sizeof(SafeLong));
	if (!compMatrix)
		{
		MrBayesPrint ("%s   Problem allocating compMatrix (%d)\n", spacer, compMatrixRowSize * numLocalTaxa * sizeof(SafeLong));
		goto errorExit;
		}
	memAllocs[ALLOC_COMPMATRIX] = YES;
	
	if (memAllocs[ALLOC_NUMSITESOFPAT] == YES)
		{
		free (numSitesOfPat);
        numSitesOfPat = NULL;
        memAllocs[ALLOC_NUMSITESOFPAT] = NO;
		}
	numSitesOfPat = (CLFlt *) SafeCalloc (numCompressedChars, sizeof(CLFlt));
	if (!numSitesOfPat)
		{
		MrBayesPrint ("%s   Problem allocating numSitesOfPat (%d)\n", spacer, numCompressedChars * sizeof(MrBFlt));
		goto errorExit;
		}
	memAllocs[ALLOC_NUMSITESOFPAT] = YES;

	if (memAllocs[ALLOC_ORIGCHAR] == YES)
		{
		free (origChar);
        origChar = NULL;
        memAllocs[ALLOC_ORIGCHAR] = NO;
		}
	origChar = (int *)SafeMalloc((size_t) (compMatrixRowSize * sizeof(int)));
	if (!origChar)
		{
		MrBayesPrint ("%s   Problem allocating origChar (%d)\n", spacer, numCompressedChars * sizeof(int));
		goto errorExit;
		}
	memAllocs[ALLOC_ORIGCHAR] = YES;

	/* ... and copy the data there */
	for (i=0; i<numLocalTaxa; i++)
		for (j=0; j<compMatrixRowSize; j++)
			compMatrix[pos(i,j,compMatrixRowSize)] = tempMatrix[pos(i,j,numLocalChar)];

	for (i=0; i<numCompressedChars; i++)
		numSitesOfPat[i] = (CLFlt) tempSitesOfPat[i];

	for (i=0; i<compMatrixRowSize; i++)
		origChar[i] = tempChar[i];

#	if defined (DEBUG_COMPRESSDATA)
	if (PrintCompMatrix() == ERROR)
		goto errorExit;
	getchar();
#	endif

	/* free the temporary variables */
	free (tempSitesOfPat);
	free (tempMatrix);
	free (isTaken);
	free (tempChar);

	return NO_ERROR;

	errorExit:
		if (tempMatrix)
		    free (tempMatrix);
		if (tempSitesOfPat)
			free (tempSitesOfPat);
		if (isTaken)
			free (isTaken);
		if (tempChar)
			free (tempChar);

		return ERROR;
}





int DataType (int part)

{
	int		i;

	for (i=0; i<numChar; i++)
		{
		if (partitionId[i][partitionNum] == part + 1)
			break;
		}

	return (charInfo[i].charType);
}





int DoLink (void)

{

	int			i, j, newLine;
	
	MrBayesPrint ("%s   Linking\n", spacer);
	
	/* update status of linkTable */
	for (j=0; j<NUM_LINKED; j++)
		{
		newLine = YES;
		for (i=0; i<numCurrentDivisions; i++)
			{
			if (tempLinkUnlink[j][i] == YES)
				{
				if (newLine == YES)
					{
					linkNum++;
					newLine = NO;
					}
				linkTable[j][i] = linkNum;
				}
			}
		}
	
#	if 0
	for (j=0; j<NUM_LINKED; j++)
		{
		MrBayesPrint ("%s   ", spacer);
		for (i=0; i<numCurrentDivisions; i++)
			MrBayesPrint ("%d", linkTable[j][i]);
		MrBayesPrint ("\n");
		}
#	endif

	/* reinitialize the temporary table */
	for (j=0; j<NUM_LINKED; j++)
		for (i=0; i<numCurrentDivisions; i++)
			tempLinkUnlink[j][i] = NO;

	/* set up parameters and moves */
	if (SetUpAnalysis (&globalSeed) == ERROR)
		return (ERROR);
	
	return (NO_ERROR);
	
}





int DoLinkParm (char *parmName, char *tkn)

{

	int			i, j, tempInt;

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before the model can be defined\n", spacer);
		return (ERROR);
		}
		
	if (inValidCommand == YES)
		{
		for (j=0; j<NUM_LINKED; j++)
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[j][i] = NO;
		inValidCommand = NO;
		}

	if (expecting == Expecting(PARAMETER))
		{
		expecting = Expecting(EQUALSIGN);
		}
	else if (expecting == Expecting(EQUALSIGN))
		{
		expecting = Expecting(LEFTPAR);
		}
	else if (expecting == Expecting(LEFTPAR))
		{
		/* initialize tempLinkUnlinkVec to no */
		for (i=0; i<numCurrentDivisions; i++)
			tempLinkUnlinkVec[i] = NO;
		fromI = toJ = -1;
		foundDash = NO;
		expecting = Expecting(NUMBER) | Expecting(ALPHA);
		}
	else if (expecting == Expecting(RIGHTPAR))
		{
		if (fromI != -1)
			tempLinkUnlinkVec[fromI-1] = YES;
		/* now copy tempLinkUnlinkVec to appropriate row of tempLinkUnlink */
		if (!strcmp(parmName, "Tratio"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_TRATIO][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Revmat"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_REVMAT][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Omega"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_OMEGA][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Statefreq"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_PI][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Shape"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_SHAPE][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Pinvar"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_PINVAR][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Correlation"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_CORREL][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Ratemultiplier"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_RATEMULT][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Switchrates"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_SWITCH][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Topology"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_TOPOLOGY][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Brlens"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_BRLENS][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Speciationrate"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_SPECRATE][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Extinctionrate"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_EXTRATE][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Popsize"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_POPSIZE][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Growthrate"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_GROWTH][i] = tempLinkUnlinkVec[i];
			} 
		else if (!strcmp(parmName, "Aamodel"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_AAMODEL][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Cpprate"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_CPPRATE][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Cppmultdev"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_CPPMULTDEV][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Cppevents"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_CPPEVENTS][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "TK02var") || !strcmp(parmName, "Bmvar"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_TK02VAR][i] = tempLinkUnlinkVec[i];
			}
        else if (!strcmp(parmName, "TK02branchrates") || !strcmp(parmName, "Bmbranchrates"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_TK02BRANCHRATES][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Igrvar") || !strcmp(parmName, "Ibrvar"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_IGRVAR][i] = tempLinkUnlinkVec[i];
			}
		else if (!strcmp(parmName, "Igrbranchlens") || !strcmp(parmName, "Ibrbranchlens"))
			{
			for (i=0; i<numCurrentDivisions; i++)
				tempLinkUnlink[P_IGRBRANCHLENS][i] = tempLinkUnlinkVec[i];
			}
		else
			{
			MrBayesPrint ("%s   Couldn't find parameter %s to link\n", spacer, parmName);
			}
		
		expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
		}
	else if (expecting == Expecting(COMMA))
		{
		foundComma = YES;
		expecting = Expecting(NUMBER);
		}
	else if (expecting == Expecting(ALPHA))
		{
		if (IsSame ("All", tkn) == DIFFERENT)
			{
			MrBayesPrint ("%s   Do not understand delimiter \"%s\"\n", spacer, tkn);
			return (ERROR);
			}
		for (i=0; i<numCurrentDivisions; i++)
			tempLinkUnlinkVec[i] = YES;
		expecting  = Expecting(RIGHTPAR);
		}
	else if (expecting == Expecting(NUMBER))
		{
		sscanf (tkn, "%d", &tempInt);
		if (tempInt > numCurrentDivisions)
			{
			MrBayesPrint ("%s   Partition delimiter is too large\n", spacer);
			return (ERROR);
			}
		if (fromI == -1)
			fromI = tempInt;
		else if (fromI != -1 && toJ == -1 && foundDash == YES && foundComma == NO)
			{
			toJ = tempInt;
			for (i=fromI-1; i<toJ; i++)
				tempLinkUnlinkVec[i] = YES;
			fromI = toJ = -1;
			foundDash = NO;
			}
		else if (fromI != -1 && toJ == -1 && foundDash == NO && foundComma == YES)
			{
			tempLinkUnlinkVec[fromI-1] = YES;
			fromI = tempInt;
			foundComma = NO;
			}
		expecting  = Expecting(COMMA);
		expecting |= Expecting(DASH);
		expecting |= Expecting(RIGHTPAR);
		}
	else if (expecting == Expecting(DASH))
		{
		foundDash = YES;
		expecting = Expecting(NUMBER);
		}
	else
		return (ERROR);

	return (NO_ERROR);
	
}





int DoLset (void)

{

	int			i, nApplied, lastActive=0;
	
	nApplied = NumActiveParts ();
	for (i=numCurrentDivisions; i>=0; i--)
		{
		if (activeParts[i] == YES)
			{
			lastActive = i;
			break;
			}
		}
			
	/* MrBayesPrint ("\n"); */
	if (numCurrentDivisions == 1)
		MrBayesPrint ("%s   Successfully set likelihood model parameters\n", spacer);
	else 
		{
		if (nApplied == numCurrentDivisions || nApplied == 0)
			{
			MrBayesPrint ("%s   Successfully set likelihood model parameters to all\n", spacer);
			MrBayesPrint ("%s      applicable data partitions \n", spacer);
			}
		else
			{
			MrBayesPrint ("%s   Successfully set likelihood model parameters to\n", spacer);
			if (nApplied == 1)
				MrBayesPrint ("%s   partition", spacer);
			else
				MrBayesPrint ("%s   partitions", spacer);
			for (i=0; i<numCurrentDivisions; i++)
				{
				if (activeParts[i] == YES)
					{
					if (i == lastActive && nApplied > 1)
						MrBayesPrint (" and %d", i+1);
					else
						MrBayesPrint (" %d", i+1);
					if (nApplied > 2 && i != lastActive)
						MrBayesPrint (",");
					}
				}
			MrBayesPrint (" (if applicable)\n");
			}
		}

	if (SetUpAnalysis (&globalSeed) == ERROR)
		return (ERROR);
	
	return (NO_ERROR);
	
}





int DoLsetParm (char *parmName, char *tkn)

{

	int			i, j, tempInt, nApplied;
	char		tempStr[100];

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before the model can be defined\n", spacer);
		return (ERROR);
		}
	if (inValidCommand == YES)
		{
		for (i=0; i<numCurrentDivisions; i++)
			activeParts[i] = NO;
		inValidCommand = NO;
		}

	if (expecting == Expecting(PARAMETER))
		{
		expecting = Expecting(EQUALSIGN);
		}
	else
		{
		/* set Applyto (Applyto) *************************************************************/
		if (!strcmp(parmName, "Applyto"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(LEFTPAR);
			else if (expecting == Expecting(LEFTPAR))
				{
				for (i=0; i<numCurrentDivisions; i++)
					activeParts[i] = NO;
				fromI = toJ = -1;
				foundDash = NO;
				expecting = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				if (fromI != -1)
					activeParts[fromI-1] = YES;
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else if (expecting == Expecting(COMMA))
				{
				foundComma = YES;
				expecting = Expecting(NUMBER);
				}
			else if (expecting == Expecting(ALPHA))
				{
				if (IsSame ("All", tkn) == DIFFERENT)
					{
					MrBayesPrint ("%s   Do not understand delimiter \"%s\"\n", spacer, tkn);
					return (ERROR);
					}
				for (i=0; i<numCurrentDivisions; i++)
					activeParts[i] = YES;
				expecting  = Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt > numCurrentDivisions)
					{
					MrBayesPrint ("%s   Partition delimiter is too large\n", spacer);
					return (ERROR);
					}
				if (fromI == -1)
					fromI = tempInt;
				else if (fromI != -1 && toJ == -1 && foundDash == YES && foundComma == NO)
					{
					toJ = tempInt;
					for (i=fromI-1; i<toJ; i++)
						activeParts[i] = YES;
					fromI = toJ = -1;
					foundDash = NO;
					}
				else if (fromI != -1 && toJ == -1 && foundDash == NO && foundComma == YES)
					{
					activeParts[fromI-1] = YES;
					fromI = tempInt;
					foundComma = NO;
					}
					
				expecting  = Expecting(COMMA);
				expecting |= Expecting(DASH);
				expecting |= Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(DASH))
				{
				foundDash = YES;
				expecting = Expecting(NUMBER);
				}
			else
				return (ERROR);
			}
		/* set Nucmodel (nucModel) ************************************************************/
		else if (!strcmp(parmName, "Nucmodel"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					tempInt = NO;
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].nucModel, tempStr);
							modelParams[i].nStates = NumStates (i);
							
							/* set state frequencies back to default */
							strcpy(modelParams[i].stateFreqPr, "Dirichlet");
							strcpy(modelParams[i].stateFreqsFixType, "Equal");
							for (j=0; j<200; j++)
								{
								modelParams[i].stateFreqsFix[j] = 0.0;   
								modelParams[i].stateFreqsDir[j] = 1.0;
								}    
							tempInt = YES;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Nucmodel to %s\n", spacer, modelParams[i].nucModel);
							else
								MrBayesPrint ("%s   Setting Nucmodel to %s for partition %d\n", spacer, modelParams[i].nucModel, i+1);
							}
						}
					if (tempInt == YES)
						MrBayesPrint ("%s   Set state frequency prior to default\n", spacer);
					}
				else
					{
					MrBayesPrint ("%s   Invalid DNA substitution model\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Nst (nst) **********************************************************************/
		else if (!strcmp(parmName, "Nst"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(NUMBER) | Expecting(ALPHA);
			else if (expecting == Expecting(NUMBER) || expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].nst, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Nst to %s\n", spacer, modelParams[i].nst);
							else
								MrBayesPrint ("%s   Setting Nst to %s for partition %d\n", spacer, modelParams[i].nst, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Nst argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Ncat (numGammaCats) ************************************************************/
		else if (!strcmp(parmName, "Ngammacat"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(NUMBER);
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt >= 2 && tempInt < MAX_GAMMA_CATS)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType != CONTINUOUS))
							{
							modelParams[i].numGammaCats = tempInt;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Ngammacat to %d\n", spacer, modelParams[i].numGammaCats);
							else
								MrBayesPrint ("%s   Setting Ngammacat to %d for partition %d\n", spacer, modelParams[i].numGammaCats, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Ngammacat argument (should be between 2 and %d)\n", spacer, MAX_GAMMA_CATS);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Usegibbs (useGibbs) *************************************************************/
		else if (!strcmp(parmName, "Usegibbs"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							if (!strcmp(tempStr, "Yes"))
                                {
                                MrBayesPrint( "%s   Downsampling of site rates ('usegibbs = yes') disabled temporarily because of conflict with likelihood calculators\n", spacer );
                                return (ERROR);
								strcpy(modelParams[i].useGibbs, "Yes");
                                }
                            else
                                {
								strcpy(modelParams[i].useGibbs, "No");
                                }

							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Usegibbs to %s (if applicable)\n", spacer, modelParams[i].useGibbs);
							else
								MrBayesPrint ("%s   Setting Usegibbs to %s (if applicable) for partition %d\n", spacer, modelParams[i].useGibbs, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid argument for Usegibbs (using Gibbs sampling of discrete gamma)\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}			
		/* set Gibbsfreq (gibbsFreq) ************************************************************/
		else if (!strcmp(parmName, "Gibbsfreq"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(NUMBER);
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt >= 1 && tempInt <= 1000)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							modelParams[i].gibbsFreq = tempInt;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Gibbsfreq to %d\n", spacer, modelParams[i].gibbsFreq);
							else
								MrBayesPrint ("%s   Setting Gibbsfreq to %d for partition %d\n", spacer, modelParams[i].gibbsFreq, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Gibbsgammafreq argument (should be between 1 and 1000)\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set NumM10GammaCats (numM10GammaCats) ************************************************************/
		else if (!strcmp(parmName, "NumM10GammaCats"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(NUMBER);
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt >= 2 && tempInt < MAX_GAMMA_CATS)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType != CONTINUOUS))
							{
							modelParams[i].numM10GammaCats = tempInt;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting NumM10GammaCats to %d\n", spacer, modelParams[i].numM10GammaCats);
							else
								MrBayesPrint ("%s   Setting NumM10GammaCats to %d for partition %d\n", spacer, modelParams[i].numM10GammaCats, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid NumM10GammaCats argument (should be between 2 and %d)\n", spacer, MAX_GAMMA_CATS);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set NumM10BetaCats (numM10BetaCats) ************************************************************/
		else if (!strcmp(parmName, "NumM10BetaCats"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(NUMBER);
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt >= 2 && tempInt < MAX_GAMMA_CATS)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType != CONTINUOUS))
							{
							modelParams[i].numM10BetaCats = tempInt;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting NumM10BetaCats to %d\n", spacer, modelParams[i].numM10BetaCats);
							else
								MrBayesPrint ("%s   Setting NumM10BetaCats to %d for partition %d\n", spacer, modelParams[i].numM10BetaCats, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid NumM10GammaCats argument (should be between 2 and %d)\n", spacer, MAX_GAMMA_CATS);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Nbetacat (numBetaCats) *****************************************************/
		else if (!strcmp(parmName, "Nbetacat"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(NUMBER);
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt >= 2 && tempInt < MAX_GAMMA_CATS)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == STANDARD || modelParams[i].dataType == RESTRICTION))
							{
							modelParams[i].numBetaCats = tempInt;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Nbetacat to %d\n", spacer, modelParams[i].numBetaCats);
							else
								MrBayesPrint ("%s   Setting Nbetacat to %d for partition %d\n", spacer, modelParams[i].numBetaCats, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Nbetacat argument (should be between 2 and %d)\n", spacer, MAX_GAMMA_CATS);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Aamodel (aaModel) **************************************************************/
		else if (!strcmp(parmName, "Aamodel"))
			{
			MrBayesPrint ("%s   Aamodel argument for lset deprecated.\n", spacer);
			MrBayesPrint ("%s   Use 'prset aamodelpr=fixed(<aamodel>)' instead.\n", spacer);
			return (ERROR);
			}
		/* set Parsmodel (useParsModel) *******************************************************/
		else if (!strcmp(parmName, "Parsmodel"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							if (!strcmp(tempStr, "Yes"))
								strcpy(modelParams[i].parsModel, "Yes");
							else
								strcpy(modelParams[i].parsModel, "No");

							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Parsmodel to %s\n", spacer, modelParams[i].parsModel);
							else
								MrBayesPrint ("%s   Setting Parsmodel to %s for partition %d\n", spacer, modelParams[i].parsModel, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid argument for using (so-called) parsimony model\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}			
		/* set Augment (augmentData) **********************************************************/
		else if (!strcmp(parmName, "Augment"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
							{
							if (!strcmp(tempStr, "Yes"))
								strcpy(modelParams[i].augmentData, "Yes");
							else
								strcpy(modelParams[i].augmentData, "No");

							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Augmentdata to %s\n", spacer, modelParams[i].augmentData);
							else
								MrBayesPrint ("%s   Setting Augmentdata to %s for partition %d\n", spacer, modelParams[i].augmentData, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid argument for data augmentation\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}			
		/* set Omegavar (wVarModel) ***********************************************************/
		else if (!strcmp(parmName, "Omegavar"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].omegaVar, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Omegavar to %s\n", spacer, modelParams[i].omegaVar);
							else
								MrBayesPrint ("%s   Setting Omegavar to %s for partition %d\n", spacer, modelParams[i].omegaVar, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid omega variation argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Code (codeModel) ***************************************************************/
		else if (!strcmp(parmName, "Code"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].geneticCode, tempStr);
							SetCode (i);
							modelParams[i].nStates = NumStates (i);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Code to %s\n", spacer, modelParams[i].geneticCode);
							else
								MrBayesPrint ("%s   Setting Code to %s for partition %d\n", spacer, modelParams[i].geneticCode, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid genetic code argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Ploidy (ploidy) ***************************************************************/
		else if (!strcmp(parmName, "Ploidy"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].ploidy, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting ploidy level to %s\n", spacer, modelParams[i].ploidy);
							else
								MrBayesPrint ("%s   Setting ploidy level to %s for partition %d\n", spacer, modelParams[i].ploidy, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid ploidy level argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Rates (ratesModel) *************************************************************/
		else if (!strcmp(parmName, "Rates"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
							{
							if (!strcmp(tempStr, "Adgamma") && (modelParams[i].dataType != DNA && modelParams[i].dataType != RNA && modelParams[i].dataType != PROTEIN))
								{
								/* we won't apply an adgamma model to anything but DNA, RNA, or PROTEIN data */
								}
							else if ((!strcmp(tempStr, "Propinv") ||  !strcmp(tempStr, "Invgamma")) && (modelParams[i].dataType == STANDARD || modelParams[i].dataType == RESTRICTION))
								{
								/* we will not apply pinvar to standard or restriction site data */
								}
							else
								{
								strcpy(modelParams[i].ratesModel, tempStr);
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Rates to %s\n", spacer, modelParams[i].ratesModel);
								else
									MrBayesPrint ("%s   Setting Rates to %s for partition %d\n", spacer, modelParams[i].ratesModel, i+1);
								}
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Rates argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Covarion (covarionModel) *******************************************************/
		else if (!strcmp(parmName, "Covarion"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA || modelParams[i].dataType == PROTEIN))
							{
							strcpy(modelParams[i].covarionModel, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Covarion to %s\n", spacer, modelParams[i].covarionModel);
							else
								MrBayesPrint ("%s   Setting Covarion to %s for partition %d\n", spacer, modelParams[i].covarionModel, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Rates argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Coding (missingType) ***********************************************************/
		else if (!strcmp(parmName, "Coding"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == RESTRICTION || modelParams[i].dataType == STANDARD))
							{
							strcpy(modelParams[i].coding, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Coding to %s\n", spacer, modelParams[i].coding);
							else
								MrBayesPrint ("%s   Setting Coding to %s for partition %d\n", spacer, modelParams[i].coding, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid argument for missing patterns\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
				
				
				
				
		else
			return (ERROR);
		}

	return (NO_ERROR);

}





int DoPropset (void)

{

	MrBayesPrint ("%s   Successfully set proposal parameters\n", spacer);
	
	return (NO_ERROR);
	
}





int DoPropsetParm (char *parmName, char *tkn)

{

	int                 i, j, k, nMatches, tempInt;
	static char		    *temp=NULL, *localTkn=NULL; /*freed at the end of the call*/
	MrBFlt				tempFloat;
	static MCMCMove	    *mv = NULL;
	static MrBFlt		*theValue, theValueMin, theValueMax;
	static int			jump, runIndex, chainIndex;
	static char			*tempName=NULL; /*not freed at the end of the call*/

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before proposal parameters can be changed\n", spacer);
		return (ERROR);
		}

	if (expecting == Expecting(PARAMETER))
		{
		if (!strcmp(parmName, "Xxxxxxxxxx"))
			{
			/* we expect a move name with possible run and chain specification as follows:
			   <move_name>$<tuning_param_name>(<run>,<chain>)=<number>   -- apply to run <run> and chain <chain>
			   <move_name>$<tuning_param_name>(,<chain>)=<number>        -- apply to chain <chain> for all runs
			   <move_name>$<tuning_param_name>(<run>,)=<number>          -- apply to all chains of run <run>
			   <move_name>$prob(<run>,<chain>)=<number>                  -- change relative proposal probability
			   <move_name>$targetrate(<run>,<chain>)=<number>            -- change target acc rate for autotuning

		       the parsing is complicated by the fact that the move name can look something like:
			   eTBR(Tau{all})
			   eTBR(Tau{1,4,5})
			   so we need to assemble the move name from several tokens that are parsed out separately;
			   here we receive only the first part (before the left parenthesis)
			*/
			
			/* copy to local move name */
            SafeStrcpy(&tempName, tkn);
			mv = NULL;
			foundComma = foundEqual = NO;
			expecting = Expecting(LEFTCURL) | Expecting(RIGHTCURL) | Expecting(COMMA) |
				Expecting(LEFTPAR) | Expecting(RIGHTPAR) | Expecting(NUMBER) | Expecting(ALPHA) |
				Expecting(DOLLAR);
			}
		else
			return (ERROR);
		}
	else if (expecting == Expecting(ALPHA))
		{
		if (mv == NULL)
			{
			/* we are still assembling the move name */
            SafeStrcat(&tempName, tkn);
			expecting = Expecting(LEFTCURL) | Expecting(RIGHTCURL) | Expecting(COMMA) |
				Expecting(LEFTPAR) | Expecting(RIGHTPAR) | Expecting(NUMBER) | Expecting(ALPHA) |
				Expecting(DOLLAR);
			}
		else
			{
			/* we have a parameter name; now find the parameter name, case insensitive */
            SafeStrcpy(&localTkn, tkn);
			for (i=0; i<(int)strlen(localTkn); i++)
				localTkn[i] = tolower(localTkn[i]);
			nMatches = j = 0;
			for (i=0; i<mv->moveType->numTuningParams; i++)
				{
				SafeStrcpy(&temp, mv->moveType->shortTuningName[i]);
				for (k=0; k<(int)strlen(temp); k++)
					temp[k] = tolower(temp[k]);
				if (strncmp(localTkn,temp,strlen(localTkn)) == 0)
					{
					j = i;
					nMatches++;
					}
				}
			if (strncmp(localTkn,"prob",strlen(localTkn)) == 0)
				{
				j = -1;
				nMatches++;
				}
			else if (strncmp(localTkn,"targetrate",strlen(localTkn)) == 0)
				{
				j = -2;
				nMatches++;
				}
			if (nMatches == 0)
				{
				MrBayesPrint ("%s   Could not find move parameter to change '%s'\n", spacer, localTkn);  
				return (ERROR);
				}
			else if (nMatches > 1)
				{
				MrBayesPrint ("%s   Several move parameters matched the abbreviated name '%s'\n", spacer, localTkn);
				return (ERROR);
				}
			
			if (j == -1)
				{
				theValue = mv->relProposalProb;
				theValueMin = 0.0;
				theValueMax = 1000.0;
				jump = 1;
				}
			else if (j == -2)
				{
				theValue = mv->targetRate;
				theValueMin = 0.10;
				theValueMax = 0.70;
				jump = 1;
				}
			else
				{
				theValue = &mv->tuningParam[0][j];
				theValueMin = mv->moveType->minimum[j];
				theValueMax = mv->moveType->maximum[j];
				jump = mv->moveType->numTuningParams;
				}
			chainIndex = -1;
			runIndex = -1;
			expecting = Expecting(LEFTPAR) | Expecting(EQUALSIGN);
			}
		}
	else if (expecting == Expecting(LEFTCURL) || expecting == Expecting(RIGHTCURL))
		{
		/* we are still assembling the move name */
		SafeStrcat (&tempName, tkn);
		expecting = Expecting(LEFTCURL) | Expecting(RIGHTCURL) | Expecting(COMMA) |
			Expecting(LEFTPAR) | Expecting(RIGHTPAR) | Expecting(NUMBER) | Expecting(ALPHA) |
			Expecting(DOLLAR);
		}
	else if (expecting == Expecting(DOLLAR))
		{
		/* we know that the name is complete now; find the move by its name, 
		   case insensitive */
		SafeStrcpy(&localTkn, tempName);
        j=(int)strlen(localTkn);
		for (i=0; i<j; i++)
			localTkn[i] = tolower(localTkn[i]);
			
		/* find the move */
		nMatches = j = 0;
		for (i=0; i<numApplicableMoves; i++)
			{
			mv = moves[i];
			SafeStrcpy(&temp,mv->name);
			for (k=0; k<(int)strlen(temp); k++)
				temp[k] = tolower(temp[k]);
			if (strncmp(temp,localTkn,strlen(localTkn)) == 0)
				{
				j = i;
				nMatches++;
				}
			}
		if (nMatches == 0)
			{
			MrBayesPrint ("%s   Could not find move '%s'\n", spacer, localTkn);   
			return (ERROR);
			}
		else if (nMatches > 1)
			{
			MrBayesPrint ("%s   Several moves matched the abbreviated name '%s'\n", spacer, localTkn);   
			return (ERROR);
			}
		else
			mv = moves[j];

		foundComma = foundEqual = NO;
		expecting = Expecting(ALPHA);
		}
	else if (expecting == Expecting(LEFTPAR))
		{
		if (mv == NULL)
			{
			/* we are still assembling the move name */
			SafeStrcat (&tempName, tkn);
			expecting = Expecting(LEFTCURL) | Expecting(RIGHTCURL) | Expecting(COMMA) |
				Expecting(LEFTPAR) | Expecting(RIGHTPAR) | Expecting(NUMBER) | Expecting(ALPHA) |
				Expecting(DOLLAR);
			}
		else /* if (mv != NULL) */
			{
			/* we will be reading in run and chain indices */
			expecting = Expecting(NUMBER) | Expecting(COMMA);
			}
		}
	else if (expecting == Expecting(NUMBER))
		{
		if (mv == NULL)
			{
			/* we are still assembling the move name */
			SafeStrcat (&tempName, tkn);
			expecting = Expecting(LEFTCURL) | Expecting(RIGHTCURL) | Expecting(COMMA) |
				Expecting(LEFTPAR) | Expecting(RIGHTPAR) | Expecting(NUMBER) | Expecting(ALPHA) |
				Expecting(DOLLAR);
			}
		else if (foundEqual == YES)
			{
			sscanf (tkn, "%lf", &tempFloat);
			if (tempFloat < theValueMin || tempFloat > theValueMax)
				{
				MrBayesPrint ("%s   The value is out of range (min = %lf; max = %lf)\n", spacer, theValueMin, theValueMax);
				return (ERROR);
				}
			if (runIndex == -1 && chainIndex == -1)
				{
				for (i=0; i<chainParams.numRuns; i++)
					{
					for (j=0; j<chainParams.numChains; j++)
						{
						*theValue = tempFloat;
						theValue += jump;
						}
					}
				}
			else if (runIndex == -1 && chainIndex >= 0)
				{
				theValue += chainIndex*jump;
				for (i=0; i<chainParams.numRuns; i++)
					{
					*theValue = tempFloat;
					theValue += chainParams.numChains*jump;
					}
				}
			else if (runIndex >= 0 && chainIndex == -1)
				{
				theValue += runIndex*chainParams.numChains*jump;
				for (i=0; i<chainParams.numChains; i++)
					{
					*theValue = tempFloat;
					theValue += jump;
					}
				}
			else /* if (runIndex >= 0 && chainIndex >= 0) */
				{
				theValue[runIndex*chainParams.numChains*jump+chainIndex*jump] = tempFloat;
				}
			expecting = Expecting (PARAMETER) | Expecting(SEMICOLON);
			}
		else /* if (foundEqual == NO) */
			{
			sscanf (tkn, "%d", &tempInt);
			if (foundComma == NO)
				{
				if (tempInt <= 0 || tempInt > chainParams.numRuns)
					{
					MrBayesPrint ("%s   Run index is out of range (min=1; max=%d)\n", spacer, chainParams.numRuns);
					return (ERROR);
					}
				runIndex = tempInt - 1;
				expecting = Expecting(COMMA);
				}
			else
				{
				if (tempInt <= 0 || tempInt > chainParams.numChains)
					{
					MrBayesPrint ("%s   Chain index is out of range (min=1; max=%d)\n", spacer, chainParams.numChains);
					return (ERROR);
					}
				chainIndex = tempInt - 1;
				expecting = Expecting(RIGHTPAR);
				}
			}
		}
	else if (expecting == Expecting(COMMA))
		{
		if (mv == NULL)
			{
			/* we are still assembling the move name */
			SafeStrcat (&tempName, tkn);
			expecting = Expecting(LEFTCURL) | Expecting(RIGHTCURL) | Expecting(COMMA) |
				Expecting(LEFTPAR) | Expecting(RIGHTPAR) | Expecting(NUMBER) | Expecting(ALPHA) |
				Expecting(DOLLAR);
			}
		else
			{
			/* we will be reading in chain index, if present */
			foundComma = YES;
			expecting = Expecting(RIGHTPAR) | Expecting(NUMBER);
			}
		}
	else if (expecting == Expecting(RIGHTPAR))
		{
		if (mv == NULL)
			{
			/* we are still assembling the move name */
			SafeStrcat (&tempName, tkn);
			expecting = Expecting(LEFTCURL) | Expecting(RIGHTCURL) | Expecting(COMMA) |
				Expecting(LEFTPAR) | Expecting(RIGHTPAR) | Expecting(NUMBER) | Expecting(ALPHA) |
				Expecting(DOLLAR);
			}
		else
			expecting = Expecting(EQUALSIGN);
		}
	else if (expecting == Expecting(EQUALSIGN))
		{
		foundEqual = YES;
		expecting = Expecting(NUMBER);
		}
	else
		return (ERROR);


    SafeFree ((void **)&temp);
    SafeFree ((void **)&localTkn);
	return (NO_ERROR);
}





int DoPrset (void)

{

	int			i, nApplied, lastActive=0;

	nApplied = NumActiveParts ();
	for (i=numCurrentDivisions; i>=0; i--)
		{
		if (activeParts[i] == YES)
			{
			lastActive = i;
			break;
			}
		}
			
	if (numCurrentDivisions == 1)
		MrBayesPrint ("%s   Successfully set prior model parameters\n", spacer);
	else 
		{
		if (nApplied == numCurrentDivisions || nApplied == 0)
			{
			MrBayesPrint ("%s   Successfully set prior model parameters to all\n", spacer);
			MrBayesPrint ("%s   applicable data partitions \n", spacer);
			}
		else
			{
			MrBayesPrint ("%s   Successfully set prior model parameters to\n", spacer);
			if (nApplied == 1)
				MrBayesPrint ("%s   partition", spacer);
			else
				MrBayesPrint ("%s   partitions", spacer);
			for (i=0; i<numCurrentDivisions; i++)
				{
				if (activeParts[i] == YES)
					{
					if (i == lastActive && nApplied > 1)
						MrBayesPrint (" and %d", i+1);
					else
						MrBayesPrint (" %d", i+1);
					if (nApplied > 2 && i != lastActive)
						MrBayesPrint (",");
					}
				}
			MrBayesPrint (" (if applicable)\n");
			}
		}
	
	if (SetUpAnalysis (&globalSeed) == ERROR)
		return (ERROR);

	return (NO_ERROR);
	
}





int DoPrsetParm (char *parmName, char *tkn)

{
    
    int			i, j, k, tempInt, nApplied, index, ns, flag=0;
	MrBFlt		tempD, sum;
	char		tempStr[100];

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before the model can be defined\n", spacer);
		return (ERROR);
		}
    if (inValidCommand == YES)
		{
		for (i=0; i<numCurrentDivisions; i++)
			activeParts[i] = NO;
		inValidCommand = NO;
		}

	if (expecting == Expecting(PARAMETER))
		{
		expecting = Expecting(EQUALSIGN);
		}
	else
		{
		/* set Applyto (Applyto) *************************************************************/
		if (!strcmp(parmName, "Applyto"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(LEFTPAR);
			else if (expecting == Expecting(LEFTPAR))
				{
				for (i=0; i<numCurrentDivisions; i++)
					activeParts[i] = NO;
				fromI = toJ = -1;
				foundDash = NO;
				expecting = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				if (fromI != -1)
					activeParts[fromI-1] = YES;
#				if 0
				for (i=0; i<numCurrentDivisions; i++)
					MrBayesPrint("%d ", activeParts[i]);
				MrBayesPrint ("\n");
#				endif
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else if (expecting == Expecting(COMMA))
				{
				foundComma = YES;
				expecting = Expecting(NUMBER);
				}
			else if (expecting == Expecting(ALPHA))
				{
				if (IsSame ("All", tkn) == DIFFERENT)
					{
					MrBayesPrint ("%s   Do not understand delimiter \"%s\"\n", spacer, tkn);
					return (ERROR);
					}
				for (i=0; i<numCurrentDivisions; i++)
					activeParts[i] = YES;
				expecting  = Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt > numCurrentDivisions)
					{
					MrBayesPrint ("%s   Partition delimiter is too large\n", spacer);
					return (ERROR);
					}
				if (fromI == -1)
					fromI = tempInt;
				else if (fromI != -1 && toJ == -1 && foundDash == YES && foundComma == NO)
					{
					toJ = tempInt;
					for (i=fromI-1; i<toJ; i++)
						activeParts[i] = YES;
					fromI = toJ = -1;
					foundDash = NO;
					}
				else if (fromI != -1 && toJ == -1 && foundDash == NO && foundComma == YES)
					{
					activeParts[fromI-1] = YES;
					fromI = tempInt;
					foundComma = NO;
					}
					
				expecting  = Expecting(COMMA);
				expecting |= Expecting(DASH);
				expecting |= Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(DASH))
				{
				foundDash = YES;
				expecting = Expecting(NUMBER);
				}
			else
				return (ERROR);
			}
		/* set Tratiopr (tRatioPr) ************************************************************/
		else if (!strcmp(parmName, "Tratiopr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					flag=0;
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].tRatioPr, tempStr);
							modelParams[i].tRatioDir[0] = modelParams[i].tRatioDir[1] = 1.0;
							modelParams[i].tRatioFix = 1.0;
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing either DNA or RNA data.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Tratiopr argument \n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].tRatioPr,"Beta"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (tempD > ALPHA_MAX)
								{
								MrBayesPrint ("%s   Beta parameter cannot be greater than %1.2lf\n", spacer, ALPHA_MAX);
								return (ERROR);
								}
							if (tempD < ALPHA_MIN)
								{
								MrBayesPrint ("%s   Beta parameter cannot be less than %1.2lf\n", spacer, ALPHA_MIN);
								return (ERROR);
								}
							modelParams[i].tRatioDir[numVars[i]++] = tempD;
							if (numVars[i] < 2)
								expecting = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Tratiopr to Beta(%1.2lf,%1.2lf)\n", spacer, modelParams[i].tRatioDir[0], modelParams[i].tRatioDir[1]);
								else
									MrBayesPrint ("%s   Setting Tratiopr to Beta(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].tRatioDir[0], modelParams[i].tRatioDir[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].tRatioPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].tRatioFix = tempD;
							if (modelParams[i].tRatioFix > KAPPA_MAX)
								{
								MrBayesPrint ("%s   Tratio cannot be greater than %1.2lf\n", spacer, KAPPA_MAX);
								return (ERROR);
								}
							if (modelParams[i].tRatioFix < 0.0)
								{
								MrBayesPrint ("%s   Tratio cannot be less than %1.2lf\n", spacer, 0.0);
								return (ERROR);
								}
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Tratiopr to Fixed(%1.2lf)\n", spacer, modelParams[i].tRatioFix);
							else
								MrBayesPrint ("%s   Setting Tratiopr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].tRatioFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Revmatpr (revMatPr) ************************************************************/
		else if (!strcmp(parmName, "Revmatpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].revMatPr, tempStr);
							modelParams[i].revMatDir[0] = modelParams[i].revMatDir[1] = 1.0;
							modelParams[i].revMatDir[2] = modelParams[i].revMatDir[3] = 1.0;
							modelParams[i].revMatDir[4] = modelParams[i].revMatDir[5] = 1.0;
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing either DNA or RNA data.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Revmatpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				tempNumStates = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				/* find out what type of prior is being set */
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						strcpy (tempStr,modelParams[i].revMatPr);
					}
				/* find and store the number */
				sscanf (tkn, "%lf", &tempD);
				if (!strcmp(tempStr,"Dirichlet"))
					{
					if (tempD > ALPHA_MAX)
						{
						MrBayesPrint ("%s   Dirichlet parameter cannot be greater than %1.2lf\n", spacer, ALPHA_MAX);
						return (ERROR);
						}
					if (tempD < ALPHA_MIN)
						{
						MrBayesPrint ("%s   Dirichlet parameter cannot be less than %1.2lf\n", spacer, ALPHA_MIN);
						return (ERROR);
						}
					}
				else if (!strcmp(tempStr,"Fixed"))
					{
					if (tempD > KAPPA_MAX)
						{
						MrBayesPrint ("%s   Rate value cannot be greater than %1.2lf\n", spacer, KAPPA_MAX);
						return (ERROR);
						}
					if (tempD < 0.0001)
						{
						MrBayesPrint ("%s   Rate value cannot be less than %1.2lf\n", spacer, 0.0001);
						return (ERROR);
						}
					}
				tempNum[tempNumStates++] = tempD;
				if (tempNumStates == 1 && !strcmp(tempStr,"Dirichlet"))
					expecting = Expecting(COMMA) | Expecting(RIGHTPAR);
				else if (tempNumStates < 6)
					expecting  = Expecting(COMMA);
				else
					expecting = Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].revMatPr,"Dirichlet"))
							{
							for (j=0; j<6; j++)
								{
								if (tempNumStates == 1)
									modelParams[i].revMatDir[j] = tempNum[0] / (MrBFlt) 6.0;
								else
									modelParams[i].revMatDir[j] = tempNum[j];
								}

							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Revmatpr to Dirichlet(%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer, 
								modelParams[i].revMatDir[0], modelParams[i].revMatDir[1], modelParams[i].revMatDir[2],
								modelParams[i].revMatDir[3], modelParams[i].revMatDir[4], modelParams[i].revMatDir[5]);
							else
								MrBayesPrint ("%s   Setting Revmatpr to Dirichlet(%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf) for partition %d\n", spacer, 
								modelParams[i].revMatDir[0], modelParams[i].revMatDir[1], modelParams[i].revMatDir[2],
								modelParams[i].revMatDir[3], modelParams[i].revMatDir[4], modelParams[i].revMatDir[5], i+1);
							}
						else if (!strcmp(modelParams[i].revMatPr,"Fixed"))
							{
							for (j=0; j<6; j++)
								modelParams[i].revMatFix[j] = tempNum[j];
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Revmatpr to Fixed(%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer, 
								modelParams[i].revMatFix[0], modelParams[i].revMatFix[1], modelParams[i].revMatFix[2],
								modelParams[i].revMatFix[3], modelParams[i].revMatFix[4], modelParams[i].revMatFix[5]);
							else
								MrBayesPrint ("%s   Setting Revmatpr to Fixed(%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf) for partition %d\n", spacer, 
								modelParams[i].revMatFix[0], modelParams[i].revMatFix[1], modelParams[i].revMatFix[2],
								modelParams[i].revMatFix[3], modelParams[i].revMatFix[4], modelParams[i].revMatFix[5], i+1);
							}
						}
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Aarevmatpr (aaRevMatPr) ********************************************************/
		else if (!strcmp(parmName, "Aarevmatpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && modelSettings[i].dataType == PROTEIN)
							{
							strcpy(modelParams[i].aaRevMatPr, tempStr);
							for (j=0; j<190; j++)
								modelParams[i].aaRevMatDir[j] = 1.0;
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing PROTEIN.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Aarevmatpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				tempNumStates = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				/* find out what type of prior is being set */
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && modelSettings[i].dataType == PROTEIN)
						strcpy (tempStr,modelParams[i].aaRevMatPr);
					}
				/* find and store the number */
				sscanf (tkn, "%lf", &tempD);
				if (!strcmp(tempStr,"Dirichlet"))
					{
					if (tempD > ALPHA_MAX)
						{
						MrBayesPrint ("%s   Dirichlet parameter cannot be greater than %1.2lf\n", spacer, ALPHA_MAX);
						return (ERROR);
						}
					if (tempD < ALPHA_MIN)
						{
						MrBayesPrint ("%s   Dirichlet parameter cannot be less than %1.2lf\n", spacer, ALPHA_MIN);
						return (ERROR);
						}
					}
				else if (!strcmp(tempStr,"Fixed"))
					{
					if (tempD > KAPPA_MAX)
						{
						MrBayesPrint ("%s   Rate value cannot be greater than %1.2lf\n", spacer, KAPPA_MAX);
						return (ERROR);
						}
					if (tempD < 0.0001)
						{
						MrBayesPrint ("%s   Rate value cannot be less than %1.2lf\n", spacer, 0.0001);
						return (ERROR);
						}
					}
				tempStateFreqs[tempNumStates++] = tempD;
				if (tempNumStates == 1 && !strcmp(tempStr,"Dirichlet"))
					expecting = Expecting(COMMA) | Expecting(RIGHTPAR);
				else if (tempNumStates < 190)
					expecting  = Expecting(COMMA);
				else
					expecting = Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType == PROTEIN)
						{
						if (!strcmp(modelParams[i].aaRevMatPr,"Dirichlet"))
							{
							for (j=0; j<190; j++)
								{
								if (tempNumStates == 1)
									modelParams[i].aaRevMatDir[j] = tempStateFreqs[0] / (MrBFlt) 190.0;
								else
									modelParams[i].aaRevMatDir[j] = tempStateFreqs[j];
								}
							if (nApplied == 0 && numCurrentDivisions == 1)
								{
								for (j=0; j<190; j++)
									if (AreDoublesEqual(modelParams[i].aaRevMatDir[0], modelParams[i].aaRevMatDir[j], 0.00001) == NO)
										break;
								if (j == 190)
									MrBayesPrint ("%s   Setting Aarevmatpr to Dirichlet(%1.2lf,%1.2lf,...)\n", spacer,
										modelParams[i].aaRevMatDir[0], modelParams[i].aaRevMatDir[0]);
								else
									{
									MrBayesPrint ("%s   Setting Aarevmatpr to Dirichlet(\n", spacer);
									for (j=0; j<190; j++)
										{
										if (j % 10 == 0)
											MrBayesPrint ("%s      ", spacer);
										MrBayesPrint ("%1.2lf", modelParams[i].aaRevMatDir[j]);
										if (j == 189)
											MrBayesPrint (")\n");
										else if ((j+1) % 10 == 0)
											MrBayesPrint (",\n");
										else
											MrBayesPrint (",");
										}
									}
								}
							else
								{
								for (j=0; j<190; j++)
									if (AreDoublesEqual(modelParams[i].aaRevMatDir[0], modelParams[i].aaRevMatDir[j], 0.00001) == NO)
										break;
								if (j == 190)
									MrBayesPrint ("%s   Setting Aarevmatpr to Dirichlet(%1.2lf,%1.2lf,...) for partition %d\n",
										spacer, modelParams[i].aaRevMatDir[0], modelParams[i].aaRevMatDir[0], i+1);
								else
									{
									MrBayesPrint ("%s   Setting Aarevmatpr to Dirichlet(\n", spacer);
									for (j=0; j<190; j++)
										{
										if (j % 10 == 0)
											MrBayesPrint ("%s      ", spacer);
										MrBayesPrint ("%1.2lf", modelParams[i].aaRevMatDir[j]);
										if (j == 189)
											MrBayesPrint (")\n");
										else if ((j+1) % 10 == 0)
											MrBayesPrint (",\n");
										else
											MrBayesPrint (",");
										}
									}
									MrBayesPrint ("%s      for partition %d\n", spacer, i+1);
								}
							}
						else if (!strcmp(modelParams[i].aaRevMatPr,"Fixed"))
							{
							for (j=0; j<190; j++)
								modelParams[i].aaRevMatFix[j] = tempStateFreqs[j];
							if (nApplied == 0 && numCurrentDivisions == 1)
								{
								for (j=0; j<190; j++)
									if (AreDoublesEqual(modelParams[i].aaRevMatFix[0], modelParams[i].aaRevMatFix[j], 0.00001) == NO)
										break;
								if (j == 190)
									MrBayesPrint ("%s   Setting Aarevmatpr to Fixed(%1.2lf,%1.2lf,...)\n", spacer, modelParams[i].aaRevMatFix[0],
										modelParams[i].aaRevMatFix[0]);
								else
									{
									MrBayesPrint ("%s   Setting Aarevmatpr to Fixed(\n", spacer);
									for (j=0; j<190; j++)
										{
										if (j % 10 == 0)
											MrBayesPrint ("%s      ", spacer);
										MrBayesPrint ("%1.2lf", modelParams[i].aaRevMatFix[j]);
										if (j == 189)
											MrBayesPrint (")\n");
										else if ((j+1) % 10 == 0)
											MrBayesPrint (",\n");
										else
											MrBayesPrint (",");
										}
									}
								}
							else
								{
								for (j=0; j<190; j++)
									if (AreDoublesEqual(modelParams[i].aaRevMatFix[0], modelParams[i].aaRevMatFix[j], 0.00001) == NO)
										break;
								if (j == 190)
									MrBayesPrint ("%s   Setting Aarevmatpr to Fixed(%1.2lf,%1.2lf,...) for partition %d\n", spacer,
										modelParams[i].aaRevMatFix[0], modelParams[i].aaRevMatFix[0], i+1);
								else
									{
									MrBayesPrint ("%s   Setting Aarevmatpr to Fixed(\n", spacer);
									for (j=0; j<190; j++)
										{
										if (j % 10 == 0)
											MrBayesPrint ("%s      ", spacer);
										MrBayesPrint ("%1.2lf", modelParams[i].aaRevMatFix[j]);
										if (j == 189)
											MrBayesPrint (")\n");
										else if ((j+1) % 10 == 0)
											MrBayesPrint (",\n");
										else
											MrBayesPrint (",");
										}
									}
									MrBayesPrint ("%s      for partition %d\n", spacer, i+1);
								}
							}
						}
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Revratepr (revSymDirPr) ****************************************************/
		else if (!strcmp(parmName, "Revratepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == ERROR)  /* we only allow symmetric dirichlet prior, so no need to store the value */
					{
					MrBayesPrint ("%s   Invalid Revratepr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						sscanf (tkn, "%lf", &tempD);
						if (tempD <= 0.0)
							{
							MrBayesPrint ("%s   Symmetric Dirichlet parameter must be positive\n", spacer);
							return (ERROR);
							}
						modelParams[i].revMatSymDir = tempD;
						if (nApplied == 0 && numCurrentDivisions == 1)
                            MrBayesPrint ("%s   Setting Revratepr to Symmetric Dirichlet(%1.2lf)\n", spacer, modelParams[i].revMatSymDir);
						else
                            MrBayesPrint ("%s   Setting Revratepr to Symmetric Dirichlet(%1.2lf) for partition %d\n", spacer, modelParams[i].revMatSymDir, i+1);
						expecting  = Expecting(RIGHTPAR);
						}
					}
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Omegapr (omegaPr) **************************************************************/
		else if (!strcmp(parmName, "Omegapr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].omegaPr, tempStr);
							modelParams[i].omegaDir[0] = modelParams[i].omegaDir[1] = 1.0;
							modelParams[i].omegaFix = 1.0;
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing either DNA or RNA data.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Omegapr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].omegaPr,"Dirichlet"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (tempD > ALPHA_MAX)
								{
								MrBayesPrint ("%s   Dirichlet parameter cannot be greater than %1.2lf\n", spacer, ALPHA_MAX);
								return (ERROR);
								}
							if (tempD < ALPHA_MIN)
								{
								MrBayesPrint ("%s   Dirichlet parameter cannot be less than %1.2lf\n", spacer, ALPHA_MIN);
								return (ERROR);
								}
							modelParams[i].omegaDir[numVars[i]++] = tempD;
							if (numVars[i] < 1)
								expecting = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Omegapr to Dirichlet(%1.2lf,%1.2lf)\n", spacer, modelParams[i].omegaDir[0], modelParams[i].omegaDir[1]);
								else
									MrBayesPrint ("%s   Setting Omegapr to Dirichlet(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].omegaDir[0], modelParams[i].omegaDir[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].omegaPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].omegaFix = tempD;
							if (modelParams[i].omegaFix > KAPPA_MAX)
								{
								MrBayesPrint ("%s   Omega ratio cannot be greater than %1.2lf\n", spacer, KAPPA_MAX);
								return (ERROR);
								}
							if (modelParams[i].omegaFix < 0.0)
								{
								MrBayesPrint ("%s   Omega ratio cannot be less than %1.2lf\n", spacer, 0.0);
								return (ERROR);
								}
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Omegapr to Fixed(%1.2lf)\n", spacer, modelParams[i].omegaFix);
							else
								MrBayesPrint ("%s   Setting Omegapr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].omegaFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Ny98omega1pr (ny98omega1pr) ********************************************************/
		else if (!strcmp(parmName, "Ny98omega1pr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].ny98omega1pr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing either DNA or RNA data.\
							Currently there is no active partition with such data. The setting is ignored.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Ny98omega1pr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].ny98omega1pr,"Beta"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].ny98omega1Beta[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].ny98omega1Beta[0] < 0 || modelParams[i].ny98omega1Beta[1] < 0)
									{
									MrBayesPrint ("%s   Beta parameter should be greater than 0\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Ny98omega1pr to Beta(%1.2lf,%1.2lf)\n", spacer, modelParams[i].ny98omega1Beta[0], modelParams[i].ny98omega1Beta[1]);
								else
									MrBayesPrint ("%s   Setting Ny98omega1pr to Beta(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].ny98omega1Beta[0], modelParams[i].ny98omega1Beta[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].ny98omega1pr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].ny98omega1Fixed = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Ny98omega1pr to Fixed(%1.2lf)\n", spacer, modelParams[i].ny98omega1Fixed);
							else
								MrBayesPrint ("%s   Setting Ny98omega1pr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].ny98omega1Fixed, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Ny98omega3pr (ny98omega3pr) ********************************************************/
		else if (!strcmp(parmName, "Ny98omega3pr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].ny98omega3pr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing either DNA or RNA data.\
							Currently there is no active partition with such data. The setting is ignored.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Ny98omega3pr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].ny98omega3pr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].ny98omega3Uni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].ny98omega3Uni[0] >= modelParams[i].ny98omega3Uni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Ny98omega3pr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].ny98omega3Uni[0], modelParams[i].ny98omega3Uni[1]);
								else
									MrBayesPrint ("%s   Setting Ny98omega3pr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].ny98omega3Uni[0], modelParams[i].ny98omega3Uni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].ny98omega3pr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].ny98omega3Exp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Ny98omega3pr to Exponential(%1.2lf)\n", spacer, modelParams[i].ny98omega3Exp);
							else
								MrBayesPrint ("%s   Setting Ny98omega3pr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].ny98omega3Exp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].ny98omega3pr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].ny98omega3Fixed = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Ny98omega3pr to Fixed(%1.2lf)\n", spacer, modelParams[i].ny98omega3Fixed);
							else
								MrBayesPrint ("%s   Setting Ny98omega3pr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].ny98omega3Fixed, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set M3omegapr (m3omegapr) ********************************************************/
		else if (!strcmp(parmName, "M3omegapr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].m3omegapr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing either DNA or RNA data.\
							Currently there is no active partition with such data. The setting is ignored.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid M3omegapr argument\n", spacer);
					return (ERROR);
					}
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].m3omegapr,"Exponential"))
							{
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting M3omegapr to Exponential\n", spacer);
							else
								MrBayesPrint ("%s   Setting M3omegapr to Exponential for partition %d\n", spacer, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				if (!strcmp(tempStr,"Exponential"))
					expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				else
					expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].m3omegapr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].m3omegaFixed[numVars[i]++] = tempD;
							if (numVars[i] == 1 || numVars[i] == 2)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].m3omegaFixed[0] >= modelParams[i].m3omegaFixed[1] || modelParams[i].m3omegaFixed[0] >= modelParams[i].m3omegaFixed[2] || modelParams[i].m3omegaFixed[1] >= modelParams[i].m3omegaFixed[2])
									{
									MrBayesPrint ("%s   The three omega values must be ordered, such that omega1 < omega2 < omega3\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting M3omegapr to Fixed(%1.2lf,%1.2lf,%1.2lf)\n", spacer, modelParams[i].m3omegaFixed[0], modelParams[i].m3omegaFixed[1], modelParams[i].m3omegaFixed[2]);
								else
									MrBayesPrint ("%s   Setting M3omegapr to Fixed(%1.2lf,%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].m3omegaFixed[0], modelParams[i].m3omegaFixed[1], modelParams[i].m3omegaFixed[2], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Codoncatfreqs (codonCatFreqPr) ********************************************************/
		else if (!strcmp(parmName, "Codoncatfreqs"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].codonCatFreqPr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing either DNA or RNA data.\
							Currently there is no active partition with such data. The setting is ignored.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Omegapurpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].codonCatFreqPr,"Dirichlet"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].codonCatDir[numVars[i]++] = tempD;
							if (numVars[i] == 1 || numVars[i] == 2)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Codoncatfreqs prior to Dirichlet(%1.2lf,%1.2lf,%1.2lf)\n", spacer, modelParams[i].codonCatDir[0], modelParams[i].codonCatDir[1], modelParams[i].codonCatDir[2]);
								else
									MrBayesPrint ("%s   Setting Codoncatfreqs prior to Dirichlet(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].codonCatDir[0], modelParams[i].codonCatDir[1], modelParams[i].codonCatDir[2], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].codonCatFreqPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].codonCatFreqFix[numVars[i]++] = tempD;
							if (numVars[i] == 1 || numVars[i] == 2)
								expecting  = Expecting(COMMA);
							else
								{
								if (AreDoublesEqual (modelParams[i].codonCatFreqFix[0] + modelParams[i].codonCatFreqFix[1] + modelParams[i].codonCatFreqFix[2], (MrBFlt) 1.0, (MrBFlt) 0.001) == NO)
									{
									MrBayesPrint ("%s   Codon category frequencies must sum to 1\n", spacer);
									return (ERROR);
									}
								
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Codoncatfreqs prior to Fixed(%1.2lf,%1.2lf,%1.2lf)\n", spacer, modelParams[i].codonCatFreqFix[0], modelParams[i].codonCatFreqFix[1], modelParams[i].codonCatFreqFix[2]);
								else
									MrBayesPrint ("%s   Setting Codoncatfreqs prior to Fixed(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].codonCatFreqFix[0], modelParams[i].codonCatFreqFix[1], modelParams[i].codonCatFreqFix[2], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}

		/* set Shapepr (shapePr) **************************************************************/
		else if (!strcmp(parmName, "Shapepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA || modelParams[i].dataType == PROTEIN || modelParams[i].dataType == RESTRICTION || modelParams[i].dataType == STANDARD))
							{
							strcpy(modelParams[i].shapePr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing data of at least one of following type: DNA, RNA, PROTEIN, RESTRICTION, STANDARD.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Shapepr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA || modelParams[i].dataType == PROTEIN || modelParams[i].dataType == RESTRICTION || modelParams[i].dataType == STANDARD))
						{
						if (!strcmp(modelParams[i].shapePr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].shapeUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].shapeUni[0] >= modelParams[i].shapeUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (modelParams[i].shapeUni[1] > MAX_SHAPE_PARAM)
									{
									MrBayesPrint ("%s   Upper value for uniform cannot be greater than %1.2lf\n", spacer, MAX_SHAPE_PARAM);
									return (ERROR);
									}
								if (modelParams[i].shapeUni[0] < MIN_SHAPE_PARAM)
									{
									MrBayesPrint ("%s   Lower value for uniform cannot be less than %1.2lf\n", spacer, MIN_SHAPE_PARAM);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Shapepr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].shapeUni[0], modelParams[i].shapeUni[1]);
								else
									MrBayesPrint ("%s   Setting Shapepr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].shapeUni[0], modelParams[i].shapeUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].shapePr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].shapeExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Shapepr to Exponential(%1.2lf)\n", spacer, modelParams[i].shapeExp);
							else
								MrBayesPrint ("%s   Setting Shapepr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].shapeExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].shapePr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].shapeFix = tempD;
							if (modelParams[i].shapeFix > MAX_SHAPE_PARAM)
								{
								MrBayesPrint ("%s   Shape parameter cannot be greater than %1.2lf\n", spacer, MAX_SHAPE_PARAM);
								return (ERROR);
								}
							if (modelParams[i].shapeFix < MIN_SHAPE_PARAM)
								{
								MrBayesPrint ("%s   Shape parameter cannot be less than %1.2lf\n", spacer, MIN_SHAPE_PARAM);
								return (ERROR);
								}
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Shapepr to Fixed(%1.2lf)\n", spacer, modelParams[i].shapeFix);
							else
								MrBayesPrint ("%s   Setting Shapepr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].shapeFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Pinvarpr (pInvarPr) ************************************************************/
		else if (!strcmp(parmName, "Pinvarpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA || modelParams[i].dataType == PROTEIN))
							{
							strcpy(modelParams[i].pInvarPr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing data of at least one of the following type: DNA, RNA, PROTEIN.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Pinvarpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA || modelParams[i].dataType == PROTEIN))
						{
						if (!strcmp(modelParams[i].pInvarPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].pInvarUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].pInvarUni[0] >= modelParams[i].pInvarUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (modelParams[i].pInvarUni[1] > 1.0)
									{
									MrBayesPrint ("%s   Upper value for uniform should be less than or equal to 1.0\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Pinvarpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].pInvarUni[0], modelParams[i].pInvarUni[1]);
								else
									MrBayesPrint ("%s   Setting Pinvarpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].pInvarUni[0], modelParams[i].pInvarUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].pInvarPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (tempD > 1.0)
								{
								MrBayesPrint ("%s   Value for Pinvar should be in the interval (0, 1)\n", spacer);
								return (ERROR);
								}
							modelParams[i].pInvarFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Pinvarpr to Fixed(%1.2lf)\n", spacer, modelParams[i].pInvarFix);
							else
								MrBayesPrint ("%s   Setting Pinvarpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].pInvarFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Ratecorrpr (adGammaCorPr) ******************************************************/
		else if (!strcmp(parmName, "Ratecorrpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].adGammaCorPr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing data of at least one of the following type: DNA, RNA.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Ratecorrpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				foundDash = NO;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting = Expecting(NUMBER) | Expecting(DASH);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].adGammaCorPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (foundDash == YES)
							tempD *= -1.0;
							modelParams[i].corrUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].corrUni[0] >= modelParams[i].corrUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (modelParams[i].corrUni[1] > 1.0)
									{
									MrBayesPrint ("%s   Upper value for uniform should be less than or equal to 1.0\n", spacer);
									return (ERROR);
									}
								if (modelParams[i].corrUni[0] < -1.0)
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than or equal to -1.0\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Ratecorrpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].corrUni[0], modelParams[i].corrUni[1]);
								else
									MrBayesPrint ("%s   Setting Ratecorrpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].corrUni[0], modelParams[i].corrUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].adGammaCorPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (foundDash == YES)
								tempD *= -1.0;
							if (tempD > 1.0 || tempD < -1.0)
								{
								MrBayesPrint ("%s   Value for Ratecorrpr should be in the interval (-1, +1)\n", spacer);
								return (ERROR);
								}
							modelParams[i].corrFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Ratecorrpr to Fixed(%1.2lf)\n", spacer, modelParams[i].corrFix);
							else
								MrBayesPrint ("%s   Setting Ratecorrpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].corrFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				foundDash = NO;
				}
			else if (expecting == Expecting(DASH))
				{
				foundDash = YES;
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER) | Expecting(DASH);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Browncorrpr (brownCorPr) ******************************************************/
		else if (!strcmp(parmName, "Browncorrpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType == CONTINUOUS)
							{
							strcpy(modelParams[i].brownCorPr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing CONTINUOUS data.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Browncorrpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				foundDash = NO;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting = Expecting(NUMBER) | Expecting(DASH);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType == CONTINUOUS)
						{
						if (!strcmp(modelParams[i].brownCorPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (foundDash == YES)
							tempD *= -1.0;
							modelParams[i].brownCorrUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].brownCorrUni[0] >= modelParams[i].brownCorrUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (modelParams[i].brownCorrUni[1] > 1.0)
									{
									MrBayesPrint ("%s   Upper value for uniform should be less than or equal to 1.0\n", spacer);
									return (ERROR);
									}
								if (modelParams[i].brownCorrUni[0] < -1.0)
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than or equal to -1.0\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Browncorrpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].brownCorrUni[0], modelParams[i].brownCorrUni[1]);
								else
									MrBayesPrint ("%s   Setting Browncorrpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].brownCorrUni[0], modelParams[i].brownCorrUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].brownCorPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (foundDash == YES)
								tempD *= -1.0;
							if (tempD > 1.0 || tempD < -1.0)
								{
								MrBayesPrint ("%s   Value for Browncorrpr should be in the interval (-1, +1)\n", spacer);
								return (ERROR);
								}
							modelParams[i].brownCorrFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Browncorrpr to Fixed(%1.2lf)\n", spacer, modelParams[i].brownCorrFix);
							else
								MrBayesPrint ("%s   Setting Browncorrpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].brownCorrFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				foundDash = NO;
				}
			else if (expecting == Expecting(DASH))
				{
				foundDash = YES;
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER) | Expecting(DASH);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Ratepr (ratePr) *****************************************************************/
		else if (!strcmp(parmName, "Ratepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
							{
							if (!strcmp(tempStr,"Variable"))
								strcpy(modelParams[i].ratePr, "Dirichlet");
							else
								strcpy(modelParams[i].ratePr, tempStr);
							modelParams[i].ratePrDir = 1.0;
							if (!strcmp(tempStr,"Variable") || !strcmp(tempStr,"Fixed"))
								{
								if (tempStr[0]=='V')
									strcat (tempStr," [Dirichlet(..,1,..)]");
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Ratepr to %s\n", spacer, tempStr);
								else
									MrBayesPrint ("%s   Setting Ratepr to %s for partition %d\n", spacer, tempStr, i+1);
								if (tempStr[0]=='V')
									strcpy (tempStr,"Variable");
								}
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing CONTINUOUS data.\
							Currently there is no active partition with such data.\n", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Ratepr argument\n", spacer);
					return (ERROR);
					}
				if (!strcmp(tempStr,"Fixed") || !strcmp(tempStr,"Variable"))
					expecting  = Expecting(PARAMETER) | Expecting(SEMICOLON);
				else
					expecting = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting = Expecting (NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				/* find next partition to fill in */
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					if ((activeParts[i] == YES || nApplied == 0) && numVars[i] == 0)
						break;
				if (i == numCurrentDivisions)
					{
					MrBayesPrint ("%s   Could not find first ratemultiplier partition\n", spacer);
					return (ERROR);
					}
				numVars[i] = 1;
				/* read in the parameter */
				sscanf (tkn, "%lf", &tempD);
				if (tempD < ALPHA_MIN || tempD > ALPHA_MAX)
					{
					MrBayesPrint ("%s   Ratemultiplier Dirichlet parameter %lf out of range\n", spacer, tempD);
					return (ERROR);
					}
				/* set the parameter */
				modelParams[i].ratePrDir = tempD;				
				/* check if all partitions have been filled in */
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && numVars[i] == 0)
						break;
					}
				/* set expecting accordingly so that we know what should be coming next */
				if (i == numCurrentDivisions)
					expecting = Expecting (RIGHTPAR);
				else
					expecting = Expecting (COMMA);
				}
			else if (expecting == Expecting (COMMA))
				expecting = Expecting (NUMBER);
			else if (expecting == Expecting (RIGHTPAR))
				{
				/* print message */
				for (i=j=0; i<numCurrentDivisions; i++)
					{
					if (numVars[i] == 1)
						{
						j++;
						if (j == 1)
							{
							MrBayesPrint ("%s   Setting Ratepr to Dirichlet(%1.2f",
								spacer, modelParams[i].ratePrDir);
							}
						else
							MrBayesPrint(",%1.2f", modelParams[i].ratePrDir);
						}
					}
				if (numCurrentDivisions == 1)
					MrBayesPrint (")\n");
				else
					{
					MrBayesPrint (") for partition");
					if (j > 1)
						MrBayesPrint ("s");
					for (i=k=0; i<numCurrentDivisions; i++)
						{
						if (numVars[i] == 1)
							{
							k++;
							if (k == j && j > 1)
								MrBayesPrint (", and %d", i+1);
							else if (k == 1)
								MrBayesPrint (" %d", i+1);
							else
								MrBayesPrint (", %d", i+1);
							}
						}
					MrBayesPrint ("\n");
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Covswitchpr (covSwitchPr) ******************************************************/
		else if (!strcmp(parmName, "Covswitchpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA || modelParams[i].dataType == PROTEIN))
							{
							strcpy(modelParams[i].covSwitchPr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    {
						MrBayesPrint ("%s   Warning: %s can be set only for partition containing data of at least one of the following type: DNA, RNA, PROTEIN.\
							Currently there is no active partition with such data. ", spacer, parmName);
						return (ERROR);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Covswitchpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA || modelParams[i].dataType == PROTEIN))
						{
						if (!strcmp(modelParams[i].covSwitchPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].covswitchUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].covswitchUni[0] >= modelParams[i].covswitchUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Covswitchpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].covswitchUni[0], modelParams[i].covswitchUni[1]);
								else
									MrBayesPrint ("%s   Setting Covswitchpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].covswitchUni[0], modelParams[i].covswitchUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].covSwitchPr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].covswitchExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Covswitchpr to Exponential(%1.2lf)\n", spacer, modelParams[i].covswitchExp);
							else
								MrBayesPrint ("%s   Setting Covswitchpr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].covswitchExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].covSwitchPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].covswitchFix[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Covswitchpr to Fixed(%1.4lf,%1.4lf)\n", spacer, modelParams[i].covswitchFix[0], modelParams[i].covswitchFix[1]);
								else
									MrBayesPrint ("%s   Setting Covswitchpr to Fixed(%1.4lf,%1.4lf) for partition %d\n", spacer, modelParams[i].covswitchFix[0], modelParams[i].covswitchFix[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Symdirihyperpr (symPiPr) ******************************************************/
		else if (!strcmp(parmName, "Symdirihyperpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				{
				foundBeta = NO;
				expecting = Expecting(ALPHA);
				}
			else if (expecting == Expecting(ALPHA))
				{
				if (foundBeta == NO)
					{
					/* expecting to see Uniform, Exponential, or Fixed */
					if (IsArgValid(tkn, tempStr) == NO_ERROR)
						{
						nApplied = NumActiveParts ();
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == STANDARD || modelParams[i].dataType == RESTRICTION))
								{
								strcpy(modelParams[i].symPiPr, tempStr);
								flag=1;
								}
							}
						if( flag == 0)
					    	{
							MrBayesPrint ("%s   Warning: %s can be set only for partition containing data of at least one of the following type: STANDARD, RESTRICTION.\
							Currently there is no active partition with such data. ", spacer, parmName);
							return (ERROR);
							}
						}
					else
						{
						MrBayesPrint ("%s   Invalid Symdirihyperpr argument\n", spacer);
						return (ERROR);
						}
					expecting  = Expecting(LEFTPAR);
					for (i=0; i<numCurrentDivisions; i++)
						numVars[i] = 0;
					foundBeta = YES;	
					}	
				else
					{
					/* expecting infinity */
					if (IsSame("Infinity", tkn) == SAME || IsSame("Infinity", tkn) == CONSISTENT_WITH)
						{
						nApplied = NumActiveParts ();
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == STANDARD || modelParams[i].dataType == RESTRICTION))
								{
								if (!strcmp(modelParams[i].symPiPr, "Fixed"))
									{
									modelParams[i].symBetaFix = -1;
									if (nApplied == 0 && numCurrentDivisions == 1)
										MrBayesPrint ("%s   Setting Symdirihyperpr to Beta(Infinity)\n", spacer);
									else
										MrBayesPrint ("%s   Setting Symdirihyperpr to Beta(Infinity) for partition %d\n", spacer, i+1);
									expecting  = Expecting(RIGHTPAR);
									}
								else
									{
									MrBayesPrint ("%s   Problem setting Symdirihyperpr\n", spacer);
									return (ERROR);
									}
								}
							}
						expecting  = Expecting(RIGHTPAR);
						}
					}		
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == STANDARD || modelParams[i].dataType == RESTRICTION))
						{
						if (!strcmp(modelParams[i].symPiPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].symBetaFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Symdirihyperpr to Fixed(%1.2lf)\n", spacer, modelParams[i].symBetaFix);
							else
								MrBayesPrint ("%s   Setting Symdirihyperpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].symBetaFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].symPiPr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].symBetaExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Symdirihyperpr to Exponential(%1.2lf)\n", spacer, modelParams[i].symBetaExp);
							else
								MrBayesPrint ("%s   Setting Symdirihyperpr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].symBetaExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].symPiPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].symBetaUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)	
								expecting = Expecting(COMMA);
							else
								{
								if (modelParams[i].symBetaUni[0] >= modelParams[i].symBetaUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Symdirihyperpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].symBetaUni[0], modelParams[i].symBetaUni[1]);
								else
									MrBayesPrint ("%s   Setting Symdirihyperpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].symBetaUni[0], modelParams[i].symBetaUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else
							{
							MrBayesPrint ("%s   Problem setting Symdirihyperpr\n", spacer);
							return (ERROR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Statefreqpr (stateFreqPr) ******************************************************/
		else if (!strcmp(parmName, "Statefreqpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsSame ("Equal", tkn) == DIFFERENT && IsSame ("Empirical", tkn) == DIFFERENT)
					{
					/* the user wants to specify a dirichlet or fixed prior */
					if (IsArgValid(tkn, tempStr) == NO_ERROR)
						{
						nApplied = NumActiveParts ();
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
								{
								strcpy(modelParams[i].stateFreqPr, tempStr);
								flag=1;
								}
							}
						if( flag == 0)
					    	{
							MrBayesPrint ("%s   Warning: %s can be set only for partition containing CONTINUOUS data.\
							Currently there is no active partition with such data. ", spacer, parmName);
							return (ERROR);
							}
						}
					else
						{
						MrBayesPrint ("%s   Invalid Statefreqpr argument\n", spacer);
						return (ERROR);
						}
                    // TODO: Here we set flat dirichlet parameters
					expecting  = Expecting(LEFTPAR);
					}
				else
					{
					/* the user wants equal or empirical state frequencies */
					nApplied = NumActiveParts ();
					if (IsSame ("Equal", tkn) == SAME || IsSame ("Equal", tkn) == CONSISTENT_WITH)
						{
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
								strcpy(modelParams[i].stateFreqsFixType, "Equal");
							}
						}
					else if (IsSame ("Empirical", tkn) == SAME || IsSame ("Empirical", tkn) == CONSISTENT_WITH)
						{
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
								strcpy(modelParams[i].stateFreqsFixType, "Empirical");
							}
						}
					else
						{
						MrBayesPrint ("%s   Invalid Statefreqpr delimiter\n", spacer);
						return (ERROR);
						}
					expecting  = Expecting(RIGHTPAR);
					}
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				tempNumStates = 0;
				expecting = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				sscanf (tkn, "%lf", &tempD);
				tempStateFreqs[tempNumStates++] = tempD;
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
						if (!strcmp(modelParams[i].stateFreqPr,"Fixed"))
							strcpy(modelParams[i].stateFreqsFixType, "User");
					}
				expecting = Expecting(COMMA) | Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType != CONTINUOUS)
						{
						ns = NumStates(i);
						if (!strcmp(modelParams[i].stateFreqPr,"Dirichlet"))
							{
							if (tempNumStates == 1)
								{
								for (j=0; j<ns; j++)
									modelParams[i].stateFreqsDir[j] = tempStateFreqs[0] / ns;
								MrBayesPrint ("%s   Setting Statefreqpr to Dirichlet(", spacer);
								for (j=0; j<ns; j++)
									{
									MrBayesPrint("%1.2lf", modelParams[i].stateFreqsDir[j]);
									if (j == ns - 1)
										MrBayesPrint (")");
									else
										MrBayesPrint (",");	
									}	
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("\n");
								else
									MrBayesPrint (" for partition %d\n", i+1); 
								modelParams[i].numDirParams = ns;
								}
							else
								{
								if (tempNumStates != ns)
									{
									MrBayesPrint ("%s   Found %d dirichlet parameters but expecting %d\n", spacer, tempNumStates, modelParams[i].nStates);
									return (ERROR);
									}
								else
									{
									modelParams[i].numDirParams = ns;
									for (j=0; j<ns; j++)
										modelParams[i].stateFreqsDir[j] = tempStateFreqs[j];
									MrBayesPrint ("%s   Setting Statefreqpr to Dirichlet(", spacer);
									for (j=0; j<ns; j++)
										{
										MrBayesPrint("%1.2lf", modelParams[i].stateFreqsDir[j]);
										if (j == ns - 1)
											MrBayesPrint (")");
										else
											MrBayesPrint (",");	
										}	
									if (nApplied == 0 && numCurrentDivisions == 1)
										MrBayesPrint ("\n");
									else
										MrBayesPrint (" for partition %d\n", i+1); 
									}
								}
							}
						else if (!strcmp(modelParams[i].stateFreqPr,"Fixed"))
							{
							if (tempNumStates == 0)
								{
								if (!strcmp(modelParams[i].stateFreqsFixType, "Equal"))
									MrBayesPrint ("%s   Setting Statefreqpr to Fixed(Equal)", spacer);
								else if (!strcmp(modelParams[i].stateFreqsFixType, "Empirical"))
									MrBayesPrint ("%s   Setting Statefreqpr to Fixed(Empirical)", spacer);
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("\n");
								else
									MrBayesPrint (" for partition %d\n", i+1); 
								}
							else 
								{
								if (tempNumStates == ns)
									{
									sum = 0.0;
									for (j=0; j<ns; j++)
										sum += tempStateFreqs[j];
									if (AreDoublesEqual (sum, (MrBFlt) 1.0, (MrBFlt) 0.001) == NO)
										{
										MrBayesPrint ("%s   State frequencies do not sum to 1.0\n", spacer);
										return (ERROR);
										}
									strcpy(modelParams[i].stateFreqsFixType, "User");
									for (j=0; j<ns; j++)
										modelParams[i].stateFreqsFix[j] = tempStateFreqs[j];
									MrBayesPrint ("%s   Setting Statefreqpr to Fixed(", spacer);
									for (j=0; j<ns; j++)
										{
										MrBayesPrint("%1.2lf", modelParams[i].stateFreqsFix[j]);
										if (j == ns - 1)
											MrBayesPrint (")");
										else
											MrBayesPrint (",");	
										}	
									if (nApplied == 0 && numCurrentDivisions == 1)
										MrBayesPrint ("\n");
									else
										MrBayesPrint (" for partition %d\n", i+1); 
									}
								else
									{
									MrBayesPrint ("%s   Found %d state frequencies but expecting %d\n", spacer, tempNumStates, modelParams[i].nStates);
									return (ERROR);
									}
								}
								
							}
						}
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Topologypr (topologyPr) ********************************************************/
		else if (!strcmp(parmName, "Topologypr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				{
				foundEqual = YES;
				expecting = Expecting(ALPHA);
				}
			else if (expecting == Expecting(ALPHA))
				{
				if (foundEqual == YES)
					{
					if (IsArgValid(tkn, tempStr) == NO_ERROR)
						{
                        /* erase previous constraints */
					    nApplied = NumActiveParts ();
					    for (i=0; i<numCurrentDivisions; i++)
						    {
						    if (activeParts[i] == YES || nApplied == 0)
							    {
							    strcpy(modelParams[i].topologyPr, tempStr);
							    /* erase previous constraints, if any */
							    for (j=0; j<numDefinedConstraints; j++)
								    modelParams[i].activeConstraints[j] = NO;
							    if (nApplied == 0 && numCurrentDivisions == 1)
								    MrBayesPrint ("%s   Setting Topologypr to %s\n", spacer, modelParams[i].topologyPr);
							    else
								    MrBayesPrint ("%s   Setting Topologypr to %s for partition %d\n", spacer, modelParams[i].topologyPr, i+1);
							    }
						    }
					    }
					else
						{
						MrBayesPrint ("%s   Invalid Topologypr argument\n", spacer);
						return (ERROR);
						}
                    /* make sure we know what to do next */
					if (!strcmp(tempStr, "Uniform") || !strcmp(tempStr, "Speciestree"))
						expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
					else
						expecting = Expecting(LEFTPAR);
					foundEqual = NO;
					}
				else
					{
                    /* find out whether we need a tree name or constraint name */
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
					    if (activeParts[i] == YES || nApplied == 0)
                            break;

					if (foundDash == YES)   /* we must be collecting constraint numbers */
						{
                        MrBayesPrint ("%s   Expecting a number\n", spacer);
						return (ERROR);
						}
					if (!strcmp(modelParams[i].topologyPr,"Constraints"))
                        {
                        /* find constraint number */
                        if (CheckString (constraintNames, numDefinedConstraints, tkn, &index) == ERROR)
						    {
						    MrBayesPrint ("%s   Could not find constraint named %s\n", spacer, tkn);
						    return (ERROR);
						    }
                        tempActiveConstraints[index] = YES;
                        expecting = Expecting(RIGHTPAR);
                        expecting |= Expecting(COMMA);
                        }
                    else
                        {
                        /* find tree number */
                        if (GetUserTreeFromName (&index, tkn) == ERROR)
						    {
						    MrBayesPrint ("%s   Could not set fixed topology from user tree '%s'\n", spacer, tkn);
						    return (ERROR);
  						    }
                        fromI = index + 1;        /* fromI is used to hold the index of the user tree, 1-based */
                        expecting = Expecting(RIGHTPAR);
                        }
					}
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				for (i=0; i<numDefinedConstraints; i++)
					tempActiveConstraints[i] = NO;
				fromI = toJ = -1;
				foundDash = foundComma = NO;
				expecting = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else if (expecting == Expecting(NUMBER))
				{
                /* find out whether we need a tree number or constraint number */
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
				    if (activeParts[i] == YES || nApplied == 0)
                        break;

                if (!strcmp(modelParams[i].topologyPr,"Constraints"))
                    {
				    if (numDefinedConstraints == 0)
					    {
					    MrBayesPrint ("%s   No constraints have been defined\n", spacer);
					    return (ERROR);
					    }
				    sscanf (tkn, "%d", &tempInt);
				    if (tempInt > numDefinedConstraints)
					    {
					    MrBayesPrint ("%s   Constraint number is too large\n", spacer);
					    return (ERROR);
					    }
                    if (fromI == -1)
                        {
				        if ( foundDash == YES )
					        {
					        MrBayesPrint ("%s   Unexpected dash\n", spacer);
					        return (ERROR);
					        }
                        fromI = tempInt;
                        tempActiveConstraints[fromI-1] = YES;
                        }
				    else if (fromI != -1 && toJ == -1 && foundDash == YES && foundComma == NO)
					    {
					    toJ = tempInt;
					    //for (i=fromI-1; i<toJ; i++)
                        for (i=fromI; i<toJ; i++)
						    tempActiveConstraints[i] = YES;
					    fromI = toJ = -1;
					    foundDash = NO;
					    }
				    else if (fromI != -1 && toJ == -1 && foundDash == NO && foundComma == YES)
					    {
					    fromI = tempInt;
                        tempActiveConstraints[fromI-1] = YES;
					    foundComma = NO;
					    }
				    expecting  = Expecting(COMMA);
				    expecting |= Expecting(DASH);
				    expecting |= Expecting(RIGHTPAR);
				    }
                else /* if (!strcmp(modelParams[i].topologyPr,"Fixed")) */
                    {
				    if (numUserTrees == 0)
					    {
					    MrBayesPrint ("%s   No user trees have been defined\n", spacer);
					    return (ERROR);
					    }
				    sscanf (tkn, "%d", &tempInt);
				    if (tempInt > numUserTrees)
					    {
					    MrBayesPrint ("%s   Tree number is too large\n", spacer);
					    return (ERROR);
					    }
				    if (tempInt < 1)
					    {
					    MrBayesPrint ("%s   Tree number is too small\n", spacer);
					    return (ERROR);
					    }
				    fromI = tempInt;
				    expecting = Expecting(RIGHTPAR);    /* only one tree number acceptable */
                    }
                }
			else if (expecting == Expecting(COMMA))
				{
				foundComma = YES;
				expecting = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else if (expecting == Expecting(DASH))
				{
				foundDash = YES;
				expecting = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
                /* find out whether we need a tree number or constraint number(s) */
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
				    if (activeParts[i] == YES || nApplied == 0)
                        break;

                if (!strcmp(modelParams[i].topologyPr,"Constraints"))
                    {
                    /* set constraints */
			        for (i=0; i<numCurrentDivisions; i++)
				        {
				        if (activeParts[i] == YES || nApplied == 0)
					        {
					        modelParams[i].numActiveConstraints = 0;
					        for (j=0; j<numDefinedConstraints; j++)
						        {
						        if (tempActiveConstraints[j] == YES)
							        {
							        modelParams[i].activeConstraints[j] = YES;
							        modelParams[i].numActiveConstraints++;
							        }
						        else
							        modelParams[i].activeConstraints[j] = NO;
						        }
					        if (modelParams[i].numActiveConstraints == 0)
						        {
						        MrBayesPrint ("%s   No constraints have been defined\n", spacer);
						        return (ERROR);
						        }
					        }
                        }
                    }
                else /* if (!strcmp(modelParams[i].topologyPr,"Constraints")) */
                    {
                    /* set start tree index */
			        for (i=0; i<numCurrentDivisions; i++)
				        {
				        if (activeParts[i] == YES || nApplied == 0)
					        modelParams[i].topologyFix = fromI-1;
                        }
					}
#				if 0
				for (i=0; i<numCurrentDivisions; i++)
					{
					MrBayesPrint ("%4d -- ", i+1);
					for (j=0; j<numDefinedConstraints; j++)
						MrBayesPrint (" %d", modelParams[i].activeConstraints[j]);
					MrBayesPrint ("\n");
					}
#				endif				
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Nodeagepr (nodeAgePr) ********************************************************/
		else if (!strcmp(parmName, "Nodeagepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							strcpy(modelParams[i].nodeAgePr, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Nodeagepr to %s\n", spacer, modelParams[i].nodeAgePr);
							else
								MrBayesPrint ("%s   Setting Nodeagepr to %s for partition %d\n", spacer, modelParams[i].nodeAgePr, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Nodeagepr argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Clockvarpr (clockVarPr) ********************************************************/
		else if (!strcmp(parmName, "Clockvarpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
                    if (strcmp(tempStr, "Bm") == 0)
                        strcpy(tempStr, "TK02");
                    else if (strcmp(tempStr, "Ibr") == 0)
                        strcpy(tempStr, "Igr");
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
                            strcpy(modelParams[i].clockVarPr, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Clockvarpr to %s\n", spacer, modelParams[i].clockVarPr);
							else
								MrBayesPrint ("%s   Setting Clockvarpr to %s for partition %d\n", spacer, modelParams[i].clockVarPr, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Clockvarpr argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Brlenspr (brlensPr) ************************************************************/
		else if (!strcmp(parmName, "Brlenspr"))
			{
            if (expecting == Expecting(EQUALSIGN))
				{
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = NO;
				foundEqual = YES;
                foundLeftPar = NO;
				expecting = Expecting(ALPHA);
				}
			else if (expecting == Expecting(ALPHA))
				{
				if (foundEqual == YES)
					{
					if (IsArgValid(tkn, tempStr) == NO_ERROR)
						{
						strcpy (colonPr, tempStr);
						nApplied = NumActiveParts ();
						for (i=0; i<numCurrentDivisions; i++)
							if (activeParts[i] == YES || nApplied == 0)
								strcpy(modelParams[i].brlensPr, tempStr);
						}
					else
						{
						MrBayesPrint ("%s   Invalid Brlenspr argument\n", spacer);
						return (ERROR);
						}
					foundEqual = NO;
					if (!strcmp(colonPr,"Fixed"))
						expecting = Expecting(LEFTPAR);
					else
						expecting = Expecting(COLON);
					}
				else if (foundLeftPar == YES)
                    {
					/*process argument of fixed() prior*/
					/* find tree number */
                    if (GetUserTreeFromName (&tempInt, tkn) == ERROR)
					    {
					    MrBayesPrint ("%s   Could not set fixed branch lengths from the user tree '%s'\n", spacer, tkn);
					    return (ERROR);
					    }
                    fromI = tempInt + 1;        /* fromI is used to hold the index of the user tree, 1-based */
                    expecting = Expecting(RIGHTPAR);
                    foundLeftPar = NO;
                    }
                else
                    {
					if (!strcmp(colonPr, "Unconstrained"))
						{
						/* have unconstrained branch lengths, which we expect to have a uniform or exponential distribution */
						nApplied = NumActiveParts ();
						if (IsSame ("Uniform", tkn) == SAME || IsSame ("Uniform", tkn) == CONSISTENT_WITH)
							{
							for (i=0; i<numCurrentDivisions; i++)
								if (activeParts[i] == YES || nApplied == 0)
									strcpy(modelParams[i].unconstrainedPr, "Uniform");
							}
						else if (IsSame ("Exponential", tkn) == SAME || IsSame ("Exponential", tkn) == CONSISTENT_WITH)
							{
							for (i=0; i<numCurrentDivisions; i++)
								if (activeParts[i] == YES || nApplied == 0)
									strcpy(modelParams[i].unconstrainedPr, "Exponential");
							}
						else
							{
							MrBayesPrint ("%s   Do not understand %s\n", spacer, tkn);
							return (ERROR);
							}
						expecting  = Expecting(LEFTPAR);
						}
					else if (!strcmp(colonPr, "Clock"))
						{
						/* otherwise we have a clock constraint and expect uniform, birthdeath, coalescence or fixed prior */
						nApplied = NumActiveParts ();
						if (IsSame ("Uniform", tkn) == SAME || IsSame ("Uniform", tkn) == CONSISTENT_WITH)
							{
					        strcpy (clockPr, "Uniform");
							for (i=0; i<numCurrentDivisions; i++)
								{
								if (activeParts[i] == YES || nApplied == 0)
									strcpy(modelParams[i].clockPr, "Uniform");
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Uniform\n", spacer);
								else
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Uniform for partition %d\n", spacer, i+1);
								}
						    expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
							}
						else if (IsSame ("Birthdeath", tkn) == SAME || IsSame ("Birthdeath", tkn) == CONSISTENT_WITH)
							{
					        strcpy (clockPr, "Birthdeath");
							for (i=0; i<numCurrentDivisions; i++)
								{
								if (activeParts[i] == YES || nApplied == 0)
									strcpy(modelParams[i].clockPr, "Birthdeath");
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Birthdeath\n", spacer);
								else
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Birthdeath for partition %d\n", spacer, i+1);
								}
						    expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
							}
						else if (IsSame ("Coalescence", tkn) == SAME || IsSame ("Coalescence", tkn) == CONSISTENT_WITH)
							{
					        strcpy (clockPr, "Coalescence");
							for (i=0; i<numCurrentDivisions; i++)
								{
								if (activeParts[i] == YES || nApplied == 0)
									strcpy(modelParams[i].clockPr, "Coalescence");
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Coalescence\n", spacer);
								else
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Coalescence for partition %d\n", spacer, i+1);
								}
						    expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
							}
						else if (IsSame ("Speciestreecoalescence", tkn) == SAME || IsSame ("Speciestreecoalescence", tkn) == CONSISTENT_WITH)
							{
					        strcpy (clockPr, "Speciestreecoalescence");
							for (i=0; i<numCurrentDivisions; i++)
								{
								if (activeParts[i] == YES || nApplied == 0)
									strcpy(modelParams[i].clockPr, "Speciestreecoalescence");
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Speciestreecoalescence\n", spacer);
								else
									MrBayesPrint ("%s   Setting Brlenspr to Clock:Speciestreecoalescence for partition %d\n", spacer, i+1);
								}
						    expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
							}
						else if (IsSame ("Fixed", tkn) == SAME || IsSame ("Fixed", tkn) == CONSISTENT_WITH)
							{
					        strcpy (clockPr, "Fixed");
							for (i=0; i<numCurrentDivisions; i++)
								{
								if (activeParts[i] == YES || nApplied == 0)
									strcpy(modelParams[i].clockPr, "Fixed");
								}
                            expecting = Expecting(LEFTPAR);     /* Proceed with tree name */
							}
						else
							{
							MrBayesPrint ("%s   Do not understand %s\n", spacer, tkn);
							return (ERROR);
							}
						}
					else
						{
						MrBayesPrint ("%s   Do not understand %s\n", spacer, tkn);
						return (ERROR);
						}
					}
				}
			else if (expecting == Expecting(LEFTPAR))
				{
                foundLeftPar = YES;
				expecting  = Expecting(NUMBER);
                if (!strcmp(colonPr,"Fixed") || (!strcmp(colonPr,"Clock") && !strcmp(clockPr,"Fixed")))
					{
					expecting |= Expecting(ALPHA);
					}
				else
					{
					for (i=0; i<numCurrentDivisions; i++)
						numVars[i] = 0;
					}
				}
			else if (expecting == Expecting(NUMBER))
				{
				if (!strcmp(colonPr, "Unconstrained"))
					{
					/* have unconstrained branch lengths */
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							if (!strcmp(modelParams[i].unconstrainedPr,"Uniform"))
								{
								sscanf (tkn, "%lf", &tempD);
								modelParams[i].brlensUni[numVars[i]++] = tempD;
								if (numVars[i] == 1)
									expecting  = Expecting(COMMA);
								else
									{
									if (modelParams[i].brlensUni[0] > 0.000001)
										{
										MrBayesPrint ("%s   Lower value for uniform must equal 0.0\n", spacer);
										return (ERROR);
										}
									if (modelParams[i].brlensUni[0] >= modelParams[i].brlensUni[1])
										{
										MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
										return (ERROR);
										}
									if (nApplied == 0 && numCurrentDivisions == 1)
										MrBayesPrint ("%s   Setting Brlenspr to Unconstrained:Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].brlensUni[0], modelParams[i].brlensUni[1]);
									else
										MrBayesPrint ("%s   Setting Brlenspr to Unconstrained:Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].brlensUni[0], modelParams[i].brlensUni[1], i+1);
									expecting  = Expecting(RIGHTPAR);
									}
								}
							else if (!strcmp(modelParams[i].unconstrainedPr,"Exponential"))
								{
								sscanf (tkn, "%lf", &tempD);
								modelParams[i].brlensExp = tempD;
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Brlenspr to Unconstrained:Exponential(%1.2lf)\n", spacer, modelParams[i].brlensExp);
								else
									MrBayesPrint ("%s   Setting Brlenspr to Unconstrained:Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].brlensExp, i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						}
					}
				else if (!strcmp(colonPr,"Fixed") || !strcmp(colonPr,"Clock"))
					{
					sscanf (tkn, "%d", &tempInt);
					if (tempInt < 1 || tempInt > numUserTrees)
						{
						MrBayesPrint ("%s   Tree needs to be in the range %d to %d\n", spacer, 1, numUserTrees);
						return (ERROR);
						}
					fromI = tempInt;
					expecting = Expecting(RIGHTPAR);
					}
                foundLeftPar = NO;
				}
			else if (expecting == Expecting(COLON))
				{
				expecting  = Expecting(ALPHA);
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				if (!strcmp(colonPr,"Fixed") || (!strcmp(colonPr,"Clock") && !strcmp(clockPr,"Fixed")))
					{
			 		/* index of a tree which set up branch lengths*/
					nApplied = NumActiveParts ();
		    		for (i=0; i<numCurrentDivisions; i++)
			        	{
			        	if (activeParts[i] == YES || nApplied == 0)
				        	modelParams[i].brlensFix = fromI-1;
                        if (!strcmp(colonPr,"Fixed"))
                            {
						    if (nApplied == 0 && numCurrentDivisions == 1)
							    MrBayesPrint ("%s   Setting Brlenspr to Fixed(%s)\n", spacer, userTree[fromI-1]->name);
						    else
							    MrBayesPrint ("%s   Setting Brlenspr to Fixed(%s) for partition %d\n", spacer, userTree[fromI-1]->name, i+1);
                            }
                        else
                            {
						    if (nApplied == 0 && numCurrentDivisions == 1)
							    MrBayesPrint ("%s   Setting Brlenspr to Fixed(%s)\n", spacer, userTree[fromI-1]->name);
						    else
							    MrBayesPrint ("%s   Setting Brlenspr to Fixed(%s) for partition %d\n", spacer, userTree[fromI-1]->name, i+1);
                            }
                        }
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Speciationpr (speciationPr) ****************************************************/
		else if (!strcmp(parmName, "Speciationpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].speciationPr, tempStr);
					}
				else
					{
					MrBayesPrint ("%s   Invalid Speciationpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].speciationPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].speciationUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].speciationUni[0] >= modelParams[i].speciationUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Speciationpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].speciationUni[0], modelParams[i].speciationUni[1]);
								else
									MrBayesPrint ("%s   Setting Speciationpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].speciationUni[0], modelParams[i].speciationUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].speciationPr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].speciationExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Speciationpr to Exponential(%1.2lf)\n", spacer, modelParams[i].speciationExp);
							else
								MrBayesPrint ("%s   Setting Speciationpr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].speciationExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].speciationPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (AreDoublesEqual(tempD, 0.0, ETA)==YES)
								{
								MrBayesPrint ("%s   Speciation rate cannot be fixed to 0.0\n", spacer);
								return (ERROR);
								}
							modelParams[i].speciationFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Speciationpr to Fixed(%1.2lf)\n", spacer, modelParams[i].speciationFix);
							else
								MrBayesPrint ("%s   Setting Speciationpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].speciationFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Extinctionpr (extinctionPr) ****************************************************/
		else if (!strcmp(parmName, "Extinctionpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].extinctionPr, tempStr);
					}
				else
					{
					MrBayesPrint ("%s   Invalid Extinctionpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].extinctionPr,"Beta"))
							{
							sscanf (tkn, "%lf", &tempD);
                            if (tempD <= 0.0)
                                {
								MrBayesPrint ("%s   Beta parameter must be positive\n", spacer);
								return (ERROR);
                                }
							modelParams[i].extinctionBeta[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Extinctionpr to Beta(%1.2lf,%1.2lf)\n", spacer, modelParams[i].extinctionBeta[0], modelParams[i].extinctionBeta[1]);
								else
									MrBayesPrint ("%s   Setting Extinctionpr to Beta(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].extinctionBeta[0], modelParams[i].extinctionBeta[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].extinctionPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
                            if (tempD < 0.0 || tempD > 1.0)
                                {
								MrBayesPrint ("%s   Relative extinction rate must be in the range [0,1]\n", spacer);
								return (ERROR);
                                }
							modelParams[i].extinctionFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Extinctionpr to Fixed(%1.2lf)\n", spacer, modelParams[i].extinctionFix);
							else
								MrBayesPrint ("%s   Setting Extinctionpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].extinctionFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set SampleStrat (sampleStrat) *****************************************************/
		else if (!strcmp(parmName, "Samplestrat"))
        {
			if (expecting == Expecting(EQUALSIGN))
            {
				expecting = Expecting(ALPHA);
            }
			else if (expecting == Expecting(ALPHA))
            {
				
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
                {
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].sampleStrat, tempStr);
					}
				else
					{
					MrBayesPrint ("%s   Invalid Samplestrat argument\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
            }
			else
            {
                MrBayesPrint ("%s   Invalid Samplestrat argument\n", spacer);
				return (ERROR);
            }
        }
		/* set Sampleprob (sampleProb) *****************************************************/
		else if (!strcmp(parmName, "Sampleprob"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(NUMBER);
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%lf", &tempD);
				if (tempD <= 1.0 && tempD > 0.0)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0))
							{
							modelParams[i].sampleProb = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Sampleprob to %1.2lf\n", spacer, modelParams[i].sampleProb);
							else
								MrBayesPrint ("%s   Setting Sampleprob to %1.2lf for partition %d\n", spacer, modelParams[i].sampleProb, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Sampleprob argument (should be between 0 and 1)\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Treeagepr (treeAgePr) *******************************************************/
		else if (!strcmp(parmName, "Treeagepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].treeAgePr, tempStr);
					}
				else
					{
					MrBayesPrint ("%s   Invalid Treeagepr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].treeAgePr,"Gamma"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].treeAgeGamma[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Treeagepr to Gamma(%1.2lf,%1.2lf)\n", spacer, modelParams[i].treeAgeGamma[0], modelParams[i].treeAgeGamma[1]);
								else
									MrBayesPrint ("%s   Setting Treeagepr to Gamma(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].treeAgeGamma[0], modelParams[i].treeAgeGamma[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].treeAgePr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].treeAgeExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Treeagepr to Exponential(%1.2lf)\n", spacer, modelParams[i].treeAgeExp);
							else
								MrBayesPrint ("%s   Setting Treeagepr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].treeAgeExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].treeAgePr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].treeAgeFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Treeagepr to Fixed(%1.2lf)\n", spacer, modelParams[i].treeAgeFix);
							else
								MrBayesPrint ("%s   Setting Treeagepr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].treeAgeFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Clockratepr (clockRatePr) ****************************************************/
		else if (!strcmp(parmName, "Clockratepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
                {
                foundDash = NO;
				expecting = Expecting(ALPHA);
                }
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].clockRatePr, tempStr);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Clockratepr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
                expecting |= Expecting(DASH);   /* negative numbers possible */
				}
            else if (expecting == Expecting(DASH))
                {
                foundDash = YES;
                expecting = Expecting(NUMBER);
                }
            else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%lf", &tempD);
                if (foundDash == YES)
                    {
                    foundDash = NO;
                    tempD *= -1.0;
                    }
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].clockRatePr,"Normal"))
							{
							if (tempD <= 0.0)
								{
								if (numVars[i] == 0)
                                    MrBayesPrint ("%s   Mean of the normal must be positive\n", spacer);
								else if (numVars[i] == 1)
                                    MrBayesPrint ("%s   Standard deviation of the normal must be positive\n", spacer);
								return (ERROR);
								}
							modelParams[i].clockRateNormal[numVars[i]] = tempD;
							numVars[i]++;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 || numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Clockratepr to Normal(%1.6lf,%1.6lf)\n", spacer, modelParams[i].clockRateNormal[0], modelParams[i].clockRateNormal[1]);
								else
									MrBayesPrint ("%s   Setting Clockratepr to Normal(%1.6lf,%1.6lf) for partition %d\n", spacer, modelParams[i].clockRateNormal[0], modelParams[i].clockRateNormal[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].clockRatePr,"Lognormal"))
							{
							modelParams[i].clockRateLognormal[numVars[i]] = tempD;
							numVars[i]++;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 || numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Clockratepr to Lognormal(%1.6lf,%1.6lf)\n", spacer, modelParams[i].clockRateLognormal[0], modelParams[i].clockRateLognormal[1]);
								else
									MrBayesPrint ("%s   Setting Clockratepr to Lognormal(%1.6lf,%1.6lf) for partition %d\n", spacer, modelParams[i].clockRateLognormal[0], modelParams[i].clockRateLognormal[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].clockRatePr,"Exponential"))
							{
							if (tempD <= 0.0)
								{
                                MrBayesPrint ("%s   Rate of the exponential must be positive\n", spacer);
								return (ERROR);
								}
							modelParams[i].clockRateExp = tempD;
							numVars[i]++;
							if (nApplied == 0 || numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Clockratepr to Exponential(%1.6lf)\n", spacer, modelParams[i].clockRateExp);
							else
								MrBayesPrint ("%s   Setting Clockratepr to Exponential(%1.6lf) for partition %d\n", spacer, modelParams[i].clockRateExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].clockRatePr,"Gamma"))
							{
							if (tempD <= 0.0)
								{
								if (numVars[i] == 0)
                                    MrBayesPrint ("%s   Shape of the gamma must be positive\n", spacer);
								else if (numVars[i] == 1)
                                    MrBayesPrint ("%s   Rate (inverse scale) of the gamma must be positive\n", spacer);
								return (ERROR);
								}
							modelParams[i].clockRateGamma[numVars[i]] = tempD;
							numVars[i]++;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 || numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Clockratepr to Gamma(%1.6lf,%1.6lf)\n", spacer, modelParams[i].clockRateGamma[0], modelParams[i].clockRateGamma[1]);
								else
									MrBayesPrint ("%s   Setting Clockratepr to Gamma(%1.6lf,%1.6lf) for partition %d\n", spacer, modelParams[i].clockRateGamma[0], modelParams[i].clockRateGamma[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].clockRatePr,"Fixed"))
							{
							if (tempD <= 0.0)
								{
                                MrBayesPrint ("%s   Fixed clock rate must be positive\n", spacer);
								return (ERROR);
								}
							modelParams[i].clockRateFix = tempD;
							numVars[i]++;
							if (nApplied == 0 || numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Clockratepr to Fixed(%1.6lf)\n", spacer, modelParams[i].clockRateFix);
							else
								MrBayesPrint ("%s   Setting Clockratepr to Fixed(%1.6lf) for partition %d\n", spacer, modelParams[i].clockRateFix, i+1);
                            for (k=0; k<numGlobalChains; k++)
                                {
                                if( UpdateClockRate(tempD, k) == ERROR) 
                                    return (ERROR);
                                }
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Popsizepr (popSizePr) **************************************************************/
		else if (!strcmp(parmName, "Popsizepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].popSizePr, tempStr);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Popsizepr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].popSizePr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].popSizeUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
                                {
							    if (tempD <= 0.0)
                                    {
                                    MrBayesPrint ("%s   Lower value for Uniform must be a positive number\n", spacer);
                                    return (ERROR);
                                    }
								expecting  = Expecting(COMMA);
                                }
							else
								{
								if (modelParams[i].popSizeUni[0] >= modelParams[i].popSizeUni[1])
									{
									MrBayesPrint ("%s   Lower value for Uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Popsizepr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].popSizeUni[0], modelParams[i].popSizeUni[1]);
								else
									MrBayesPrint ("%s   Setting Popsizepr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].popSizeUni[0], modelParams[i].popSizeUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].popSizePr,"Lognormal"))
							{
							sscanf (tkn, "%lf", &tempD);
                            modelParams[i].popSizeLognormal[numVars[i]++] = tempD;
							if (numVars[i] == 1)
                                {
								expecting  = Expecting(COMMA);
                                }
							else
								{
								if (modelParams[i].popSizeLognormal[1] <= 0.0)
                                    {
                                    MrBayesPrint ("%s   Standard deviation of Lognormal must be a positive number\n", spacer);
                                    return (ERROR);
                                    }
								if (nApplied == 0 && numCurrentDivisions == 1)
                                    MrBayesPrint ("%s   Setting Popsizepr to Lognormal(%1.2lf,%1.2lf)\n", spacer, modelParams[i].popSizeLognormal[0], modelParams[i].popSizeLognormal[1]);
								else
									MrBayesPrint ("%s   Setting Popsizepr to Lognormal(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].popSizeLognormal[0], modelParams[i].popSizeLognormal[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].popSizePr,"Normal"))
							{
							sscanf (tkn, "%lf", &tempD);
                            modelParams[i].popSizeNormal[numVars[i]++] = tempD;
							if (numVars[i] == 1)
                                {
								if (modelParams[i].popSizeNormal[0] <= 0.0)
                                    {
                                    MrBayesPrint ("%s   Mean of Truncated Normal must be a positive number\n", spacer);
                                    return (ERROR);
                                    }
								expecting  = Expecting(COMMA);
                                }
							else
								{
								if (modelParams[i].popSizeNormal[1] <= 0.0)
                                    {
                                    MrBayesPrint ("%s   Standard deviation of Truncated Normal must be a positive number\n", spacer);
                                    return (ERROR);
                                    }
								if (nApplied == 0 && numCurrentDivisions == 1)
                                    MrBayesPrint ("%s   Setting Popsizepr to Truncated Normal(%1.2lf,%1.2lf)\n", spacer, modelParams[i].popSizeNormal[0], modelParams[i].popSizeNormal[1]);
								else
									MrBayesPrint ("%s   Setting Popsizepr to Truncated Normal(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].popSizeNormal[0], modelParams[i].popSizeNormal[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].popSizePr,"Gamma"))
							{
							sscanf (tkn, "%lf", &tempD);
                            modelParams[i].popSizeLognormal[numVars[i]++] = tempD;
							if (numVars[i] == 1)
                                {
								if (modelParams[i].popSizeGamma[0] <= 0.0)
                                    {
                                    MrBayesPrint ("%s   Shape (alpha) of Gamma must be a positive number\n", spacer);
                                    return (ERROR);
                                    }
								expecting  = Expecting(COMMA);
                                }
							else
								{
								if (modelParams[i].popSizeGamma[1] <= 0.0)
                                    {
                                    MrBayesPrint ("%s   Rate (beta) of Gamma must be a positive number\n", spacer);
                                    return (ERROR);
                                    }
								if (nApplied == 0 && numCurrentDivisions == 1)
                                    MrBayesPrint ("%s   Setting Popsizepr to Gamma(%1.2lf,%1.2lf)\n", spacer, modelParams[i].popSizeGamma[0], modelParams[i].popSizeGamma[1]);
								else
									MrBayesPrint ("%s   Setting Popsizepr to Gamma(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].popSizeGamma[0], modelParams[i].popSizeGamma[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].popSizePr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (AreDoublesEqual(tempD, 0.0, ETA)==YES)
								{
								MrBayesPrint ("%s   Popsizepr cannot be fixed to 0.0\n", spacer);
								return (ERROR);
								}
							modelParams[i].popSizeFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Popsizepr to Fixed(%1.2lf)\n", spacer, modelParams[i].popSizeFix);
							else
								MrBayesPrint ("%s   Setting Popsizepr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].popSizeFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Popvarpr (popVarPr) **************************************************************/
		else if (!strcmp(parmName, "Popvarpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].popVarPr, tempStr);
						}
			        if (nApplied == 0 && numCurrentDivisions == 1)
				        MrBayesPrint ("%s   Setting Popvarpr to %s\n", spacer, modelParams[i].popVarPr);
                    else for (i=0; i<numCurrentDivisions; i++)
                        {
                        if (activeParts[i] == YES)
    				        MrBayesPrint ("%s   Setting Popvarpr to %s for partition %d\n", spacer, modelParams[i].popVarPr, i+1);
                        }
			        expecting  = Expecting(PARAMETER) | Expecting(SEMICOLON);
					}
				else
					{
					MrBayesPrint ("%s   Invalid Popvarpr argument\n", spacer);
					return (ERROR);
					}
				}
			else
				return (ERROR);
			}
		/* set Compound Poisson Process rate prior (cppRatePr) *********************************************************/
		else if (!strcmp(parmName, "Cppratepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].cppRatePr, tempStr);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Cppratepr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].cppRatePr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].cppRateExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Cppratepr to Exponential(%1.2lf)\n", spacer, modelParams[i].cppRateExp);
							else
								MrBayesPrint ("%s   Setting Cppratepr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].cppRateExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].cppRatePr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (tempD < CPPLAMBDA_MIN || tempD > CPPLAMBDA_MAX)
								{
								MrBayesPrint ("%s   CPP rate must be in the range %f - %f\n", spacer, CPPLAMBDA_MIN, CPPLAMBDA_MAX);
								return (ERROR);
								}
							modelParams[i].cppRateFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Cppratepr to Fixed(%1.2lf)\n", spacer, modelParams[i].cppRateFix);
							else
								MrBayesPrint ("%s   Setting Cppratepr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].cppRateFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Compound Poisson Process rate multiplier standard deviation (log scale) ***********************/
		else if (!strcmp(parmName, "Cppmultdevpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].cppMultDevPr, tempStr);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Cppmultdevpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].cppMultDevPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (tempD < POSREAL_MIN || tempD > POSREAL_MAX)
								{
								MrBayesPrint ("%s   The log standard deviation of rate multipliers must be in the range %f - %f\n", spacer, POSREAL_MIN, POSREAL_MAX);
								return (ERROR);
								}
							modelParams[i].cppMultDevFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Cppmultdevpr to Fixed(%1.2lf)\n", spacer, modelParams[i].cppMultDevFix);
							else
								MrBayesPrint ("%s   Setting Cppmultdevpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].cppMultDevFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set prior for variance of lognormal of autocorrelated rates (tk02varPr) ***********************/
		else if (!strcmp(parmName, "TK02varpr") || !strcmp(parmName,"Bmvarpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].tk02varPr, tempStr);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid TK02varpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].tk02varPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].tk02varUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].tk02varUni[0] >= modelParams[i].tk02varUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting TK02varpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].tk02varUni[0], modelParams[i].tk02varUni[1]);
								else
									MrBayesPrint ("%s   Setting TK02varpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].tk02varUni[0], modelParams[i].tk02varUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].tk02varPr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].tk02varExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting TK02varpr to Exponential(%1.2lf)\n", spacer, modelParams[i].tk02varExp);
							else
								MrBayesPrint ("%s   Setting TK02varpr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].tk02varExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].tk02varPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (tempD < TK02VAR_MIN || tempD > TK02VAR_MAX)
								{
								MrBayesPrint ("%s   Ratevar (nu) must be in the range %f - %f\n", spacer, TK02VAR_MIN, TK02VAR_MAX);
								return (ERROR);
								}
							modelParams[i].tk02varFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting TK02varpr to Fixed(%1.2lf)\n", spacer, modelParams[i].tk02varFix);
							else
								MrBayesPrint ("%s   Setting TK02varpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].tk02varFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set prior for shape of independent branch rate gamma distribution (igrvarPr) ***********************/
		else if (!strcmp(parmName, "Igrvarpr") || !strcmp(parmName, "Ibrvarpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].igrvarPr, tempStr);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Igrvarpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if (activeParts[i] == YES || nApplied == 0)
						{
						if (!strcmp(modelParams[i].igrvarPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].igrvarUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].igrvarUni[0] >= modelParams[i].igrvarUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Igrvarpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].igrvarUni[0], modelParams[i].igrvarUni[1]);
								else
									MrBayesPrint ("%s   Setting Igrvarpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].igrvarUni[0], modelParams[i].igrvarUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].igrvarPr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].igrvarExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Igrvarpr to Exponential(%1.2lf)\n", spacer, modelParams[i].igrvarExp);
							else
								MrBayesPrint ("%s   Setting Igrvarpr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].igrvarExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].igrvarPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (tempD < IGRVAR_MIN || tempD > IGRVAR_MAX)
								{
								MrBayesPrint ("%s   Igrvar must be in the range %f - %f\n", spacer, IGRVAR_MIN, IGRVAR_MAX);
								return (ERROR);
								}
							modelParams[i].igrvarFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Igrvarpr to Fixed(%1.2lf)\n", spacer, modelParams[i].igrvarFix);
							else
								MrBayesPrint ("%s   Setting Igrvarpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].igrvarFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Growthpr (growthPr) **************************************************************/
		else if (!strcmp(parmName, "Growthpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				{
				expecting = Expecting(ALPHA);
				isNegative = NO;
				}
			else if (expecting == Expecting(ALPHA))
				{
				isNegative = NO;
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							strcpy(modelParams[i].growthPr, tempStr);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Growthpr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER) | Expecting(DASH);
				}
			else if (expecting == Expecting(DASH))
				{
				expecting  = Expecting(NUMBER);
				isNegative = YES;
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].growthPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (isNegative == YES)
								tempD *= -1.0;
							modelParams[i].growthUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].growthUni[0] >= modelParams[i].growthUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Growthpr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].growthUni[0], modelParams[i].growthUni[1]);
								else
									MrBayesPrint ("%s   Setting Growthpr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].growthUni[0], modelParams[i].growthUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].growthPr,"Normal"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (isNegative == YES)
								tempD *= -1.0;
							modelParams[i].growthNorm[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].growthNorm[1] < 0.0)
									{
									MrBayesPrint ("%s   Variance for normal distribution should be greater than zero\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Growthpr to Normal(%1.2lf,%1.2lf)\n", spacer, modelParams[i].growthNorm[0], modelParams[i].growthNorm[1]);
								else
									MrBayesPrint ("%s   Setting Growthpr to Normal(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].growthNorm[0], modelParams[i].growthNorm[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].growthPr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].growthExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Growthpr to Exponential(%1.2lf)\n", spacer, modelParams[i].growthExp);
							else
								MrBayesPrint ("%s   Setting Growthpr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].growthExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].growthPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							if (isNegative == YES)
								tempD *= -1.0;
							modelParams[i].growthFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Growthpr to Fixed(%1.2lf)\n", spacer, modelParams[i].growthFix);
							else
								MrBayesPrint ("%s   Setting Growthpr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].growthFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				isNegative = NO;
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set Aamodelpr (aaModelPr) **************************************************************/
		else if (!strcmp(parmName, "Aamodelpr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				{
				expecting = Expecting(ALPHA);
				foundAaSetting = foundExp = modelIsFixed = foundDash = NO;
				fromI = 0;
				for (i=0; i<10; i++)
					tempAaModelPrs[i] = 0.0;
				}
			else if (expecting == Expecting(ALPHA))
				{
				if (foundAaSetting == NO)
					{
					if (IsArgValid(tkn, tempStr) == NO_ERROR)
						{
						nApplied = NumActiveParts ();
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0))
								{
								strcpy(modelParams[i].aaModelPr, tempStr);
								if (!strcmp(modelParams[i].aaModelPr, "Mixed"))
									{
									if (nApplied == 0 && numCurrentDivisions == 1)
										MrBayesPrint ("%s   Setting Aamodelpr to %s\n", spacer, modelParams[i].aaModelPr);
									else
										MrBayesPrint ("%s   Setting Aamodelpr to %s for partition %d\n", spacer, modelParams[i].aaModelPr, i+1);
									}
								}
							}
						}
					else
						{
						MrBayesPrint ("%s   Invalid Aamodelpr argument\n", spacer);
						return (ERROR);
						}
					foundAaSetting = YES;
					if (!strcmp(tempStr, "Fixed"))
						{
						modelIsFixed = YES;
						expecting = Expecting(LEFTPAR);
						}
					else
						{
						expecting = Expecting(LEFTPAR) | Expecting(PARAMETER) | Expecting(SEMICOLON);
						}
					}
				else
					{
					if (modelIsFixed == YES)
						{
						if (IsSame ("Poisson", tkn) == SAME      || IsSame ("Poisson", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Poisson");
						else if (IsSame ("Equalin", tkn) == SAME || IsSame ("Equalin", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Equalin");
						else if (IsSame ("Jones", tkn) == SAME   || IsSame ("Jones", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Jones");
						else if (IsSame ("Dayhoff", tkn) == SAME || IsSame ("Dayhoff", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Dayhoff");
						else if (IsSame ("Mtrev", tkn) == SAME   || IsSame ("Mtrev", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Mtrev");
						else if (IsSame ("Mtmam", tkn) == SAME   || IsSame ("Mtmam", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Mtmam");
						else if (IsSame ("Wag", tkn) == SAME     || IsSame ("Wag", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Wag");
						else if (IsSame ("Rtrev", tkn) == SAME   || IsSame ("Rtrev", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Rtrev");
						else if (IsSame ("Cprev", tkn) == SAME   || IsSame ("Cprev", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Cprev");
						else if (IsSame ("Vt", tkn) == SAME      || IsSame ("Vt", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Vt");
						else if (IsSame ("Blosum", tkn) == SAME  || IsSame ("Blosum", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Blosum");
						else if (IsSame ("Blossum", tkn) == SAME  || IsSame ("Blossum", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Blosum");
						else if (IsSame ("Gtr", tkn) == SAME     || IsSame ("Gtr", tkn) == CONSISTENT_WITH)
							strcpy (tempStr, "Gtr");
						else
							{
							MrBayesPrint ("%s   Invalid amino acid model\n", spacer);
							return (ERROR);
							}
						nApplied = NumActiveParts ();
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0))
								{
								if (!strcmp(modelParams[i].aaModelPr, "Fixed"))
									{
									strcpy(modelParams[i].aaModel, tempStr);
									if (nApplied == 0 && numCurrentDivisions == 1)
										MrBayesPrint ("%s   Setting Aamodelpr to Fixed(%s)\n", spacer, modelParams[i].aaModel);
									else
										MrBayesPrint ("%s   Setting Aamodelpr to Fixed(%s) for partition %d\n", spacer, modelParams[i].aaModel, i+1);
									}
								else
									{
									MrBayesPrint ("%s   You cannot assign an amino acid matrix for mixed models\n", spacer);
									return (ERROR);
									}
								}
							}
						expecting = Expecting(RIGHTPAR);
						}
					else
						{
						if (IsSame ("Exponential", tkn) == SAME || IsSame ("Exponential", tkn) == CONSISTENT_WITH)
							{
							foundExp = YES;
							expecting = Expecting(LEFTPAR);
							}
						else	
							{
							MrBayesPrint ("%s   Invalid argument \"%s\"\n", spacer, tkn);
							return (ERROR);
							}
						}

					}
				}
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%lf", &tempD);
				if (fromI >= 10)
					{
					MrBayesPrint ("%s   Too many arguments in Aamodelpr\n", spacer);
					return (ERROR);
					}
				if (modelIsFixed == NO)
					{
					if (foundExp == YES)
						{
						if (foundDash == YES)
							tempAaModelPrs[fromI++] = -tempD;
						else
							tempAaModelPrs[fromI++] = tempD;
						expecting  = Expecting(RIGHTPAR);
						}
					else
						{
						if (foundDash == YES)
							{
							MrBayesPrint ("%s   Unexpected \"-\" in Aamodelpr\n", spacer);
							return (ERROR);
							}
						else
							{
							if (tempD <= 0.000000000001)
								tempAaModelPrs[fromI++] = -1000000000;
							else
								tempAaModelPrs[fromI++] = (MrBFlt) log(tempD);
							}
						expecting  = Expecting(COMMA) | Expecting(RIGHTPAR);
						}
					foundDash = NO;
					}
				else
					{
					MrBayesPrint ("%s   Not expecting a number\n", spacer);
					return (ERROR);
					}
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				if (modelIsFixed == YES)
					expecting  = Expecting(ALPHA);
				else
					{
					if (foundExp == YES)
						expecting  = Expecting(NUMBER) | Expecting(DASH);
					else
						expecting  = Expecting(NUMBER) | Expecting(ALPHA);
					}
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				if (modelIsFixed == YES)
					expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				else
					{
					if (foundExp == YES)
						{
						foundExp = NO;
						expecting = Expecting(COMMA) | Expecting(RIGHTPAR);
						}
					else	
						{
						if (fromI < 10)
							{
							MrBayesPrint ("%s   Too few arguments in Aamodelpr\n", spacer);
							return (ERROR);
							}
						nApplied = NumActiveParts ();
						for (i=0; i<numCurrentDivisions; i++)
							{
							if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType == PROTEIN)
								{
								if (!strcmp(modelParams[i].aaModelPr, "Fixed"))
									{
									MrBayesPrint ("%s   You cannot assign model prior probabilities for a fixed amino acid model\n", spacer);
									return (ERROR);
									}
								else
									{
									for (j=0; j<10; j++)
										modelParams[i].aaModelPrProbs[j] = tempAaModelPrs[j];
									}
								}
							}
						expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
						}
					}				
				}
			else if (expecting == Expecting(DASH))
				{
				if (foundExp == YES)
					{
					foundDash = YES;
					expecting  = Expecting(NUMBER);
					}
				else
					{
					MrBayesPrint ("%s   Invalid argument \"%s\"\n", spacer, tkn);
					return (ERROR);
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				if (modelIsFixed == YES)
					{
					MrBayesPrint ("%s   Not expecting \"%s\"\n", spacer, tkn);
					return (ERROR);
					}
				else
					expecting  = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else
				return (ERROR);
			}		
		/* set Brownscalepr (brownScalesPr) ****************************************************/
		else if (!strcmp(parmName, "Brownscalepr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType == CONTINUOUS)
							{
							strcpy(modelParams[i].brownScalesPr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    	{
							MrBayesPrint ("%s   Warning: %s can be set only for partition containing CONTINUOUS data.\
							Currently there is no active partition with such data. ", spacer, parmName);
							return (ERROR);
							}
					}
				else
					{
					MrBayesPrint ("%s   Invalid Brownscalepr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && modelParams[i].dataType == CONTINUOUS)
						{
						if (!strcmp(modelParams[i].brownScalesPr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].brownScalesUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].brownScalesUni[0] >= modelParams[i].brownScalesUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Brownscalepr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].brownScalesUni[0], modelParams[i].brownScalesUni[1]);
								else
									MrBayesPrint ("%s   Setting Brownscalepr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].brownScalesUni[0], modelParams[i].brownScalesUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].brownScalesPr,"Gamma"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].brownScalesGamma[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting Brownscalepr to Gamma Mean=%1.2lf Var=%1.2lf\n", spacer, modelParams[i].brownScalesGamma[0], modelParams[i].brownScalesGamma[1]);
								else
									MrBayesPrint ("%s   Setting Brownscalepr to Gamma Mean=%1.2lf Var=%1.2lf for partition %d\n", spacer, modelParams[i].brownScalesGamma[0], modelParams[i].brownScalesGamma[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].brownScalesPr,"Gammamean"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].brownScalesGammaMean = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Brownscalepr to Gamma Mean=<char. ave.> Var=%1.2lf\n", spacer, modelParams[i].brownScalesGammaMean);
							else
								MrBayesPrint ("%s   Setting Brownscalepr to Gamma Mean=<char.ave.> Var=%1.2lf for partition %d\n", spacer, modelParams[i].brownScalesGammaMean, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].brownScalesPr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].brownScalesFix = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting Brownscalepr to Fixed(%1.2lf)\n", spacer, modelParams[i].brownScalesFix);
							else
								MrBayesPrint ("%s   Setting Brownscalepr to Fixed(%1.2lf) for partition %d\n", spacer, modelParams[i].brownScalesFix, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set M10betapr (m10betapr) ********************************************************/
		else if (!strcmp(parmName, "M10betapr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].m10betapr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    	{
							MrBayesPrint ("%s   Warning: %s can be set only for partition containing data of at least one of the following type: DNA, RNA.\
							Currently there is no active partition with such data. ", spacer, parmName);
							return (ERROR);
							}
					}
				else
					{
					MrBayesPrint ("%s   Invalid M10betapr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].m10betapr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].m10betaUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].m10betaUni[0] >= modelParams[i].m10betaUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting M10betapr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].m10betaUni[0], modelParams[i].m10betaUni[1]);
								else
									MrBayesPrint ("%s   Setting M10betapr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].m10betaUni[0], modelParams[i].m10betaUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].m10betapr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].m10betaExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting M10betapr to Exponential(%1.2lf)\n", spacer, modelParams[i].m10betaExp);
							else
								MrBayesPrint ("%s   Setting M10betapr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].m10betaExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].m10betapr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].m10betaFix[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting M10betapr to Fixed(%1.2lf,%1.2lf)\n", spacer, modelParams[i].m10betaFix[0], modelParams[i].m10betaFix[1]);
								else
									MrBayesPrint ("%s   Setting M10betapr to Fixed(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].m10betaFix[0], modelParams[i].m10betaFix[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set M10gammapr (m10gammapr) ********************************************************/
		else if (!strcmp(parmName, "M10gammapr"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].m10gammapr, tempStr);
							flag=1;
							}
						}
					if( flag == 0)
					    	{
							MrBayesPrint ("%s   Warning: %s can be set only for partition containing data of at least one of the following type: DNA, RNA.\
							Currently there is no active partition with such data. ", spacer, parmName);
							return (ERROR);
							}
					}
				else
					{
					MrBayesPrint ("%s   Invalid M10gammapr argument\n", spacer);
					return (ERROR);
					}
				expecting  = Expecting(LEFTPAR);
				for (i=0; i<numCurrentDivisions; i++)
					numVars[i] = 0;
				}
			else if (expecting == Expecting(LEFTPAR))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(NUMBER))
				{
				nApplied = NumActiveParts ();
				for (i=0; i<numCurrentDivisions; i++)
					{
					if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
						{
						if (!strcmp(modelParams[i].m10gammapr,"Uniform"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].m10gammaUni[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (modelParams[i].m10gammaUni[0] >= modelParams[i].m10gammaUni[1])
									{
									MrBayesPrint ("%s   Lower value for uniform should be greater than upper value\n", spacer);
									return (ERROR);
									}
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting M10gammapr to Uniform(%1.2lf,%1.2lf)\n", spacer, modelParams[i].m10gammaUni[0], modelParams[i].m10gammaUni[1]);
								else
									MrBayesPrint ("%s   Setting M10gammapr to Uniform(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].m10gammaUni[0], modelParams[i].m10gammaUni[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						else if (!strcmp(modelParams[i].m10gammapr,"Exponential"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].m10gammaExp = tempD;
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting M10gammapr to Exponential(%1.2lf)\n", spacer, modelParams[i].m10gammaExp);
							else
								MrBayesPrint ("%s   Setting M10gammapr to Exponential(%1.2lf) for partition %d\n", spacer, modelParams[i].m10gammaExp, i+1);
							expecting  = Expecting(RIGHTPAR);
							}
						else if (!strcmp(modelParams[i].m10gammapr,"Fixed"))
							{
							sscanf (tkn, "%lf", &tempD);
							modelParams[i].m10gammaFix[numVars[i]++] = tempD;
							if (numVars[i] == 1)
								expecting  = Expecting(COMMA);
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Setting M10gammapr to Fixed(%1.2lf,%1.2lf)\n", spacer, modelParams[i].m10gammaFix[0], modelParams[i].m10gammaFix[1]);
								else
									MrBayesPrint ("%s   Setting M10gammapr to Fixed(%1.2lf,%1.2lf) for partition %d\n", spacer, modelParams[i].m10gammaFix[0], modelParams[i].m10gammaFix[1], i+1);
								expecting  = Expecting(RIGHTPAR);
								}
							}
						}
					}
				}
			else if (expecting == Expecting(COMMA))
				{
				expecting  = Expecting(NUMBER);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}

		else
			return (ERROR);
		}

	return (NO_ERROR);

}





int DoReport (void)

{

	/* TODO: smart update */
	if (SetUpAnalysis (&globalSeed) == ERROR)
		return (ERROR);
	return (NO_ERROR);

}





int DoReportParm (char *parmName, char *tkn)

{
	

	int			i, tempInt, nApplied;
	char		tempStr[100];

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before the report settings can be altered\n", spacer);
		return (ERROR);
		}
	if (inValidCommand == YES)
		{
		for (i=0; i<numCurrentDivisions; i++)
			activeParts[i] = NO;
		inValidCommand = NO;
		}

	if (expecting == Expecting(PARAMETER))
		{
		expecting = Expecting(EQUALSIGN);
		}
	else
		{
		/* set Applyto (Applyto) *************************************************************/
		if (!strcmp(parmName, "Applyto"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(LEFTPAR);
			else if (expecting == Expecting(LEFTPAR))
				{
				for (i=0; i<numCurrentDivisions; i++)
					activeParts[i] = NO;
				fromI = toJ = -1;
				foundDash = NO;
				expecting = Expecting(NUMBER) | Expecting(ALPHA);
				}
			else if (expecting == Expecting(RIGHTPAR))
				{
				if (fromI != -1)
					activeParts[fromI-1] = YES;
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else if (expecting == Expecting(COMMA))
				{
				foundComma = YES;
				expecting = Expecting(NUMBER);
				}
			else if (expecting == Expecting(ALPHA))
				{
				if (IsSame ("All", tkn) == DIFFERENT)
					{
					MrBayesPrint ("%s   Do not understand delimiter \"%s\"\n", spacer, tkn);
					return (ERROR);
					}
				for (i=0; i<numCurrentDivisions; i++)
					activeParts[i] = YES;
				expecting  = Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(NUMBER))
				{
				sscanf (tkn, "%d", &tempInt);
				if (tempInt > numCurrentDivisions)
					{
					MrBayesPrint ("%s   Partition delimiter is too large\n", spacer);
					return (ERROR);
					}
				if (fromI == -1)
					fromI = tempInt;
				else if (fromI != -1 && toJ == -1 && foundDash == YES && foundComma == NO)
					{
					toJ = tempInt;
					for (i=fromI-1; i<toJ; i++)
						activeParts[i] = YES;
					fromI = toJ = -1;
					foundDash = NO;
					}
				else if (fromI != -1 && toJ == -1 && foundDash == NO && foundComma == YES)
					{
					activeParts[fromI-1] = YES;
					fromI = tempInt;
					foundComma = NO;
					}
					
				expecting  = Expecting(COMMA);
				expecting |= Expecting(DASH);
				expecting |= Expecting(RIGHTPAR);
				}
			else if (expecting == Expecting(DASH))
				{
				foundDash = YES;
				expecting = Expecting(NUMBER);
				}
			else
				return (ERROR);
			}
		/* set report format of tratio ***************************************************/
		else if (!strcmp(parmName, "Tratio"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					tempInt = NO;
					for (i=0; i<numCurrentDivisions; i++)
						{
						/* check that data type is correct; we do not know yet if the user will specify
						a nst=2 model so we cannot check that tratio is an active parameter */
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].tratioFormat, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting transition/transversion rate ratio (tratio) format to %s\n", spacer, modelParams[i].tratioFormat);
							else
								MrBayesPrint ("%s   Setting transition/transversion rate ratio (tratio) format to %s for partition %d\n", spacer, modelParams[i].tratioFormat, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid transition/transversion rate ratio (tratio) format \n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set report format of revmat ***************************************************/
		else if (!strcmp(parmName, "Revmat"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					tempInt = NO;
					for (i=0; i<numCurrentDivisions; i++)
						{
						/* check that data type is correct; we do not know yet if the user will specify
						   a nst=6 model so we cannot check that revmat is an active parameter */
						if ((activeParts[i] == YES || nApplied == 0) && (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA))
							{
							strcpy(modelParams[i].revmatFormat, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting reversible rate matrix (revmat) format to %s\n", spacer, modelParams[i].revmatFormat);
							else
								MrBayesPrint ("%s   Setting reversible rate matrix (revmat) format to %s for partition %d\n", spacer, modelParams[i].revmatFormat, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid reversible rate matrix (revmat) format \n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set report format of ratemult *************************************************/
		else if (!strcmp(parmName, "Ratemult"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					tempInt = NO;
					for (i=0; i<numCurrentDivisions; i++)
						{
						/* we do not know yet if the user will specify variable rates across partitions 
						   so only check that we have more than one partition in the model */
						if ((activeParts[i] == YES || nApplied == 0) && numCurrentDivisions > 1)
							{
							strcpy(modelParams[i].ratemultFormat, tempStr);
							if (nApplied == 0 && numCurrentDivisions == 1)
								MrBayesPrint ("%s   Setting rate multiplier (ratemult) format to %s\n", spacer, modelParams[i].ratemultFormat);
							else
								MrBayesPrint ("%s   Setting rate multiplier (ratemult) format to %s for partition %d\n", spacer, modelParams[i].ratemultFormat, i+1);
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid rate multiplier (ratemult) format \n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set report format of tree ***************************************************/
		else if (!strcmp(parmName, "Tree"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					tempInt = NO;
					for (i=0; i<numCurrentDivisions; i++)
						{
						strcpy(modelParams[i].treeFormat, tempStr);
						if (nApplied == 0 && numCurrentDivisions == 1)
							MrBayesPrint ("%s   Setting tree report format to %s\n", spacer, modelParams[i].treeFormat);
						else
							MrBayesPrint ("%s   Setting tree report format to %s for partition %d\n", spacer, modelParams[i].treeFormat, i+1);
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid tree report format \n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set inferancstates ***********************************************************/
		else if (!strcmp(parmName, "Ancstates"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							strcpy(modelParams[i].inferAncStates,tempStr);
							if (!strcmp(tempStr,"Yes"))
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Reporting ancestral states (if applicable)\n", spacer);
								else
									MrBayesPrint ("%s   Reporting ancestral states for partition %d (if applicable)\n", spacer, i+1);
								}
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Not reporting ancestral states\n", spacer);
								else
									MrBayesPrint ("%s   Not reporting ancestral states for partition %d\n", spacer, i+1);
								}
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid ancstates option\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set inferSiteRates ***************************************************************/
		else if (!strcmp(parmName, "Siterates"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							strcpy (modelParams[i].inferSiteRates, tempStr);
							if (!strcmp(tempStr,"Yes"))
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Reporting site rates (if applicable)\n", spacer);
								else
									MrBayesPrint ("%s   Reporting site rates for partition %d (if applicable)\n", spacer, i+1);
								}
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Not reporting site rates\n", spacer);
								else
									MrBayesPrint ("%s   Not reporting site rates for partition %d\n", spacer, i+1);
								}
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid siterates option\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		/* set inferpossel *************************************************************/
		else if (!strcmp(parmName, "Possel"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							strcpy (modelParams[i].inferPosSel, tempStr);
							if (!strcmp(tempStr, "Yes"))
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Reporting positive selection (if applicable)\n", spacer);
								else
									MrBayesPrint ("%s   Reporting positive selection for partition %d (if applicable)\n", spacer, i+1);
								}
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Not reporting positive selection\n", spacer);
								else
									MrBayesPrint ("%s   Not reporting positive selection for partition %d\n", spacer, i+1);
								}
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid possel option\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
            }
		/* set inferSiteOmegas *************************************************************/
		else if (!strcmp(parmName, "Siteomega"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting (ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					nApplied = NumActiveParts ();
					for (i=0; i<numCurrentDivisions; i++)
						{
						if (activeParts[i] == YES || nApplied == 0)
							{
							strcpy (modelParams[i].inferSiteOmegas, tempStr);
							if (!strcmp(tempStr, "Yes"))
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Reporting site omega values (if applicable)\n", spacer);
								else
									MrBayesPrint ("%s   Reporting site omega values for partition %d (if applicable)\n", spacer, i+1);
								}
							else
								{
								if (nApplied == 0 && numCurrentDivisions == 1)
									MrBayesPrint ("%s   Not reporting site omega values\n", spacer);
								else
									MrBayesPrint ("%s   Not reporting site omega values for partition %d\n", spacer, i+1);
								}
							}
						}
					}
				else
					{
					MrBayesPrint ("%s   Invalid siteomega option\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		else
			{
			return (ERROR);
			}
		}

	return (NO_ERROR);		
}





int DoStartvals (void)

{

    MrBayesPrint ("%s   Successfully set starting values\n", spacer);
    /*
    for (i=0; i<numParams; i++)
        assert (IsTreeConsistent(&params[i], 0, 0) == YES);
    */
	return (NO_ERROR);
	
}





int DoStartvalsParm (char *parmName, char *tkn)

{

	int                 i, j, k, nMatches, tempInt, treeIndex, chainId, ret;
	static char		    *temp=NULL;
	MrBFlt				tempFloat, *value, *subValue;
	Tree				*theTree, *usrTree;
	PolyTree			*thePolyTree;
    MrBFlt              minRate, maxRate, clockRate;
	static Param	    *param = NULL;
	static MrBFlt		*theValue, theValueMin, theValueMax;
	static int			useSubvalues, useStdStateFreqs, useIntValues, numExpectedValues, nValuesRead, runIndex, chainIndex, foundName, foundDash;
	static char			*tempName=NULL;

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before starting values can be changed\n", spacer);
		return (ERROR);
		}

	if (expecting == Expecting(PARAMETER))
		{
		if (!strcmp(parmName, "Xxxxxxxxxx"))
			{
			/* we expect a parameter name with possible run and chain specification as follows:
			   <param_name>(<run>,<chain>)=(<number>,...)   -- apply to run <run> and chain <chain>
			   <param_name>(,<chain>)=(<number>,...)        -- apply to chain <chain> for all runs
			   <param_name>(<run>,)=(<number>,...)          -- apply to all chains of run <run>

		       topology and branch length parameters are specified like
			   <param_name>(<run>,<chain>)=<tree_name>|<Newick_tree_spec>

			   parameter names will often be followed by partition specifiers like:
			   pi{all}
			   pinvar{1,4,5}
			   so we need to assemble the parameter name from several tokens that are parsed out separately;
			   here we receive only the first part (before the left curly, if present)
			*/
			
			/* copy to local parameter name */
			SafeStrcpy (&tempName, tkn);
			param = NULL;
			runIndex = chainIndex = -1;
			useSubvalues = NO;
            useIntValues = NO;
            useStdStateFreqs = NO;
			foundComma = foundEqual = foundName = foundDash = NO;
			expecting = Expecting(LEFTCURL) | Expecting(LEFTPAR) | 	Expecting(EQUALSIGN);
			}
		else
			return (ERROR);
		}
	else if (expecting == Expecting(ALPHA))
		{
		if (param == NULL)
			{
			/* we are still assembling the parameter name */
			SafeStrcat (&tempName, tkn);
			expecting = Expecting(RIGHTCURL) | Expecting(COMMA) | Expecting(LEFTPAR) | Expecting(EQUALSIGN);
			}
		else
			{
			/* we have a tree name; now find the tree based on name, case insensitive */
			if (GetUserTreeFromName (&treeIndex, tkn) == ERROR || treeIndex == -1)
				{
				MrBayesPrint ("%s   Error in finding user tree\n", spacer);
				return (ERROR);
				}

			/* set the tree parameter */
			for (i=0; i<chainParams.numRuns; i++)
				{
				if (runIndex != -1 && i != runIndex)
					continue;
				for (j=0; j<chainParams.numChains; j++)
					{
					if (chainIndex != -1 && j != chainIndex)
						continue;
                    chainId = i*chainParams.numChains + j;
                    if (param->paramType != P_POPSIZE)
                        {
                        if (param->paramType == P_TOPOLOGY || param->paramType == P_BRLENS || param->paramType == P_SPECIESTREE)
                            {
                            /* topology or brlens or speciestree params */
					        theTree = GetTree (param, chainId, 0);
					        usrTree = GetTree (param, chainId, 1); /* use as scratch space */
                            }
                        else
                            {
                            /* relaxed clock params */
                            theTree = GetTree (modelSettings[param->relParts[0]].brlens, chainId, 0);
                            usrTree = GetTree (modelSettings[param->relParts[0]].brlens, chainId, 1);
                            }
					    CopyToTreeFromTree (usrTree, theTree);
                        if (param->paramType == P_SPECIESTREE)
                            thePolyTree = AllocatePolyTree(numSpecies);
                        else
                            thePolyTree = AllocatePolyTree (numTaxa);
					    CopyToPolyTreeFromPolyTree (thePolyTree, userTree[treeIndex]);
					    if (param->paramType == P_SPECIESTREE)
                            {
                            ResetIntNodeIndices(thePolyTree);
                            }
                        else
                            {
                            PrunePolyTree (thePolyTree);
                            ResetTipIndices(thePolyTree);
                            }
					    RandResolve (NULL, thePolyTree, &globalSeed, theTree->isRooted);
                        if (param->paramType == P_SPECIESTREE)
                            ret=CopyToSpeciesTreeFromPolyTree (usrTree, thePolyTree);
					    else
                            ret=CopyToTreeFromPolyTree (usrTree, thePolyTree);
					    FreePolyTree (thePolyTree);
                        if(ret==ERROR)
                            return ERROR;
                        }
                    else
                        {
                        /* param->paramType == P_POPSIZE */
                        theTree = GetTree (modelSettings[param->relParts[0]].speciesTree, chainId, 0);
                        usrTree = GetTree (modelSettings[param->relParts[0]].speciesTree, chainId, 1);
					    CopyToTreeFromTree (usrTree, theTree);
                        thePolyTree = AllocatePolyTree(numSpecies);
					    CopyToPolyTreeFromPolyTree (thePolyTree, userTree[treeIndex]);
                        ResetIntNodeIndices(thePolyTree);
					    RandResolve (NULL, thePolyTree, &globalSeed, theTree->isRooted);
                        CopyToSpeciesTreeFromPolyTree (usrTree, thePolyTree);
					    FreePolyTree (thePolyTree);
                        }
					if (param->paramType == P_TOPOLOGY)
						{
						if (theTree->checkConstraints == YES && CheckSetConstraints (usrTree) == ERROR)
							{
							MrBayesPrint ("%s   Could not set the constraints for topology parameter '%s'\n", spacer, param->name);
							return (ERROR);
							}
						if (ResetTopologyFromTree (theTree, usrTree) == ERROR)
							{
							MrBayesPrint ("%s   Could not set the topology parameter '%s'\n", spacer, param->name);
							return (ERROR);
							}
						if (theTree->checkConstraints == YES && CheckSetConstraints (theTree)==ERROR)
							{
							MrBayesPrint ("%s   Could not set the constraints for topology parameter '%s'\n", spacer, param->name);
							return (ERROR);
							}
                        FillTopologySubParams (param, chainId, 0, &globalSeed);
                        //MrBayesPrint ("%s   Branch lengths and relaxed clock subparamiters of a paramiter '%s' are reset.\n", spacer, param->name);
                        if (param->paramId == TOPOLOGY_SPECIESTREE)
                            FillSpeciesTreeParams(&globalSeed, chainId, chainId+1);
                        assert (IsTreeConsistent(param, chainId, 0) == YES);
						}
					else if (param->paramType == P_BRLENS)
						{
                        if (usrTree->allDownPass[0]->length == 0.0)
							{
							MrBayesPrint ("%s   User tree '%s' does not have branch lengths so it cannot be used in setting parameter '%s'\n", spacer, userTree[treeIndex]->name, param->name);
							return (ERROR);
							}
                     /*   if (theTree->isClock == YES && IsClockSatisfied (usrTree,0.001) == NO)
							{
							MrBayesPrint ("%s   Branch lengths of the user tree '%s' does not satisfy clock in setting parameter '%s'\n", spacer, userTree[treeIndex]->name, param->name);
							ShowNodes(usrTree->root,0,YES);
							return (ERROR);
							}
                     */
                      /*  if (theTree->isCalibrated == YES && IsCalibratedClockSatisfied (usrTree,0.001) == NO) //no calibration is set up in usertree so do not do this check
							{
							MrBayesPrint ("%s   Branch lengths of the user tree '%s' does not satisfy clock calibrations in setting parameter '%s'\n", spacer, userTree[treeIndex]->name, param->name);
							return (ERROR);
							}
                     */
						if (AreTopologiesSame (theTree, usrTree) == NO)
							{
							MrBayesPrint ("%s   Topology of user tree '%s' wrong in setting parameter '%s'\n", spacer, userTree[treeIndex]->name, param->name);
							return (ERROR);
							}
                        //assert (IsTreeConsistent(param, chainId, 0) == YES);
                            /* reset node depths to ensure that non-dated tips have node depth 0.0 */
                        /*if (usrTree->isClock == YES)
                            SetNodeDepths(usrTree);
                        */ 
                        if (ResetBrlensFromTree (theTree, usrTree) == ERROR)
							{
							MrBayesPrint ("%s   Could not set parameter '%s' from user tree '%s'\n", spacer, param->name, userTree[treeIndex]->name);
							return (ERROR);
							}
                        if (theTree->isClock == YES && !strcmp(modelParams[theTree->relParts[0]].treeAgePr,"Fixed"))
                            {
                            if (!strcmp(modelParams[theTree->relParts[0]].clockPr,"Uniform"))
                                ResetRootHeight (theTree, modelParams[theTree->relParts[0]].treeAgeFix);
                            }
                        /* the test will find suitable clock rate and ages of nodes in theTree */
                        if (theTree->isClock == YES && IsClockSatisfied (theTree,0.001) == NO)
							{
							MrBayesPrint ("%s   Non-calibrated tips are not at the same level after setting up starting tree branch lengthes(%s) from user tree '%s'.\n", spacer, param->name, userTree[treeIndex]->name);
							ShowNodes(theTree->root,0,YES);
							return (ERROR);
							}
                        if (theTree->isCalibrated == YES && IsCalibratedClockSatisfied (theTree,&minRate,&maxRate, 0.001) == NO)
							{
							MrBayesPrint ("%s   Problem setting calibrated tree parameters\n", spacer);
							return (ERROR);
							}
                        if (theTree->isCalibrated == YES && !strcmp(modelParams[theTree->relParts[0]].clockRatePr, "Fixed"))
                            {
                            clockRate = modelParams[theTree->relParts[0]].clockRateFix;
                            if(( clockRate < minRate && AreDoublesEqual (clockRate, minRate , 0.0001) == NO ) || ( clockRate > maxRate && AreDoublesEqual (clockRate, maxRate , 0.0001) == NO ))
                                {
					            MrBayesPrint("%s   Fixed branch lengths do not satisfy fixed clockrate", spacer);
					            return (ERROR);
					            }
                            }
                        theTree->fromUserTree=YES;
                        
						FillBrlensSubParams (param, chainId, 0);
                        //MrBayesPrint ("%s   Rrelaxed clock subparamiters of a paramiter '%s' are reset.\n", spacer, param->name);
                        //assert (IsTreeConsistent(param, chainId, 0) == YES);
                        if (param->paramId == BRLENS_CLOCK_SPCOAL)
                            FillSpeciesTreeParams(&globalSeed, chainId, chainId+1);
                        //assert (IsTreeConsistent(param, chainId, 0) == YES);
						}
					else if (param->paramType == P_CPPEVENTS || param->paramType == P_TK02BRANCHRATES || param->paramType == P_IGRBRANCHLENS)
						{
                        if( theTree->isCalibrated == YES && theTree->fromUserTree == NO )
                            {/*if theTree is not set from user tree then we can not garanty that branch lenghts will stay the same by the time we start mcmc run because of clockrate adjustment.*/
                            MrBayesPrint ("%s    Set starting values for branch lenghtes first! Starting value of relaxed paramiters could be set up only for trees where branch lengths are already set up from user tree.\n", spacer, param->name);
							return (ERROR);
                            }
						if ( theTree->isCalibrated == NO && IsClockSatisfied (usrTree, 0.0001) == NO ) // user tree is not calibrated so do not check it if calibration is in place
							{
							MrBayesPrint ("%s   Branch lengths of the user tree '%s' do not satisfy clock in setting parameter '%s'\n", spacer, userTree[treeIndex], param->name);
							ShowNodes(usrTree->root,0,YES);
							return (ERROR);
							}
                        /*   
                        if (theTree->isCalibrated == YES && IsCalibratedClockSatisfied (usrTree,0.0001) == NO)
							{
							MrBayesPrint ("%s   Branch lengths of the user tree '%s' do not satisfy clock calibrations in setting parameter '%s'\n", spacer, userTree[treeIndex]->name, param->name);
							return (ERROR);
							}
                        */
						if (AreTopologiesSame (theTree, usrTree) == NO)
							{
							MrBayesPrint ("%s   Topology of user tree '%s' is wrong in setting parameter '%s'\n", spacer, userTree[treeIndex]->name, param->name);
							return (ERROR);
							}
						if (SetRelaxedClockParam (param, chainId, 0, userTree[treeIndex]) == ERROR)
							{
							MrBayesPrint ("%s   Could not set parameter '%s' from user tree '%s'\n", spacer, param->name, userTree[treeIndex]->name);
							return (ERROR);
							}
                        //assert (IsTreeConsistent(param, chainId, 0) == YES);
						}
                    else if (param->paramType == P_POPSIZE)
                        {
                        if (AreTopologiesSame (theTree, usrTree) == NO)
							{
							MrBayesPrint ("%s   Topology of user tree '%s' is wrong in setting parameter '%s'\n", spacer, userTree[treeIndex]->name, param->name);
							return (ERROR);
							}
						if (SetPopSizeParam (param, chainId, 0, userTree[treeIndex]) == ERROR)
							{
							MrBayesPrint ("%s   Could not set parameter '%s' from user tree '%s'\n", spacer, param->name, userTree[treeIndex]->name);
							return (ERROR);
							}
                        }
                    else if (param->paramType == P_SPECIESTREE)
                        {
                        if (IsSpeciesTreeConsistent (usrTree, chainId) == NO)
							{
							MrBayesPrint ("%s   User-specified species tree '%s' is inconsistent with gene trees\n", spacer, userTree[treeIndex]->name);
							return (ERROR);
							}
						if (CopyToTreeFromTree (theTree, usrTree) == ERROR)
							{
							MrBayesPrint ("%s   Could not set the species tree parameter '%s'\n", spacer, param->name);
							return (ERROR);
							}
                        assert (IsTreeConsistent(param, chainId, 0) == YES);
                        }
					}
                    /*
                for (j=0; j<numParams; j++)
                    assert (IsTreeConsistent(&params[j],0,0) == YES);
                    */
				}
			expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
			}
		}
	else if (expecting == Expecting(LEFTCURL))
		{
		/* we are still assembling the parameter name */
		SafeStrcat (&tempName, tkn);
		expecting = Expecting(NUMBER) | Expecting(ALPHA) | Expecting(LEFTPAR) | Expecting(EQUALSIGN);
		}
	else if (expecting == Expecting(RIGHTCURL))
		{
		/* we are still assembling the parameter name */
		SafeStrcat (&tempName, tkn);
		foundComma = NO; /*! if there was a comma in the partition part, we should reset this variable. Otherwise we can't parse something like A{1,2}(3,4) */
		expecting = Expecting(LEFTPAR) | Expecting(EQUALSIGN);
		}
	else if (expecting == Expecting(LEFTPAR))
		{
		if (foundEqual == NO)
			{
			foundName = YES;	/* we found the name */
			/* we will be reading in run and chain indices */
			expecting = Expecting(NUMBER) | Expecting(COMMA);
			}
		else
            {
			expecting = Expecting(NUMBER);
            expecting |= Expecting(DASH);
            }
		}
    else if (expecting == Expecting(DASH))
        {
        foundDash = YES;
        expecting = Expecting(NUMBER);
        }
	else if (expecting == Expecting(NUMBER))
		{
		if (foundName == NO)
			{
			/* we are still assembling the parameter name */
			SafeStrcat (&tempName, tkn);
			expecting = Expecting(COMMA) | Expecting(LEFTPAR) | Expecting(RIGHTCURL) | Expecting(EQUALSIGN);
			}
		else if (foundEqual == YES)
			{
            theValueMin = param->min;
            theValueMax = param->max;

            /* we are reading in a parameter value */
            if (param->paramType==P_OMEGA && nValuesRead==numExpectedValues && useSubvalues == NO)
                {
                /* continue with subvalues */
                nValuesRead = 0;
                numExpectedValues = param->nSubValues/2;
                useSubvalues = YES;
                theValueMin = ETA;
                theValueMax = 1.0;
                }
            if (param->paramType==P_OMEGA && nValuesRead==numExpectedValues && useSubvalues == NO)
                {
                /* continue with subvalues */
                nValuesRead = 0;
                numExpectedValues = param->nSubValues/2;
                useSubvalues = YES;
                theValueMin = ETA;
                theValueMax = 1.0;
                }
            if (param->nIntValues > 0 && nValuesRead==numExpectedValues && useIntValues == NO)
                {
                /* continue with intValues */
                nValuesRead = 0;
                numExpectedValues = param->nIntValues;
                useIntValues = YES;
                }
            if (param->paramType==P_PI && modelSettings[param->relParts[0]].dataType == STANDARD && param->paramId != SYMPI_EQUAL
                && nValuesRead==numExpectedValues && useStdStateFreqs == NO)
                {
                /* we have read alpha_symdir, continue with multistate char state freqs */
                nValuesRead = 0;
                numExpectedValues = param->nStdStateFreqs;
                if (param->hasBinaryStd == YES)
                    numExpectedValues -= 2 * modelSettings[param->relParts[0]].numBetaCats;
                useStdStateFreqs = YES;
                theValueMin = ETA;
                theValueMax = 1.0;
                }
			nValuesRead++;
			if (nValuesRead > numExpectedValues)
				{
				if (param->paramType == P_OMEGA)
                    MrBayesPrint ("%s   Only %d values were expected for parameter '%s'\n", spacer, param->nValues+param->nSubValues/2, param->name);
                else if (param->nIntValues > 0)   
                    MrBayesPrint ("%s   Only %d values were expected for parameter '%s'\n", spacer, param->nValues+param->nIntValues, param->name);
                else
                    MrBayesPrint ("%s   Only %d values were expected for parameter '%s'\n", spacer, numExpectedValues, param->name);
				return (ERROR);
				}
            if (useIntValues == YES)
			    sscanf (tkn, "%d", &tempInt);
			else
                sscanf (tkn, "%lf", &tempFloat);
            if (foundDash == YES)
                {
                if (useIntValues == NO)
                    tempFloat = -tempFloat;
                else
                    tempInt = -tempInt;
                foundDash = NO;
                }
			if (useIntValues == NO && (tempFloat < theValueMin || tempFloat > theValueMax))
				{
				MrBayesPrint ("%s   The value is out of range (min = %lf; max = %lf)\n", spacer, theValueMin, theValueMax);
				return (ERROR);
				}
			for (i=0; i<chainParams.numRuns; i++)
				{
				if (runIndex != -1 && runIndex != i)
					continue;
				for (j=0; j<chainParams.numChains; j++)
					{
					if (chainIndex != -1 && chainIndex != j)
						continue;
					if (useIntValues == YES)
                        {
                        GetParamIntVals (param, i*chainParams.numChains+j, 0)[nValuesRead-1] = tempInt;
                        }
                    else
                        {
                        if (useSubvalues == NO && useStdStateFreqs == NO)
						    theValue = GetParamVals (param, i*chainParams.numChains+j, 0);
					    else if (useSubvalues == YES)
						    theValue = GetParamSubVals (param, i*chainParams.numChains+j, 0);
					    else if (useStdStateFreqs == YES)
                            {
                            theValue = GetParamStdStateFreqs (param, i*chainParams.numChains+j, 0);
                            if (param->hasBinaryStd == YES)
                                theValue += 2 * modelSettings[param->relParts[0]].numBetaCats;
                            }
                        else
                            return (ERROR);
                        if( param->paramType == P_CLOCKRATE )
                            {
                            if( UpdateClockRate(tempFloat, i*chainParams.numChains+j) == ERROR) 
                                {
                                return (ERROR);
                                }
                            }
					    theValue[nValuesRead-1] = tempFloat;
                        }
					}
				}
			expecting = Expecting (COMMA) | Expecting(RIGHTPAR);
			}
		else /* if (foundEqual == NO) */
			{
			sscanf (tkn, "%d", &tempInt);
			if (foundComma == NO)
				{
				if (tempInt <= 0 || tempInt > chainParams.numRuns)
					{
					MrBayesPrint ("%s   Run index is out of range (min=1; max=%d)\n", spacer, chainParams.numRuns);
					return (ERROR);
					}
				runIndex = tempInt - 1;
				expecting = Expecting(COMMA);
				}
			else
				{
				if (tempInt <= 0 || tempInt > chainParams.numChains)
					{
					MrBayesPrint ("%s   Chain index is out of range (min=1; max=%d)\n", spacer, chainParams.numChains);
					return (ERROR);
					}
				chainIndex = tempInt - 1;
				foundComma = NO;
				expecting = Expecting(RIGHTPAR);
				}
			}
		}
	else if (expecting == Expecting(COMMA))
		{
		if (foundEqual == YES)
			{
			/* we expect another parameter value */
			expecting = Expecting(NUMBER);
			}
		else /* if (foundEqual == NO) */
			{
			/* we will be reading in chain index, if present */
			foundComma = YES;
			expecting = Expecting(RIGHTPAR) | Expecting(NUMBER); 
            /* if the comma is in a list of partitions (so between { and }) we have to add the comma to the parameter name */
			if (param == NULL && strchr(tempName, '}')==NULL && strchr(tempName, '{')!=NULL ) 
			  SafeStrcat (&tempName, ",");
			}
		}
	else if (expecting == Expecting(RIGHTPAR))
		{
		if (foundEqual == NO)
			{
			/* this is the end of the run and chain specification */
			expecting = Expecting(EQUALSIGN);
			}
		else /* if (foundEqual == YES) */
			{
			/* this is the end of the parameter values */
			if (nValuesRead != numExpectedValues)
				{
                MrBayesPrint ("%s   Expected %d values but only found %d values for parameter '%s'\n", spacer, numExpectedValues, nValuesRead, param->name);
				return (ERROR);
				}
   			/* Post processing needed for some parameters */
            if (param->paramType == P_SHAPE || param->paramType == P_CORREL)
                {
                for (i=0; i<chainParams.numRuns; i++)
				    {
				    if (runIndex != -1 && runIndex != i)
					    continue;
				    for (j=0; j<chainParams.numChains; j++)
					    {
					    if (chainIndex != -1 && chainIndex != j)
						    continue;
                        value = GetParamVals(param,i*chainParams.numChains+j,0);
                        subValue = GetParamSubVals(param,i*chainParams.numChains+j,0);
					    if (param->paramType == P_SHAPE)
						    {
			                if (DiscreteGamma (subValue, value[0], value[0], param->nSubValues, 0) == ERROR)
                				return (ERROR);
						    }
						else if (param->paramType == P_CORREL)
                            AutodGamma (subValue, value[0], (int)(sqrt(param->nSubValues) + 0.5));
                        }
                    }
				}
			expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
			}
		}
	else if (expecting == Expecting(EQUALSIGN))
		{
		foundEqual = YES;
		foundName = YES;

		/* we now know that the name is complete; try to find the parameter with this name (case insensitive) */
		for (i=0; i<(int)strlen(tempName); i++)
			tempName[i] = tolower(tempName[i]);
		
		/* first check exact matches */
		nMatches = j = 0;
		for (i=0; i<numParams; i++)
			{
			param = &params[i];
			SafeStrcpy (&temp, param->name);
			for (k=0; k<(int)strlen(temp); k++)
				temp[k] = tolower(temp[k]);
			if (strcmp(tempName,temp) == 0)
				{
				j = i;
				nMatches++;
				}
			}
		/* now check unambiguous abbreviation matches */
		if (nMatches == 0)
			{
			nMatches = j = 0;
			for (i=0; i<numParams; i++)
				{
				param = &params[i];
				SafeStrcpy (&temp, param->name);
				for (k=0; k<(int)strlen(temp); k++)
					temp[k] = tolower(temp[k]);
				if (strncmp(tempName,temp,strlen(tempName)) == 0)
					{
					j = i;
					nMatches++;
					}
				}
			}

		if (nMatches == 0)
			{
            extern char *tokenP;
            MrBayesPrint ("%s   Could not find parameter '%s': ignoring values\n", spacer, tempName);
            while(*tokenP && *tokenP++!=')') {}; 
            expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
			return (*tokenP ? NO_ERROR:ERROR);
			}
		else if (nMatches > 1)
			{
			MrBayesPrint ("%s   Several parameters matched the abbreviated name '%s'\n", spacer, tempName);
			return (ERROR);
			}
			
		param = &params[j];
		if (param->printParam == NO && !(param->paramType == P_TOPOLOGY && strcmp(modelParams[param->relParts[0]].topologyPr,"Fixed")!=0)
                                    && !param->paramType == P_CPPEVENTS
                                    && !param->paramType == P_TK02BRANCHRATES
                                    && !param->paramType == P_IGRBRANCHLENS
                                    && !(param->paramType == P_POPSIZE && param->nValues > 1))
			{
			MrBayesPrint ("%s   The parameter '%s' is fixed so the starting value cannot be set\n", spacer, param->name);
			return (ERROR);
			}
		if (param->paramType == P_BRLENS || param->paramType == P_TOPOLOGY || param->paramType == P_CPPEVENTS ||
            param->paramType == P_TK02BRANCHRATES || param->paramType == P_IGRBRANCHLENS || param->paramType == P_SPECIESTREE ||
            (param->paramType == P_POPSIZE && param->nValues > 1))
			{
			/* all these parameters are set from a tree */
			expecting = Expecting (ALPHA);
			}
		else
			/* run of the mill character */
			{
			theValueMin = param->min;
			theValueMax = param->max;
			if ((param->paramType == P_PI && modelParams[param->relParts[0]].dataType != STANDARD))
				{
				useSubvalues = YES;
                useIntValues = NO;
				numExpectedValues = param->nSubValues;
				}
            else if (param->nValues == 0 && param->nIntValues > 0)
                {
                useSubvalues = NO;
                useIntValues = YES;
                numExpectedValues = param->nIntValues;
                }
            else if (param->nValues > 0)
				{
				useSubvalues = NO;
                useIntValues = NO;
				numExpectedValues = param->nValues;
				}
            else
                {
			    MrBayesPrint ("%s   Not expecting any values for parameter '%s'\n", spacer, param->name);
			    return (ERROR);
                }
			nValuesRead = 0;
			expecting = Expecting(LEFTPAR);
			}
		}
	else
		return (ERROR);

    SafeFree ((void **)&temp);
	return (NO_ERROR);
}





int DoUnlink (void)

{

	int			i, j;
	
	MrBayesPrint ("%s   Unlinking\n", spacer);
	
	/* update status of linkTable */
	for (j=0; j<NUM_LINKED; j++)
		{
		for (i=0; i<numCurrentDivisions; i++)
			{
			if (tempLinkUnlink[j][i] == YES)
				{
				linkTable[j][i] = ++linkNum;
				}
			}
		}
	
#	if 0
	for (j=0; j<NUM_LINKED; j++)
		{
		MrBayesPrint ("%s   ", spacer);
		for (i=0; i<numCurrentDivisions; i++)
			MrBayesPrint ("%d", linkTable[j][i]);
		MrBayesPrint ("\n");
		}
#	endif

	/* reinitialize the temporary table */
	for (j=0; j<NUM_LINKED; j++)
		for (i=0; i<numCurrentDivisions; i++)
			tempLinkUnlink[j][i] = NO;

	/* set up parameters and moves */
	if (SetUpAnalysis (&globalSeed) == ERROR)
		return (ERROR);

	return (NO_ERROR);
	
}





int DoShowMcmcTrees (void)

{

	int			run, chain, chainIndex, i;
	Tree		*t;

	for (run=0; run<chainParams.numRuns; run++)
		{
		for (chain=0; chain<chainParams.numChains; chain++)
			{
			chainIndex = run*chainParams.numChains + chain;
			for (i=0; i<numTrees; i++)
				{
				t = GetTreeFromIndex (i, chainIndex, 0);
				if (t->isRooted == YES)
					MrBayesPrint ("\n   Tree '%s' [rooted]:\n\n", t->name);
				else
					MrBayesPrint ("\n   Tree '%s' [unrooted]:\n\n", t->name);
				if (ShowTree (t) == ERROR)
					return (ERROR);
				else
					MrBayesPrint ("\n");
				}
			}
		}

	return (NO_ERROR);

}





int DoShowModel (void)

{

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before the model can be defined\n", spacer);
		return (ERROR);
		}

	if (ShowModel() == ERROR)
		return (ERROR);

	return (NO_ERROR);

}





int DoShowMoves (void)

{

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before moves can be assigned\n", spacer);
		return (ERROR);
		}

	MrBayesPrint ("%s   Moves that will be used by MCMC sampler (rel. proposal prob. > 0.0):\n\n", spacer);
	if (ShowMoves(YES) == ERROR)
		return (ERROR);

	if (showmovesParams.allavailable == YES)
		{
		MrBayesPrint ("%s   Other available moves (rel. proposal prob. = 0.0):\n\n", spacer);
		if (ShowMoves(NO) == ERROR)
			return (ERROR);
		}
	else
		MrBayesPrint ("%s   Use 'Showmoves allavailable=yes' to see a list of all available moves\n", spacer);

	return (NO_ERROR);

}





int DoShowmovesParm (char *parmName, char *tkn)

{

	char	tempStr[100];
	
	if (expecting == Expecting(PARAMETER))
		{
		expecting = Expecting(EQUALSIGN);
		}
	else
		{
		/* set Allavailable **********************************************************/
		if (!strcmp(parmName, "Allavailable"))
			{
			if (expecting == Expecting(EQUALSIGN))
				expecting = Expecting(ALPHA);
			else if (expecting == Expecting(ALPHA))
				{
				if (IsArgValid(tkn, tempStr) == NO_ERROR)
					{
					if (!strcmp(tempStr, "Yes"))
						showmovesParams.allavailable = YES;
					else
						showmovesParams.allavailable = NO;
					}
				else
					{
					MrBayesPrint ("%s   Invalid argument for allavailable\n", spacer);
					return (ERROR);
					}
				expecting = Expecting(PARAMETER) | Expecting(SEMICOLON);
				}
			else
				return (ERROR);
			}
		else
			return (ERROR);

		}
		
	return (NO_ERROR);
}





int DoShowParams (void)

{

	if (defMatrix == NO)
		{
		MrBayesPrint ("%s   A matrix must be specified before model parameters can be shown\n", spacer);
		return (ERROR);
		}

	if (ShowParameters(YES, YES, YES) == ERROR)
		return (ERROR);

	return (NO_ERROR);

}





/*------------------------------------------------------------------------
|
|	FillNormalParams: Allocate and fill in non-tree parameters
|
-------------------------------------------------------------------------*/
int FillNormalParams (SafeLong *seed, int fromChain, int toChain)

{

	int			i, j, k, chn, tempInt, *intValue;
	MrBFlt		*bs, *value, *subValue, scaler;
    Tree        *tree;
	Param		*p;
	ModelInfo	*m;
	ModelParams	*mp;

	

	/* fill in values for nontree params for state 0 of chains */
	for (chn=fromChain; chn<toChain; chn++)
		{
		for (k=0; k<numParams; k++)
			{
			p  = &params[k];
			mp = &modelParams[p->relParts[0]];
			m  = &modelSettings[p->relParts[0]];

			/* find model settings and nStates, pInvar, invar cond likes */

			value = GetParamVals (p, chn, 0);
			subValue = GetParamSubVals (p, chn, 0);
            intValue = GetParamIntVals (p, chn, 0);

			if (p->paramType == P_TRATIO)
				{
				/* Fill in tratios **************************************************************************************/
				if (p->paramId == TRATIO_DIR)
					value[0] = 1.0;
				else if (p->paramId == TRATIO_FIX)
					value[0] = mp->tRatioFix;
				}
			else if (p->paramType == P_REVMAT)
				{
				/* Fill in revMat ***************************************************************************************/
				/* rates are stored in order, AC or AR first, using the Dirichlet parameterization */
				if (p->paramId == REVMAT_DIR)
					{
					for (j=0; j<p->nValues; j++)
						value[j] = 1.0 / (MrBFlt) (p->nValues);
					}
				else if (p->paramId == REVMAT_FIX)
					{
					scaler = 0.0;
					if (m->dataType == PROTEIN)
						{
						for (j=0; j<190; j++)
							scaler += (value[j] = mp->aaRevMatFix[j]);
						for (j=0; j<190; j++)
							value[j] /= scaler;
						}
					else
						{
						for (j=0; j<6; j++)
							scaler += (value[j] = mp->revMatFix[j]);
						for (j=0; j<6; j++)
							value[j] /= scaler;
						}
					}
				else if (p->paramId == REVMAT_MIX)
                    {
                    for (j=0; j<6; j++)
                        {
                        value[j] = 1.0 / 6.0;
                        intValue[j] = 0;
                        }
                    }
				}
			else if (p->paramType == P_OMEGA)
				{
				/* Fill in omega ****************************************************************************************/
				if (p->nValues == 1)
					{
					if (p->paramId == OMEGA_DIR)
						value[0] = 1.0;
					else if (p->paramId == OMEGA_FIX)
						value[0] = mp->omegaFix;
					}
				else
					{
					if (!strcmp(mp->omegaVar, "Ny98"))
						{
						if (p->paramId == OMEGA_BUD || p->paramId == OMEGA_BUF || p->paramId == OMEGA_BED ||
						    p->paramId == OMEGA_BEF || p->paramId == OMEGA_BFD || p->paramId == OMEGA_BFF)
							value[0] = RandomNumber(seed);
						else if (p->paramId == OMEGA_FUD || p->paramId == OMEGA_FUF || p->paramId == OMEGA_FED ||
						         p->paramId == OMEGA_FEF || p->paramId == OMEGA_FFD || p->paramId == OMEGA_FFF)
							value[0] = mp->ny98omega1Fixed;
						value[1] = 1.0;
						if (p->paramId == OMEGA_BUD || p->paramId == OMEGA_BUF || p->paramId == OMEGA_FUD ||
						    p->paramId == OMEGA_FUF)
							value[2] = mp->ny98omega3Uni[0] + RandomNumber(seed) * (mp->ny98omega3Uni[1] - mp->ny98omega3Uni[0]);
						else if (p->paramId == OMEGA_BED || p->paramId == OMEGA_BEF || p->paramId == OMEGA_FED ||
						         p->paramId == OMEGA_FEF)
							value[2] =  (1.0 + -(1.0/mp->ny98omega3Exp) * log(1.0 - RandomNumber(seed)));
						else
							value[2] = mp->ny98omega3Fixed;
						if (p->paramId == OMEGA_BUD || p->paramId == OMEGA_BED || p->paramId == OMEGA_BFD || 
							p->paramId == OMEGA_FUD || p->paramId == OMEGA_FED || p->paramId == OMEGA_FFD) 
						    {
							subValue[3] = mp->codonCatDir[0];
							subValue[4] = mp->codonCatDir[1];
							subValue[5] = mp->codonCatDir[2];
							DirichletRandomVariable (&subValue[3], &subValue[0], 3, seed);
							}
						else
							{
							subValue[0] = mp->codonCatFreqFix[0];
							subValue[1] = mp->codonCatFreqFix[1];
							subValue[2] = mp->codonCatFreqFix[2];
							subValue[3] = 0.0;
							subValue[4] = 0.0;
							subValue[5] = 0.0;
							}
						}
					else if (!strcmp(mp->omegaVar, "M3"))
						{
						if (p->paramId == OMEGA_FD || p->paramId == OMEGA_FF)
							{
							value[0] = mp->m3omegaFixed[0];
							value[1] = mp->m3omegaFixed[1];
							value[2] = mp->m3omegaFixed[2];
							}
						else
							{
							value[0] =  0.1;
							value[1] =  1.0;
							value[2] =  3.0;
							}
						if (p->paramId == OMEGA_ED || p->paramId == OMEGA_FD) 
						    {
							subValue[3] = mp->codonCatDir[0];
							subValue[4] = mp->codonCatDir[1];
							subValue[5] = mp->codonCatDir[2];
							DirichletRandomVariable (&subValue[3], &subValue[0], 3, seed);
							}
						else
							{
							subValue[0] = mp->codonCatFreqFix[0];
							subValue[1] = mp->codonCatFreqFix[1];
							subValue[2] = mp->codonCatFreqFix[2];
							subValue[3] = 0.0;
							subValue[4] = 0.0;
							subValue[5] = 0.0;
							}
						}
					else if (!strcmp(mp->omegaVar, "M10"))
						{
						if (p->paramId == OMEGA_10UUB || p->paramId == OMEGA_10UEB || p->paramId == OMEGA_10UFB ||
						    p->paramId == OMEGA_10EUB || p->paramId == OMEGA_10EEB || p->paramId == OMEGA_10EFB ||
						    p->paramId == OMEGA_10FUB || p->paramId == OMEGA_10FEB || p->paramId == OMEGA_10FFB) 
						    {
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 2] = mp->codonCatDir[0];
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 3] = mp->codonCatDir[1];
							DirichletRandomVariable (&subValue[mp->numM10BetaCats + mp->numM10GammaCats + 2], &subValue[mp->numM10BetaCats + mp->numM10GammaCats + 0], 2, seed);
							}
						else
							{
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 0] = mp->codonCatFreqFix[0];
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 1] = mp->codonCatFreqFix[1];
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 2] = 0.0;
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 3] = 0.0;
							}
							
						for (i=0; i<mp->numM10BetaCats; i++)
							subValue[i] = subValue[mp->numM10BetaCats + mp->numM10GammaCats + 0] / mp->numM10BetaCats;
						for (i=mp->numM10BetaCats; i<mp->numM10BetaCats+mp->numM10GammaCats; i++)
							subValue[i] = subValue[mp->numM10BetaCats + mp->numM10GammaCats + 1] / mp->numM10GammaCats;

						if (p->paramId == OMEGA_10FUB || p->paramId == OMEGA_10FUF || p->paramId == OMEGA_10FEB ||
						    p->paramId == OMEGA_10FEF || p->paramId == OMEGA_10FFB || p->paramId == OMEGA_10FFF)
							{
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 4] = mp->m10betaFix[0];
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 5] = mp->m10betaFix[1];
							}
						else
							{
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 4] = 1.0;
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 5] = 1.0;
							}

						if (p->paramId == OMEGA_10UFB || p->paramId == OMEGA_10UFF || p->paramId == OMEGA_10EFB ||
						    p->paramId == OMEGA_10EFF || p->paramId == OMEGA_10FFB || p->paramId == OMEGA_10FFF)
							{
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 6] = mp->m10gammaFix[0];
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 7] = mp->m10gammaFix[1];
							}
						else
							{
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 6] = 1.0;
							subValue[mp->numM10BetaCats + mp->numM10GammaCats + 7] = 1.0;
							}
							
						BetaBreaks (subValue[mp->numM10BetaCats + mp->numM10GammaCats + 4], subValue[mp->numM10BetaCats + mp->numM10GammaCats + 5], &value[0], mp->numM10BetaCats);
						if (DiscreteGamma (&value[mp->numM10BetaCats], subValue[mp->numM10BetaCats + mp->numM10GammaCats + 6], subValue[mp->numM10BetaCats + mp->numM10GammaCats + 7], mp->numM10GammaCats, 0) == ERROR)
							return (ERROR);
						for (i=0; i<mp->numM10GammaCats; i++)
							value[mp->numM10BetaCats + i] += 1.0;

							
						}
					else
						{
						
						}
					}
				}
			else if (p->paramType == P_PI)
				{
				/* Fill in state frequencies ****************************************************************************/
				/* note that standard chars are mainly dealt with in ProcessStdChars in mcmc.c */
				if (p->paramId == SYMPI_UNI || p->paramId == SYMPI_UNI_MS)
					value[0] = 1.0;
				else if (p->paramId == SYMPI_EXP || p->paramId == SYMPI_EXP_MS)
					value[0] = 1.0;

				else if (p->paramId == SYMPI_FIX || p->paramId == SYMPI_FIX_MS)
					value[0] = mp->symBetaFix;

				else if (p->paramId == PI_DIR)
					{
					if (mp->numDirParams != mp->nStates && mp->numDirParams != 0)
						{
						MrBayesPrint ("%s   Mismatch between number of dirichlet parameters (%d) and the number of states (%d)\n", spacer, mp->numDirParams, m->numStates);
						return ERROR;
						}

					/* if user has not set dirichlet parameters, go with default */
					/* overall variance equals number of states */
					if (mp->numDirParams == 0)
						for (i=0; i<mp->nStates; i++)
							value[i] = mp->stateFreqsDir[i] = 1.0;
					else
						for (i=0; i<m->numStates; i++)
							value[i] = mp->stateFreqsDir[i];

					/* now fill in subvalues */
					for (i=0; i<m->numStates; i++)
						subValue[i] =  (1.0 / mp->nStates);
					}

				else if (p->paramId == PI_USER)
					{
					for (i=0; i<m->numStates; i++)
						subValue[i] =  mp->stateFreqsFix[i];
					}
					
				else if (p->paramId == PI_FIXED)
					{
					if (!strcmp(mp->aaModelPr, "Fixed"))
						{
						if (!strcmp(mp->aaModel, "Jones"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = jonesPi[i];
							}
						else if (!strcmp(mp->aaModel, "Dayhoff"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = dayhoffPi[i];
							}
						else if (!strcmp(mp->aaModel, "Mtrev"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = mtrev24Pi[i];
							}
						else if (!strcmp(mp->aaModel, "Mtmam"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = mtmamPi[i];
							}
						else if (!strcmp(mp->aaModel, "Wag"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = wagPi[i];
							}
						else if (!strcmp(mp->aaModel, "Rtrev"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = rtrevPi[i];
							}
						else if (!strcmp(mp->aaModel, "Cprev"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = cprevPi[i];
							}
						else if (!strcmp(mp->aaModel, "Vt"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = vtPi[i];
							}
						else if (!strcmp(mp->aaModel, "Blosum"))
							{
							for (i=0; i<mp->nStates; i++)
								subValue[i] = blosPi[i];
							}
						}
					}

				else if (p->paramId == PI_EMPIRICAL)
					{
					if (GetEmpiricalFreqs (p->relParts, p->nRelParts) == ERROR)
						return (ERROR);
					for (i=0; i<mp->nStates; i++)
						subValue[i] = empiricalFreqs[i];
					}

				else if (p->paramId == PI_EQUAL)
					{
					for (i=0; i<mp->nStates; i++)
						subValue[i] =  (1.0 / mp->nStates);
					}
				}
			else if (p->paramType == P_SHAPE)
				{
				/* Fill in gamma values ********************************************************************************/
				/* first get hyperprior */
				if (p->paramId == SHAPE_UNI)
					{
					value[0] = 100.0;
					if (value[0] < mp->shapeUni[0] || value[0] > mp->shapeUni[1])
						value[0] = mp->shapeUni[0] + (mp->shapeUni[1] - mp->shapeUni[0]) *  0.5;
					}
				else if (p->paramId == SHAPE_EXP)
					value[0] = 100.0;
				else if (p->paramId == SHAPE_FIX)
					value[0] = mp->shapeFix;
				/* now fill in rates */
				if (DiscreteGamma (subValue, value[0], value[0], mp->numGammaCats, 0) == ERROR)
					return (ERROR);
				}
			else if (p->paramType == P_PINVAR)
				{
				/* Fill in pInvar ***************************************************************************************/
				if (p->paramId == PINVAR_UNI)
					value[0] = 0.0;

				else if (p->paramId == PINVAR_FIX)
					value[0] =  mp->pInvarFix;
				}
			else if (p->paramType == P_CORREL)
				{
				/* Fill in correlation parameter of adgamma model *******************************************************/
				if (p->paramId == CORREL_UNI)
					value[0] = 0.0;

				else if (p->paramId == CORREL_FIX)
					value[0] =  mp->corrFix;
				
				/* Fill in correlation matrices */
				AutodGamma (subValue, value[0], mp->numGammaCats);
				}
			else if (p->paramType == P_SWITCH)
				{
				/* Fill in switchRates for covarion model ***************************************************************/
				for (j=0; j<2; j++)
					{
					if (p->paramId == SWITCH_UNI)
						value[j] = RandomNumber(seed) * (mp->covswitchUni[1] - mp->covswitchUni[0]) + mp->covswitchUni[0];

					else if (p->paramId == SWITCH_EXP)
						value[j] =   (-(1.0/mp->covswitchExp) * log(1.0 - RandomNumber(seed)));

					else if (p->paramId == SWITCH_FIX)
						value[j] = mp->covswitchFix[j];
					}
				}
			else if (p->paramType == P_RATEMULT)
				{
				/* Fill in division rates *****************************************************************************/
				for (j=0; j<p->nValues; j++)
					{
					value[j] = 1.0;
					/* fill in more info about the divisions if this is a true rate multiplier
					   and not a base rate */
					if (p->nSubValues > 0)
						{
						/* num uncompressed chars */
						subValue[j] =  (modelSettings[p->relParts[j]].numUncompressedChars);
						/* Dirichlet parameters */
						subValue[p->nValues + j] = modelParams[p->relParts[j]].ratePrDir;
						}
					}
				}
			else if (p->paramType == P_GENETREERATE)
				{
				/* Fill in division rates *****************************************************************************/
				for (j=0; j<p->nValues; j++)
					{
					value[j] = 1.0;

				    /* Dirichlet parameters fixed to 1.0 for now */
				    subValue[p->nValues + j] = 1.0;

                    /* Get number of uncompressed chars from tree */
                    tree = GetTreeFromIndex(j, 0, 0);
                    subValue[j] = 0.0;
					for (i=0; i<tree->nRelParts; i++)
						{
						/* num uncompressed chars */
						subValue[j] +=  (modelSettings[tree->relParts[i]].numUncompressedChars);
						}
					}
				}
			else if (p->paramType == P_SPECRATE)
				{
				/* Fill in speciation rates *****************************************************************************/
				if (p->paramId == SPECRATE_FIX)
					value[0] = mp->speciationFix;
				else 
					value[0] = 1.0;
				}
			else if (p->paramType == P_EXTRATE)
				{
				/* Fill in extinction rates *****************************************************************************/
				if (p->paramId == EXTRATE_FIX)
					value[0] = mp->extinctionFix;
				else
					value[0] =  0.2;
				}
			else if (p->paramType == P_POPSIZE)
				{
				/* Fill in population size ****************************************************************************************/
				for (j=0; j<p->nValues; j++)
                    {
                    if (p->paramId == POPSIZE_UNI)
    					value[j] = RandomNumber(seed) * (mp->popSizeUni[1] - mp->popSizeUni[0]) + mp->popSizeUni[0];
    				else if (p->paramId == POPSIZE_LOGNORMAL)
                        value[j] =   exp(mp->popSizeLognormal[0]);
    				else if (p->paramId == POPSIZE_NORMAL)
                        value[j] =   mp->popSizeNormal[0];
    				else if (p->paramId == POPSIZE_GAMMA)
                        value[j] =   mp->popSizeGamma[0] / mp->popSizeGamma[1];
				    else if (p->paramId == POPSIZE_FIX)
					    value[j] = mp->popSizeFix;
                    }
				}
			else if (p->paramType == P_AAMODEL)
				{
				/* Fill in theta ****************************************************************************************/
				if (p->paramId == AAMODEL_MIX)
					{
					/* amino acid model ID's
						AAMODEL_POISSON			0
						AAMODEL_JONES			1
						AAMODEL_DAY				2
						AAMODEL_MTREV			3
						AAMODEL_MTMAM			4
						AAMODEL_WAG				5
						AAMODEL_RTREV			6 
						AAMODEL_CPREV           7 
						AAMODEL_VT				8
						AAMODEL_BLOSUM			9 */

					/* set the amino acid model  (the meaning of the numbers is defined) */
					tempInt = (int)(RandomNumber(seed) * 10);
					value[0] = tempInt;
					
					/* we need to make certain that the aa frequencies are filled in correctly */
					bs = GetParamSubVals (m->stateFreq, chn, 0);
					if (tempInt == AAMODEL_POISSON)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] =  (1.0 / 20.0);
						}
					else if (tempInt == AAMODEL_JONES)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = jonesPi[i];
						}
					else if (tempInt == AAMODEL_DAY)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = dayhoffPi[i];
						}
					else if (tempInt == AAMODEL_MTREV)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = mtrev24Pi[i];
						}
					else if (tempInt == AAMODEL_MTMAM)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = mtmamPi[i];
						}
					else if (tempInt == AAMODEL_WAG)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = wagPi[i];
						}
					else if (tempInt == AAMODEL_RTREV)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = rtrevPi[i];
						}
					else if (tempInt == AAMODEL_CPREV)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = cprevPi[i];
						}
					else if (tempInt == AAMODEL_VT)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = vtPi[i];
						}
					else if (tempInt == AAMODEL_BLOSUM)
						{
						for (i=0; i<mp->nStates; i++)
							bs[i] = blosPi[i];
						}
						
					for (i=0; i<p->nSubValues; i++)
						{
						subValue[i] = mp->aaModelPrProbs[i];
						}
					
					}
				}
			else if (p->paramType == P_CPPRATE)
				{
				/* Fill in lambda (cpp rate) ********************************************************************************************/
				if (p->paramId == CPPRATE_EXP)
					value[0] = (-(1.0/mp->cppRateExp) * log(1.0 - RandomNumber(seed)));
				
				else if (p->paramId == CPPRATE_FIX)
					value[0] = mp->cppRateFix;
				}
			else if (p->paramType == P_CPPMULTDEV)
				{
				/* Fill in log standard deviation (for relaxed clock rate multiplier) ***********************************************************/
				if (p->paramId == CPPMULTDEV_FIX)
					value[0] = mp->cppMultDevFix;
				}
			else if (p->paramType == P_CPPEVENTS)
				{
				/* We fill in these when we fill in tree params **************************************************************************/
				}
			else if (p->paramType == P_TK02VAR)
				{
				/* Fill in variance of relaxed clock lognormal **************************************************************************/
				if (p->paramId == TK02VAR_UNI)
					value[0] = RandomNumber(seed) * (mp->tk02varUni[1] - mp->tk02varUni[0]) + mp->tk02varUni[0];
				
				else if (p->paramId == TK02VAR_EXP)
					value[0] =   (-(1.0/mp->tk02varExp) * log(1.0 - RandomNumber(seed)));
				
				else if (p->paramId == TK02VAR_FIX)
					value[0] = mp->tk02varFix;
				}
			else if (p->paramType == P_TK02BRANCHRATES)
				{
				/* We fill in these when we fill in tree params **************************************************************************/
				}
			else if (p->paramType == P_IGRVAR)
				{
				/* Fill in variance of relaxed clock lognormal **************************************************************************/
				if (p->paramId == IGRVAR_UNI)
					value[0] = RandomNumber(seed) * (mp->igrvarUni[1] - mp->igrvarUni[0]) + mp->igrvarUni[0];
				
				else if (p->paramId == IGRVAR_EXP)
					value[0] =   (-(1.0/mp->igrvarExp) * log(1.0 - RandomNumber(seed)));
				
				else if (p->paramId == IGRVAR_FIX)
					value[0] = mp->igrvarFix;
				}
			else if (p->paramType == P_IGRBRANCHLENS)
				{
				/* We fill in these when we fill in tree params **************************************************************************/
				}
			else if (p->paramType == P_CLOCKRATE)
				{
				/* Fill in base rate of molecular clock **************************************************************************/
				if (p->paramId == CLOCKRATE_FIX)
					value[0] = mp->clockRateFix;
                else if (p->paramId == CLOCKRATE_NORMAL)
                    value[0] = mp->clockRateNormal[0];
                else if (p->paramId == CLOCKRATE_LOGNORMAL)
                    value[0] = exp(mp->clockRateLognormal[0]);
                else if (p->paramId == CLOCKRATE_EXP)
                    value[0] = 1.0/(mp->clockRateExp);
                else if (p->paramId == CLOCKRATE_GAMMA)
                    value[0] = mp->clockRateGamma[0]/mp->clockRateGamma[1];
				}
			}	/* next param */
        }	/* next chain */

	return NO_ERROR;
}




	
int FillRelPartsString (Param *p, char **relPartString)

{

	int			i, n, filledString;
	char		*tempStr;
	int             tempStrSize=50;

	tempStr = (char *) SafeMalloc((size_t) (tempStrSize * sizeof(char)));
	if (!tempStr)
		{
		MrBayesPrint ("%s   Problem allocating tempString (%d)\n", spacer, tempStrSize * sizeof(char));
		return (ERROR);
		}

	if (numCurrentDivisions == 1)
		{
		filledString = NO;
		SafeStrcpy (relPartString, "");
		}
	else
		{
		filledString = YES;
		if (p->nRelParts == numCurrentDivisions)
			{
			SafeStrcpy (relPartString, "{all}");
			}
		else
			{
			SafeStrcpy (relPartString, "{");
			for (i=n=0; i<p->nRelParts; i++)
				{
				n++;
				SafeSprintf(&tempStr, &tempStrSize, "%d", p->relParts[i] + 1);
				SafeStrcat (relPartString, tempStr);
				if (n < p->nRelParts)
					SafeStrcat (relPartString, ",");
				}
			SafeStrcat (relPartString, "}");
			}
		}
	free (tempStr);
	return (filledString);

}





/* FillTopologySubParams: Fill subparams (brlens) for a topology */
int FillTopologySubParams (Param *param, int chn, int state, SafeLong *seed)
{
	int		    i,returnVal;
	Tree	    *tree, *tree1;
	Param	    *q;
	MrBFlt      clockRate;
    PolyTree    *sourceTree;
    MrBFlt      minRate,maxRate;

	tree = GetTree (param, chn, state);
	
    for (i=1; i<param->nSubParams; i++)
		{
		q = param->subParams[i];
		tree1 = GetTree (q, chn, state);
		if (CopyToTreeFromTree(tree1, tree) == ERROR)
			return (ERROR);
		}
	for (i=0; i<param->nSubParams; i++)
		{
		q = param->subParams[i];
		tree = GetTree (q, chn, state);
		if (q->paramId == BRLENS_FIXED || q->paramId == BRLENS_CLOCK_FIXED)
			{
			if (param->paramId == TOPOLOGY_NCL_FIXED ||
				param->paramId == TOPOLOGY_NCL_FIXED_HOMO ||
				param->paramId == TOPOLOGY_NCL_FIXED_HETERO ||
				param->paramId == TOPOLOGY_CL_FIXED  ||
				param->paramId == TOPOLOGY_RCL_FIXED ||
				param->paramId == TOPOLOGY_CCL_FIXED ||
				param->paramId == TOPOLOGY_RCCL_FIXED||
				param->paramId == TOPOLOGY_FIXED)
                {
                sourceTree = AllocatePolyTree(numTaxa);
				CopyToPolyTreeFromPolyTree (sourceTree, userTree[modelParams[q->relParts[0]].brlensFix]);
				PrunePolyTree (sourceTree);
                ResetTipIndices (sourceTree);
                ResetIntNodeIndices (sourceTree);
				if (tree->isRooted != sourceTree->isRooted)
					{
					MrBayesPrint("%s   Cannot set fixed branch lengths because of mismatch in rootedness", spacer);
                    FreePolyTree (sourceTree);
                    return (ERROR);
					}
				if (CopyToTreeFromPolyTree(tree,sourceTree) == ERROR)
					{
					MrBayesPrint("%s   Problem setting fixed branch lengths", spacer);
                    FreePolyTree (sourceTree);
					return (ERROR);
					}
                FreePolyTree (sourceTree);
				if (tree->isClock == YES && IsClockSatisfied(tree, 1E-6) == NO)
					{
					MrBayesPrint("%s   Fixed branch lengths do not satisfy clock", spacer);
					return (ERROR);
					}
				if (tree->isCalibrated == YES && IsCalibratedClockSatisfied(tree,&minRate,&maxRate, 1E-6) == NO)
					{
					MrBayesPrint("%s   Fixed branch lengths do not satisfy calibrations", spacer);
					return (ERROR);
					}
                if (tree->isCalibrated == YES && !strcmp(modelParams[tree->relParts[0]].clockRatePr, "Fixed"))
                    {
                    clockRate = modelParams[tree->relParts[0]].clockRateFix;
                    if(( clockRate < minRate && AreDoublesEqual (clockRate, minRate , 0.0001) == NO ) || ( clockRate > maxRate && AreDoublesEqual (clockRate, maxRate , 0.0001) == NO ))
                        {
					    MrBayesPrint("%s   Fixed branch lengths do not satisfy fixed clockrate", spacer);
					    return (ERROR);
					    }
                    }

                tree->fromUserTree=YES;
                returnVal = NO_ERROR;
                }
			else
				{
				MrBayesPrint("%s   Fixed branch lengths can only be used for a fixed topology\n", spacer);
				return (ERROR);
				}
			}
	    else if (tree->isCalibrated == YES)
			{
            assert (tree->isClock == YES);
			clockRate = *GetParamVals(modelSettings[tree->relParts[0]].clockRate, chn, state );
			returnVal = InitCalibratedBrlens (tree, clockRate, seed);
            if ( IsClockSatisfied (tree,0.001) == NO)
				{
				MrBayesPrint ("%s   Branch lengths of the tree does not satisfy clock\n",  spacer);
				return (ERROR);
				}
            tree->fromUserTree=NO;
			}
		else if (tree->isClock == YES)
			returnVal = InitClockBrlens (tree);
		else
			returnVal = InitBrlens (tree, 0.1);

		if( returnVal == ERROR )
			return (ERROR);

		if( FillBrlensSubParams (q, chn, state) == ERROR )
			return (ERROR);
		}

	return (NO_ERROR);
}





/* FillBrlensSubParams: Fill any relaxed clock subparams of a brlens param */
int FillBrlensSubParams (Param *param, int chn, int state)

{
	int			i, j, *nEvents;
	MrBFlt		*brlen, *branchRate, **position, **rateMult;
	Tree		*tree;
	TreeNode	*p;
	Param		*q;

	tree = GetTree (param, chn, state);
	
	for (i=0; i<param->nSubParams; i++)
		{
		q = param->subParams[i];
		if (q->paramType == P_CPPEVENTS)
			{
			nEvents = q->nEvents[2*chn+state];
			position = q->position[2*chn+state];
			rateMult = q->rateMult[2*chn+state];
			brlen = GetParamSubVals (q, chn, state);
			for (j=0; j<tree->nNodes-1; j++)
				{
				p = tree->allDownPass[j];
				if (nEvents[p->index] != 0)
					{
					free (position[p->index]);
					position[p->index] = NULL;
					free (rateMult[p->index]);
					rateMult[p->index] = NULL;
					}
				nEvents[p->index] = 0;
                assert( j==tree->nNodes-2 || fabs(p->length - (p->anc->nodeDepth - p->nodeDepth)) < 0.000001);
				brlen[p->index] = p->length;
				}
			}
		else if (q->paramType == P_TK02BRANCHRATES || q->paramType == P_IGRBRANCHLENS)
			{
			branchRate = GetParamVals (q, chn, state);
			brlen = GetParamSubVals (q, chn, state);
			for (j=0; j<tree->nNodes-1; j++)
				{
				p = tree->allDownPass[j];
                assert( j==tree->nNodes-2 || fabs(p->length - (p->anc->nodeDepth - p->nodeDepth)) < 0.000001);
				branchRate[p->index] = 1.0;
				brlen[p->index] = p->length;
				}
			}
		}

	return (NO_ERROR);
}





/*Note: In PruneConstraintPartitions() we can not relay on specific rootnes of a tree since different partitions may theoreticly have different clock models, whil constraints apply to all partitions/trees */
int PruneConstraintPartitions()
{

    int				i, j, constraintId, nLongsNeeded;
	

    nLongsNeeded = (numLocalTaxa - 1) / nBitsInALong + 1;

    for (constraintId=0; constraintId<numDefinedConstraints; constraintId++)
		{
	    definedConstraintPruned[constraintId] = (SafeLong *) SafeRealloc ((void *)definedConstraintPruned[constraintId], nLongsNeeded*sizeof(SafeLong));
	    if (!definedConstraintPruned[constraintId])
		    {
		    MrBayesPrint ("%s   Problems allocating constraintPartition in PruneConstraintPartitions", spacer);
		    return (ERROR);
		    }

        /* initialize bits in partition to add; get rid of deleted taxa in the process */
        ClearBits(definedConstraintPruned[constraintId], nLongsNeeded);
        for (i=j=0; i<numTaxa; i++)
            {
            if (taxaInfo[i].isDeleted == YES)
                continue;
            if (IsBitSet(i, definedConstraint[constraintId]) == YES)
                SetBit(j, definedConstraintPruned[constraintId]);
            j++;
            }
        assert (j == numLocalTaxa);


        if (definedConstraintsType[constraintId] == PARTIAL )
            {
        	definedConstraintTwoPruned[constraintId] = (SafeLong *) SafeRealloc ((void *)definedConstraintTwoPruned[constraintId], nLongsNeeded*sizeof(SafeLong));
	        if (!definedConstraintTwoPruned[constraintId])
		        {
		        MrBayesPrint ("%s   Problems allocating constraintPartition in PruneConstraintPartitions", spacer);
		        return (ERROR);
		        }

            /* initialize bits in partition to add; get rid of deleted taxa in the process */
            ClearBits(definedConstraintTwoPruned[constraintId], nLongsNeeded);
            for (i=j=0; i<numTaxa; i++)
                {
                if (taxaInfo[i].isDeleted == YES)
                    continue;
                if (IsBitSet(i, definedConstraintTwo[constraintId]) == YES)
                    SetBit(j, definedConstraintTwoPruned[constraintId]);
                j++;
                }
            assert (j == numLocalTaxa);
            }
        else if (definedConstraintsType[constraintId] == NEGATIVE || (definedConstraintsType[constraintId] == HARD) )
            {
            /* Here we create definedConstraintTwoPruned[constraintId] which is complemente of definedConstraintPruned[constraintId] */
            definedConstraintTwoPruned[constraintId] = (SafeLong *) SafeRealloc ((void *)definedConstraintTwoPruned[constraintId], nLongsNeeded*sizeof(SafeLong));
	        if (!definedConstraintTwoPruned[constraintId])
		        {
		        MrBayesPrint ("%s   Problems allocating constraintPartition in PruneConstraintPartitions", spacer);
		        return (ERROR);
		        }

            /* initialize bits in partition to add; get rid of deleted taxa in the process */
            ClearBits(definedConstraintTwoPruned[constraintId], nLongsNeeded);
            for (i=j=0; i<numTaxa; i++)
                {
                if (taxaInfo[i].isDeleted == YES)
                    continue;
                if (IsBitSet(i, definedConstraint[constraintId]) == NO)
                    SetBit(j, definedConstraintTwoPruned[constraintId]);
                j++;
                }
            assert (j == numLocalTaxa);         
            }
    }
    return NO_ERROR;

}





int DoesTreeSatisfyConstraints(Tree *t){

    int         i, k, numTaxa, nLongsNeeded;
    TreeNode    *p;
    int         CheckFirst, CheckSecond; /*Flag indicating wheather coresonding set(first/second) of partial constraint has to be checked*/
#if ! defined (NDEBUG)
    int 	locs_count=0;
#endif

    if( t->checkConstraints == NO)
        return YES;
    /* get some handy numbers */
    numTaxa = t->nNodes - t->nIntNodes - (t->isRooted == YES ? 1 : 0);
    nLongsNeeded = (numTaxa - 1) / nBitsInALong + 1;

    if ( t->bitsets == NULL)
        {
        AllocateTreePartitions(t);
        }
    else
        {
        ResetTreePartitions(t);  /*Inefficient function, rewrite faster version*/
        }
#if ! defined (NDEBUG)
     for (i=0; i<t->nIntNodes; i++)
        {
        p = t->intDownPass[i];
        if(p->isLocked == YES)
        	{
        	if ( IsUnionEqThird (definedConstraintPruned[p->lockID], definedConstraintPruned[p->lockID], p->partition, nLongsNeeded) == NO && IsUnionEqThird (definedConstraintTwoPruned[p->lockID], definedConstraintTwoPruned[p->lockID], p->partition, nLongsNeeded) == NO)
        		{
        		printf ("DEBUG ERROR: Locked node does not represent right partition. \n");
        		return ABORT;
        		}
        	else
        		{
        		locs_count++;
        		}
        	}
        }

    if(locs_count != t->nLocks)
    	{
    	printf("DEBUG ERROR: lock_count:%d should be lock_count:%d\n", locs_count, t->nLocks);
    	return ABORT;
    	}
#endif

    for (k=0; k<numDefinedConstraints; k++)
        {
#if ! defined (NDEBUG)
        if( t->constraints[k] == YES && definedConstraintsType[k] == HARD )
            {
            if( t->isRooted == YES )
                {
                CheckFirst = YES;
                CheckSecond = NO; 
                }
            else
                {
                /*exactly one of next two will be YES*/
                CheckFirst = IsBitSet(localOutGroup, definedConstraintPruned[k])==YES ? NO : YES;
                CheckSecond = IsBitSet(localOutGroup, definedConstraintTwoPruned[k])==YES ? NO : YES;
                assert( (CheckFirst^CheckSecond)==1 );
                }

            for (i=0; i<t->nIntNodes; i++)
	            {
                p = t->intDownPass[i];
                if (p->anc != NULL)
		            {
                    if(CheckFirst==YES &&  IsPartNested(definedConstraintPruned[k], p->partition, nLongsNeeded) && IsPartNested(p->partition,definedConstraintPruned[k], nLongsNeeded) )
                        break;
                    if(CheckSecond==YES &&  IsPartNested(definedConstraintTwoPruned[k], p->partition, nLongsNeeded) && IsPartNested(p->partition, definedConstraintTwoPruned[k], nLongsNeeded) )
                        break;
                    }
	            }

            if( i==t->nIntNodes )
                {
                printf ("DEBUG ERROR: Hard constraint is not satisfied. \n");
                return ABORT;
                //assert(0);
                }
            }
#endif

        if( t->constraints[k] == NO || definedConstraintsType[k] == HARD )
            continue;

        if( definedConstraintsType[k] == PARTIAL )
            {
            /* alternative way
            if (t->isRooted == NO && !IsBitSet(localOutGroup, definedConstraintPruned[k]))
                {
		        m = FirstTaxonInPartition (constraintPartition, nLongsNeeded);
		        for (i=0; t->nodes[i].index != m; i++)
			        ;
		        p = &t->nodes[i];

                p=p->anc;
                while( !IsPartNested(definedConstraintPruned[k], p->partition, nLongsNeeded) )
                    p=p->anc;

                if( IsSectionEmpty(definedConstraintTwoPruned[k], p->partition, nLongsNeeded) )
                    continue;
                }
            */
            if( t->isRooted == YES )
                {
                CheckFirst = YES;
                CheckSecond = NO; /*In rooted case even if we have a node with partition fully containing second set and not containing the first set it would not sutisfy the constraint*/
                }
            else
                {
                if ( NumBits(definedConstraintTwoPruned[k], nLongsNeeded) == 1)
                    continue;
                /*one or two of the next two statments will be YES*/
                CheckFirst = IsBitSet(localOutGroup, definedConstraintPruned[k])==YES ? NO : YES;
                CheckSecond = IsBitSet(localOutGroup, definedConstraintTwoPruned[k])==YES ? NO : YES;
                assert( (CheckFirst|CheckSecond)==1 );
                }
            for (i=0; i<t->nIntNodes; i++)
	            {
                p = t->intDownPass[i];
                if (p->anc != NULL)
		            { 
                    if( CheckFirst==YES && IsPartNested( definedConstraintPruned[k], p->partition, nLongsNeeded) && IsSectionEmpty(definedConstraintTwoPruned[k], p->partition, nLongsNeeded) )
                        break;
                    if( CheckSecond==YES && IsPartNested(definedConstraintTwoPruned[k], p->partition, nLongsNeeded) && IsSectionEmpty(definedConstraintPruned[k], p->partition, nLongsNeeded) )
                        break;
		            }
	            }
            if( i==t->nIntNodes )
                return NO;
            }
        else
            {
            assert(definedConstraintsType[k] == NEGATIVE);
            if( t->isRooted == YES )
                {
                CheckFirst = YES;
                CheckSecond = NO; 
                }
            else
                {
                /*exactly one of next two will be YES*/
                CheckFirst = IsBitSet(localOutGroup, definedConstraintPruned[k])==YES ? NO : YES;
                CheckSecond = IsBitSet(localOutGroup, definedConstraintTwoPruned[k])==YES ? NO : YES;
                assert( (CheckFirst^CheckSecond)==1 );
                }

            for (i=0; i<t->nIntNodes; i++)
	            {
                p = t->intDownPass[i];
                if (p->anc != NULL)
		            {
                    if(CheckFirst==YES &&  !IsPartNested(definedConstraintPruned[k], p->partition, nLongsNeeded) && !IsSectionEmpty(definedConstraintPruned[k], p->partition, nLongsNeeded) )
                        break;
                    if(CheckSecond==YES &&  !IsPartNested(definedConstraintTwoPruned[k], p->partition, nLongsNeeded) && !IsSectionEmpty(definedConstraintTwoPruned[k], p->partition, nLongsNeeded) )
                        break;
                    }
	            }
            if( i==t->nIntNodes )
                return NO;
            }
        }
    return YES;

}




/*------------------------------------------------------------------
|
|	FillTreeParams: Fill in trees and branch lengths
|					Note: should be run after FillNormalParams becouse
|                   clockrate needs to be set if calibrated tree needs
|                   to be filled.
|
------------------------------------------------------------------*/
int FillTreeParams (SafeLong *seed, int fromChain, int toChain)

{

	int			i, k, chn, nTaxa, tmp;
	Param		*p, *q;
	Tree		*tree;
	PolyTree	*constraintTree;
    PolyTree	*constraintTreeRef;

    if( PruneConstraintPartitions() == ERROR )
        return ERROR;

	/* Build starting trees for state 0 */
	for (chn=fromChain; chn<toChain; chn++)
		{
		for (k=0; k<numParams; k++)
			{
			p = &params[k];
			if (p->paramType == P_TOPOLOGY)
				{
                // assert (p->nSubParams == 1); // this assert seems to be wrong -- FR 2010-12-25
				q = p->subParams[0];
				tree = GetTree (q, chn, 0);
				if (tree->isRooted == YES)
					nTaxa = tree->nNodes - tree->nIntNodes - 1;
				else
					nTaxa = tree->nNodes - tree->nIntNodes;
                /* fixed topology */
				if (p->paramId == TOPOLOGY_NCL_FIXED ||
					p->paramId == TOPOLOGY_NCL_FIXED_HOMO ||
					p->paramId == TOPOLOGY_NCL_FIXED_HETERO ||
					p->paramId == TOPOLOGY_CL_FIXED  ||
					p->paramId == TOPOLOGY_RCL_FIXED ||
					p->paramId == TOPOLOGY_CCL_FIXED ||
					p->paramId == TOPOLOGY_RCCL_FIXED||
					p->paramId == TOPOLOGY_FIXED     ||
					p->paramId == TOPOLOGY_PARSIMONY_FIXED
					)
                    {
                    constraintTree = AllocatePolyTree (numTaxa);
					CopyToPolyTreeFromPolyTree (constraintTree, userTree[modelParams[p->relParts[0]].topologyFix]);
					PrunePolyTree (constraintTree);
                    ResetTipIndices(constraintTree);
                    ResetIntNodeIndices(constraintTree);
                    RandResolve (NULL, constraintTree, seed, constraintTree->isRooted);
                    if (tree->nIntNodes != constraintTree->nIntNodes)
						{
                        if(tree->isRooted != constraintTree->isRooted )
                            {
						    MrBayesPrint ("%s   Could not fix topology because user tree '%s' differs in rootedness with the model tree.\n", spacer, userTree[modelParams[p->relParts[0]].topologyFix]->name);
                            MrBayesPrint ("%s   The user tree %s is%srooted, while expected model tree is%srooted.\n", spacer, userTree[modelParams[p->relParts[0]].topologyFix]->name, (constraintTree->isRooted?" ":" not "), (tree->isRooted?" ":" not "));
                            }
                        else
                            MrBayesPrint ("%s   Could not fix topology because user tree '%s' is not fully resolved.\n", spacer, userTree[modelParams[p->relParts[0]].topologyFix]->name);
    					FreePolyTree (constraintTree);
                        return (ERROR);
						}
                    if (CopyToTreeFromPolyTree(tree, constraintTree) == ERROR)
						{
						MrBayesPrint ("%s   Could not fix topology according to user tree '%s'\n", spacer, userTree[modelParams[p->relParts[0]].topologyFix]->name);
    					FreePolyTree (constraintTree);
						return (ERROR);
						}
					FreePolyTree (constraintTree);
                    }
                /* constrained topology */
                else if (tree->nConstraints > 0)
					{
					constraintTreeRef = AllocatePolyTree (nTaxa);
					if (!constraintTreeRef)
						return (ERROR);
					if (BuildConstraintTree (tree, constraintTreeRef, localTaxonNames) == ERROR)
						{
						FreePolyTree (constraintTreeRef);
						return (ERROR);
						}
                    if ( AllocatePolyTreePartitions (constraintTreeRef) == ERROR )
                        return (ERROR);

                    constraintTree = AllocatePolyTree (nTaxa);
					if (!constraintTree)
						return (ERROR);
                    if ( AllocatePolyTreePartitions (constraintTree) == ERROR )
                        return (ERROR);

                    for(i=0;i<100;i++)
                        {
                        CopyToPolyTreeFromPolyTree(constraintTree,constraintTreeRef);
                        tmp = RandResolve (tree, constraintTree, &globalSeed, tree->isRooted);
                        if ( tmp != NO_ERROR )
                            {
					        if (tmp  == ERROR)
						        {
						        FreePolyTree (constraintTree);
						        return (ERROR);
						        }
                            else
                                {   
                                assert (tmp  == ABORT);
                                continue;
                                }
                            }
                   
					    CopyToTreeFromPolyTree(tree, constraintTree);
                        if( DoesTreeSatisfyConstraints(tree)==YES )
                            break;
                        }
#if defined (DEBUG_CONSTRAINTS)
					if (theTree->checkConstraints == YES && CheckConstraints (tree) == ERROR)
						{
						printf ("Error in constraints of starting tree\n");
						getchar();
						}
#endif
					FreePolyTree (constraintTree);
                    FreePolyTree (constraintTreeRef);
                    if(i==100)
                        {
                        MrBayesPrint ("%s   Could not build a starting tree satisfying all constraints\n", spacer); 					
                        return (ERROR);
                        }
					}
				/* random topology */
                else
					{
					if (tree->isRooted == YES)
						{
						if (BuildRandomRTopology (tree, &globalSeed) == ERROR)
							return (ERROR);
						}
					else
						{
						if (BuildRandomUTopology (tree, &globalSeed) == ERROR)
							return (ERROR);
						if (MoveCalculationRoot (tree, localOutGroup) == ERROR)
							return (ERROR);
						}
					}
				if (LabelTree (tree, localTaxonNames) == ERROR)
					return (ERROR);
				if (q == p)
					continue;	/* this is a parsimony tree without branch lengths */
				if (InitializeTreeCalibrations (tree) == ERROR)
					return (ERROR);
				if (FillTopologySubParams(p, chn, 0, seed)== ERROR)
					return (ERROR);
				}
			}
		}

    if (numTopologies > 1 && !strcmp(modelParams[0].topologyPr,"Speciestree"))
        {
        if (FillSpeciesTreeParams(seed, fromChain, toChain) == ERROR)
            return (ERROR);
        }

	return (NO_ERROR);
}





void FreeCppEvents (Param *p)
{
    int i, j;
    
    if (p->paramType != P_CPPEVENTS)
        return;

	if (p->nEvents != NULL)
        {
        free (p->nEvents[0]);
	    free (p->nEvents);
	    for (i=0; i<numGlobalChains; i++)
			{
			for (j=0; j<2*numLocalTaxa; j++)
				{
				free (p->position[2*i][j]);
				free (p->rateMult[2*i][j]);
				free (p->position[2*i+1][j]);
				free (p->rateMult[2*i+1][j]);
				}
			}
		free (p->position[0]);
		free (p->position);
		free (p->rateMult[0]);
		free (p->rateMult);
		p->nEvents = NULL;
		p->position = NULL;
		p->rateMult = NULL;
		}
}





int FreeModel (void)

{

	int		i;
	Param	*p;
	
	if (memAllocs[ALLOC_MODEL] == YES)
		{
		for (i=0; i<numCurrentDivisions; i++)
    		free (modelParams[i].activeConstraints);
        free (modelParams);
		free (modelSettings);
		memAllocs[ALLOC_MODEL] = NO;
		}
	if (memAllocs[ALLOC_MOVES] == YES)
		{
		for (i=0; i<numApplicableMoves; i++)
			FreeMove (moves[i]);
		free (moves);
		moves = NULL;
		numApplicableMoves = 0;
		memAllocs[ALLOC_MOVES] = NO;
		}
	if (memAllocs[ALLOC_COMPMATRIX] == YES)
		{
		free (compMatrix);
		memAllocs[ALLOC_COMPMATRIX] = NO;
		}
	if (memAllocs[ALLOC_NUMSITESOFPAT] == YES)
		{
		free (numSitesOfPat);
		memAllocs[ALLOC_NUMSITESOFPAT] = NO;
		}
	if (memAllocs[ALLOC_COMPCOLPOS] == YES)
		{
		free (compColPos);
		memAllocs[ALLOC_COMPCOLPOS] = NO;
		}
	if (memAllocs[ALLOC_COMPCHARPOS] == YES)
		{
		free (compCharPos);
		memAllocs[ALLOC_COMPCHARPOS] = NO;
		}
	if (memAllocs[ALLOC_ORIGCHAR] == YES)
		{
		free (origChar);
		memAllocs[ALLOC_ORIGCHAR] = NO;
		}
	if (memAllocs[ALLOC_STDTYPE] == YES)
		{
		free (stdType);
		memAllocs[ALLOC_STDTYPE] = NO;
		}
	if (memAllocs[ALLOC_STDSTATEFREQS] == YES)
		{
		free (stdStateFreqs);
		stdStateFreqs = NULL;
		memAllocs[ALLOC_STDSTATEFREQS] = NO;
		}
	if (memAllocs[ALLOC_PARAMVALUES] == YES)
		{
		for (i=0; i<numParams; i++)
			{
			p = &params[i];
			if (p->paramType == P_CPPEVENTS)
				FreeCppEvents(p);
			}
		free (paramValues);
		paramValues = NULL;
        free (intValues);
        intValues = NULL;
        paramValsRowSize = intValsRowSize = 0;
		memAllocs[ALLOC_PARAMVALUES] = NO;
		}
	if (memAllocs[ALLOC_PARAMS] == YES)
		{
		for (i=0; i<numParams; i++)
            {
            SafeFree ((void **)&params[i].name);
            if (params[i].paramHeader)
                {
                free (params[i].paramHeader);
                params[i].paramHeader = NULL;
                }
            }
        free (params);
		free (relevantParts);
		params = NULL;
		relevantParts = NULL;
        numParams = 0;
		memAllocs[ALLOC_PARAMS] = NO;
		}
	if (memAllocs[ALLOC_MCMCTREES] == YES)
		{
		free (mcmcTree);
		free (subParamPtrs);
		mcmcTree = NULL;
		subParamPtrs = NULL;
		memAllocs[ALLOC_MCMCTREES] = NO;
		}
	if (memAllocs[ALLOC_SYMPIINDEX] == YES)
		{
		free (sympiIndex);
		memAllocs[ALLOC_SYMPIINDEX] = NO;
		}
	if (memAllocs[ALLOC_LOCTAXANAMES] == YES)
		{
		free (localTaxonNames);
		memAllocs[ALLOC_LOCTAXANAMES] = NO;
		}
	if (memAllocs[ALLOC_LOCALTAXONCALIBRATION] == YES)
		{
		free (localTaxonCalibration);
		memAllocs[ALLOC_LOCALTAXONCALIBRATION] = NO;
		}

	return (NO_ERROR);

}





void FreeMove (MCMCMove *mv)

{
	free (mv->tuningParam[0]);
	free (mv->tuningParam);
	free (mv->relProposalProb);
	free (mv->nAccepted);
	free (mv->name);
	free (mv);
}





/* Compute empirical state freq are return it in global array empiricalFreqs[] */
int GetEmpiricalFreqs (int *relParts, int nRelParts)

{

	int				i, j, k, m, n, thePartition, nuc[20], ns, temp, isDNA, isProtein, firstRel;
	MrBFlt			freqN[20], sum, sumN[20]/*, rawCounts[20]*/;

	isDNA = isProtein = NO;
	ns = 0;
	firstRel = 0;
	for (i=0; i<nRelParts; i++)
		{
		thePartition = relParts[i];
		if (i == 0)
			{
			if (modelParams[thePartition].dataType == DNA || modelParams[i].dataType == RNA)
				{
				isDNA = YES;
				ns = 4;
				}
			else if (modelParams[thePartition].dataType == PROTEIN)
				{
				isProtein = YES;
				ns = 20;
				}
			else if (modelParams[thePartition].dataType == RESTRICTION)
				{
				ns = 2;
				}
			else
				{
				MrBayesPrint ("%s   Cannot get empirical state frequencies for this datatype (%d)\n", spacer, modelSettings[i].dataType);
				return (ERROR);
				}
			firstRel = thePartition;
			}
		else
			{
			if (modelParams[thePartition].dataType == DNA || modelParams[i].dataType == RNA)
				temp = 4;
			else if (modelParams[thePartition].dataType == PROTEIN)
				temp = 20;
			else if (modelParams[thePartition].dataType == RESTRICTION)
				temp = 2;
			else
				{
				MrBayesPrint ("%s   Unknown data type in GetEmpiricalFreqs\n", spacer);
				return (ERROR);
				}
			if (ns != temp)
				{
				MrBayesPrint ("%s   Averaging state frequencies over partitions with different data types\n", spacer);
				return (ERROR);
				}
			}
		}
	if (ns == 0)
		{
		MrBayesPrint ("%s   Could not find a relevant partition\n", spacer);
		return (ERROR);
		}

	for (i=0; i<200; i++)
		empiricalFreqs[i] = 0.0;
	
	for (m=0; m<ns; m++)
		freqN[m] =  1.0 / ns;
		
	/* for (m=0; m<ns; m++)
	   rawCounts[m] = 0.0; NEVER USED */
		
	for (m=0; m<ns; m++)
		sumN[m] = 0.0;
	for (k=0; k<nRelParts; k++)
		{
		thePartition = relParts[k];
		for (i=0; i<numTaxa; i++)
			{
			if (taxaInfo[i].isDeleted == NO)
				{
				for (j=0; j<numChar; j++)
					{
					if (charInfo[j].isExcluded == NO && partitionId[j][partitionNum] - 1 == thePartition)
						{
						if (isDNA == YES)
							GetPossibleNucs (matrix[pos(i,j,numChar)], nuc);
						else if (isProtein == YES)
							GetPossibleAAs (matrix[pos(i,j,numChar)], nuc);
						else
							GetPossibleRestrictionSites (matrix[pos(i,j,numChar)], nuc);
						sum = 0.0;
						for (m=0; m<ns; m++)
							sum += freqN[m] * nuc[m];
						for (m=0; m<ns; m++)
							sumN[m] += freqN[m] * nuc[m] / sum;
						}
					}
				}
			}
		}
	sum = 0.0;
	for (m=0; m<ns; m++)
		sum += sumN[m];
	for (m=0; m<ns; m++)
		freqN[m] = sumN[m] / sum;

	if (modelParams[firstRel].dataType == DNA || modelParams[firstRel].dataType == RNA)
		{
		if (!strcmp(modelParams[firstRel].nucModel, "4by4"))
			{
			for (m=0; m<ns; m++)
				empiricalFreqs[m] = freqN[m];
			}
		else if (!strcmp(modelParams[firstRel].nucModel, "Doublet"))
			{
			i = 0;
			for (m=0; m<ns; m++)
				for (n=0; n<ns; n++)
					empiricalFreqs[i++] = freqN[m] * freqN[n];
			}
		else
			{
			if (!strcmp(modelParams[firstRel].geneticCode, "Universal"))
				{
				for (i=0; i<61; i++)
					empiricalFreqs[i] = freqN[modelParams[firstRel].codonNucs[i][0]] * freqN[modelParams[firstRel].codonNucs[i][1]] * freqN[modelParams[firstRel].codonNucs[i][2]];
				}
			else if (!strcmp(modelParams[firstRel].geneticCode, "Vertmt"))
				{
				for (i=0; i<60; i++)
					empiricalFreqs[i] = freqN[modelParams[firstRel].codonNucs[i][0]] * freqN[modelParams[firstRel].codonNucs[i][1]] * freqN[modelParams[firstRel].codonNucs[i][2]];
				}
			else if (!strcmp(modelParams[firstRel].geneticCode, "Mycoplasma"))
				{
				for (i=0; i<62; i++)
					empiricalFreqs[i] = freqN[modelParams[firstRel].codonNucs[i][0]] * freqN[modelParams[firstRel].codonNucs[i][1]] * freqN[modelParams[firstRel].codonNucs[i][2]];
				}
			else if (!strcmp(modelParams[firstRel].geneticCode, "Yeast"))
				{
				for (i=0; i<62; i++)
					empiricalFreqs[i] = freqN[modelParams[firstRel].codonNucs[i][0]] * freqN[modelParams[firstRel].codonNucs[i][1]] * freqN[modelParams[firstRel].codonNucs[i][2]];
				}
			else if (!strcmp(modelParams[firstRel].geneticCode, "Ciliates"))
				{
				for (i=0; i<63; i++)
					empiricalFreqs[i] = freqN[modelParams[firstRel].codonNucs[i][0]] * freqN[modelParams[firstRel].codonNucs[i][1]] * freqN[modelParams[firstRel].codonNucs[i][2]];
				}
			else if (!strcmp(modelParams[firstRel].geneticCode, "Metmt"))
				{
				for (i=0; i<62; i++)
					empiricalFreqs[i] = freqN[modelParams[firstRel].codonNucs[i][0]] * freqN[modelParams[firstRel].codonNucs[i][1]] * freqN[modelParams[firstRel].codonNucs[i][2]];
				}
			sum = 0.0;
			for (i=0; i<64; i++)
				sum += empiricalFreqs[i];
			for (i=0; i<64; i++)
				empiricalFreqs[i] /= sum;
			
			}
		}
	else
		{
		for (m=0; m<ns; m++)
			empiricalFreqs[m] = freqN[m];
		}
		
	return (NO_ERROR);

}





int GetNumDivisionChars (void)

{

	int			c, d, n;
	ModelInfo	*m;

	/* count number of characters in each division */
	for (d=0; d<numCurrentDivisions; d++)
		{
		m = &modelSettings[d];
		
		n = 0;
		for (c=0; c<numChar; c++)
			{
			if (charInfo[c].isExcluded == NO && partitionId[c][partitionNum] == d+1)
				n++;
			}
		if (m->dataType == DNA || m->dataType == RNA)
			{
			if (m->nucModelId == NUCMODEL_DOUBLET)
				n *= 2;
			else if (m->nucModelId == NUCMODEL_CODON)
				n *= 3;
			}

		m->numUncompressedChars = n;
		}
	
	return (NO_ERROR);
	
}





int	*GetParamIntVals (Param *parm, int chain, int state)

{

	return parm->intValues + (2 * chain + state) * intValsRowSize;
	
}





MrBFlt	*GetParamStdStateFreqs (Param *parm, int chain, int state)

{
	
	return parm->stdStateFreqs + (2 * chain + state) * stdStateFreqsRowSize;
	
}





MrBFlt	*GetParamSubVals (Param *parm, int chain, int state)

{

	return parm->subValues + (2 * chain + state) * paramValsRowSize;
	
}





MrBFlt	*GetParamVals (Param *parm, int chain, int state)

{
	
	return parm->values + (2 * chain + state) * paramValsRowSize;
	
}





void GetPossibleAAs (int aaCode, int aa[])

{

	int		m;
	
	for (m=0; m<20; m++)
		aa[m] = 0;
		
	if (aaCode > 0 && aaCode <= 20)
		aa[aaCode-1] = 1;
	else
		{
		for (m=0; m<20; m++)
			aa[m] = 1;
		}
#	if 0
	printf ("%2d -- ", aaCode);
	for (m=0; m<20; m++)
		printf("%d", aa[m]);
	printf ("\n");
#	endif

}





void GetPossibleNucs (int nucCode, int nuc[])

{

	if (nucCode == 1)
		{
		nuc[0] = 1;
		nuc[1] = 0;
		nuc[2] = 0;
		nuc[3] = 0;
		}
	else if (nucCode == 2)
		{
		nuc[0] = 0;
		nuc[1] = 1;
		nuc[2] = 0;
		nuc[3] = 0;
		}
	else if (nucCode == 3)
		{
		nuc[0] = 1;
		nuc[1] = 1;
		nuc[2] = 0;
		nuc[3] = 0;
		}
	else if (nucCode == 4)
		{
		nuc[0] = 0;
		nuc[1] = 0;
		nuc[2] = 1;
		nuc[3] = 0;
		}
	else if (nucCode == 5)
		{
		nuc[0] = 1;
		nuc[1] = 0;
		nuc[2] = 1;
		nuc[3] = 0;
		}
	else if (nucCode == 6)
		{
		nuc[0] = 0;
		nuc[1] = 1;
		nuc[2] = 1;
		nuc[3] = 0;
		}
	else if (nucCode == 7)
		{
		nuc[0] = 1;
		nuc[1] = 1;
		nuc[2] = 1;
		nuc[3] = 0;
		}
	else if (nucCode == 8)
		{
		nuc[0] = 0;
		nuc[1] = 0;
		nuc[2] = 0;
		nuc[3] = 1;
		}
	else if (nucCode == 9)
		{
		nuc[0] = 1;
		nuc[1] = 0;
		nuc[2] = 0;
		nuc[3] = 1;
		}
	else if (nucCode == 10)
		{
		nuc[0] = 0;
		nuc[1] = 1;
		nuc[2] = 0;
		nuc[3] = 1;
		}
	else if (nucCode == 11)
		{
		nuc[0] = 1;
		nuc[1] = 1;
		nuc[2] = 0;
		nuc[3] = 1;
		}
	else if (nucCode == 12)
		{
		nuc[0] = 0;
		nuc[1] = 0;
		nuc[2] = 1;
		nuc[3] = 1;
		}
	else if (nucCode == 13)
		{
		nuc[0] = 1;
		nuc[1] = 0;
		nuc[2] = 1;
		nuc[3] = 1;
		}
	else if (nucCode == 14)
		{
		nuc[0] = 0;
		nuc[1] = 1;
		nuc[2] = 1;
		nuc[3] = 1;
		}
	else
		{
		nuc[0] = 1;
		nuc[1] = 1;
		nuc[2] = 1;
		nuc[3] = 1;
		}

}




void GetPossibleRestrictionSites (int resSiteCode, int *sites)

{

	int		m;
	
	for (m=0; m<2; m++)
		sites[m] = 0;
		
	if (resSiteCode == 1)
		sites[0] = 1;
	else if (resSiteCode == 2)
		sites[1] = 1;
	else
		sites[0] = sites[1] = 1;

#	if 0
	printf ("%2d -- ", aaCode);
	for (m=0; m<20; m++)
		printf("%d", aa[m]);
	printf ("\n");
#	endif

}




Tree *GetTree (Param *parm, int chain, int state)

{

	return mcmcTree[parm->treeIndex + ((2 * chain + state) * numTrees)];
	
}





Tree *GetTreeFromIndex (int index, int chain, int state)

{

	return mcmcTree[index + ((2 * chain + state) * numTrees)];
	
}





/*-----------------------------------------------------------
|
|   GetUserTreeFromName: Do case-insensitive search for user
|      tree, return index if match, -1 and ERROR if error
|
------------------------------------------------------------*/
int GetUserTreeFromName (int *index, char *treeName)

{
    int     i, j, k, nMatches;
    char    localName[100], temp[100];

    (*index) = -1;  /* appropriate return if no match */

    if ((int)strlen(treeName) > 99)
		{
		MrBayesPrint ("%s   Too many characters in tree name\n", spacer);
		return (ERROR);
		}

    strcpy (localName, treeName);
	for (i=0; i<(int)strlen(localName); i++)
		localName[i] = tolower(localName[i]);

    nMatches = j = 0;
	for (i=0; i<numUserTrees; i++)
		{
		strcpy (temp, userTree[i]->name);
		for (k=0; k<(int)strlen(temp); k++)
			temp[k] = tolower(temp[k]);
		if (strcmp(localName,temp) == 0)
			{
			j = i;
			nMatches++;
			}
		}
	if (nMatches==0)
        {
        for (i=0; i<numUserTrees; i++)
		    {
	        strcpy (temp, userTree[i]->name);
	        for (k=0; k<(int)strlen(temp); k++)
		        temp[k] = tolower(temp[k]);
			if (strncmp(localName,temp,strlen(localName)) == 0)
			    {
			    j = i;
			    nMatches++;
			    }
		    }
        }
	if (nMatches == 0)
		{
		MrBayesPrint ("%s   Could not find tree '%s'\n", spacer, localName);  
		return (ERROR);
		}
	else if (nMatches > 1)
		{
		MrBayesPrint ("%s   Several trees matched the abbreviated name '%s'\n", spacer, localName);
		return (ERROR);
		}
	else
        {
		(*index) = j;
        return (NO_ERROR);
        }
}





int InitializeLinks (void)

{

	int			i, j;
	
	linkNum = 0;
	for (i=0; i<NUM_LINKED; i++)
		{
		for (j=0; j<numCurrentDivisions; j++)
			linkTable[i][j] = linkNum;
		}

	return (NO_ERROR);
	
}





/* InitializeTreeCalibrations: Set calibrations for tree nodes */
int InitializeTreeCalibrations (Tree *t)

{
	int			i;
	TreeNode	*p;
	
	if (t->isCalibrated == NO)
		return (NO_ERROR);
	
	/* Set tip calibrations */
    for (i=0; i<t->nNodes; i++)
		{
		p = t->allDownPass[i];
		if (p->left == NULL && p->right == NULL && localTaxonCalibration[p->index]->prior != unconstrained)
			{
			p->isDated = YES;
			p->calibration = localTaxonCalibration[p->index];
			if (p->calibration->prior == fixed)
				p->age = p->calibration->age;
			else if (p->calibration->prior == uniform)
				p->age = p->calibration->min;
			else
				{
				assert(p->calibration->prior == offsetExponential);
				p->age = p->calibration->offset;
				}
			}
        else if (p->left == NULL && p->right == NULL)
            {
            p->isDated = NO;
            p->calibration = NULL;
            p->age = -1.0;
            }            
		}

    /* Initialize interior calibrations */
    if (CheckSetConstraints(t) == ERROR)
        return (ERROR);

	return (NO_ERROR);
}





int IsApplicable (Param *param)
{
    if (param == NULL)
        return NO;

    return YES;
}





int IsApplicableTreeAgeMove (Param *param)
{
    Tree        *t;
    TreeNode    *p;

    if (param == NULL)
        return NO;

    if (param->paramType != P_BRLENS)
        return NO;
    
    t = GetTree (param, 0, 0);

    p = t->root->left;
    if (p->isDated == NO)
        return NO;
    if (p->calibration->prior == fixed)
        return NO;
    else
        return YES;
}




int IsModelSame (int whichParam, int part1, int part2, int *isApplic1, int *isApplic2)

{

	int			i, isSame, isFirstNucleotide, isSecondNucleotide, isFirstProtein, isSecondProtein, nDiff, temp1, temp2;

	isSame = YES;
	*isApplic1 = YES;
	*isApplic2 = YES;
	isFirstNucleotide = isSecondNucleotide = NO;
	if (modelSettings[part1].dataType == DNA || modelSettings[part1].dataType == RNA)
		isFirstNucleotide = YES;
	if (modelSettings[part2].dataType == DNA || modelSettings[part2].dataType == RNA)
		isSecondNucleotide = YES;		
	isFirstProtein = isSecondProtein = NO;
	if (modelSettings[part1].dataType == PROTEIN)
		isFirstProtein = YES;
	if (modelSettings[part2].dataType == PROTEIN)
		isSecondProtein = YES;		
	
	if (whichParam == P_TRATIO)
		{
		/* Check the ti/tv rate ratio for partitions 1 and 2. */

		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and tratio does not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and tratio does not apply */
		
		/* Check that the data are nucleotide for both partitions 1 and 2 */
		if (isFirstNucleotide == NO)
			*isApplic1 = NO; /* part1 is not nucleotide data so tratio does not apply */
		if (isSecondNucleotide == NO)
			*isApplic2 = NO; /* part2 is not nucleotide data so tratio does not apply */
		
		/* check that nst=2 for both partitions */
		if (strcmp(modelParams[part1].nst, "2"))
			*isApplic1 = NO; /* part1 does not have nst=2 and tratio does not apply */
		if (strcmp(modelParams[part2].nst, "2"))
			*isApplic2 = NO; /* part2 does not have nst=2 and tratio does not apply */
		
		/* Check if part1 & part2 are restriction */
		if (modelParams[part1].dataType == RESTRICTION)
			*isApplic1 = NO;
		if (modelParams[part2].dataType == RESTRICTION)
			*isApplic2 = NO;

		/* If Nst = 2 for both part1 and part2, we now need to check if the prior is the same for both. */
		if (!strcmp(modelParams[part1].tRatioPr,"Beta") && !strcmp(modelParams[part2].tRatioPr,"Beta"))
			{
			if (AreDoublesEqual (modelParams[part1].tRatioDir[0], modelParams[part2].tRatioDir[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].tRatioDir[1], modelParams[part2].tRatioDir[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].tRatioPr,"Fixed") && !strcmp(modelParams[part2].tRatioPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].tRatioFix, modelParams[part2].tRatioFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */

		/* Check to see if tratio is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if tratio is inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_REVMAT)
		{
		/* Check the GTR rates for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and GTR rates do not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and GTR rates do not apply */

		/* Check that the data are nucleotide or protein for both partitions 1 and 2 */
		if (isFirstNucleotide == NO && isFirstProtein == NO)
			*isApplic1 = NO; /* part1 is not nucleotide or protein data so GTR rates do not apply */
		if (isSecondNucleotide == NO && isSecondProtein == NO)
			*isApplic2 = NO; /* part2 is not nucleotide or protein data so GTR rates do not apply */

		/* check that nst=6 or mixed for both partitions if nucleotide */
		if (isFirstNucleotide == YES && strcmp(modelParams[part1].nst, "6") && strcmp(modelParams[part1].nst, "Mixed"))
			*isApplic1 = NO; /* part1 does not have nst=6/Mixed and GTR rates do not apply */
		if (isSecondNucleotide == YES && strcmp(modelParams[part2].nst, "6") && strcmp(modelParams[part2].nst, "Mixed"))
			*isApplic2 = NO; /* part2 does not have nst=6/Mixed and GTR rates do not apply */
			
		/* check that model is GTR for both partitions if protein */
		if (isFirstProtein == YES && (strcmp(modelParams[part1].aaModel,"Gtr")!=0 || strcmp(modelParams[part1].aaModelPr,"Fixed")!=0))
			*isApplic1 = NO;
		if (isSecondProtein == YES && (strcmp(modelParams[part2].aaModel,"Gtr")!=0 || strcmp(modelParams[part2].aaModelPr,"Fixed")!=0))
			*isApplic2 = NO;

		/* check that data type is the same for both partitions */
		if (isFirstNucleotide == YES && isSecondNucleotide == NO)
			isSame = NO;
		if (isFirstProtein == YES && isSecondProtein == NO)
			isSame = NO;

		/* GTR applies to both part1 and part2. We now need to check if the prior is the same for both. */
		if (isFirstNucleotide == YES)
			{
			if (strcmp(modelParams[part1].nst, modelParams[part2].nst) != 0)
                isSame = NO;
			if (!strcmp(modelParams[part1].nst,"Mixed"))
				{
				if (AreDoublesEqual (modelParams[part1].revMatSymDir, modelParams[part2].revMatSymDir, (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
            else if (!strcmp(modelParams[part1].nst,"6") && !strcmp(modelParams[part1].revMatPr,"Dirichlet") && !strcmp(modelParams[part2].revMatPr,"Dirichlet"))
				{
				for (i=0; i<6; i++)
					{
					if (AreDoublesEqual (modelParams[part1].revMatDir[i], modelParams[part2].revMatDir[i], (MrBFlt) 0.00001) == NO)
						isSame = NO;
					}
				}
			else if (!strcmp(modelParams[part1].nst,"6") && !strcmp(modelParams[part1].revMatPr,"Fixed") && !strcmp(modelParams[part2].revMatPr,"Fixed"))
				{
				for (i=0; i<6; i++)
					{
					if (AreDoublesEqual (modelParams[part1].revMatFix[i], modelParams[part2].revMatFix[i], (MrBFlt) 0.00001) == NO)
						isSame = NO;
					}
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			}
		else /* if (isFirstProtein == YES) */
			{
			if (!strcmp(modelParams[part1].aaRevMatPr,"Dirichlet") && !strcmp(modelParams[part2].aaRevMatPr,"Dirichlet"))
				{
				for (i=0; i<190; i++)
					{
					if (AreDoublesEqual (modelParams[part1].aaRevMatDir[i], modelParams[part2].aaRevMatDir[i], (MrBFlt) 0.00001) == NO)
						isSame = NO;
					}
				}
			else if (!strcmp(modelParams[part1].aaRevMatPr,"Fixed") && !strcmp(modelParams[part2].aaRevMatPr,"Fixed"))
				{
				for (i=0; i<190; i++)
					{
					if (AreDoublesEqual (modelParams[part1].aaRevMatFix[i], modelParams[part2].aaRevMatFix[i], (MrBFlt) 0.00001) == NO)
						isSame = NO;
					}
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			}

		/* Check to see if the GTR rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if GTR rates are inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_OMEGA)
		{
		/* Check the nonsynonymous/synonymous rate ratio for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and nonsynonymous/synonymous rate ratio does not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and nonsynonymous/synonymous rate ratio does not apply */
		
		/* Check that the data are nucleotide for both partitions 1 and 2 */
		if (isFirstNucleotide == NO)
			*isApplic1 = NO; /* part1 is not nucleotide data so a nonsynonymous/synonymous rate ratio does not apply */
		if (isSecondNucleotide == NO)
			*isApplic2 = NO; /* part2 is not nucleotide data so a nonsynonymous/synonymous rate ratio does not apply */
		
		/* Check that the model structure is the same for both. The nucmodel should be "codon". */
		if (strcmp(modelParams[part1].nucModel, "Codon"))
			*isApplic1 = NO; /* part1 does not have Nucmodel = Codon and nonsynonymous/synonymous rate ratio does not apply */
		if (strcmp(modelParams[part2].nucModel, "Codon"))
			*isApplic2 = NO; /* part2 does not have Nucmodel = Codon and nonsynonymous/synonymous rate ratio does not apply */
		
		/* Assuming that Nucmodel = Codon for both part1 and part2, we now need to check if the prior is the
		   same for both. */
		if (!strcmp(modelParams[part1].omegaVar, "M3") && !strcmp(modelParams[part2].omegaVar, "M3"))
			{
			if (!strcmp(modelParams[part1].m3omegapr, "Exponential") && !strcmp(modelParams[part2].m3omegapr, "Exponential"))
				{
				}
			else if (!strcmp(modelParams[part1].m3omegapr, "Fixed") && !strcmp(modelParams[part1].m3omegapr, "Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].m3omegaFixed[0], modelParams[part2].m3omegaFixed[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].m3omegaFixed[1], modelParams[part2].m3omegaFixed[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */

			if (!strcmp(modelParams[part1].codonCatFreqPr, "Dirichlet") && !strcmp(modelParams[part2].codonCatFreqPr, "Dirichlet"))
				{
				if (AreDoublesEqual (modelParams[part1].codonCatDir[0], modelParams[part2].codonCatDir[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatDir[1], modelParams[part2].codonCatDir[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatDir[2], modelParams[part2].codonCatDir[2], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else if (!strcmp(modelParams[part1].codonCatFreqPr, "Fixed") && !strcmp(modelParams[part1].codonCatFreqPr, "Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[0], modelParams[part2].codonCatFreqFix[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[1], modelParams[part2].codonCatFreqFix[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[2], modelParams[part2].codonCatFreqFix[2], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			}
		else if (!strcmp(modelParams[part1].omegaVar, "M10") && !strcmp(modelParams[part2].omegaVar, "M10"))
			{			
			if (!strcmp(modelParams[part1].codonCatFreqPr, "Dirichlet") && !strcmp(modelParams[part2].codonCatFreqPr, "Dirichlet"))
				{
				if (AreDoublesEqual (modelParams[part1].codonCatDir[0], modelParams[part2].codonCatDir[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatDir[1], modelParams[part2].codonCatDir[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else if (!strcmp(modelParams[part1].codonCatFreqPr, "Fixed") && !strcmp(modelParams[part1].codonCatFreqPr, "Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[0], modelParams[part2].codonCatFreqFix[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[1], modelParams[part2].codonCatFreqFix[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			}
		else if (!strcmp(modelParams[part1].omegaVar, "Ny98") && !strcmp(modelParams[part2].omegaVar, "Ny98"))
			{
			if (!strcmp(modelParams[part1].ny98omega1pr, "Beta") && !strcmp(modelParams[part2].ny98omega1pr, "Beta"))
				{
				if (AreDoublesEqual (modelParams[part1].ny98omega1Beta[0], modelParams[part2].ny98omega1Beta[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].ny98omega1Beta[1], modelParams[part2].ny98omega1Beta[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else if (!strcmp(modelParams[part1].ny98omega1pr, "Fixed") && !strcmp(modelParams[part1].ny98omega1pr, "Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].ny98omega1Fixed, modelParams[part2].ny98omega1Fixed, (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			if (!strcmp(modelParams[part1].ny98omega3pr, "Uniform") && !strcmp(modelParams[part2].ny98omega3pr, "Uniform"))
				{
				if (AreDoublesEqual (modelParams[part1].ny98omega3Uni[0], modelParams[part2].ny98omega3Uni[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].ny98omega3Uni[1], modelParams[part2].ny98omega3Uni[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else if (!strcmp(modelParams[part1].ny98omega3pr, "Exponential") && !strcmp(modelParams[part1].ny98omega3pr, "Exponential"))
				{
				if (AreDoublesEqual (modelParams[part1].ny98omega3Exp, modelParams[part2].ny98omega3Exp, (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else if (!strcmp(modelParams[part1].ny98omega3pr, "Fixed") && !strcmp(modelParams[part1].ny98omega3pr, "Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].ny98omega3Fixed, modelParams[part2].ny98omega3Fixed, (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			if (!strcmp(modelParams[part1].codonCatFreqPr, "Dirichlet") && !strcmp(modelParams[part2].codonCatFreqPr, "Dirichlet"))
				{
				if (AreDoublesEqual (modelParams[part1].codonCatDir[0], modelParams[part2].codonCatDir[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatDir[1], modelParams[part2].codonCatDir[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatDir[2], modelParams[part2].codonCatDir[2], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else if (!strcmp(modelParams[part1].codonCatFreqPr, "Fixed") && !strcmp(modelParams[part1].codonCatFreqPr, "Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[0], modelParams[part2].codonCatFreqFix[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[1], modelParams[part2].codonCatFreqFix[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].codonCatFreqFix[2], modelParams[part2].codonCatFreqFix[2], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			}
		else if (!strcmp(modelParams[part1].omegaVar, "Equal") && !strcmp(modelParams[part2].omegaVar, "Equal"))
			{
			if (!strcmp(modelParams[part1].omegaPr,"Dirichlet") && !strcmp(modelParams[part2].omegaPr,"Dirichlet"))
				{
				if (AreDoublesEqual (modelParams[part1].omegaDir[0], modelParams[part2].omegaDir[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].omegaDir[1], modelParams[part2].omegaDir[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else if (!strcmp(modelParams[part1].omegaPr,"Fixed") && !strcmp(modelParams[part2].omegaPr,"Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].omegaFix, modelParams[part2].omegaFix, (MrBFlt) 0.00001) == NO)
					isSame = NO;
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
		
		/* Check to see if omega is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if omega is inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_PI)
		{
		/* Check the state frequencies for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and state frequencies do not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and state frequencies do not apply */

		/* Check that the data are not CONTINUOUS for partitions 1 and 2 */
		if (modelParams[part1].dataType == CONTINUOUS)
			*isApplic1 = NO; /* state frequencies do not make sense for part1 */
		if (modelParams[part2].dataType == CONTINUOUS)
			*isApplic2 = NO; /* state frequencies do not make sense for part2 */
			
		/* Now, check that the data are the same (i.e., both nucleotide or both amino acid, or whatever). */
		if (isFirstNucleotide != isSecondNucleotide)
			isSame = NO; /* data are not both nucleotide or both note nucleotide */
		else if (modelParams[part1].dataType != modelParams[part2].dataType && isFirstNucleotide == NO)
			isSame = NO; /* data are not the same */

		/* Check that the model structure is the same for both partitions */
		if (strcmp(modelParams[part1].nucModel, modelParams[part2].nucModel))
			isSame = NO; /* the nucleotide models are different */
		if (strcmp(modelParams[part1].covarionModel, modelParams[part2].covarionModel) && !(!strcmp(modelParams[part1].nucModel, "Codon") && !strcmp(modelParams[part2].nucModel, "Codon")))
			isSame = NO; /* the models have different covarion struture */
			
		/* If both partitions have nucmodel=codon, then we also have to make certain that the same genetic code is used. */
		if (!strcmp(modelParams[part1].nucModel, "Codon") && !strcmp(modelParams[part2].nucModel, "Codon"))
			{
			if (strcmp(modelParams[part1].geneticCode,modelParams[part2].geneticCode))
				isSame = NO; /* the models have different genetic codes */
			}
		
		/* Let's see if the prior is the same. */
		if (modelParams[part1].dataType == STANDARD && modelParams[part2].dataType == STANDARD)
			{
			/* The data are morphological (STANDARD). The state frequencies are specified by a
			   symmetric beta distribution, the parameter of which needs to be the same to apply to both
			   partitions. Note that symPiPr = -1 is equivalent to setting the variance to 0.0. */
			if (!strcmp(modelParams[part1].symPiPr,"Uniform") && !strcmp(modelParams[part2].symPiPr,"Uniform"))
				{
				if (AreDoublesEqual (modelParams[part1].symBetaUni[0], modelParams[part2].symBetaUni[0], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].symBetaUni[1], modelParams[part2].symBetaUni[1], (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (modelParams[part1].numBetaCats != modelParams[part2].numBetaCats)
					isSame = NO;	/* can't link because the discrete beta approximation is different */
				}
			else if (!strcmp(modelParams[part1].symPiPr,"Exponential") && !strcmp(modelParams[part2].symPiPr,"Exponential"))
				{
				if (AreDoublesEqual (modelParams[part1].symBetaExp, modelParams[part2].symBetaExp, (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (modelParams[part1].numBetaCats != modelParams[part2].numBetaCats)
					isSame = NO;	/* can't link because the discrete beta approximation is different */
				}
			else if (!strcmp(modelParams[part1].symPiPr,"Fixed") && !strcmp(modelParams[part2].symPiPr,"Fixed"))
				{
				if (AreDoublesEqual (modelParams[part1].symBetaFix, modelParams[part2].symBetaFix, (MrBFlt) 0.00001) == NO)
					isSame = NO;
				if (AreDoublesEqual (modelParams[part1].symBetaFix, (MrBFlt) -1.0, (MrBFlt) 0.00001) == NO && modelParams[part1].numBetaCats != modelParams[part2].numBetaCats)
					isSame = NO;	/* can't link because the discrete beta approximation is different */
				}
			else
				isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
			}
		if (modelSettings[part1].dataType == PROTEIN && modelSettings[part2].dataType == PROTEIN)
			{
			/* We are dealing with protein data. */
			if (!strcmp(modelParams[part1].aaModelPr, modelParams[part2].aaModelPr))
				{
				if (!strcmp(modelParams[part1].aaModelPr, "Fixed"))
					{
					/* only have a single, fixed, amino acid rate matrix */
					if (!strcmp(modelParams[part1].aaModel, modelParams[part2].aaModel))
						{}
					else
						isSame = NO; /* we have different amino acid models, and the state frequencies must be different */
					/* if we have an equalin model or Gtr model, then we need to check the prior on the state frequencies */
					if (!strcmp(modelParams[part1].aaModel, "Equalin") || !strcmp(modelParams[part1].aaModel, "Gtr"))
						{
						if (!strcmp(modelParams[part1].stateFreqPr, modelParams[part2].stateFreqPr))
							{
							/* the prior form is the same */
							if (!strcmp(modelParams[part1].stateFreqPr, "Dirichlet")) /* both prior models must be dirichlet */
								{
								for (i=0; i<modelParams[part1].nStates; i++)
									if (AreDoublesEqual (modelParams[part1].stateFreqsDir[i], modelParams[part2].stateFreqsDir[i], (MrBFlt) 0.00001) == NO)
										isSame = NO; /* the dirichlet parameters are different */
								}
							else /* both prior models must be fixed */
								{
								if (!strcmp(modelParams[part1].stateFreqsFixType, modelParams[part2].stateFreqsFixType))
									{
									/* if (!strcmp(modelParams[part1].stateFreqsFixType, "Empirical"))
										isSame = NO;     Even though it is unlikely that the empirical values for both partitions are exactly the same, we will
										                 allow isSame to equal YES. This means pooled base frequencies are used to determine the empirical
										                 base frequencies. The user can still unlink this parameter. */
									if (!strcmp(modelParams[part1].stateFreqsFixType, "User"))
										{
										for (i=0; i<modelParams[part1].nStates; i++)
											if (AreDoublesEqual (modelParams[part1].stateFreqsDir[i], modelParams[part2].stateFreqsDir[i], (MrBFlt) 0.00001) == NO)
												isSame = NO; /* the user-specified base frequencies are different */
										}
									/* if the frequencies were both fixed to "equal", we are golden, and they are the same */
									}
								else
									isSame = NO; /* the fixed parameters must be the same. The only other possibility is that the
									                user specified equal or empirical for one partition and then specified specific
									                numbers (user) for the other _and_ happened to set the user values to the equal
									                or empirical values. We ignore this possibility. */
								}
							}
						}
					}
				else
					{
					/* averaging over models */
					if (linkTable[P_AAMODEL][part1] != linkTable[P_AAMODEL][part2])
						isSame = NO; /* the amino acid model is mixed, but independently estimated */
					}
				}
			}
		else
			{
			/* Otherwise, we are dealing with RESTRICTION or NUCLEOTIDE data. The dirichlet should be the same
			   for both partitions. */
			if (!strcmp(modelParams[part1].stateFreqPr, modelParams[part2].stateFreqPr))
				{
				/* the prior form is the same */
				if (!strcmp(modelParams[part1].stateFreqPr, "Dirichlet")) /* both prior models must be dirichlet */
					{
					for (i=0; i<modelParams[part1].nStates; i++)
						if (AreDoublesEqual (modelParams[part1].stateFreqsDir[i], modelParams[part2].stateFreqsDir[i], (MrBFlt) 0.00001) == NO)
							isSame = NO; /* the dirichlet parameters are different */
					}
				else /* both prior models must be fixed */
					{
					if (!strcmp(modelParams[part1].stateFreqsFixType, modelParams[part2].stateFreqsFixType))
						{
						/* if (!strcmp(modelParams[part1].stateFreqsFixType, "Empirical"))
							isSame = NO;     Even though it is unlikely that the empirical values for both partitions are exactly the same, we will
							                 allow isSame to equal YES. This means pooled base frequencies are used to determine the empirical
							                 base frequencies. The user can still unlink this parameter. */
						if (!strcmp(modelParams[part1].stateFreqsFixType, "User"))
							{
							for (i=0; i<modelParams[part1].nStates; i++)
								if (AreDoublesEqual (modelParams[part1].stateFreqsDir[i], modelParams[part2].stateFreqsDir[i], (MrBFlt) 0.00001) == NO)
									isSame = NO; /* the user-specified base frequencies are different */
							}
						/* if the frequencies were both fixed to "equal", we are golden, and they are the same */
						}
					else
						isSame = NO; /* the fixed parameters must be the same. The only other possibility is that the
						                user specified equal or empirical for one partition and then specified specific
						                numbers (user) for the other _and_ happened to set the user values to the equal
						                or empirical values. We ignore this possibility. */
					}
				}
			}

		/* Check to see if the state frequencies are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if the state frequencies are inapplicable for either partition, then the parameter cannot be the same */
		
		}
	else if (whichParam == P_SHAPE)
		{
		/* Check the gamma shape parameter for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and gamma shape parameter does not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and gamma shape parameter does not apply */

		/* Check that the data are not CONTINUOUS for partitions 1 and 2 */
		if (modelParams[part1].dataType == CONTINUOUS)
			*isApplic1 = NO; /* the gamma shape parameter does not make sense for part1 */
		if (modelParams[part2].dataType == CONTINUOUS)
			*isApplic2 = NO; /* the gamma shape parameter does not make sense for part2 */

		/* Now, check that the data are the same (i.e., both nucleotide or both amino acid, or whatever). */
		if (isFirstNucleotide != isSecondNucleotide)
			isSame = NO; /* data are not both nucleotide */
		else if ( modelParams[part1].dataType != modelParams[part2].dataType && isFirstNucleotide == NO)
			isSame = NO; /* data are not the same */

		/* Let's check that the gamma shape parameter is even relevant for the two partitions */
		if (!strcmp(modelParams[part1].ratesModel, "Equal") || !strcmp(modelParams[part1].ratesModel, "Propinv"))
			*isApplic1 = NO; /* the gamma shape parameter does not make sense for part1 */
		if (!strcmp(modelParams[part2].ratesModel, "Equal") || !strcmp(modelParams[part2].ratesModel, "Propinv"))
			*isApplic2 = NO; /* the gamma shape parameter does not make sense for part2 */
		
		/* We may have a nucleotide model. Make certain the models are not of type codon. */
		if (!strcmp(modelParams[part1].nucModel, "Codon"))
			*isApplic1 = NO; /* we have a codon model for part1, and a gamma shape parameter does not apply */
		if (!strcmp(modelParams[part2].nucModel, "Codon"))
			*isApplic2 = NO;/* we have a codon model for part2, and a gamma shape parameter does not apply */

		/* Check that the model structure is the same for both partitions */
		if ((!strcmp(modelParams[part1].nucModel, "4by4") || !strcmp(modelParams[part1].nucModel, "Doublet")) && !strcmp(modelParams[part2].nucModel, "Codon"))
			isSame = NO; /* the nucleotide models are incompatible with the same shape parameter */
		if ((!strcmp(modelParams[part2].nucModel, "4by4") || !strcmp(modelParams[part2].nucModel, "Doublet")) && !strcmp(modelParams[part1].nucModel, "Codon"))
			isSame = NO; /* the nucleotide models are incompatible with the same shape parameter */
		/*if (strcmp(modelParams[part1].covarionModel, modelParams[part2].covarionModel))
			isSame = NO;*/ /* the models have different covarion struture */  /* NOTE: Perhaps we should allow the possiblity that
			                                                                         the gamma shape parameter is the same for the
			                                                                         case where one partition has a covarion model
			                                                                         but the other does not and both datatypes are
			                                                                         the same. */
		
		/* Check that the number of rate categories is the same */
		if (modelParams[part1].numGammaCats != modelParams[part2].numGammaCats)
			isSame = NO; /* the number of rate categories is not the same, so we cannot set the parameter to be equal for both partitions */

		/* Check that the priors are the same. */
		if (!strcmp(modelParams[part1].shapePr,"Uniform") && !strcmp(modelParams[part2].shapePr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].shapeUni[0], modelParams[part2].shapeUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].shapeUni[1], modelParams[part2].shapeUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].shapePr,"Exponential") && !strcmp(modelParams[part2].shapePr,"Exponential"))
			{
			if (AreDoublesEqual (modelParams[part1].shapeExp, modelParams[part2].shapeExp, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].shapePr,"Fixed") && !strcmp(modelParams[part2].shapePr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].shapeFix, modelParams[part2].shapeFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
		
		/* Check to see if the shape parameter is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if the shape parameter is inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_PINVAR)
		{
		/* Check the proportion of invariable sites parameter for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and proportion of invariable sites parameter does not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and proportion of invariable sites parameter does not apply */

		/* Check that the data are not CONTINUOUS for partitions 1 and 2 */
		if (modelParams[part1].dataType == CONTINUOUS)
			*isApplic1 = NO; /* the proportion of invariable sites parameter does not make sense for part1 */
		if (modelParams[part2].dataType == CONTINUOUS)
			*isApplic2 = NO; /* the proportion of invariable sites parameter does not make sense for part2 */

		/* Now, check that the data are the same (i.e., both nucleotide or both amino acid, or whatever). */
		if (isFirstNucleotide != isSecondNucleotide)
			isSame = NO; /* data are not both nucleotide */
		else if ( modelParams[part1].dataType != modelParams[part2].dataType && isFirstNucleotide == NO)
			isSame = NO; /* data are not the same */

		/* Let's check that proportion of invariable sites parameter is even relevant for the two partitions */
		if (!strcmp(modelParams[part1].ratesModel, "Equal") || !strcmp(modelParams[part1].ratesModel, "Gamma") || !strcmp(modelParams[part1].ratesModel, "Adgamma"))
			*isApplic1 = NO; /* the proportion of invariable sites parameter does not make sense for part1 */
		if (!strcmp(modelParams[part2].ratesModel, "Equal") || !strcmp(modelParams[part2].ratesModel, "Gamma") || !strcmp(modelParams[part2].ratesModel, "Adgamma"))
			*isApplic2 = NO; /* the proportion of invariable sites parameter does not make sense for part2 */
			
		/* It is not sensible to have a covarion model and a proportion of invariable sites */
		if (!strcmp(modelParams[part1].covarionModel, "Yes"))
			*isApplic1 = NO;
		if (!strcmp(modelParams[part2].covarionModel, "Yes"))
			*isApplic2 = NO;
		
		/* We have a nucleotide model. Make certain the models are not of type codon. */
		if (!strcmp(modelParams[part1].nucModel, "Codon"))
			*isApplic1 = NO; /* we have a codon model for part1, and a proportion of invariable sites parameter does not apply */
		if (!strcmp(modelParams[part2].nucModel, "Codon"))
			*isApplic2 = NO;/* we have a codon model for part2, and a proportion of invariable sites parameter does not apply */

		/* Check that the model structure is the same for both partitions */
		if (strcmp(modelParams[part1].nucModel, modelParams[part2].nucModel))
			isSame = NO; /* the nucleotide models are different */
		if (strcmp(modelParams[part1].covarionModel, modelParams[part2].covarionModel))
			isSame = NO; /* the models have different covarion struture */
		
		/* check the priors */
		if (!strcmp(modelParams[part1].pInvarPr,"Uniform") && !strcmp(modelParams[part2].pInvarPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].pInvarUni[0], modelParams[part2].pInvarUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].pInvarUni[1], modelParams[part2].pInvarUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].pInvarPr,"Fixed") && !strcmp(modelParams[part2].pInvarPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].pInvarFix, modelParams[part2].pInvarFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */
		
		/* Check to see if the switching rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if the switching rates are inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_CORREL)
		{
		/* Check the autocorrelation parameter for gamma rates on partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and autocorrelation parameter does not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and autocorrelation parameter does not apply */

		/* Check that the data are either DNA, RNA, or PROTEIN for partitions 1 and 2 */
		if (modelSettings[part1].dataType != DNA && modelSettings[part1].dataType != RNA && modelSettings[part1].dataType != PROTEIN)
			*isApplic1 = NO; /* the switching rates do not make sense for part1 */
		if (modelSettings[part2].dataType != DNA && modelSettings[part2].dataType != RNA && modelSettings[part2].dataType != PROTEIN)
			*isApplic2 = NO; /* the switching rates do not make sense for part2 */
			
		/* Now, check that the data are the same (i.e., both nucleotide or both amino acid). */
		if (isFirstNucleotide != isSecondNucleotide)
			isSame = NO; /* one or the other is nucleotide, so they cannot be the same */
		else if (modelSettings[part1].dataType != modelSettings[part2].dataType && isFirstNucleotide == NO)
			isSame = NO; /* data are not both nucleotide or both amino acid */

		/* Let's check that autocorrelation parameter is even relevant for the two partitions */
		if (strcmp(modelParams[part1].ratesModel, "Adgamma"))
			*isApplic1 = NO; /* the autocorrelation parameter does not make sense for part1 */
		if (strcmp(modelParams[part2].ratesModel, "Adgamma"))
			*isApplic2 = NO; /* the autocorrelation parameter does not make sense for part2 */

		/* Assuming that we have a nucleotide model, make certain the models are not of type codon. */
		if (!strcmp(modelParams[part1].nucModel, "Codon"))
			*isApplic1 = NO; /* we have a codon model for part1, and a autocorrelation parameter does not apply */
		if (!strcmp(modelParams[part2].nucModel, "Codon"))
			*isApplic2 = NO; /* we have a codon model for part2, and a autocorrelation parameter does not apply */
		
		/* Check that the model structure is the same for both partitions */
		if (strcmp(modelParams[part1].nucModel, modelParams[part2].nucModel))
			isSame = NO; /* the nucleotide models are different */
		if (strcmp(modelParams[part1].covarionModel, modelParams[part2].covarionModel))
			isSame = NO; /* the models have different covarion struture */

		/* Check the priors for both partitions. */
		if (!strcmp(modelParams[part1].adGammaCorPr,"Uniform") && !strcmp(modelParams[part2].adGammaCorPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].corrUni[0], modelParams[part2].corrUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].corrUni[1], modelParams[part2].corrUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].adGammaCorPr,"Fixed") && !strcmp(modelParams[part2].adGammaCorPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].corrFix, modelParams[part2].corrFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */

		/* Check to see if the switching rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if the switching rates are inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_SWITCH)
		{
		/* Check the covarion switching rates on partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and switching rates do not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and switching rates do not apply */
		
		/* Check that the data are either DNA, RNA, or PROTEIN for partitions 1 and 2 */
		if (modelSettings[part1].dataType != DNA && modelSettings[part1].dataType != RNA && modelSettings[part1].dataType != PROTEIN)
			*isApplic1 = NO; /* the switching rates do not make sense for part1 */
		if (modelSettings[part2].dataType != DNA && modelSettings[part2].dataType != RNA && modelSettings[part2].dataType != PROTEIN)
			*isApplic2 = NO; /* the switching rates do not make sense for part2 */
			
		/* Now, check that the data are the same (i.e., both nucleotide or both amino acid). */
		if (isFirstNucleotide != isSecondNucleotide)
			isSame = NO; /* one or the other is nucleotide, so they cannot be the same */
		else if (modelSettings[part1].dataType != modelSettings[part2].dataType && isFirstNucleotide == NO)
			isSame = NO; /* data are not both nucleotide or both amino acid */

		/* Lets check that covarion model has been selected for partitions 1 and 2 */
		if (!strcmp(modelParams[part1].covarionModel, "No"))
			*isApplic1 = NO; /* the switching rates do not make sense for part1 */
		if (!strcmp(modelParams[part2].covarionModel, "No"))
			*isApplic2 = NO; /* the switching rates do not make sense for part2 */

		/* If we have a nucleotide model make certain the models are not of type codon or doublet. */
		if (!strcmp(modelParams[part1].nucModel, "Codon") || !strcmp(modelParams[part1].nucModel, "Doublet"))
			*isApplic1 = NO; /* we have a codon model for part1, and a covarion switch parameter does not apply */
		if (!strcmp(modelParams[part2].nucModel, "Codon") || !strcmp(modelParams[part2].nucModel, "Doublet"))
			*isApplic2 = NO; /* we have a codon model for part2, and a covarion switch parameter does not apply */

		/* Check that the priors are the same. */
		if (!strcmp(modelParams[part1].covSwitchPr,"Uniform") && !strcmp(modelParams[part2].covSwitchPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].covswitchUni[0], modelParams[part2].covswitchUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].covswitchUni[1], modelParams[part2].covswitchUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].covSwitchPr,"Exponential") && !strcmp(modelParams[part2].covSwitchPr,"Exponential"))
			{
			if (AreDoublesEqual (modelParams[part1].covswitchExp, modelParams[part2].covswitchExp, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].covSwitchPr,"Fixed") && !strcmp(modelParams[part2].covSwitchPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].covswitchFix[0], modelParams[part2].covswitchFix[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].covswitchFix[1], modelParams[part2].covswitchFix[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */

		/* Check to see if the switching rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if the switching rates are inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_RATEMULT)
		{
		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		   silly and doesn't take this information into account) and a rate multiplier is nonsensical. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
		
		/* Check that the branch lengths are at least proportional. */
		if (IsModelSame (P_BRLENS, part1, part2, &temp1, &temp2) == NO)
			isSame = NO;
		if (linkTable[P_BRLENS][part1] != linkTable[P_BRLENS][part2])
			isSame = NO;

		/* See if the rate prior is the same for the partitions */
		if (strcmp(modelParams[part1].ratePr, modelParams[part2].ratePr) != 0)
			isSame = NO;

		/* Check to see if rate multipliers are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 
			
		}
	else if (whichParam == P_TOPOLOGY)
		{
		/* Check the topology for partitions 1 and 2. */
		
		/* If the prior is different, then the topologies cannot be the same. */
		if (strcmp(modelParams[part1].topologyPr, modelParams[part2].topologyPr))
			isSame = NO;

		/* If both partitions have topologies constrained, then we need to make certain that the constraints are the same. */
		/* This also guarantees that any calibrations will be the same. */
		if (!strcmp(modelParams[part1].topologyPr, "Constraints") && !strcmp(modelParams[part2].topologyPr, "Constraints"))
			{
			if (modelParams[part1].numActiveConstraints != modelParams[part2].numActiveConstraints)
				isSame = NO;
			else
				{
				nDiff = 0;
				for (i=0; i<numDefinedConstraints; i++)
					if (modelParams[part1].activeConstraints[i] != modelParams[part2].activeConstraints[i])
						nDiff++;
				if (nDiff != 0)
					isSame = NO;
				}
			}
        }
    else if (whichParam == P_BRLENS)
		{
		/* Check the branch lengths for partitions 1 and 2. */

		/* First, if the topologies are different, the same branch lengths cannot apply. */
		if (IsModelSame (P_TOPOLOGY, part1, part2, &temp1, &temp2) == NO)
			isSame = NO;
		if (linkTable[P_TOPOLOGY][part1] != linkTable[P_TOPOLOGY][part2])
			isSame = NO;

		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		   silly and doesn't take this information into account). */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO;

		/* Check to see if the branch lengths are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO;

		/* Make sure the branch lengths have the same priors and are not unlinked */
		if (*isApplic1 == YES && *isApplic2 == YES)
			{
			/* We are dealing with real branch lengths (not parsimony) for both partitions */
			
			/* The branch length prior should be the same */
			if (strcmp(modelParams[part1].brlensPr, modelParams[part2].brlensPr))
				isSame = NO;
				
			/* if both partitions have unconstrained brlens, then we need to check that the priors on the branch lengths are the same */
			if (!strcmp(modelParams[part1].brlensPr, "Unconstrained") && !strcmp(modelParams[part2].brlensPr, "Unconstrained"))
				{
				if (strcmp(modelParams[part1].unconstrainedPr, modelParams[part2].unconstrainedPr))
					isSame = NO;
				else
					{
					if (!strcmp(modelParams[part1].unconstrainedPr, "Uniform"))
						{
						if (AreDoublesEqual (modelParams[part1].brlensUni[0], modelParams[part2].brlensUni[0], (MrBFlt) 0.00001) == NO)
							isSame = NO;
						if (AreDoublesEqual (modelParams[part1].brlensUni[1], modelParams[part2].brlensUni[1], (MrBFlt) 0.00001) == NO)
							isSame = NO;
						}
					else
						{
						if (AreDoublesEqual (modelParams[part1].brlensExp, modelParams[part2].brlensExp, (MrBFlt) 0.00001) == NO)
							isSame = NO;
						}
					}
				}
			
			/* if both partitions have clock brlens, then we need to check that the priors on the clock are the same */
			if (!strcmp(modelParams[part1].brlensPr, "Clock") && !strcmp(modelParams[part2].brlensPr, "Clock"))
				{
				if (strcmp(modelParams[part1].clockPr, modelParams[part2].clockPr))
					isSame = NO;
				else
					{
					if (!strcmp(modelParams[part1].clockPr, "Birthdeath"))
						{
						if (!strcmp(modelParams[part1].speciationPr,"Uniform") && !strcmp(modelParams[part2].speciationPr,"Uniform"))
							{
							if (AreDoublesEqual (modelParams[part1].speciationUni[0], modelParams[part2].speciationUni[0], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							if (AreDoublesEqual (modelParams[part1].speciationUni[1], modelParams[part2].speciationUni[1], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].speciationPr,"Exponential") && !strcmp(modelParams[part2].speciationPr,"Exponential"))
							{
							if (AreDoublesEqual (modelParams[part1].speciationExp, modelParams[part2].speciationExp, (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].speciationPr,"Fixed") && !strcmp(modelParams[part2].speciationPr,"Fixed"))
							{
							if (AreDoublesEqual (modelParams[part1].speciationFix, modelParams[part2].speciationFix, (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else
							isSame = NO;

						if (!strcmp(modelParams[part1].extinctionPr,"Beta") && !strcmp(modelParams[part2].extinctionPr,"Beta"))
							{
							if (AreDoublesEqual (modelParams[part1].extinctionBeta[0], modelParams[part2].extinctionBeta[0], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							if (AreDoublesEqual (modelParams[part1].extinctionBeta[1], modelParams[part2].extinctionBeta[1], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].extinctionPr,"Fixed") && !strcmp(modelParams[part2].extinctionPr,"Fixed"))
							{
							if (AreDoublesEqual (modelParams[part1].extinctionFix, modelParams[part2].extinctionFix, (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else
							isSame = NO;

						if (AreDoublesEqual (modelParams[part1].sampleProb, modelParams[part2].sampleProb, 0.00001) == NO)
                            isSame = NO;
						if (strcmp(modelParams[part1].sampleStrat,modelParams[part2].sampleStrat))
                            isSame = NO;
							
                        }
					else if (!strcmp(modelParams[part1].clockPr, "Coalescence") || !strcmp(modelParams[part1].clockPr, "Speciestreecoalescence"))
						{
						if (!strcmp(modelParams[part1].popSizePr,"Uniform") && !strcmp(modelParams[part2].popSizePr,"Uniform"))
							{
							if (AreDoublesEqual (modelParams[part1].popSizeUni[0], modelParams[part2].popSizeUni[0], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							if (AreDoublesEqual (modelParams[part1].popSizeUni[1], modelParams[part2].popSizeUni[1], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].popSizePr,"Lognormal") && !strcmp(modelParams[part2].popSizePr,"Lognormal"))
							{
							if (AreDoublesEqual (modelParams[part1].popSizeLognormal[0], modelParams[part2].popSizeLognormal[0], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							if (AreDoublesEqual (modelParams[part1].popSizeLognormal[1], modelParams[part2].popSizeLognormal[1], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].popSizePr,"Normal") && !strcmp(modelParams[part2].popSizePr,"Normal"))
							{
							if (AreDoublesEqual (modelParams[part1].popSizeNormal[0], modelParams[part2].popSizeNormal[0], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							if (AreDoublesEqual (modelParams[part1].popSizeNormal[1], modelParams[part2].popSizeNormal[1], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].popSizePr,"Gamma") && !strcmp(modelParams[part2].popSizePr,"Gamma"))
							{
							if (AreDoublesEqual (modelParams[part1].popSizeGamma[0], modelParams[part2].popSizeGamma[0], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							if (AreDoublesEqual (modelParams[part1].popSizeGamma[1], modelParams[part2].popSizeGamma[1], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].popSizePr,"Fixed") && !strcmp(modelParams[part2].popSizePr,"Fixed"))
							{
							if (AreDoublesEqual (modelParams[part1].popSizeFix, modelParams[part2].popSizeFix, (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else
							isSame = NO;

                        if (strcmp(modelParams[part1].ploidy, modelParams[part2].ploidy) != 0)
                            isSame = NO;
						}
					if (strcmp(modelParams[part1].clockPr, "Uniform") == 0 && strcmp(modelParams[part1].nodeAgePr, "Calibrated") != 0)
						{
						if (strcmp(modelParams[part1].treeAgePr,modelParams[part2].treeAgePr) != 0)
							isSame = NO;
						else if (!strcmp(modelParams[part1].treeAgePr,"Fixed"))
							{
							if (AreDoublesEqual (modelParams[part1].treeAgeFix, modelParams[part2].treeAgeFix, (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].treeAgePr,"Exponential"))
							{
							if (AreDoublesEqual (modelParams[part1].treeAgeExp, modelParams[part2].treeAgeExp, (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						else if (!strcmp(modelParams[part1].treeAgePr,"Gamma"))
							{
							if (AreDoublesEqual (modelParams[part1].treeAgeGamma[0], modelParams[part2].treeAgeGamma[0], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							if (AreDoublesEqual (modelParams[part1].treeAgeGamma[1], modelParams[part2].treeAgeGamma[1], (MrBFlt) 0.00001) == NO)
								isSame = NO;
							}
						}
					}

				/* if the same clock prior, we need to check calibrations */
				if (strcmp(modelParams[part1].nodeAgePr,modelParams[part2].nodeAgePr) != 0)
					isSame = NO;
                
                /* If fixed clock brlens, check if the brlens come from the same tree */
			    if (!strcmp(modelParams[part1].clockPr, "Fixed") && !strcmp(modelParams[part2].clockPr, "Fixed"))
				    {
				    if (modelParams[part1].brlensFix != modelParams[part2].brlensFix)
		                isSame = NO;
				    }
                }
			/* If fixed brlens, check if the brlens come from the same tree */
			if (!strcmp(modelParams[part1].brlensPr, "Fixed") && !strcmp(modelParams[part2].brlensPr, "Fixed"))
				{
				if (modelParams[part1].brlensFix != modelParams[part2].brlensFix)
					isSame = NO;
				}
			}
		}
	else if (whichParam == P_SPECRATE)
		{
		/* Check the speciation rates for partitions 1 and 2. */

		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		   silly and doesn't take this information into account) and a speciation rate cannot be estimated. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
			
		/* Check that the branch length prior is a clock:birthdeath for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;
		if (strcmp(modelParams[part1].clockPr, "Birthdeath"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockPr, "Birthdeath"))
			*isApplic2 = NO;
		
		/* Now, check that the prior on the speciation rates are the same. */
		if (!strcmp(modelParams[part1].speciationPr,"Uniform") && !strcmp(modelParams[part2].speciationPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].speciationUni[0], modelParams[part2].speciationUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].speciationUni[1], modelParams[part2].speciationUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].speciationPr,"Exponential") && !strcmp(modelParams[part2].speciationPr,"Exponential"))
			{
			if (AreDoublesEqual (modelParams[part1].speciationExp, modelParams[part2].speciationExp, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].speciationPr,"Fixed") && !strcmp(modelParams[part2].speciationPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].speciationFix, modelParams[part2].speciationFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
		
		/* Check to see if the speciation rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 

		}
	else if (whichParam == P_EXTRATE)
		{
		/* Check the extinction rates for partitions 1 and 2. */

		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		   silly and doesn't take this information into account) and a extinction rate cannot be estimated. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
			
		/* Check that the branch length prior is a clock:birthdeath for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;
		if (strcmp(modelParams[part1].clockPr, "Birthdeath"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockPr, "Birthdeath"))
			*isApplic2 = NO;
		
		/* Now, check that the prior on the extinction rates are the same. */
		if (!strcmp(modelParams[part1].extinctionPr,"Beta") && !strcmp(modelParams[part2].extinctionPr,"Beta"))
			{
			if (AreDoublesEqual (modelParams[part1].extinctionBeta[0], modelParams[part2].extinctionBeta[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].extinctionBeta[1], modelParams[part2].extinctionBeta[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].extinctionPr,"Fixed") && !strcmp(modelParams[part2].extinctionPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].extinctionFix, modelParams[part2].extinctionFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
		
		/* Check to see if the speciation rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 

		}
	else if (whichParam == P_POPSIZE)
		{
		/* Check population size for partitions 1 and 2. */

		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		   silly and doesn't take this information into account) and population size cannot be estimated. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
			
		/* Check that the branch length prior is a clock:coalescence or clock:speciestreecoalescence for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;
		if (strcmp(modelParams[part1].clockPr, "Coalescence") != 0 && strcmp(modelParams[part1].clockPr, "Speciestreecoalescence") != 0)
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockPr, "Coalescence") != 0 && strcmp(modelParams[part2].clockPr, "Speciestreecoalescence") != 0)
			*isApplic2 = NO;
		
		/* Now, check that the prior on population size is the same. */
		if (!strcmp(modelParams[part1].popSizePr,"Uniform") && !strcmp(modelParams[part2].popSizePr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].popSizeUni[0], modelParams[part2].popSizeUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].popSizeUni[1], modelParams[part2].popSizeUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].popSizePr,"Lognormal") && !strcmp(modelParams[part2].popSizePr,"Lognormal"))
			{
			if (AreDoublesEqual (modelParams[part1].popSizeLognormal[0], modelParams[part2].popSizeLognormal[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].popSizeLognormal[1], modelParams[part2].popSizeLognormal[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].popSizePr,"Normal") && !strcmp(modelParams[part2].popSizePr,"Normal"))
			{
			if (AreDoublesEqual (modelParams[part1].popSizeNormal[0], modelParams[part2].popSizeNormal[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].popSizeNormal[1], modelParams[part2].popSizeNormal[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].popSizePr,"Gamma") && !strcmp(modelParams[part2].popSizePr,"Gamma"))
			{
			if (AreDoublesEqual (modelParams[part1].popSizeGamma[0], modelParams[part2].popSizeGamma[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].popSizeGamma[1], modelParams[part2].popSizeGamma[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].popSizePr,"Fixed") && !strcmp(modelParams[part2].popSizePr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].popSizeFix, modelParams[part2].popSizeFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
		
		/* Check to see if population size is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 

		}
	else if (whichParam == P_GROWTH)
		{
		/* Check growth rate for partitions 1 and 2. */

		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		   silly and doesn't take this information into account) and growth rate cannot be estimated. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
			
		/* Check that the branch length prior is a clock:coalescence for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;
		if (strcmp(modelParams[part1].clockPr, "Coalescence"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockPr, "Coalescence"))
			*isApplic2 = NO;
		
		/* Now, check that the prior on growth rate is the same. */
		if (!strcmp(modelParams[part1].growthPr,"Uniform") && !strcmp(modelParams[part2].growthPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].growthUni[0], modelParams[part2].growthUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].growthUni[1], modelParams[part2].growthUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].growthPr,"Exponential") && !strcmp(modelParams[part2].growthPr,"Exponential"))
			{
			if (AreDoublesEqual (modelParams[part1].growthExp, modelParams[part2].growthExp, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].growthPr,"Fixed") && !strcmp(modelParams[part2].growthPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].growthFix, modelParams[part2].growthFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
		
		/* Check to see if growth rate is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 
		}
	else if (whichParam == P_AAMODEL)
		{
		/* Check the amino acid model settings for partitions 1 and 2. */

		/* Check if the model is parsimony for either partition */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; /* part1 has a parsimony model and aamodel does not apply */
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; /* part2 has a parsimony model and aamodel does not apply */
		
		/* Check that the data are protein for both partitions 1 and 2 */
		if (isFirstProtein == NO)
			*isApplic1 = NO; /* part1 is not amino acid data so tratio does not apply */
		if (isSecondProtein == NO)
			*isApplic2 = NO; /* part2 is not amino acid data so tratio does not apply */
			
		/* If the model is fixed for a partition, then it is not a free parameter and
		   we set it to isApplic = NO */
		if (!strcmp(modelParams[part1].aaModelPr,"Fixed"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].aaModelPr,"Fixed"))
			*isApplic2 = NO; 

		/* We now need to check if the prior is the same for both. */
		if (!strcmp(modelParams[part1].aaModelPr,"Mixed") && !strcmp(modelParams[part2].aaModelPr,"Mixed"))
			{
			}
		else if (!strcmp(modelParams[part1].aaModelPr,"Fixed") && !strcmp(modelParams[part2].aaModelPr,"Fixed"))
			{
			if (strcmp(modelParams[part1].aaModel,modelParams[part2].aaModel))
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */

		/* Check to see if amino acid model is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if tratio is inapplicable for either partition, then the parameter cannot be the same */

		}
	else if (whichParam == P_BRCORR)
		{
		/* Check the correlation parameter for brownian motion 1 and 2. */
		
		/* Check that the data are either CONTINUOUS for partitions 1 and 2 */
		if (modelParams[part1].dataType != CONTINUOUS)
			*isApplic1 = NO; /* the correlation parameter does not make sense for part1 */
		if (modelParams[part2].dataType != CONTINUOUS)
			*isApplic2 = NO; /* the correlation parameter does not make sense for part2 */
			
		/* Now, check that the data are the same. */
		if (modelParams[part1].dataType != modelParams[part2].dataType)
			isSame = NO; /* data are not both continuous */

		/* Check the priors for both partitions. */
		if (!strcmp(modelParams[part1].brownCorPr,"Uniform") && !strcmp(modelParams[part2].brownCorPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].brownCorrUni[0], modelParams[part2].brownCorrUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].brownCorrUni[1], modelParams[part2].brownCorrUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].brownCorPr,"Fixed") && !strcmp(modelParams[part2].brownCorPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].brownCorrFix, modelParams[part2].brownCorrFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */

		/* Check to see if the correlation parameters are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if the correlation parameters are inapplicable for either partition, then the parameter cannot be the same */
		}
	else if (whichParam == P_BRSIGMA)
		{
		/* Check the sigma parameter for brownian motion 1 and 2. */
		
		/* Check that the data are either CONTINUOUS for partitions 1 and 2 */
		if (modelParams[part1].dataType != CONTINUOUS)
			*isApplic1 = NO; /* the sigma parameter does not make sense for part1 */
		if (modelParams[part2].dataType != CONTINUOUS)
			*isApplic2 = NO; /* the sigma parameter does not make sense for part2 */
			
		/* Now, check that the data are the same. */
		if (modelParams[part1].dataType != modelParams[part2].dataType)
			isSame = NO; /* data are not both continuous */

		/* Check the priors for both partitions. */
		if (!strcmp(modelParams[part1].brownScalesPr,"Uniform") && !strcmp(modelParams[part2].brownScalesPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].brownScalesUni[0], modelParams[part2].brownScalesUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].brownScalesUni[1], modelParams[part2].brownScalesUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].brownScalesPr,"Fixed") && !strcmp(modelParams[part2].brownScalesPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].brownScalesFix, modelParams[part2].brownScalesFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].brownScalesPr,"Gamma") && !strcmp(modelParams[part2].brownScalesPr,"Gamma"))
			{
			if (AreDoublesEqual (modelParams[part1].brownScalesGamma[0], modelParams[part2].brownScalesGamma[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].brownScalesGamma[1], modelParams[part2].brownScalesGamma[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].brownScalesPr,"Gammamean") && !strcmp(modelParams[part2].brownScalesPr,"Gammamean"))
			{
			if (AreDoublesEqual (modelParams[part1].brownScalesGammaMean, modelParams[part2].brownScalesGammaMean, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO; /* the priors are not the same, so we cannot set the parameter to be equal for both partitions */

		/* Check to see if the sigma parameters are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; /* if the sigma parameters are inapplicable for either partition, then the parameter cannot be the same */
		}
	else if (whichParam == P_CPPRATE)
		{
		/* Check cpp rate for partitions 1 and 2. */
	
		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		silly and doesn't take this information into account) and cpp rate cannot be estimated. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 

		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

		/* Check that the clock rate prior is cpp for both partitions */
		if (strcmp(modelParams[part1].clockVarPr, "Cpp"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockVarPr, "Cpp"))
			*isApplic2 = NO;
		
		/* Now, check that the prior on cpp rate is the same. */
		if (!strcmp(modelParams[part1].cppRatePr,"Exponential") && !strcmp(modelParams[part2].cppRatePr,"Exponential"))
			{
			if (AreDoublesEqual (modelParams[part1].cppRateExp, modelParams[part2].cppRateExp, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].cppRatePr,"Fixed") && !strcmp(modelParams[part2].cppRatePr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].cppRateFix, modelParams[part2].cppRateFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
	
		/* Check to see if cpp rate is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 	
		}
	else if (whichParam == P_CPPMULTDEV)
		{
		/* Check cpp multiplier deviation prior for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		silly and doesn't take this information into account) and this parameter is inapplicable. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
		
		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

		/* Check that the clock rate prior is cpp for both partitions */
		if (strcmp(modelParams[part1].clockVarPr, "Cpp"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockVarPr, "Cpp"))
			*isApplic2 = NO;
		
		/* Now, check that the prior on sigma is the same. */
		if (!strcmp(modelParams[part1].cppMultDevPr,"Fixed") && !strcmp(modelParams[part2].cppMultDevPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].cppMultDevFix, modelParams[part2].cppMultDevFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
		
		/* Check to see if cpp multiplier sigma is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 	
		}
	else if (whichParam == P_CPPEVENTS)
		{
		/* Check cpp events for partitions 1 and 2. */
	
		/* Check if the model is parsimony for either partition. If so, then branch lengths do not apply (as parsimony is very
		silly and doesn't take this information into account) and cpp events are inapplicable. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 

		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

		/* Check that the clock rate prior is cpp for both partitions */
		if (strcmp(modelParams[part1].clockVarPr, "Cpp"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockVarPr, "Cpp"))
			*isApplic2 = NO;
		
		/* Now, check that the cpp parameter is the same */
		if (IsModelSame (P_CPPRATE, part1, part2, &temp1, &temp2) == NO)
			isSame = NO;
        if (linkTable[P_CPPRATE][part1] != linkTable[P_CPPRATE][part2])
            isSame = NO;
	
		/* ... and that the psigamma parameter is the same */
		if (IsModelSame (P_CPPMULTDEV, part1, part2, &temp1, &temp2) == NO)
			isSame = NO;
        if (linkTable[P_CPPMULTDEV][part1] != linkTable[P_CPPRATE][part2])
            isSame = NO;
	
        /* Not same if branch lengths are not the same */
        if (IsModelSame(P_BRLENS, part1, part2, &temp1, &temp2) == NO)
            isSame = NO;
        if (linkTable[P_BRLENS][part1] != linkTable[P_BRLENS][part2])
            isSame = NO;

		/* Set isSame to NO if cpp events are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO;
		}
	else if (whichParam == P_TK02VAR)
		{
		/* Check prior for variance of rate autocorrelation for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		silly and doesn't take this information into account) and ratevar is inapplicable. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
		
		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

		/* Check that the clock rate prior is tk02 for both partitions */
		if (strcmp(modelParams[part1].clockVarPr, "TK02"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockVarPr, "TK02"))
			*isApplic2 = NO;
		
		/* Now, check that the prior on tk02 variance is the same. */
		if (!strcmp(modelParams[part1].tk02varPr,"Uniform") && !strcmp(modelParams[part2].tk02varPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].tk02varUni[0], modelParams[part2].tk02varUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].tk02varUni[1], modelParams[part2].tk02varUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].tk02varPr,"Exponential") && !strcmp(modelParams[part2].tk02varPr,"Exponential"))
			{
			if (AreDoublesEqual (modelParams[part1].tk02varExp, modelParams[part2].tk02varExp, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].tk02varPr,"Fixed") && !strcmp(modelParams[part2].tk02varPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].tk02varFix, modelParams[part2].tk02varFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
		
		/* Check to see if tk02 variance is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 	
		}
	else if (whichParam == P_TK02BRANCHRATES)
		{
		/* Check TK02 relaxed clock branch rates for partitions 1 and 2. */
	
		/* Check if the model is parsimony for either partition. If so, then branch lengths do not apply (as parsimony is very
		silly and doesn't take this information into account) and tk02 relaxed clock branch rates are inapplicable. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 

		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

		/* Check that the clock rate prior is tk02 for both partitions */
		if (strcmp(modelParams[part1].clockVarPr, "TK02"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockVarPr, "TK02"))
			*isApplic2 = NO;
		
		/* Now, check that the tk02 variance parameter is the same */
		if (IsModelSame (P_TK02VAR, part1, part2, &temp1, &temp2) == NO)
			isSame = NO;
        if (linkTable[P_TK02VAR][part1] != linkTable[P_TK02VAR][part2])
            isSame = NO;

        /* Not same if branch lengths are not the same */
        if (IsModelSame(P_BRLENS, part1, part2, &temp1, &temp2) == NO)
            isSame = NO;
        if (linkTable[P_BRLENS][part1] != linkTable[P_BRLENS][part2])
            isSame = NO;

		/* Set isSame to NO if tk02 branch rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO;
		}
	else if (whichParam == P_IGRVAR)
		{
		/* Check prior for igr gamma shape for partitions 1 and 2. */
		
		/* Check if the model is parsimony for either partition. If so, then the branch lengths cannot apply (as parsimony is very
		silly and doesn't take this information into account) and igr gamma shape is inapplicable. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 
		
		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

		/* Check that the clock rate prior is igr for both partitions */
		if (strcmp(modelParams[part1].clockVarPr, "Igr"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockVarPr, "Igr"))
			*isApplic2 = NO;
		
		/* Now, check that the prior on igr gamma shape is the same. */
		if (!strcmp(modelParams[part1].igrvarPr,"Uniform") && !strcmp(modelParams[part2].igrvarPr,"Uniform"))
			{
			if (AreDoublesEqual (modelParams[part1].igrvarUni[0], modelParams[part2].igrvarUni[0], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			if (AreDoublesEqual (modelParams[part1].igrvarUni[1], modelParams[part2].igrvarUni[1], (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].igrvarPr,"Exponential") && !strcmp(modelParams[part2].igrvarPr,"Exponential"))
			{
			if (AreDoublesEqual (modelParams[part1].igrvarExp, modelParams[part2].igrvarExp, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else if (!strcmp(modelParams[part1].igrvarPr,"Fixed") && !strcmp(modelParams[part2].igrvarPr,"Fixed"))
			{
			if (AreDoublesEqual (modelParams[part1].igrvarFix, modelParams[part2].igrvarFix, (MrBFlt) 0.00001) == NO)
				isSame = NO;
			}
		else
			isSame = NO;
		
		/* Check to see if tk02 variance is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO; 	
		}
	else if (whichParam == P_IGRBRANCHLENS)
		{
		/* Check IGR relaxed clock branch rates for partitions 1 and 2. */
	
		/* Check if the model is parsimony for either partition. If so, then branch lengths do not apply (as parsimony is very
		silly and doesn't take this information into account) and tk02 relaxed clock branch rates are inapplicable. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 

		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

		/* Check that the clock rate prior is igr for both partitions */
		if (strcmp(modelParams[part1].clockVarPr, "Igr"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].clockVarPr, "Igr"))
			*isApplic2 = NO;
		
        /* Now, check that the igr gamma shape parameter is the same */
		if (IsModelSame (P_IGRVAR, part1, part2, &temp1, &temp2) == NO)
			isSame = NO;
        if (linkTable[P_IGRVAR][part1] != linkTable[P_IGRVAR][part2])
            isSame = NO;
	
        /* Not same if branch lengths are not the same */
        if (IsModelSame(P_BRLENS, part1, part2, &temp1, &temp2) == NO)
            isSame = NO;
        if (linkTable[P_BRLENS][part1] != linkTable[P_BRLENS][part2])
            isSame = NO;

		/* Set isSame to NO if igr branch rates are inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO;
		}
	else if (whichParam == P_CLOCKRATE)
		{
		/* Check base substitution rates of clock tree for partitions 1 and 2. */
	
        /* Check if the model is parsimony for either partition. If so, then branch lengths do not apply (as parsimony is very
		silly and doesn't take this information into account) and clock branch rates are inapplicable. */
		if (!strcmp(modelParams[part1].parsModel, "Yes"))
			*isApplic1 = NO; 
		if (!strcmp(modelParams[part2].parsModel, "Yes"))
			*isApplic2 = NO; 

		/* Check that the branch length prior is clock for both partitions. */
		if (strcmp(modelParams[part1].brlensPr, "Clock"))
			*isApplic1 = NO;
		if (strcmp(modelParams[part2].brlensPr, "Clock"))
			*isApplic2 = NO;

        /* Set isSame to NO if base substitution rate parameter is inapplicable for either partition. */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO;
		}
	else if (whichParam == P_SPECIESTREE)
		{
		/* Species tree; check that it is used in both partitions */
        if (strcmp(modelParams[part1].topologyPr, "Speciestree") != 0)
            *isApplic1 = NO;
        if (strcmp(modelParams[part2].topologyPr, "Speciestree") != 0)
            *isApplic2 = NO;

        /* Not same if inapplicable to either partition */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO;
        }
	else if (whichParam == P_GENETREERATE)
		{
		/* Gene tree rate; check that it is used in both partitions */
        if (strcmp(modelParams[part1].topologyPr, "Speciestree") != 0)
            *isApplic1 = NO;
        if (strcmp(modelParams[part2].topologyPr, "Speciestree") != 0)
            *isApplic2 = NO;

        /* Not same if inapplicable to either partition */
		if ((*isApplic1) == NO || (*isApplic2) == NO)
			isSame = NO;
        }
	else
		{
		MrBayesPrint ("%s   Could not find parameter in IsModelSame\n", spacer);
		return (NO);
		}
	
	return (isSame);
	
}





int Link (void)

{

	int			i, j;
	
	for (j=0; j<NUM_LINKED; j++)
		{
		MrBayesPrint ("%4d -- ", j+1);
		for (i=0; i<numCurrentDivisions; i++)
			MrBayesPrint (" %2d", tempLinkUnlink[j][i]);
		MrBayesPrint ("\n");
		}
		
	return (NO_ERROR);
	
}





int NumActiveParts (void)

{

	int		i, nApplied;
	
	nApplied = 0;
	for (i=0; i<numCurrentDivisions; i++)
		if (activeParts[i] == YES)
			nApplied++;

	return (nApplied);
	
}





int NumNonExcludedChar (void)

{

	int		i, n;
	
	/* count number of non-excluded characters */
	n = 0;
	for (i=0; i<numChar; i++)
		{
		if (charInfo[i].isExcluded == NO)
			{
			n++;
			}
		}
	
	return n;
	
}





int NumStates (int part)

{

	if (modelParams[part].dataType == DNA || modelParams[part].dataType == RNA)
		{
		if (!strcmp(modelParams[part].nucModel, "4by4"))
			return (4);
		else if (!strcmp(modelParams[part].nucModel, "Doublet"))
			return (16);
        else if (!strcmp(modelParams[part].nucModel, "Protein"))
            return (20);
		else
			{
			if (!strcmp(modelParams[part].geneticCode, "Universal"))
				return (61);
			else if (!strcmp(modelParams[part].geneticCode, "Vertmt"))
				return (60);
			else if (!strcmp(modelParams[part].geneticCode, "Mycoplasma"))
				return (62);
			else if (!strcmp(modelParams[part].geneticCode, "Yeast"))
				return (62);
			else if (!strcmp(modelParams[part].geneticCode, "Ciliates"))
				return (63);
			else if (!strcmp(modelParams[part].geneticCode, "Metmt"))
				return (62);
			}
		}
	else if (modelParams[part].dataType == PROTEIN)
		{
		return (20);
		}
	else if (modelParams[part].dataType == RESTRICTION)
		{
		return (2);
		}
	else if (modelParams[part].dataType == STANDARD)
		{
		return (10);
		}
		
	return (-1);

}





/*-----------------------------------------------------------------------
|
|	PrintCompMatrix: Print compressed matrix
|
------------------------------------------------------------------------*/
int PrintCompMatrix (void)

{
	int				i, j, k, c, d;
	ModelInfo		*m;
	ModelParams		*mp;
	char			tempName[100];
	char			(*whichChar)(int);

	extern char		WhichAA (int x);
	extern char		WhichNuc (int x);
	extern char		WhichRes (int x);
	extern char		WhichStand (int x);


	if (!compMatrix)
		return ERROR;

	whichChar = &WhichNuc;
	
	for (d=0; d<numCurrentDivisions; d++)
		{
		m = &modelSettings[d];
		mp = &modelParams[d];

		if (mp->dataType == DNA || mp->dataType == RNA)
			whichChar = &WhichNuc;
		if (mp->dataType == PROTEIN)
			whichChar = &WhichAA;
		if (mp->dataType == RESTRICTION)
			whichChar = &WhichRes;
		if (mp->dataType == STANDARD)
			whichChar = &WhichStand;

		MrBayesPrint ("\nCompressed matrix for division %d\n\n", d+1);
		
		k = 66;
		if (mp->dataType == CONTINUOUS)
			k /= 4;

		for (c=m->compMatrixStart; c<m->compMatrixStop; c+=k)
			{
			for (i=0; i<numLocalTaxa; i++)
				{
				strcpy (tempName, localTaxonNames[i]);
				MrBayesPrint ("%-10.10s   ", tempName);
				for (j=c; j<c+k; j++)
					{
					if (j >= m->compMatrixStop)
						break;
					if (mp->dataType == CONTINUOUS)
						MrBayesPrint ("%3d ", compMatrix[pos(i,j,compMatrixRowSize)]);
					else
						MrBayesPrint ("%c", whichChar((int)compMatrix[pos(i,j,compMatrixRowSize)]));
					}
				MrBayesPrint("\n");
				}
			MrBayesPrint("\nNo. sites    ");
			for (j=c; j<c+k; j++)
				{
				if (j >= m->compMatrixStop)
					break;
				i = (int) numSitesOfPat[m->compCharStart + (0*numCompressedChars) + (j - m->compMatrixStart)/m->nCharsPerSite]; /* NOTE: We are printing the unadulterated site pat nums */
				if (i>9)
					i = 'A' + i - 10;
				else
					i = '0' + i;
				if (mp->dataType == CONTINUOUS)
					MrBayesPrint("   %c ", i);
				else
					{
					if ((j-m->compMatrixStart) % m->nCharsPerSite == 0)
						MrBayesPrint ("%c", i);
					else
						MrBayesPrint(" ");
					}
				}
			MrBayesPrint ("\nOrig. char   ");
			for (j=c; j<c+k; j++)
				{
				if (j >= m->compMatrixStop)
					break;
				i = origChar[j];
				if (i>9)
					i = '0' + (i % 10);
				else
					i = '0' +i;
				if (mp->dataType == CONTINUOUS)
					MrBayesPrint("   %c ", i);
				else
					MrBayesPrint ("%c", i);
				}

			if (mp->dataType == STANDARD && m->nStates != NULL)
				{
				MrBayesPrint ("\nNo. states   ");
				for (j=c; j<c+k; j++)
					{
					if (j >= m->compMatrixStop)
						break;
					i = m->nStates[j-m->compCharStart];
					MrBayesPrint ("%d", i);
					}
				MrBayesPrint ("\nCharType     ");
				for (j=c; j<c+k; j++)
					{
					if (j >= m->compMatrixStop)
						break;
					i = m->cType[j-m->compMatrixStart];
					if (i == ORD)
						MrBayesPrint ("%c", 'O');
					else if (i == UNORD)
						MrBayesPrint ("%c", 'U');
					else
						MrBayesPrint ("%c", 'I');
					}
				MrBayesPrint ("\ntiIndex      ");
				for (j=c; j<c+k; j++)
					{
					if (j >= m->compMatrixStop)
						break;
					i = m->tiIndex[j-m->compCharStart];
					MrBayesPrint ("%d", i % 10);
					}
				MrBayesPrint ("\nbsIndex      ");
				for (j=c; j<c+k; j++)
					{
					if (j >= m->compMatrixStop)
						break;
					i = m->bsIndex[j-m->compCharStart];
					MrBayesPrint ("%d", i % 10);
					}
				}
			MrBayesPrint ("\n\n");
			}
		MrBayesPrint ("Press return to continue\n");
		getchar();
		}	/* next division */

	return NO_ERROR;

}





/*----------------------------------------------------------------------
|
|	PrintMatrix: Print data matrix
|
|
------------------------------------------------------------------------*/
int PrintMatrix (void)

{

	int				i, j=0, c, printWidth, nextColumn;

	extern char		WhichAA (int x);
	extern char		WhichNuc (int x);
	extern char		WhichRes (int x);
	extern char		WhichStand (int x);


	if (!matrix)
		return ERROR;
	
	MrBayesPrint ("\nData matrix\n\n");
	
	printWidth = 79;

	for (c=0; c<numChar; c=j)
		{
		for (i=0; i<numTaxa; i++)
			{
			MrBayesPrint ("%-10.10s   ", taxaNames[i]);
			j = c;
			for (nextColumn=13; nextColumn < printWidth; nextColumn++)
				{
				if (j >= numChar)
					break;
				if (charInfo[j].charType == CONTINUOUS && nextColumn < printWidth - 3)
					break;
				if (charInfo[j].charType == CONTINUOUS)
					{	
					MrBayesPrint ("%3d ", matrix[pos(i,j,numChar)]);
					nextColumn += 3;
					}
				else if (charInfo[j].charType == DNA || charInfo[j].charType == RNA)
					MrBayesPrint ("%c", WhichNuc(matrix[pos(i,j,numChar)]));
				else if (charInfo[j].charType == PROTEIN)
					MrBayesPrint ("%c", WhichAA(matrix[pos(i,j,numChar)]));
				else if (charInfo[j].charType == RESTRICTION)
					MrBayesPrint ("%c", WhichRes(matrix[pos(i,j,numChar)]));
				else if (charInfo[j].charType == STANDARD)
					MrBayesPrint ("%c", WhichStand(matrix[pos(i,j,numChar)]));
				j++;
				}
			MrBayesPrint("\n");
			}
		MrBayesPrint ("\n");
		}

	return NO_ERROR;

}





/*--------------------------------------------------------------
|
|	ProcessStdChars: process standard characters
|
---------------------------------------------------------------*/
int ProcessStdChars (SafeLong *seed)

{

	int				c, d, i, j, k, n, ts, index, numStandardChars, origCharPos, *bsIndex;
    char            piHeader[30];
	ModelInfo		*m;
	ModelParams		*mp=NULL;
	Param			*p;

	/* set character type, no. states, ti index and bs index for standard characters */
	/* first calculate how many standard characters we have */
	numStandardChars = 0;
	for (d=0; d<numCurrentDivisions; d++)
		{
		mp = &modelParams[d];
		m = &modelSettings[d];

		if (mp->dataType != STANDARD)
			continue;

		numStandardChars += m->numChars;
		}
	
	/* return if there are no standard characters */
	if (numStandardChars == 0)
		return (NO_ERROR);

	/* we are still here so we have standard characters and need to deal with them */
	
	/* first allocate space for stdType, stateSize, tiIndex, bsIndex */
	if (memAllocs[ALLOC_STDTYPE] == YES)
		{
		free (stdType);
        stdType = NULL;
		memAllocs[ALLOC_STDTYPE] = NO;
		}
	stdType = (int *)SafeCalloc((size_t) (4 * numStandardChars), sizeof(int));
	if (!stdType)
		{
		MrBayesPrint ("%s   Problem allocating stdType (%d ints)\n", 4 * numStandardChars);
		return ERROR;
		}
	memAllocs[ALLOC_STDTYPE] = YES;
	stateSize = stdType + numStandardChars;
	tiIndex = stateSize + numStandardChars;
	bsIndex = tiIndex + numStandardChars;

	/* then fill in stdType and stateSize, set pointers */
	/* also fill in isTiNeeded for each division and tiIndex for each character */
	for (d=j=0; d<numCurrentDivisions; d++)
		{
		mp = &modelParams[d];
		m = &modelSettings[d];
		
		if (mp->dataType != STANDARD)
			continue;

		m->cType = stdType + j;
		m->nStates = stateSize + j;
		m->tiIndex = tiIndex + j;
		m->bsIndex = bsIndex + j;

		m->cijkLength = 0;
        m->nCijkParts = 0;
		for (c=0; c<m->numChars; c++)
			{
			if (origChar[c+m->compMatrixStart] < 0)
				{
				/* this is a dummy character */
				m->cType[c] = UNORD;
				m->nStates[c] = 2;
				}
			else
				{
				/* this is an ordinary character */
				m->cType[c] = charInfo[origChar[c + m->compMatrixStart]].ctype;
				m->nStates[c] = charInfo[origChar[c + m->compMatrixStart]].numStates;
				}
			
			/* check ctype settings */
			if (m->nStates[c] < 2)
				{
				MrBayesPrint ("%s   WARNING: Compressed character %d (original character %d) of division %d has less \n", spacer, c+m->compCharStart,origChar[c+m->compCharStart]+1, d+1);
				MrBayesPrint ("%s            than two observed states; it will be assumed to have two states.\n", spacer);
				m->nStates[c] = 2;
				}
			if (m->nStates[c] > 6 && m->cType[c] != UNORD)
				{
				MrBayesPrint ("%s   Only unordered model supported for characters with more than 6 states\n", spacer);
				return ERROR;
				}
			if (m->nStates[c] == 2 && m->cType[c] == ORD)
				m->cType[c] = UNORD;
			if (m->cType[c] == IRREV)
				{
				MrBayesPrint ("%s   Irreversible model not yet supported\n", spacer);
				return ERROR;
				}
			
			/* find max number of states */
			if (m->nStates[c] > m->numModelStates)
				m->numModelStates = m->nStates[c];

			/* update Cijk info */
			if (strcmp(mp->symPiPr,"Fixed") != 0 || AreDoublesEqual(mp->symBetaFix, -1.0, 0.00001) == NO)
				{
				/* Asymmetry between stationary state frequencies -- we need one cijk and eigenvalue
					set for each multistate character */
				if (m->nStates[c] > 2 && (m->cType[c] == UNORD || m->cType[c] == ORD))
					{
					ts = m->nStates[c];
					m->cijkLength += (ts * ts * ts) + (2 * ts);
					m->nCijkParts++;
					}
				}

			/* set the ti probs needed */
			if (m->stateFreq->nValues == 0 || m->nStates[c] == 2)
                {
    			if (m->cType[c] == UNORD)
	    			m->isTiNeeded[m->nStates[c]-2] = YES;
		    	if (m->cType[c] == ORD)
			    	m->isTiNeeded[m->nStates[c]+6] = YES;
			    if (m->cType[c] == IRREV)
				    m->isTiNeeded[m->nStates[c]+11] = YES;
                }
			}

		/* set ti index for each compressed character first         */
		/* set bs index	later (below)								*/

        /* set base index, valid for binary chars */
        m->tiIndex[c] = 0;

		/* first adjust for unordered characters */
        for (k=0; k<9; k++)
			{
			if (m->isTiNeeded [k] == NO)
				continue;

			for (c=0; c<m->numChars; c++)
				{
				if (m->cType[c] != UNORD || m->nStates[c] > k + 2)
					{
					m->tiIndex[c] += (k + 2) * (k + 2) * m->numGammaCats;
					}
				}
			}

		/* second for ordered characters */
		for (k=9; k<13; k++)
			{
			if (m->isTiNeeded [k] == NO)
				continue;

			for (c=0; c<m->numChars; c++)
				{
				if (m->cType[c] == IRREV || (m->cType[c] == ORD && m->nStates[c] > k - 6))
					{
					m->tiIndex[c] += (k - 6) * (k - 6) * m->numGammaCats;
					}
				}
			}

		/* third for irrev characters */
		for (k=13; k<18; k++)
			{
			if (m->isTiNeeded [k] == NO)
				continue;

			for (c=0; c<m->numChars; c++)
				{
				if (m->cType[c] == IRREV && m->nStates[c] > k - 11)
					{
					m->tiIndex[c] += (k - 11) * (k - 11) * m->numGammaCats;
					}
				}
			}

		/* finally take beta cats into account in tiIndex        */
		/* the beta cats will only be used for binary characters */
        /* multistate characters get their ti indices reset here */
		if (m->numBetaCats > 1 && m->isTiNeeded[0] == YES)
			{
            k = 4 * m->numBetaCats * m->numGammaCats;   /* offset for binary character ti probs */
			for (c=0; c<m->numChars; c++)
				{
				if (m->nStates[c] > 2)
					{
					m->tiIndex[c] = k;
                    k += m->nStates[c] * m->nStates[c] * m->numGammaCats;
					}
				}
			}
		j += m->numChars;
		}
	
	/* deal with bsIndex */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType != P_PI || modelParams[p->relParts[0]].dataType != STANDARD)
			continue;
		p->nSympi = 0;
        p->hasBinaryStd = NO;
		for (i=0; i<p->nRelParts; i++)
			if (modelSettings[p->relParts[i]].isTiNeeded[0] == YES)
				break;
        if (i < p->nRelParts)
            p->hasBinaryStd = YES;
		if (p->paramId == SYMPI_EQUAL)
			{
			/* calculate the number of state frequencies needed */
			/* also set bsIndex appropriately                   */
			for (n=index=0; n<9; n++)
				{
				for (i=0; i<p->nRelParts; i++)
					if (modelSettings[p->relParts[i]].isTiNeeded[n] == YES)
						break;
				if (i < p->nRelParts)
					{
					for (i=0; i<p->nRelParts; i++)
						{
						m = &modelSettings[p->relParts[i]];
						for (c=0; c<m->numChars; c++)
							{
							if (m->cType[c] != UNORD || m->nStates[c] > n + 2)
								{
								m->bsIndex[c] += (n + 2);
								}
							}
						}
					index += (n + 2);
					}
				}
			for (n=9; n<13; n++)
				{
				for (i=0; i<p->nRelParts; i++)
					if (modelSettings[p->relParts[i]].isTiNeeded[n] == YES)
						break;
				if (i < p->nRelParts)
					{
					for (i=0; i<p->nRelParts; i++)
						{
						m = &modelSettings[p->relParts[i]];
						for (c=0; c<m->numChars; c++)
							{
							if (m->cType[c] == ORD && m->nStates[c] > n - 6)
								{
								m->bsIndex[c] += (n - 6);
								}
							}
						}
					index += (n - 6);
					}
				}
			p->nStdStateFreqs = index;
			}
		else
			{
			/* if not equal we need space for beta category frequencies */
            index = 0;
			if (p->hasBinaryStd == YES)
				index += (2 * modelSettings[p->relParts[0]].numBetaCats);
			/* as well as one set of frequencies for each multistate character */
			for (i=0; i<p->nRelParts; i++)
				{
				m = &modelSettings[p->relParts[i]];
				for (c=0; c<m->numChars; c++)
					{
					if (m->nStates[c] > 2 && (m->cType[c] == UNORD || m->cType[c] == ORD))
						{
						m->bsIndex[c] = index;
						index += m->nStates[c];
						p->nSympi++;
						}
					}
				}
			}
		p->nStdStateFreqs = index;
		}
	
	/* allocate space for bsIndex, sympiIndex, stdStateFreqs; then fill */

	/* first count number of sympis needed */
	for (k=n=i=0; k<numParams; k++)
		{
		p = &params[k];
		n += p->nSympi;		/* nsympi calculated above */
		}
	
	/* then allocate and fill in */
	if (n > 0)
		{
		if (memAllocs[ALLOC_SYMPIINDEX] == YES)
			{
			sympiIndex = (int *) SafeRealloc ((void *) sympiIndex, 3*n * sizeof (int));
			for (i=0; i<3*n; i++)
				sympiIndex[i] = 0;
			}
		else
			sympiIndex = (int *) SafeCalloc (3*n, sizeof (int));
		if (!sympiIndex)
			{
			MrBayesPrint ("%s   Problem allocating sympiIndex\n", spacer);
			return (ERROR);
			}
		else
			memAllocs[ALLOC_SYMPIINDEX] = YES;
		
		/* set up sympi pointers and fill sympiIndex */
		for (k=i=0; k<numParams; k++)
			{
			p = &params[k];
			if (p->nSympi > 0)
				{
                p->printParam = YES;    /* print even if fixed alpha_symdir */
				index = 0;
				p->sympiBsIndex = sympiIndex + i;
				p->sympinStates = sympiIndex + i + n;
				p->sympiCType = sympiIndex + i + (2 * n);
				for (j=0; j<p->nRelParts; j++)
					{						
					m = &modelSettings[p->relParts[j]];
					for (c=0; c<m->numChars; c++)
						{
						if (m->nStates[c] > 2 && (m->cType[c] == UNORD || m->cType[c] == ORD))
							{
							p->sympinStates[index] = m->nStates[c];
							p->sympiBsIndex[index] = m->bsIndex[c];
							p->sympiCType[index] = m->cType[c];
                            origCharPos = origChar[m->compCharStart + c];
                            for (ts=0; ts<m->nStates[c]; ts++)
                                {
                                sprintf(piHeader, "\tpi_%d(%d)", origCharPos+1, ts);
                                SafeStrcat(&p->paramHeader, piHeader);
                                }
							index++;
							}
						}
					}
				assert (index == p->nSympi);
				i += p->nSympi;
				}
			}
        assert (i == n);
		}
	
	/* count space needed for state frequencies */
	for (k=n=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType != P_PI || modelParams[p->relParts[0]].dataType != STANDARD)
			continue;
		n += p->nStdStateFreqs;
		}
	
	stdStateFreqsRowSize = n;
	
	/* allocate space */
	if (memAllocs[ALLOC_STDSTATEFREQS] == YES)
		{
		free (stdStateFreqs);
        stdStateFreqs = NULL;
        memAllocs[ALLOC_STDSTATEFREQS] = NO;
		}
	stdStateFreqs = (MrBFlt *) SafeCalloc (n * 2 * numGlobalChains, sizeof (MrBFlt));
	if (!stdStateFreqs)
		{
		MrBayesPrint ("%s   Problem allocating stdStateFreqs in ProcessStdChars\n", spacer);
		return (ERROR);
		}
	else
		memAllocs[ALLOC_STDSTATEFREQS] = YES;
	
	/* set pointers */
	for (k=n=0; k<numParams; k++)
		{
		p = &params[k];
		if (p->paramType != P_PI || modelParams[p->relParts[0]].dataType != STANDARD)
			continue;
		p->stdStateFreqs = stdStateFreqs + n;
		n += p->nStdStateFreqs;
		}
	
	FillStdStateFreqs( 0 , numGlobalChains, seed);

	return (NO_ERROR);
	
}





/*--------------------------------------------------------------
|
|  FillStdStateFreqs: fills stationary frequencies for standard data divisions of chains  in range [chfrom, chto)
|
---------------------------------------------------------------*/
void FillStdStateFreqs(int chfrom, int chto, SafeLong *seed)
{

	int		chn, n, i, j, k, b, c, nb, index;
	MrBFlt	*subValue, sum, symDir[10];
	Param	*p;

	for (chn=chfrom; chn<chto; chn++)
		{
		for (k=0; k<numParams; k++)
			{
			p = &params[k];
			if (p->paramType != P_PI || modelParams[p->relParts[0]].dataType != STANDARD)
				continue;
			subValue = GetParamStdStateFreqs (p, chn, 0);
			if (p->paramId == SYMPI_EQUAL)
				{
				for (n=index=0; n<9; n++)
					{
					for (i=0; i<p->nRelParts; i++)
						if (modelSettings[p->relParts[i]].isTiNeeded[n] == YES)
							break;
					if (i < p->nRelParts)
						{
						for (j=0; j<(n+2); j++)
							{
							subValue[index++] =  (1.0 / (n + 2));
							}
						}
					}
				for (n=9; n<13; n++)
					{
					for (i=0; i<p->nRelParts; i++)
						if (modelSettings[p->relParts[i]].isTiNeeded[n] == YES)
							break;
					if (i < p->nRelParts)
						{
						for (j=0; j<(n-6); j++)
							{
							subValue[index++] =  (1.0 / (n - 6));
							}
						}
					}
				}

			/* Deal with transition asymmetry for standard characters */
			/* First, fill in stationary frequencies for beta categories if needed; */
			/* discard category frequencies (assume equal) */
			if (p->paramId == SYMPI_FIX || p->paramId == SYMPI_UNI || p->paramId == SYMPI_EXP
				|| p->paramId == SYMPI_FIX_MS || p->paramId == SYMPI_UNI_MS || p->paramId == SYMPI_EXP_MS)
				{
				if (p->hasBinaryStd == YES)
					{
					nb=modelParams[p->relParts[0]].numBetaCats;
					BetaBreaks (p->values[0], p->values[0], subValue, nb);
					b = 2*nb;
					for (i=b-2; i>0; i-=2)
						{
						subValue[i] = subValue[i/2];
						}
					for (i=1; i<b; i+=2)
						{
						subValue[i] =  (1.0 - subValue[i-1]);
						}
					subValue += (2 * nb);
					}
				
				/* Then fill in state frequencies for multistate chars, one set for each */
				for (i=0; i<10; i++)
					symDir[i] = p->values[0];
			
				for (c=0; c<p->nSympi; c++)
					{
					/* now fill in subvalues */
					DirichletRandomVariable (symDir, subValue, p->sympinStates[c], seed);
					sum = 0.0;
					for (i=0; i<p->sympinStates[c]; i++)
						{
						if (subValue[i] < 0.0001)
							subValue[i] =  0.0001;
						sum += subValue[i];
						}
					for (i=0; i<modelParams[p->relParts[0]].nStates; i++)
						subValue[i] /= sum;
					subValue += p->sympinStates[c];
					}
				}		
			}	/* next parameter */
		}	/* next chain */
}





int SetAARates (void)

{

	int			i, j;
	MrBFlt		diff, sum, scaler;
	
	/* A R N D C Q E G H I L K M F P S T W Y V */

	/* jones */
	aaJones[ 0][ 0] =   0; aaJones[ 0][ 1] =  58; aaJones[ 0][ 2] =  54; aaJones[ 0][ 3] =  81; aaJones[ 0][ 4] =  56; 
	aaJones[ 0][ 5] =  57; aaJones[ 0][ 6] = 105; aaJones[ 0][ 7] = 179; aaJones[ 0][ 8] =  27; aaJones[ 0][ 9] =  36; 
	aaJones[ 0][10] =  30; aaJones[ 0][11] =  35; aaJones[ 0][12] =  54; aaJones[ 0][13] =  15; aaJones[ 0][14] = 194; 
	aaJones[ 0][15] = 378; aaJones[ 0][16] = 475; aaJones[ 0][17] =   9; aaJones[ 0][18] =  11; aaJones[ 0][19] = 298; 
	aaJones[ 1][ 0] =  58; aaJones[ 1][ 1] =   0; aaJones[ 1][ 2] =  45; aaJones[ 1][ 3] =  16; aaJones[ 1][ 4] = 113; 
	aaJones[ 1][ 5] = 310; aaJones[ 1][ 6] =  29; aaJones[ 1][ 7] = 137; aaJones[ 1][ 8] = 328; aaJones[ 1][ 9] =  22; 
	aaJones[ 1][10] =  38; aaJones[ 1][11] = 646; aaJones[ 1][12] =  44; aaJones[ 1][13] =   5; aaJones[ 1][14] =  74; 
	aaJones[ 1][15] = 101; aaJones[ 1][16] =  64; aaJones[ 1][17] = 126; aaJones[ 1][18] =  20; aaJones[ 1][19] =  17; 
	aaJones[ 2][ 0] =  54; aaJones[ 2][ 1] =  45; aaJones[ 2][ 2] =   0; aaJones[ 2][ 3] = 528; aaJones[ 2][ 4] =  34; 
	aaJones[ 2][ 5] =  86; aaJones[ 2][ 6] =  58; aaJones[ 2][ 7] =  81; aaJones[ 2][ 8] = 391; aaJones[ 2][ 9] =  47; 
	aaJones[ 2][10] =  12; aaJones[ 2][11] = 263; aaJones[ 2][12] =  30; aaJones[ 2][13] =  10; aaJones[ 2][14] =  15; 
	aaJones[ 2][15] = 503; aaJones[ 2][16] = 232; aaJones[ 2][17] =   8; aaJones[ 2][18] =  70; aaJones[ 2][19] =  16; 
	aaJones[ 3][ 0] =  81; aaJones[ 3][ 1] =  16; aaJones[ 3][ 2] = 528; aaJones[ 3][ 3] =   0; aaJones[ 3][ 4] =  10; 
	aaJones[ 3][ 5] =  49; aaJones[ 3][ 6] = 767; aaJones[ 3][ 7] = 130; aaJones[ 3][ 8] = 112; aaJones[ 3][ 9] =  11; 
	aaJones[ 3][10] =   7; aaJones[ 3][11] =  26; aaJones[ 3][12] =  15; aaJones[ 3][13] =   4; aaJones[ 3][14] =  15; 
	aaJones[ 3][15] =  59; aaJones[ 3][16] =  38; aaJones[ 3][17] =   4; aaJones[ 3][18] =  46; aaJones[ 3][19] =  31; 
	aaJones[ 4][ 0] =  56; aaJones[ 4][ 1] = 113; aaJones[ 4][ 2] =  34; aaJones[ 4][ 3] =  10; aaJones[ 4][ 4] =   0; 
	aaJones[ 4][ 5] =   9; aaJones[ 4][ 6] =   5; aaJones[ 4][ 7] =  59; aaJones[ 4][ 8] =  69; aaJones[ 4][ 9] =  17; 
	aaJones[ 4][10] =  23; aaJones[ 4][11] =   7; aaJones[ 4][12] =  31; aaJones[ 4][13] =  78; aaJones[ 4][14] =  14; 
	aaJones[ 4][15] = 223; aaJones[ 4][16] =  42; aaJones[ 4][17] = 115; aaJones[ 4][18] = 209; aaJones[ 4][19] =  62; 
	aaJones[ 5][ 0] =  57; aaJones[ 5][ 1] = 310; aaJones[ 5][ 2] =  86; aaJones[ 5][ 3] =  49; aaJones[ 5][ 4] =   9; 
	aaJones[ 5][ 5] =   0; aaJones[ 5][ 6] = 323; aaJones[ 5][ 7] =  26; aaJones[ 5][ 8] = 597; aaJones[ 5][ 9] =   9; 
	aaJones[ 5][10] =  72; aaJones[ 5][11] = 292; aaJones[ 5][12] =  43; aaJones[ 5][13] =   4; aaJones[ 5][14] = 164; 
	aaJones[ 5][15] =  53; aaJones[ 5][16] =  51; aaJones[ 5][17] =  18; aaJones[ 5][18] =  24; aaJones[ 5][19] =  20; 
	aaJones[ 6][ 0] = 105; aaJones[ 6][ 1] =  29; aaJones[ 6][ 2] =  58; aaJones[ 6][ 3] = 767; aaJones[ 6][ 4] =   5; 
	aaJones[ 6][ 5] = 323; aaJones[ 6][ 6] =   0; aaJones[ 6][ 7] = 119; aaJones[ 6][ 8] =  26; aaJones[ 6][ 9] =  12; 
	aaJones[ 6][10] =   9; aaJones[ 6][11] = 181; aaJones[ 6][12] =  18; aaJones[ 6][13] =   5; aaJones[ 6][14] =  18; 
	aaJones[ 6][15] =  30; aaJones[ 6][16] =  32; aaJones[ 6][17] =  10; aaJones[ 6][18] =   7; aaJones[ 6][19] =  45; 
	aaJones[ 7][ 0] = 179; aaJones[ 7][ 1] = 137; aaJones[ 7][ 2] =  81; aaJones[ 7][ 3] = 130; aaJones[ 7][ 4] =  59; 
	aaJones[ 7][ 5] =  26; aaJones[ 7][ 6] = 119; aaJones[ 7][ 7] =   0; aaJones[ 7][ 8] =  23; aaJones[ 7][ 9] =   6; 
	aaJones[ 7][10] =   6; aaJones[ 7][11] =  27; aaJones[ 7][12] =  14; aaJones[ 7][13] =   5; aaJones[ 7][14] =  24; 
	aaJones[ 7][15] = 201; aaJones[ 7][16] =  33; aaJones[ 7][17] =  55; aaJones[ 7][18] =   8; aaJones[ 7][19] =  47; 
	aaJones[ 8][ 0] =  27; aaJones[ 8][ 1] = 328; aaJones[ 8][ 2] = 391; aaJones[ 8][ 3] = 112; aaJones[ 8][ 4] =  69; 
	aaJones[ 8][ 5] = 597; aaJones[ 8][ 6] =  26; aaJones[ 8][ 7] =  23; aaJones[ 8][ 8] =   0; aaJones[ 8][ 9] =  16; 
	aaJones[ 8][10] =  56; aaJones[ 8][11] =  45; aaJones[ 8][12] =  33; aaJones[ 8][13] =  40; aaJones[ 8][14] = 115; 
	aaJones[ 8][15] =  73; aaJones[ 8][16] =  46; aaJones[ 8][17] =   8; aaJones[ 8][18] = 573; aaJones[ 8][19] =  11; 
	aaJones[ 9][ 0] =  36; aaJones[ 9][ 1] =  22; aaJones[ 9][ 2] =  47; aaJones[ 9][ 3] =  11; aaJones[ 9][ 4] =  17; 
	aaJones[ 9][ 5] =   9; aaJones[ 9][ 6] =  12; aaJones[ 9][ 7] =   6; aaJones[ 9][ 8] =  16; aaJones[ 9][ 9] =   0; 
	aaJones[ 9][10] = 229; aaJones[ 9][11] =  21; aaJones[ 9][12] = 479; aaJones[ 9][13] =  89; aaJones[ 9][14] =  10; 
	aaJones[ 9][15] =  40; aaJones[ 9][16] = 245; aaJones[ 9][17] =   9; aaJones[ 9][18] =  32; aaJones[ 9][19] = 961; 
	aaJones[10][ 0] =  30; aaJones[10][ 1] =  38; aaJones[10][ 2] =  12; aaJones[10][ 3] =   7; aaJones[10][ 4] =  23; 
	aaJones[10][ 5] =  72; aaJones[10][ 6] =   9; aaJones[10][ 7] =   6; aaJones[10][ 8] =  56; aaJones[10][ 9] = 229; 
	aaJones[10][10] =   0; aaJones[10][11] =  14; aaJones[10][12] = 388; aaJones[10][13] = 248; aaJones[10][14] = 102; 
	aaJones[10][15] =  59; aaJones[10][16] =  25; aaJones[10][17] =  52; aaJones[10][18] =  24; aaJones[10][19] = 180; 
	aaJones[11][ 0] =  35; aaJones[11][ 1] = 646; aaJones[11][ 2] = 263; aaJones[11][ 3] =  26; aaJones[11][ 4] =   7; 
	aaJones[11][ 5] = 292; aaJones[11][ 6] = 181; aaJones[11][ 7] =  27; aaJones[11][ 8] =  45; aaJones[11][ 9] =  21; 
	aaJones[11][10] =  14; aaJones[11][11] =   0; aaJones[11][12] =  65; aaJones[11][13] =   4; aaJones[11][14] =  21; 
	aaJones[11][15] =  47; aaJones[11][16] = 103; aaJones[11][17] =  10; aaJones[11][18] =   8; aaJones[11][19] =  14; 
	aaJones[12][ 0] =  54; aaJones[12][ 1] =  44; aaJones[12][ 2] =  30; aaJones[12][ 3] =  15; aaJones[12][ 4] =  31; 
	aaJones[12][ 5] =  43; aaJones[12][ 6] =  18; aaJones[12][ 7] =  14; aaJones[12][ 8] =  33; aaJones[12][ 9] = 479; 
	aaJones[12][10] = 388; aaJones[12][11] =  65; aaJones[12][12] =   0; aaJones[12][13] =  43; aaJones[12][14] =  16; 
	aaJones[12][15] =  29; aaJones[12][16] = 226; aaJones[12][17] =  24; aaJones[12][18] =  18; aaJones[12][19] = 323; 
	aaJones[13][ 0] =  15; aaJones[13][ 1] =   5; aaJones[13][ 2] =  10; aaJones[13][ 3] =   4; aaJones[13][ 4] =  78; 
	aaJones[13][ 5] =   4; aaJones[13][ 6] =   5; aaJones[13][ 7] =   5; aaJones[13][ 8] =  40; aaJones[13][ 9] =  89; 
	aaJones[13][10] = 248; aaJones[13][11] =   4; aaJones[13][12] =  43; aaJones[13][13] =   0; aaJones[13][14] =  17; 
	aaJones[13][15] =  92; aaJones[13][16] =  12; aaJones[13][17] =  53; aaJones[13][18] = 536; aaJones[13][19] =  62; 
	aaJones[14][ 0] = 194; aaJones[14][ 1] =  74; aaJones[14][ 2] =  15; aaJones[14][ 3] =  15; aaJones[14][ 4] =  14; 
	aaJones[14][ 5] = 164; aaJones[14][ 6] =  18; aaJones[14][ 7] =  24; aaJones[14][ 8] = 115; aaJones[14][ 9] =  10; 
	aaJones[14][10] = 102; aaJones[14][11] =  21; aaJones[14][12] =  16; aaJones[14][13] =  17; aaJones[14][14] =   0; 
	aaJones[14][15] = 285; aaJones[14][16] = 118; aaJones[14][17] =   6; aaJones[14][18] =  10; aaJones[14][19] =  23; 
	aaJones[15][ 0] = 378; aaJones[15][ 1] = 101; aaJones[15][ 2] = 503; aaJones[15][ 3] =  59; aaJones[15][ 4] = 223; 
	aaJones[15][ 5] =  53; aaJones[15][ 6] =  30; aaJones[15][ 7] = 201; aaJones[15][ 8] =  73; aaJones[15][ 9] =  40; 
	aaJones[15][10] =  59; aaJones[15][11] =  47; aaJones[15][12] =  29; aaJones[15][13] =  92; aaJones[15][14] = 285; 
	aaJones[15][15] =   0; aaJones[15][16] = 477; aaJones[15][17] =  35; aaJones[15][18] =  63; aaJones[15][19] =  38; 
	aaJones[16][ 0] = 475; aaJones[16][ 1] =  64; aaJones[16][ 2] = 232; aaJones[16][ 3] =  38; aaJones[16][ 4] =  42; 
	aaJones[16][ 5] =  51; aaJones[16][ 6] =  32; aaJones[16][ 7] =  33; aaJones[16][ 8] =  46; aaJones[16][ 9] = 245; 
	aaJones[16][10] =  25; aaJones[16][11] = 103; aaJones[16][12] = 226; aaJones[16][13] =  12; aaJones[16][14] = 118; 
	aaJones[16][15] = 477; aaJones[16][16] =   0; aaJones[16][17] =  12; aaJones[16][18] =  21; aaJones[16][19] = 112; 
	aaJones[17][ 0] =   9; aaJones[17][ 1] = 126; aaJones[17][ 2] =   8; aaJones[17][ 3] =   4; aaJones[17][ 4] = 115; 
	aaJones[17][ 5] =  18; aaJones[17][ 6] =  10; aaJones[17][ 7] =  55; aaJones[17][ 8] =   8; aaJones[17][ 9] =   9; 
	aaJones[17][10] =  52; aaJones[17][11] =  10; aaJones[17][12] =  24; aaJones[17][13] =  53; aaJones[17][14] =   6; 
	aaJones[17][15] =  35; aaJones[17][16] =  12; aaJones[17][17] =   0; aaJones[17][18] =  71; aaJones[17][19] =  25; 
	aaJones[18][ 0] =  11; aaJones[18][ 1] =  20; aaJones[18][ 2] =  70; aaJones[18][ 3] =  46; aaJones[18][ 4] = 209; 
	aaJones[18][ 5] =  24; aaJones[18][ 6] =   7; aaJones[18][ 7] =   8; aaJones[18][ 8] = 573; aaJones[18][ 9] =  32; 
	aaJones[18][10] =  24; aaJones[18][11] =   8; aaJones[18][12] =  18; aaJones[18][13] = 536; aaJones[18][14] =  10; 
	aaJones[18][15] =  63; aaJones[18][16] =  21; aaJones[18][17] =  71; aaJones[18][18] =   0; aaJones[18][19] =  16; 
	aaJones[19][ 0] = 298; aaJones[19][ 1] =  17; aaJones[19][ 2] =  16; aaJones[19][ 3] =  31; aaJones[19][ 4] =  62; 
	aaJones[19][ 5] =  20; aaJones[19][ 6] =  45; aaJones[19][ 7] =  47; aaJones[19][ 8] =  11; aaJones[19][ 9] = 961; 
	aaJones[19][10] = 180; aaJones[19][11] =  14; aaJones[19][12] = 323; aaJones[19][13] =  62; aaJones[19][14] =  23; 
	aaJones[19][15] =  38; aaJones[19][16] = 112; aaJones[19][17] =  25; aaJones[19][18] =  16; aaJones[19][19] =   0; 

	jonesPi[ 0] = 0.076748;
	jonesPi[ 1] = 0.051691;
	jonesPi[ 2] = 0.042645;
	jonesPi[ 3] = 0.051544;
	jonesPi[ 4] = 0.019803;
	jonesPi[ 5] = 0.040752;
	jonesPi[ 6] = 0.061830;
	jonesPi[ 7] = 0.073152;
	jonesPi[ 8] = 0.022944;
	jonesPi[ 9] = 0.053761;
	jonesPi[10] = 0.091904;
	jonesPi[11] = 0.058676;
	jonesPi[12] = 0.023826;
	jonesPi[13] = 0.040126;
	jonesPi[14] = 0.050901;
	jonesPi[15] = 0.068765;
	jonesPi[16] = 0.058565;
	jonesPi[17] = 0.014261;
	jonesPi[18] = 0.032102;
	jonesPi[19] = 0.066005;

	/* dayhoff */
	aaDayhoff[ 0][ 0] =   0; aaDayhoff[ 0][ 1] =  27; aaDayhoff[ 0][ 2] =  98; aaDayhoff[ 0][ 3] = 120; aaDayhoff[ 0][ 4] =  36; 
	aaDayhoff[ 0][ 5] =  89; aaDayhoff[ 0][ 6] = 198; aaDayhoff[ 0][ 7] = 240; aaDayhoff[ 0][ 8] =  23; aaDayhoff[ 0][ 9] =  65; 
	aaDayhoff[ 0][10] =  41; aaDayhoff[ 0][11] =  26; aaDayhoff[ 0][12] =  72; aaDayhoff[ 0][13] =  18; aaDayhoff[ 0][14] = 250; 
	aaDayhoff[ 0][15] = 409; aaDayhoff[ 0][16] = 371; aaDayhoff[ 0][17] =   0; aaDayhoff[ 0][18] =  24; aaDayhoff[ 0][19] = 208; 
	aaDayhoff[ 1][ 0] =  27; aaDayhoff[ 1][ 1] =   0; aaDayhoff[ 1][ 2] =  32; aaDayhoff[ 1][ 3] =   0; aaDayhoff[ 1][ 4] =  23; 
	aaDayhoff[ 1][ 5] = 246; aaDayhoff[ 1][ 6] =   1; aaDayhoff[ 1][ 7] =   9; aaDayhoff[ 1][ 8] = 240; aaDayhoff[ 1][ 9] =  64; 
	aaDayhoff[ 1][10] =  15; aaDayhoff[ 1][11] = 464; aaDayhoff[ 1][12] =  90; aaDayhoff[ 1][13] =  14; aaDayhoff[ 1][14] = 103; 
	aaDayhoff[ 1][15] = 154; aaDayhoff[ 1][16] =  26; aaDayhoff[ 1][17] = 201; aaDayhoff[ 1][18] =   8; aaDayhoff[ 1][19] =  24; 
	aaDayhoff[ 2][ 0] =  98; aaDayhoff[ 2][ 1] =  32; aaDayhoff[ 2][ 2] =   0; aaDayhoff[ 2][ 3] = 905; aaDayhoff[ 2][ 4] =   0; 
	aaDayhoff[ 2][ 5] = 103; aaDayhoff[ 2][ 6] = 148; aaDayhoff[ 2][ 7] = 139; aaDayhoff[ 2][ 8] = 535; aaDayhoff[ 2][ 9] =  77; 
	aaDayhoff[ 2][10] =  34; aaDayhoff[ 2][11] = 318; aaDayhoff[ 2][12] =   1; aaDayhoff[ 2][13] =  14; aaDayhoff[ 2][14] =  42; 
	aaDayhoff[ 2][15] = 495; aaDayhoff[ 2][16] = 229; aaDayhoff[ 2][17] =  23; aaDayhoff[ 2][18] =  95; aaDayhoff[ 2][19] =  15; 
	aaDayhoff[ 3][ 0] = 120; aaDayhoff[ 3][ 1] =   0; aaDayhoff[ 3][ 2] = 905; aaDayhoff[ 3][ 3] =   0; aaDayhoff[ 3][ 4] =   0; 
	aaDayhoff[ 3][ 5] = 134; aaDayhoff[ 3][ 6] = 1153; aaDayhoff[ 3][ 7] = 125; aaDayhoff[ 3][ 8] =  86; aaDayhoff[ 3][ 9] =  24; 
	aaDayhoff[ 3][10] =   0; aaDayhoff[ 3][11] =  71; aaDayhoff[ 3][12] =   0; aaDayhoff[ 3][13] =   0; aaDayhoff[ 3][14] =  13; 
	aaDayhoff[ 3][15] =  95; aaDayhoff[ 3][16] =  66; aaDayhoff[ 3][17] =   0; aaDayhoff[ 3][18] =   0; aaDayhoff[ 3][19] =  18; 
	aaDayhoff[ 4][ 0] =  36; aaDayhoff[ 4][ 1] =  23; aaDayhoff[ 4][ 2] =   0; aaDayhoff[ 4][ 3] =   0; aaDayhoff[ 4][ 4] =   0; 
	aaDayhoff[ 4][ 5] =   0; aaDayhoff[ 4][ 6] =   0; aaDayhoff[ 4][ 7] =  11; aaDayhoff[ 4][ 8] =  28; aaDayhoff[ 4][ 9] =  44; 
	aaDayhoff[ 4][10] =   0; aaDayhoff[ 4][11] =   0; aaDayhoff[ 4][12] =   0; aaDayhoff[ 4][13] =   0; aaDayhoff[ 4][14] =  19; 
	aaDayhoff[ 4][15] = 161; aaDayhoff[ 4][16] =  16; aaDayhoff[ 4][17] =   0; aaDayhoff[ 4][18] =  96; aaDayhoff[ 4][19] =  49; 
	aaDayhoff[ 5][ 0] =  89; aaDayhoff[ 5][ 1] = 246; aaDayhoff[ 5][ 2] = 103; aaDayhoff[ 5][ 3] = 134; aaDayhoff[ 5][ 4] =   0; 
	aaDayhoff[ 5][ 5] =   0; aaDayhoff[ 5][ 6] = 716; aaDayhoff[ 5][ 7] =  28; aaDayhoff[ 5][ 8] = 606; aaDayhoff[ 5][ 9] =  18; 
	aaDayhoff[ 5][10] =  73; aaDayhoff[ 5][11] = 153; aaDayhoff[ 5][12] = 114; aaDayhoff[ 5][13] =   0; aaDayhoff[ 5][14] = 153; 
	aaDayhoff[ 5][15] =  56; aaDayhoff[ 5][16] =  53; aaDayhoff[ 5][17] =   0; aaDayhoff[ 5][18] =   0; aaDayhoff[ 5][19] =  35; 
	aaDayhoff[ 6][ 0] = 198; aaDayhoff[ 6][ 1] =   1; aaDayhoff[ 6][ 2] = 148; aaDayhoff[ 6][ 3] = 1153; aaDayhoff[ 6][ 4] =   0; 
	aaDayhoff[ 6][ 5] = 716; aaDayhoff[ 6][ 6] =   0; aaDayhoff[ 6][ 7] =  81; aaDayhoff[ 6][ 8] =  43; aaDayhoff[ 6][ 9] =  61; 
	aaDayhoff[ 6][10] =  11; aaDayhoff[ 6][11] =  83; aaDayhoff[ 6][12] =  30; aaDayhoff[ 6][13] =   0; aaDayhoff[ 6][14] =  51; 
	aaDayhoff[ 6][15] =  79; aaDayhoff[ 6][16] =  34; aaDayhoff[ 6][17] =   0; aaDayhoff[ 6][18] =  22; aaDayhoff[ 6][19] =  37; 
	aaDayhoff[ 7][ 0] = 240; aaDayhoff[ 7][ 1] =   9; aaDayhoff[ 7][ 2] = 139; aaDayhoff[ 7][ 3] = 125; aaDayhoff[ 7][ 4] =  11; 
	aaDayhoff[ 7][ 5] =  28; aaDayhoff[ 7][ 6] =  81; aaDayhoff[ 7][ 7] =   0; aaDayhoff[ 7][ 8] =  10; aaDayhoff[ 7][ 9] =   0; 
	aaDayhoff[ 7][10] =   7; aaDayhoff[ 7][11] =  27; aaDayhoff[ 7][12] =  17; aaDayhoff[ 7][13] =  15; aaDayhoff[ 7][14] =  34; 
	aaDayhoff[ 7][15] = 234; aaDayhoff[ 7][16] =  30; aaDayhoff[ 7][17] =   0; aaDayhoff[ 7][18] =   0; aaDayhoff[ 7][19] =  54; 
	aaDayhoff[ 8][ 0] =  23; aaDayhoff[ 8][ 1] = 240; aaDayhoff[ 8][ 2] = 535; aaDayhoff[ 8][ 3] =  86; aaDayhoff[ 8][ 4] =  28; 
	aaDayhoff[ 8][ 5] = 606; aaDayhoff[ 8][ 6] =  43; aaDayhoff[ 8][ 7] =  10; aaDayhoff[ 8][ 8] =   0; aaDayhoff[ 8][ 9] =   7; 
	aaDayhoff[ 8][10] =  44; aaDayhoff[ 8][11] =  26; aaDayhoff[ 8][12] =   0; aaDayhoff[ 8][13] =  48; aaDayhoff[ 8][14] =  94; 
	aaDayhoff[ 8][15] =  35; aaDayhoff[ 8][16] =  22; aaDayhoff[ 8][17] =  27; aaDayhoff[ 8][18] = 127; aaDayhoff[ 8][19] =  44; 
	aaDayhoff[ 9][ 0] =  65; aaDayhoff[ 9][ 1] =  64; aaDayhoff[ 9][ 2] =  77; aaDayhoff[ 9][ 3] =  24; aaDayhoff[ 9][ 4] =  44; 
	aaDayhoff[ 9][ 5] =  18; aaDayhoff[ 9][ 6] =  61; aaDayhoff[ 9][ 7] =   0; aaDayhoff[ 9][ 8] =   7; aaDayhoff[ 9][ 9] =   0; 
	aaDayhoff[ 9][10] = 257; aaDayhoff[ 9][11] =  46; aaDayhoff[ 9][12] = 336; aaDayhoff[ 9][13] = 196; aaDayhoff[ 9][14] =  12; 
	aaDayhoff[ 9][15] =  24; aaDayhoff[ 9][16] = 192; aaDayhoff[ 9][17] =   0; aaDayhoff[ 9][18] =  37; aaDayhoff[ 9][19] = 889; 
	aaDayhoff[10][ 0] =  41; aaDayhoff[10][ 1] =  15; aaDayhoff[10][ 2] =  34; aaDayhoff[10][ 3] =   0; aaDayhoff[10][ 4] =   0; 
	aaDayhoff[10][ 5] =  73; aaDayhoff[10][ 6] =  11; aaDayhoff[10][ 7] =   7; aaDayhoff[10][ 8] =  44; aaDayhoff[10][ 9] = 257; 
	aaDayhoff[10][10] =   0; aaDayhoff[10][11] =  18; aaDayhoff[10][12] = 527; aaDayhoff[10][13] = 157; aaDayhoff[10][14] =  32; 
	aaDayhoff[10][15] =  17; aaDayhoff[10][16] =  33; aaDayhoff[10][17] =  46; aaDayhoff[10][18] =  28; aaDayhoff[10][19] = 175; 
	aaDayhoff[11][ 0] =  26; aaDayhoff[11][ 1] = 464; aaDayhoff[11][ 2] = 318; aaDayhoff[11][ 3] =  71; aaDayhoff[11][ 4] =   0; 
	aaDayhoff[11][ 5] = 153; aaDayhoff[11][ 6] =  83; aaDayhoff[11][ 7] =  27; aaDayhoff[11][ 8] =  26; aaDayhoff[11][ 9] =  46; 
	aaDayhoff[11][10] =  18; aaDayhoff[11][11] =   0; aaDayhoff[11][12] = 243; aaDayhoff[11][13] =   0; aaDayhoff[11][14] =  33; 
	aaDayhoff[11][15] =  96; aaDayhoff[11][16] = 136; aaDayhoff[11][17] =   0; aaDayhoff[11][18] =  13; aaDayhoff[11][19] =  10; 
	aaDayhoff[12][ 0] =  72; aaDayhoff[12][ 1] =  90; aaDayhoff[12][ 2] =   1; aaDayhoff[12][ 3] =   0; aaDayhoff[12][ 4] =   0; 
	aaDayhoff[12][ 5] = 114; aaDayhoff[12][ 6] =  30; aaDayhoff[12][ 7] =  17; aaDayhoff[12][ 8] =   0; aaDayhoff[12][ 9] = 336; 
	aaDayhoff[12][10] = 527; aaDayhoff[12][11] = 243; aaDayhoff[12][12] =   0; aaDayhoff[12][13] =  92; aaDayhoff[12][14] =  17; 
	aaDayhoff[12][15] =  62; aaDayhoff[12][16] = 104; aaDayhoff[12][17] =   0; aaDayhoff[12][18] =   0; aaDayhoff[12][19] = 258; 
	aaDayhoff[13][ 0] =  18; aaDayhoff[13][ 1] =  14; aaDayhoff[13][ 2] =  14; aaDayhoff[13][ 3] =   0; aaDayhoff[13][ 4] =   0; 
	aaDayhoff[13][ 5] =   0; aaDayhoff[13][ 6] =   0; aaDayhoff[13][ 7] =  15; aaDayhoff[13][ 8] =  48; aaDayhoff[13][ 9] = 196; 
	aaDayhoff[13][10] = 157; aaDayhoff[13][11] =   0; aaDayhoff[13][12] =  92; aaDayhoff[13][13] =   0; aaDayhoff[13][14] =  11; 
	aaDayhoff[13][15] =  46; aaDayhoff[13][16] =  13; aaDayhoff[13][17] =  76; aaDayhoff[13][18] = 698; aaDayhoff[13][19] =  12; 
	aaDayhoff[14][ 0] = 250; aaDayhoff[14][ 1] = 103; aaDayhoff[14][ 2] =  42; aaDayhoff[14][ 3] =  13; aaDayhoff[14][ 4] =  19; 
	aaDayhoff[14][ 5] = 153; aaDayhoff[14][ 6] =  51; aaDayhoff[14][ 7] =  34; aaDayhoff[14][ 8] =  94; aaDayhoff[14][ 9] =  12; 
	aaDayhoff[14][10] =  32; aaDayhoff[14][11] =  33; aaDayhoff[14][12] =  17; aaDayhoff[14][13] =  11; aaDayhoff[14][14] =   0; 
	aaDayhoff[14][15] = 245; aaDayhoff[14][16] =  78; aaDayhoff[14][17] =   0; aaDayhoff[14][18] =   0; aaDayhoff[14][19] =  48; 
	aaDayhoff[15][ 0] = 409; aaDayhoff[15][ 1] = 154; aaDayhoff[15][ 2] = 495; aaDayhoff[15][ 3] =  95; aaDayhoff[15][ 4] = 161; 
	aaDayhoff[15][ 5] =  56; aaDayhoff[15][ 6] =  79; aaDayhoff[15][ 7] = 234; aaDayhoff[15][ 8] =  35; aaDayhoff[15][ 9] =  24; 
	aaDayhoff[15][10] =  17; aaDayhoff[15][11] =  96; aaDayhoff[15][12] =  62; aaDayhoff[15][13] =  46; aaDayhoff[15][14] = 245; 
	aaDayhoff[15][15] =   0; aaDayhoff[15][16] = 550; aaDayhoff[15][17] =  75; aaDayhoff[15][18] =  34; aaDayhoff[15][19] =  30; 
	aaDayhoff[16][ 0] = 371; aaDayhoff[16][ 1] =  26; aaDayhoff[16][ 2] = 229; aaDayhoff[16][ 3] =  66; aaDayhoff[16][ 4] =  16; 
	aaDayhoff[16][ 5] =  53; aaDayhoff[16][ 6] =  34; aaDayhoff[16][ 7] =  30; aaDayhoff[16][ 8] =  22; aaDayhoff[16][ 9] = 192; 
	aaDayhoff[16][10] =  33; aaDayhoff[16][11] = 136; aaDayhoff[16][12] = 104; aaDayhoff[16][13] =  13; aaDayhoff[16][14] =  78; 
	aaDayhoff[16][15] = 550; aaDayhoff[16][16] =   0; aaDayhoff[16][17] =   0; aaDayhoff[16][18] =  42; aaDayhoff[16][19] = 157; 
	aaDayhoff[17][ 0] =   0; aaDayhoff[17][ 1] = 201; aaDayhoff[17][ 2] =  23; aaDayhoff[17][ 3] =   0; aaDayhoff[17][ 4] =   0; 
	aaDayhoff[17][ 5] =   0; aaDayhoff[17][ 6] =   0; aaDayhoff[17][ 7] =   0; aaDayhoff[17][ 8] =  27; aaDayhoff[17][ 9] =   0; 
	aaDayhoff[17][10] =  46; aaDayhoff[17][11] =   0; aaDayhoff[17][12] =   0; aaDayhoff[17][13] =  76; aaDayhoff[17][14] =   0; 
	aaDayhoff[17][15] =  75; aaDayhoff[17][16] =   0; aaDayhoff[17][17] =   0; aaDayhoff[17][18] =  61; aaDayhoff[17][19] =   0; 
	aaDayhoff[18][ 0] =  24; aaDayhoff[18][ 1] =   8; aaDayhoff[18][ 2] =  95; aaDayhoff[18][ 3] =   0; aaDayhoff[18][ 4] =  96; 
	aaDayhoff[18][ 5] =   0; aaDayhoff[18][ 6] =  22; aaDayhoff[18][ 7] =   0; aaDayhoff[18][ 8] = 127; aaDayhoff[18][ 9] =  37; 
	aaDayhoff[18][10] =  28; aaDayhoff[18][11] =  13; aaDayhoff[18][12] =   0; aaDayhoff[18][13] = 698; aaDayhoff[18][14] =   0; 
	aaDayhoff[18][15] =  34; aaDayhoff[18][16] =  42; aaDayhoff[18][17] =  61; aaDayhoff[18][18] =   0; aaDayhoff[18][19] =  28; 
	aaDayhoff[19][ 0] = 208; aaDayhoff[19][ 1] =  24; aaDayhoff[19][ 2] =  15; aaDayhoff[19][ 3] =  18; aaDayhoff[19][ 4] =  49; 
	aaDayhoff[19][ 5] =  35; aaDayhoff[19][ 6] =  37; aaDayhoff[19][ 7] =  54; aaDayhoff[19][ 8] =  44; aaDayhoff[19][ 9] = 889; 
	aaDayhoff[19][10] = 175; aaDayhoff[19][11] =  10; aaDayhoff[19][12] = 258; aaDayhoff[19][13] =  12; aaDayhoff[19][14] =  48; 
	aaDayhoff[19][15] =  30; aaDayhoff[19][16] = 157; aaDayhoff[19][17] =   0; aaDayhoff[19][18] =  28; aaDayhoff[19][19] =   0;

	dayhoffPi[ 0] = 0.087127;
	dayhoffPi[ 1] = 0.040904;
	dayhoffPi[ 2] = 0.040432;
	dayhoffPi[ 3] = 0.046872;
	dayhoffPi[ 4] = 0.033474;
	dayhoffPi[ 5] = 0.038255;
	dayhoffPi[ 6] = 0.049530;
	dayhoffPi[ 7] = 0.088612;
	dayhoffPi[ 8] = 0.033618;
	dayhoffPi[ 9] = 0.036886;
	dayhoffPi[10] = 0.085357;
	dayhoffPi[11] = 0.080482;
	dayhoffPi[12] = 0.014753;
	dayhoffPi[13] = 0.039772;
	dayhoffPi[14] = 0.050680;
	dayhoffPi[15] = 0.069577;
	dayhoffPi[16] = 0.058542;
	dayhoffPi[17] = 0.010494;
	dayhoffPi[18] = 0.029916;
	dayhoffPi[19] = 0.064718;

	/* mtrev24 */
	aaMtrev24[ 0][ 0] =   0.00; aaMtrev24[ 0][ 1] =  23.18; aaMtrev24[ 0][ 2] =  26.95; aaMtrev24[ 0][ 3] =  17.67; aaMtrev24[ 0][ 4] =  59.93;
	aaMtrev24[ 0][ 5] =   1.90; aaMtrev24[ 0][ 6] =   9.77; aaMtrev24[ 0][ 7] = 120.71; aaMtrev24[ 0][ 8] =  13.90; aaMtrev24[ 0][ 9] =  96.49;
	aaMtrev24[ 0][10] =  25.46; aaMtrev24[ 0][11] =   8.36; aaMtrev24[ 0][12] = 141.88; aaMtrev24[ 0][13] =   6.37; aaMtrev24[ 0][14] =  54.31;
	aaMtrev24[ 0][15] = 387.86; aaMtrev24[ 0][16] = 480.72; aaMtrev24[ 0][17] =   1.90; aaMtrev24[ 0][18] =   6.48; aaMtrev24[ 0][19] = 195.06;
	aaMtrev24[ 1][ 0] =  23.18; aaMtrev24[ 1][ 1] =   0.00; aaMtrev24[ 1][ 2] =  13.24; aaMtrev24[ 1][ 3] =   1.90; aaMtrev24[ 1][ 4] = 103.33;
	aaMtrev24[ 1][ 5] = 220.99; aaMtrev24[ 1][ 6] =   1.90; aaMtrev24[ 1][ 7] =  23.03; aaMtrev24[ 1][ 8] = 165.23; aaMtrev24[ 1][ 9] =   1.90;
	aaMtrev24[ 1][10] =  15.58; aaMtrev24[ 1][11] = 141.40; aaMtrev24[ 1][12] =   1.90; aaMtrev24[ 1][13] =   4.69; aaMtrev24[ 1][14] =  23.64;
	aaMtrev24[ 1][15] =   6.04; aaMtrev24[ 1][16] =   2.08; aaMtrev24[ 1][17] =  21.95; aaMtrev24[ 1][18] =   1.90; aaMtrev24[ 1][19] =   7.64;
	aaMtrev24[ 2][ 0] =  26.95; aaMtrev24[ 2][ 1] =  13.24; aaMtrev24[ 2][ 2] =   0.00; aaMtrev24[ 2][ 3] = 794.38; aaMtrev24[ 2][ 4] =  58.94;
	aaMtrev24[ 2][ 5] = 173.56; aaMtrev24[ 2][ 6] =  63.05; aaMtrev24[ 2][ 7] =  53.30; aaMtrev24[ 2][ 8] = 496.13; aaMtrev24[ 2][ 9] =  27.10;
	aaMtrev24[ 2][10] =  15.16; aaMtrev24[ 2][11] = 608.70; aaMtrev24[ 2][12] =  65.41; aaMtrev24[ 2][13] =  15.20; aaMtrev24[ 2][14] =  73.31;
	aaMtrev24[ 2][15] = 494.39; aaMtrev24[ 2][16] = 238.46; aaMtrev24[ 2][17] =  10.68; aaMtrev24[ 2][18] = 191.36; aaMtrev24[ 2][19] =   1.90;
	aaMtrev24[ 3][ 0] =  17.67; aaMtrev24[ 3][ 1] =   1.90; aaMtrev24[ 3][ 2] = 794.38; aaMtrev24[ 3][ 3] =   0.00; aaMtrev24[ 3][ 4] =   1.90;
	aaMtrev24[ 3][ 5] =  55.28; aaMtrev24[ 3][ 6] = 583.55; aaMtrev24[ 3][ 7] =  56.77; aaMtrev24[ 3][ 8] = 113.99; aaMtrev24[ 3][ 9] =   4.34;
	aaMtrev24[ 3][10] =   1.90; aaMtrev24[ 3][11] =   2.31; aaMtrev24[ 3][12] =   1.90; aaMtrev24[ 3][13] =   4.98; aaMtrev24[ 3][14] =  13.43;
	aaMtrev24[ 3][15] =  69.02; aaMtrev24[ 3][16] =  28.01; aaMtrev24[ 3][17] =  19.86; aaMtrev24[ 3][18] =  21.21; aaMtrev24[ 3][19] =   1.90;
	aaMtrev24[ 4][ 0] =  59.93; aaMtrev24[ 4][ 1] = 103.33; aaMtrev24[ 4][ 2] =  58.94; aaMtrev24[ 4][ 3] =   1.90; aaMtrev24[ 4][ 4] =   0.00;
	aaMtrev24[ 4][ 5] =  75.24; aaMtrev24[ 4][ 6] =   1.90; aaMtrev24[ 4][ 7] =  30.71; aaMtrev24[ 4][ 8] = 141.49; aaMtrev24[ 4][ 9] =  62.73;
	aaMtrev24[ 4][10] =  25.65; aaMtrev24[ 4][11] =   1.90; aaMtrev24[ 4][12] =   6.18; aaMtrev24[ 4][13] =  70.80; aaMtrev24[ 4][14] =  31.26;
	aaMtrev24[ 4][15] = 277.05; aaMtrev24[ 4][16] = 179.97; aaMtrev24[ 4][17] =  33.60; aaMtrev24[ 4][18] = 254.77; aaMtrev24[ 4][19] =   1.90;
	aaMtrev24[ 5][ 0] =   1.90; aaMtrev24[ 5][ 1] = 220.99; aaMtrev24[ 5][ 2] = 173.56; aaMtrev24[ 5][ 3] =  55.28; aaMtrev24[ 5][ 4] =  75.24;
	aaMtrev24[ 5][ 5] =   0.00; aaMtrev24[ 5][ 6] = 313.56; aaMtrev24[ 5][ 7] =   6.75; aaMtrev24[ 5][ 8] = 582.40; aaMtrev24[ 5][ 9] =   8.34;
	aaMtrev24[ 5][10] =  39.70; aaMtrev24[ 5][11] = 465.58; aaMtrev24[ 5][12] =  47.37; aaMtrev24[ 5][13] =  19.11; aaMtrev24[ 5][14] = 137.29;
	aaMtrev24[ 5][15] =  54.11; aaMtrev24[ 5][16] =  94.93; aaMtrev24[ 5][17] =   1.90; aaMtrev24[ 5][18] =  38.82; aaMtrev24[ 5][19] =  19.00;
	aaMtrev24[ 6][ 0] =   9.77; aaMtrev24[ 6][ 1] =   1.90; aaMtrev24[ 6][ 2] =  63.05; aaMtrev24[ 6][ 3] = 583.55; aaMtrev24[ 6][ 4] =   1.90;
	aaMtrev24[ 6][ 5] = 313.56; aaMtrev24[ 6][ 6] =   0.00; aaMtrev24[ 6][ 7] =  28.28; aaMtrev24[ 6][ 8] =  49.12; aaMtrev24[ 6][ 9] =   3.31;
	aaMtrev24[ 6][10] =   1.90; aaMtrev24[ 6][11] = 313.86; aaMtrev24[ 6][12] =   1.90; aaMtrev24[ 6][13] =   2.67; aaMtrev24[ 6][14] =  12.83;
	aaMtrev24[ 6][15] =  54.71; aaMtrev24[ 6][16] =  14.82; aaMtrev24[ 6][17] =   1.90; aaMtrev24[ 6][18] =  13.12; aaMtrev24[ 6][19] =  21.14;
	aaMtrev24[ 7][ 0] = 120.71; aaMtrev24[ 7][ 1] =  23.03; aaMtrev24[ 7][ 2] =  53.30; aaMtrev24[ 7][ 3] =  56.77; aaMtrev24[ 7][ 4] =  30.71;
	aaMtrev24[ 7][ 5] =   6.75; aaMtrev24[ 7][ 6] =  28.28; aaMtrev24[ 7][ 7] =   0.00; aaMtrev24[ 7][ 8] =   1.90; aaMtrev24[ 7][ 9] =   5.98;
	aaMtrev24[ 7][10] =   2.41; aaMtrev24[ 7][11] =  22.73; aaMtrev24[ 7][12] =   1.90; aaMtrev24[ 7][13] =   1.90; aaMtrev24[ 7][14] =   1.90;
	aaMtrev24[ 7][15] = 125.93; aaMtrev24[ 7][16] =  11.17; aaMtrev24[ 7][17] =  10.92; aaMtrev24[ 7][18] =   3.21; aaMtrev24[ 7][19] =   2.53;
	aaMtrev24[ 8][ 0] =  13.90; aaMtrev24[ 8][ 1] = 165.23; aaMtrev24[ 8][ 2] = 496.13; aaMtrev24[ 8][ 3] = 113.99; aaMtrev24[ 8][ 4] = 141.49;
	aaMtrev24[ 8][ 5] = 582.40; aaMtrev24[ 8][ 6] =  49.12; aaMtrev24[ 8][ 7] =   1.90; aaMtrev24[ 8][ 8] =   0.00; aaMtrev24[ 8][ 9] =  12.26;
	aaMtrev24[ 8][10] =  11.49; aaMtrev24[ 8][11] = 127.67; aaMtrev24[ 8][12] =  11.97; aaMtrev24[ 8][13] =  48.16; aaMtrev24[ 8][14] =  60.97;
	aaMtrev24[ 8][15] =  77.46; aaMtrev24[ 8][16] =  44.78; aaMtrev24[ 8][17] =   7.08; aaMtrev24[ 8][18] = 670.14; aaMtrev24[ 8][19] =   1.90;
	aaMtrev24[ 9][ 0] =  96.49; aaMtrev24[ 9][ 1] =   1.90; aaMtrev24[ 9][ 2] =  27.10; aaMtrev24[ 9][ 3] =   4.34; aaMtrev24[ 9][ 4] =  62.73;
	aaMtrev24[ 9][ 5] =   8.34; aaMtrev24[ 9][ 6] =   3.31; aaMtrev24[ 9][ 7] =   5.98; aaMtrev24[ 9][ 8] =  12.26; aaMtrev24[ 9][ 9] =   0.00;
	aaMtrev24[ 9][10] = 329.09; aaMtrev24[ 9][11] =  19.57; aaMtrev24[ 9][12] = 517.98; aaMtrev24[ 9][13] =  84.67; aaMtrev24[ 9][14] =  20.63;
	aaMtrev24[ 9][15] =  47.70; aaMtrev24[ 9][16] = 368.43; aaMtrev24[ 9][17] =   1.90; aaMtrev24[ 9][18] =  25.01; aaMtrev24[ 9][19] =1222.94;
	aaMtrev24[10][ 0] =  25.46; aaMtrev24[10][ 1] =  15.58; aaMtrev24[10][ 2] =  15.16; aaMtrev24[10][ 3] =   1.90; aaMtrev24[10][ 4] =  25.65;
	aaMtrev24[10][ 5] =  39.70; aaMtrev24[10][ 6] =   1.90; aaMtrev24[10][ 7] =   2.41; aaMtrev24[10][ 8] =  11.49; aaMtrev24[10][ 9] = 329.09;
	aaMtrev24[10][10] =   0.00; aaMtrev24[10][11] =  14.88; aaMtrev24[10][12] = 537.53; aaMtrev24[10][13] = 216.06; aaMtrev24[10][14] =  40.10;
	aaMtrev24[10][15] =  73.61; aaMtrev24[10][16] = 126.40; aaMtrev24[10][17] =  32.44; aaMtrev24[10][18] =  44.15; aaMtrev24[10][19] =  91.67;
	aaMtrev24[11][ 0] =   8.36; aaMtrev24[11][ 1] = 141.40; aaMtrev24[11][ 2] = 608.70; aaMtrev24[11][ 3] =   2.31; aaMtrev24[11][ 4] =   1.90;
	aaMtrev24[11][ 5] = 465.58; aaMtrev24[11][ 6] = 313.86; aaMtrev24[11][ 7] =  22.73; aaMtrev24[11][ 8] = 127.67; aaMtrev24[11][ 9] =  19.57;
	aaMtrev24[11][10] =  14.88; aaMtrev24[11][11] =   0.00; aaMtrev24[11][12] =  91.37; aaMtrev24[11][13] =   6.44; aaMtrev24[11][14] =  50.10;
	aaMtrev24[11][15] = 105.79; aaMtrev24[11][16] = 136.33; aaMtrev24[11][17] =  24.00; aaMtrev24[11][18] =  51.17; aaMtrev24[11][19] =   1.90;
	aaMtrev24[12][ 0] = 141.88; aaMtrev24[12][ 1] =   1.90; aaMtrev24[12][ 2] =  65.41; aaMtrev24[12][ 3] =   1.90; aaMtrev24[12][ 4] =   6.18;
	aaMtrev24[12][ 5] =  47.37; aaMtrev24[12][ 6] =   1.90; aaMtrev24[12][ 7] =   1.90; aaMtrev24[12][ 8] =  11.97; aaMtrev24[12][ 9] = 517.98;
	aaMtrev24[12][10] = 537.53; aaMtrev24[12][11] =  91.37; aaMtrev24[12][12] =   0.00; aaMtrev24[12][13] =  90.82; aaMtrev24[12][14] =  18.84;
	aaMtrev24[12][15] = 111.16; aaMtrev24[12][16] = 528.17; aaMtrev24[12][17] =  21.71; aaMtrev24[12][18] =  39.96; aaMtrev24[12][19] = 387.54;
	aaMtrev24[13][ 0] =   6.37; aaMtrev24[13][ 1] =   4.69; aaMtrev24[13][ 2] =  15.20; aaMtrev24[13][ 3] =   4.98; aaMtrev24[13][ 4] =  70.80;
	aaMtrev24[13][ 5] =  19.11; aaMtrev24[13][ 6] =   2.67; aaMtrev24[13][ 7] =   1.90; aaMtrev24[13][ 8] =  48.16; aaMtrev24[13][ 9] =  84.67;
	aaMtrev24[13][10] = 216.06; aaMtrev24[13][11] =   6.44; aaMtrev24[13][12] =  90.82; aaMtrev24[13][13] =   0.00; aaMtrev24[13][14] =  17.31;
	aaMtrev24[13][15] =  64.29; aaMtrev24[13][16] =  33.85; aaMtrev24[13][17] =   7.84; aaMtrev24[13][18] = 465.58; aaMtrev24[13][19] =   6.35;
	aaMtrev24[14][ 0] =  54.31; aaMtrev24[14][ 1] =  23.64; aaMtrev24[14][ 2] =  73.31; aaMtrev24[14][ 3] =  13.43; aaMtrev24[14][ 4] =  31.26;
	aaMtrev24[14][ 5] = 137.29; aaMtrev24[14][ 6] =  12.83; aaMtrev24[14][ 7] =   1.90; aaMtrev24[14][ 8] =  60.97; aaMtrev24[14][ 9] =  20.63;
	aaMtrev24[14][10] =  40.10; aaMtrev24[14][11] =  50.10; aaMtrev24[14][12] =  18.84; aaMtrev24[14][13] =  17.31; aaMtrev24[14][14] =   0.00;
	aaMtrev24[14][15] = 169.90; aaMtrev24[14][16] = 128.22; aaMtrev24[14][17] =   4.21; aaMtrev24[14][18] =  16.21; aaMtrev24[14][19] =   8.23;
	aaMtrev24[15][ 0] = 387.86; aaMtrev24[15][ 1] =   6.04; aaMtrev24[15][ 2] = 494.39; aaMtrev24[15][ 3] =  69.02; aaMtrev24[15][ 4] = 277.05;
	aaMtrev24[15][ 5] =  54.11; aaMtrev24[15][ 6] =  54.71; aaMtrev24[15][ 7] = 125.93; aaMtrev24[15][ 8] =  77.46; aaMtrev24[15][ 9] =  47.70;
	aaMtrev24[15][10] =  73.61; aaMtrev24[15][11] = 105.79; aaMtrev24[15][12] = 111.16; aaMtrev24[15][13] =  64.29; aaMtrev24[15][14] = 169.90;
	aaMtrev24[15][15] =   0.00; aaMtrev24[15][16] = 597.21; aaMtrev24[15][17] =  38.58; aaMtrev24[15][18] =  64.92; aaMtrev24[15][19] =   1.90;
	aaMtrev24[16][ 0] = 480.72; aaMtrev24[16][ 1] =   2.08; aaMtrev24[16][ 2] = 238.46; aaMtrev24[16][ 3] =  28.01; aaMtrev24[16][ 4] = 179.97;
	aaMtrev24[16][ 5] =  94.93; aaMtrev24[16][ 6] =  14.82; aaMtrev24[16][ 7] =  11.17; aaMtrev24[16][ 8] =  44.78; aaMtrev24[16][ 9] = 368.43;
	aaMtrev24[16][10] = 126.40; aaMtrev24[16][11] = 136.33; aaMtrev24[16][12] = 528.17; aaMtrev24[16][13] =  33.85; aaMtrev24[16][14] = 128.22;
	aaMtrev24[16][15] = 597.21; aaMtrev24[16][16] =   0.00; aaMtrev24[16][17] =   9.99; aaMtrev24[16][18] =  38.73; aaMtrev24[16][19] = 204.54;
	aaMtrev24[17][ 0] =   1.90; aaMtrev24[17][ 1] =  21.95; aaMtrev24[17][ 2] =  10.68; aaMtrev24[17][ 3] =  19.86; aaMtrev24[17][ 4] =  33.60;
	aaMtrev24[17][ 5] =   1.90; aaMtrev24[17][ 6] =   1.90; aaMtrev24[17][ 7] =  10.92; aaMtrev24[17][ 8] =   7.08; aaMtrev24[17][ 9] =   1.90;
	aaMtrev24[17][10] =  32.44; aaMtrev24[17][11] =  24.00; aaMtrev24[17][12] =  21.71; aaMtrev24[17][13] =   7.84; aaMtrev24[17][14] =   4.21;
	aaMtrev24[17][15] =  38.58; aaMtrev24[17][16] =   9.99; aaMtrev24[17][17] =   0.00; aaMtrev24[17][18] =  26.25; aaMtrev24[17][19] =   5.37;
	aaMtrev24[18][ 0] =   6.48; aaMtrev24[18][ 1] =   1.90; aaMtrev24[18][ 2] = 191.36; aaMtrev24[18][ 3] =  21.21; aaMtrev24[18][ 4] = 254.77;
	aaMtrev24[18][ 5] =  38.82; aaMtrev24[18][ 6] =  13.12; aaMtrev24[18][ 7] =   3.21; aaMtrev24[18][ 8] = 670.14; aaMtrev24[18][ 9] =  25.01;
	aaMtrev24[18][10] =  44.15; aaMtrev24[18][11] =  51.17; aaMtrev24[18][12] =  39.96; aaMtrev24[18][13] = 465.58; aaMtrev24[18][14] =  16.21;
	aaMtrev24[18][15] =  64.92; aaMtrev24[18][16] =  38.73; aaMtrev24[18][17] =  26.25; aaMtrev24[18][18] =   0.00; aaMtrev24[18][19] =   1.90;
	aaMtrev24[19][ 0] = 195.06; aaMtrev24[19][ 1] =   7.64; aaMtrev24[19][ 2] =   1.90; aaMtrev24[19][ 3] =   1.90; aaMtrev24[19][ 4] =   1.90;
	aaMtrev24[19][ 5] =  19.00; aaMtrev24[19][ 6] =  21.14; aaMtrev24[19][ 7] =   2.53; aaMtrev24[19][ 8] =   1.90; aaMtrev24[19][ 9] =1222.94;
	aaMtrev24[19][10] =  91.67; aaMtrev24[19][11] =   1.90; aaMtrev24[19][12] = 387.54; aaMtrev24[19][13] =   6.35; aaMtrev24[19][14] =   8.23;
	aaMtrev24[19][15] =   1.90; aaMtrev24[19][16] = 204.54; aaMtrev24[19][17] =   5.37; aaMtrev24[19][18] =   1.90; aaMtrev24[19][19] =   0.00;

	mtrev24Pi[ 0] = 0.072;
	mtrev24Pi[ 1] = 0.019;
	mtrev24Pi[ 2] = 0.039;
	mtrev24Pi[ 3] = 0.019;
	mtrev24Pi[ 4] = 0.006;
	mtrev24Pi[ 5] = 0.025;
	mtrev24Pi[ 6] = 0.024;
	mtrev24Pi[ 7] = 0.056;
	mtrev24Pi[ 8] = 0.028;
	mtrev24Pi[ 9] = 0.088;
	mtrev24Pi[10] = 0.168;
	mtrev24Pi[11] = 0.023;
	mtrev24Pi[12] = 0.054;
	mtrev24Pi[13] = 0.061;
	mtrev24Pi[14] = 0.054;
	mtrev24Pi[15] = 0.072;
	mtrev24Pi[16] = 0.086;
	mtrev24Pi[17] = 0.029;
	mtrev24Pi[18] = 0.033;
	mtrev24Pi[19] = 0.043;
	
	/* mtmam */
	aaMtmam[ 0][ 0] =   0; aaMtmam[ 0][ 1] =  32; aaMtmam[ 0][ 2] =   2; aaMtmam[ 0][ 3] =  11; aaMtmam[ 0][ 4] =   0;
	aaMtmam[ 0][ 5] =   0; aaMtmam[ 0][ 6] =   0; aaMtmam[ 0][ 7] =  78; aaMtmam[ 0][ 8] =   8; aaMtmam[ 0][ 9] =  75;
	aaMtmam[ 0][10] =  21; aaMtmam[ 0][11] =   0; aaMtmam[ 0][12] =  76; aaMtmam[ 0][13] =   0; aaMtmam[ 0][14] =  53;
	aaMtmam[ 0][15] = 342; aaMtmam[ 0][16] = 681; aaMtmam[ 0][17] =   5; aaMtmam[ 0][18] =   0; aaMtmam[ 0][19] = 398;
	aaMtmam[ 1][ 0] =  32; aaMtmam[ 1][ 1] =   0; aaMtmam[ 1][ 2] =   4; aaMtmam[ 1][ 3] =   0; aaMtmam[ 1][ 4] = 186;
	aaMtmam[ 1][ 5] = 246; aaMtmam[ 1][ 6] =   0; aaMtmam[ 1][ 7] =  18; aaMtmam[ 1][ 8] = 232; aaMtmam[ 1][ 9] =   0;
	aaMtmam[ 1][10] =   6; aaMtmam[ 1][11] =  50; aaMtmam[ 1][12] =   0; aaMtmam[ 1][13] =   0; aaMtmam[ 1][14] =   9;
	aaMtmam[ 1][15] =   3; aaMtmam[ 1][16] =   0; aaMtmam[ 1][17] =  16; aaMtmam[ 1][18] =   0; aaMtmam[ 1][19] =   0;
	aaMtmam[ 2][ 0] =   2; aaMtmam[ 2][ 1] =   4; aaMtmam[ 2][ 2] =   0; aaMtmam[ 2][ 3] = 864; aaMtmam[ 2][ 4] =   0;
	aaMtmam[ 2][ 5] =   8; aaMtmam[ 2][ 6] =   0; aaMtmam[ 2][ 7] =  47; aaMtmam[ 2][ 8] = 458; aaMtmam[ 2][ 9] =  19;
	aaMtmam[ 2][10] =   0; aaMtmam[ 2][11] = 408; aaMtmam[ 2][12] =  21; aaMtmam[ 2][13] =   6; aaMtmam[ 2][14] =  33;
	aaMtmam[ 2][15] = 446; aaMtmam[ 2][16] = 110; aaMtmam[ 2][17] =   6; aaMtmam[ 2][18] = 156; aaMtmam[ 2][19] =   0;
	aaMtmam[ 3][ 0] =  11; aaMtmam[ 3][ 1] =   0; aaMtmam[ 3][ 2] = 864; aaMtmam[ 3][ 3] =   0; aaMtmam[ 3][ 4] =   0;
	aaMtmam[ 3][ 5] =  49; aaMtmam[ 3][ 6] = 569; aaMtmam[ 3][ 7] =  79; aaMtmam[ 3][ 8] =  11; aaMtmam[ 3][ 9] =   0;
	aaMtmam[ 3][10] =   0; aaMtmam[ 3][11] =   0; aaMtmam[ 3][12] =   0; aaMtmam[ 3][13] =   5; aaMtmam[ 3][14] =   2;
	aaMtmam[ 3][15] =  16; aaMtmam[ 3][16] =   0; aaMtmam[ 3][17] =   0; aaMtmam[ 3][18] =   0; aaMtmam[ 3][19] =  10;
	aaMtmam[ 4][ 0] =   0; aaMtmam[ 4][ 1] = 186; aaMtmam[ 4][ 2] =   0; aaMtmam[ 4][ 3] =   0; aaMtmam[ 4][ 4] =   0;
	aaMtmam[ 4][ 5] =   0; aaMtmam[ 4][ 6] =   0; aaMtmam[ 4][ 7] =   0; aaMtmam[ 4][ 8] = 305; aaMtmam[ 4][ 9] =  41;
	aaMtmam[ 4][10] =  27; aaMtmam[ 4][11] =   0; aaMtmam[ 4][12] =   0; aaMtmam[ 4][13] =   7; aaMtmam[ 4][14] =   0;
	aaMtmam[ 4][15] = 347; aaMtmam[ 4][16] = 114; aaMtmam[ 4][17] =  65; aaMtmam[ 4][18] = 530; aaMtmam[ 4][19] =   0;
	aaMtmam[ 5][ 0] =   0; aaMtmam[ 5][ 1] = 246; aaMtmam[ 5][ 2] =   8; aaMtmam[ 5][ 3] =  49; aaMtmam[ 5][ 4] =   0;
	aaMtmam[ 5][ 5] =   0; aaMtmam[ 5][ 6] = 274; aaMtmam[ 5][ 7] =   0; aaMtmam[ 5][ 8] = 550; aaMtmam[ 5][ 9] =   0;
	aaMtmam[ 5][10] =  20; aaMtmam[ 5][11] = 242; aaMtmam[ 5][12] =  22; aaMtmam[ 5][13] =   0; aaMtmam[ 5][14] =  51;
	aaMtmam[ 5][15] =  30; aaMtmam[ 5][16] =   0; aaMtmam[ 5][17] =   0; aaMtmam[ 5][18] =  54; aaMtmam[ 5][19] =  33;
	aaMtmam[ 6][ 0] =   0; aaMtmam[ 6][ 1] =   0; aaMtmam[ 6][ 2] =   0; aaMtmam[ 6][ 3] = 569; aaMtmam[ 6][ 4] =   0;
	aaMtmam[ 6][ 5] = 274; aaMtmam[ 6][ 6] =   0; aaMtmam[ 6][ 7] =  22; aaMtmam[ 6][ 8] =  22; aaMtmam[ 6][ 9] =   0;
	aaMtmam[ 6][10] =   0; aaMtmam[ 6][11] = 215; aaMtmam[ 6][12] =   0; aaMtmam[ 6][13] =   0; aaMtmam[ 6][14] =   0;
	aaMtmam[ 6][15] =  21; aaMtmam[ 6][16] =   4; aaMtmam[ 6][17] =   0; aaMtmam[ 6][18] =   0; aaMtmam[ 6][19] =  20;
	aaMtmam[ 7][ 0] =  78; aaMtmam[ 7][ 1] =  18; aaMtmam[ 7][ 2] =  47; aaMtmam[ 7][ 3] =  79; aaMtmam[ 7][ 4] =   0;
	aaMtmam[ 7][ 5] =   0; aaMtmam[ 7][ 6] =  22; aaMtmam[ 7][ 7] =   0; aaMtmam[ 7][ 8] =   0; aaMtmam[ 7][ 9] =   0;
	aaMtmam[ 7][10] =   0; aaMtmam[ 7][11] =   0; aaMtmam[ 7][12] =   0; aaMtmam[ 7][13] =   0; aaMtmam[ 7][14] =   0;
	aaMtmam[ 7][15] = 112; aaMtmam[ 7][16] =   0; aaMtmam[ 7][17] =   0; aaMtmam[ 7][18] =   1; aaMtmam[ 7][19] =   5;
	aaMtmam[ 8][ 0] =   8; aaMtmam[ 8][ 1] = 232; aaMtmam[ 8][ 2] = 458; aaMtmam[ 8][ 3] =  11; aaMtmam[ 8][ 4] = 305;
	aaMtmam[ 8][ 5] = 550; aaMtmam[ 8][ 6] =  22; aaMtmam[ 8][ 7] =   0; aaMtmam[ 8][ 8] =   0; aaMtmam[ 8][ 9] =   0;
	aaMtmam[ 8][10] =  26; aaMtmam[ 8][11] =   0; aaMtmam[ 8][12] =   0; aaMtmam[ 8][13] =   0; aaMtmam[ 8][14] =  53;
	aaMtmam[ 8][15] =  20; aaMtmam[ 8][16] =   1; aaMtmam[ 8][17] =   0; aaMtmam[ 8][18] =1525; aaMtmam[ 8][19] =   0;
	aaMtmam[ 9][ 0] =  75; aaMtmam[ 9][ 1] =   0; aaMtmam[ 9][ 2] =  19; aaMtmam[ 9][ 3] =   0; aaMtmam[ 9][ 4] =  41;
	aaMtmam[ 9][ 5] =   0; aaMtmam[ 9][ 6] =   0; aaMtmam[ 9][ 7] =   0; aaMtmam[ 9][ 8] =   0; aaMtmam[ 9][ 9] =   0;
	aaMtmam[ 9][10] = 232; aaMtmam[ 9][11] =   6; aaMtmam[ 9][12] = 378; aaMtmam[ 9][13] =  57; aaMtmam[ 9][14] =   5;
	aaMtmam[ 9][15] =   0; aaMtmam[ 9][16] = 360; aaMtmam[ 9][17] =   0; aaMtmam[ 9][18] =  16; aaMtmam[ 9][19] =2220;
	aaMtmam[10][ 0] =  21; aaMtmam[10][ 1] =   6; aaMtmam[10][ 2] =   0; aaMtmam[10][ 3] =   0; aaMtmam[10][ 4] =  27;
	aaMtmam[10][ 5] =  20; aaMtmam[10][ 6] =   0; aaMtmam[10][ 7] =   0; aaMtmam[10][ 8] =  26; aaMtmam[10][ 9] = 232;
	aaMtmam[10][10] =   0; aaMtmam[10][11] =   4; aaMtmam[10][12] = 609; aaMtmam[10][13] = 246; aaMtmam[10][14] =  43;
	aaMtmam[10][15] =  74; aaMtmam[10][16] =  34; aaMtmam[10][17] =  12; aaMtmam[10][18] =  25; aaMtmam[10][19] = 100;
	aaMtmam[11][ 0] =   0; aaMtmam[11][ 1] =  50; aaMtmam[11][ 2] = 408; aaMtmam[11][ 3] =   0; aaMtmam[11][ 4] =   0;
	aaMtmam[11][ 5] = 242; aaMtmam[11][ 6] = 215; aaMtmam[11][ 7] =   0; aaMtmam[11][ 8] =   0; aaMtmam[11][ 9] =   6;
	aaMtmam[11][10] =   4; aaMtmam[11][11] =   0; aaMtmam[11][12] =  59; aaMtmam[11][13] =   0; aaMtmam[11][14] =  18;
	aaMtmam[11][15] =  65; aaMtmam[11][16] =  50; aaMtmam[11][17] =   0; aaMtmam[11][18] =  67; aaMtmam[11][19] =   0;
	aaMtmam[12][ 0] =  76; aaMtmam[12][ 1] =   0; aaMtmam[12][ 2] =  21; aaMtmam[12][ 3] =   0; aaMtmam[12][ 4] =   0;
	aaMtmam[12][ 5] =  22; aaMtmam[12][ 6] =   0; aaMtmam[12][ 7] =   0; aaMtmam[12][ 8] =   0; aaMtmam[12][ 9] = 378;
	aaMtmam[12][10] = 609; aaMtmam[12][11] =  59; aaMtmam[12][12] =   0; aaMtmam[12][13] =  11; aaMtmam[12][14] =   0;
	aaMtmam[12][15] =  47; aaMtmam[12][16] = 691; aaMtmam[12][17] =  13; aaMtmam[12][18] =   0; aaMtmam[12][19] = 832;
	aaMtmam[13][ 0] =   0; aaMtmam[13][ 1] =   0; aaMtmam[13][ 2] =   6; aaMtmam[13][ 3] =   5; aaMtmam[13][ 4] =   7;
	aaMtmam[13][ 5] =   0; aaMtmam[13][ 6] =   0; aaMtmam[13][ 7] =   0; aaMtmam[13][ 8] =   0; aaMtmam[13][ 9] =  57;
	aaMtmam[13][10] = 246; aaMtmam[13][11] =   0; aaMtmam[13][12] =  11; aaMtmam[13][13] =   0; aaMtmam[13][14] =  17;
	aaMtmam[13][15] =  90; aaMtmam[13][16] =   8; aaMtmam[13][17] =   0; aaMtmam[13][18] = 682; aaMtmam[13][19] =   6;
	aaMtmam[14][ 0] =  53; aaMtmam[14][ 1] =   9; aaMtmam[14][ 2] =  33; aaMtmam[14][ 3] =   2; aaMtmam[14][ 4] =   0;
	aaMtmam[14][ 5] =  51; aaMtmam[14][ 6] =   0; aaMtmam[14][ 7] =   0; aaMtmam[14][ 8] =  53; aaMtmam[14][ 9] =   5;
	aaMtmam[14][10] =  43; aaMtmam[14][11] =  18; aaMtmam[14][12] =   0; aaMtmam[14][13] =  17; aaMtmam[14][14] =   0;
	aaMtmam[14][15] = 202; aaMtmam[14][16] =  78; aaMtmam[14][17] =   7; aaMtmam[14][18] =   8; aaMtmam[14][19] =   0;
	aaMtmam[15][ 0] = 342; aaMtmam[15][ 1] =   3; aaMtmam[15][ 2] = 446; aaMtmam[15][ 3] =  16; aaMtmam[15][ 4] = 347;
	aaMtmam[15][ 5] =  30; aaMtmam[15][ 6] =  21; aaMtmam[15][ 7] = 112; aaMtmam[15][ 8] =  20; aaMtmam[15][ 9] =   0;
	aaMtmam[15][10] =  74; aaMtmam[15][11] =  65; aaMtmam[15][12] =  47; aaMtmam[15][13] =  90; aaMtmam[15][14] = 202;
	aaMtmam[15][15] =   0; aaMtmam[15][16] = 614; aaMtmam[15][17] =  17; aaMtmam[15][18] = 107; aaMtmam[15][19] =   0;
	aaMtmam[16][ 0] = 681; aaMtmam[16][ 1] =   0; aaMtmam[16][ 2] = 110; aaMtmam[16][ 3] =   0; aaMtmam[16][ 4] = 114;
	aaMtmam[16][ 5] =   0; aaMtmam[16][ 6] =   4; aaMtmam[16][ 7] =   0; aaMtmam[16][ 8] =   1; aaMtmam[16][ 9] = 360;
	aaMtmam[16][10] =  34; aaMtmam[16][11] =  50; aaMtmam[16][12] = 691; aaMtmam[16][13] =   8; aaMtmam[16][14] =  78;
	aaMtmam[16][15] = 614; aaMtmam[16][16] =   0; aaMtmam[16][17] =   0; aaMtmam[16][18] =   0; aaMtmam[16][19] = 237;
	aaMtmam[17][ 0] =   5; aaMtmam[17][ 1] =  16; aaMtmam[17][ 2] =   6; aaMtmam[17][ 3] =   0; aaMtmam[17][ 4] =  65;
	aaMtmam[17][ 5] =   0; aaMtmam[17][ 6] =   0; aaMtmam[17][ 7] =   0; aaMtmam[17][ 8] =   0; aaMtmam[17][ 9] =   0;
	aaMtmam[17][10] =  12; aaMtmam[17][11] =   0; aaMtmam[17][12] =  13; aaMtmam[17][13] =   0; aaMtmam[17][14] =   7;
	aaMtmam[17][15] =  17; aaMtmam[17][16] =   0; aaMtmam[17][17] =   0; aaMtmam[17][18] =  14; aaMtmam[17][19] =   0;
	aaMtmam[18][ 0] =   0; aaMtmam[18][ 1] =   0; aaMtmam[18][ 2] = 156; aaMtmam[18][ 3] =   0; aaMtmam[18][ 4] = 530;
	aaMtmam[18][ 5] =  54; aaMtmam[18][ 6] =   0; aaMtmam[18][ 7] =   1; aaMtmam[18][ 8] =1525; aaMtmam[18][ 9] =  16;
	aaMtmam[18][10] =  25; aaMtmam[18][11] =  67; aaMtmam[18][12] =   0; aaMtmam[18][13] = 682; aaMtmam[18][14] =   8;
	aaMtmam[18][15] = 107; aaMtmam[18][16] =   0; aaMtmam[18][17] =  14; aaMtmam[18][18] =   0; aaMtmam[18][19] =   0;
	aaMtmam[19][ 0] = 398; aaMtmam[19][ 1] =   0; aaMtmam[19][ 2] =   0; aaMtmam[19][ 3] =  10; aaMtmam[19][ 4] =   0;
	aaMtmam[19][ 5] =  33; aaMtmam[19][ 6] =  20; aaMtmam[19][ 7] =   5; aaMtmam[19][ 8] =   0; aaMtmam[19][ 9] =2220;
	aaMtmam[19][10] = 100; aaMtmam[19][11] =   0; aaMtmam[19][12] = 832; aaMtmam[19][13] =   6; aaMtmam[19][14] =   0;
	aaMtmam[19][15] =   0; aaMtmam[19][16] = 237; aaMtmam[19][17] =   0; aaMtmam[19][18] =   0; aaMtmam[19][19] =   0;

	mtmamPi[ 0] = 0.0692;
	mtmamPi[ 1] = 0.0184;
	mtmamPi[ 2] = 0.0400;
	mtmamPi[ 3] = 0.0186;
	mtmamPi[ 4] = 0.0065;
	mtmamPi[ 5] = 0.0238;
	mtmamPi[ 6] = 0.0236;
	mtmamPi[ 7] = 0.0557;
	mtmamPi[ 8] = 0.0277;
	mtmamPi[ 9] = 0.0905;
	mtmamPi[10] = 0.1675;
	mtmamPi[11] = 0.0221;
	mtmamPi[12] = 0.0561;
	mtmamPi[13] = 0.0611;
	mtmamPi[14] = 0.0536;
	mtmamPi[15] = 0.0725;
	mtmamPi[16] = 0.0870;
	mtmamPi[17] = 0.0293;
	mtmamPi[18] = 0.0340;
	mtmamPi[19] = 0.0428;
	
	/* rtRev */
	aartREV[ 0][ 0] =   0; aartREV[ 1][ 0] =  34; aartREV[ 2][ 0] =  51; aartREV[ 3][ 0] =  10; aartREV[ 4][ 0] = 439;
	aartREV[ 5][ 0] =  32; aartREV[ 6][ 0] =  81; aartREV[ 7][ 0] = 135; aartREV[ 8][ 0] =  30; aartREV[ 9][ 0] =   1;
	aartREV[10][ 0] =  45; aartREV[11][ 0] =  38; aartREV[12][ 0] = 235; aartREV[13][ 0] =   1; aartREV[14][ 0] =  97;
	aartREV[15][ 0] = 460; aartREV[16][ 0] = 258; aartREV[17][ 0] =   5; aartREV[18][ 0] =  55; aartREV[19][ 0] = 197;
	aartREV[ 0][ 1] =  34; aartREV[ 1][ 1] =   0; aartREV[ 2][ 1] =  35; aartREV[ 3][ 1] =  30; aartREV[ 4][ 1] =  92;
	aartREV[ 5][ 1] = 221; aartREV[ 6][ 1] =  10; aartREV[ 7][ 1] =  41; aartREV[ 8][ 1] =  90; aartREV[ 9][ 1] =  24;
	aartREV[10][ 1] =  18; aartREV[11][ 1] = 593; aartREV[12][ 1] =  57; aartREV[13][ 1] =   7; aartREV[14][ 1] =  24;
	aartREV[15][ 1] = 102; aartREV[16][ 1] =  64; aartREV[17][ 1] =  13; aartREV[18][ 1] =  47; aartREV[19][ 1] =  29;
	aartREV[ 0][ 2] =  51; aartREV[ 1][ 2] =  35; aartREV[ 2][ 2] =   0; aartREV[ 3][ 2] = 384; aartREV[ 4][ 2] = 128;
	aartREV[ 5][ 2] = 236; aartREV[ 6][ 2] =  79; aartREV[ 7][ 2] =  94; aartREV[ 8][ 2] = 320; aartREV[ 9][ 2] =  35;
	aartREV[10][ 2] =  15; aartREV[11][ 2] = 123; aartREV[12][ 2] =   1; aartREV[13][ 2] =  49; aartREV[14][ 2] =  33;
	aartREV[15][ 2] = 294; aartREV[16][ 2] = 148; aartREV[17][ 2] =  16; aartREV[18][ 2] =  28; aartREV[19][ 2] =  21;
	aartREV[ 0][ 3] =  10; aartREV[ 1][ 3] =  30; aartREV[ 2][ 3] = 384; aartREV[ 3][ 3] =   0; aartREV[ 4][ 3] =   1;
	aartREV[ 5][ 3] =  78; aartREV[ 6][ 3] = 542; aartREV[ 7][ 3] =  61; aartREV[ 8][ 3] =  91; aartREV[ 9][ 3] =   1;
	aartREV[10][ 3] =   5; aartREV[11][ 3] =  20; aartREV[12][ 3] =   1; aartREV[13][ 3] =   1; aartREV[14][ 3] =  55;
	aartREV[15][ 3] = 136; aartREV[16][ 3] =  55; aartREV[17][ 3] =   1; aartREV[18][ 3] =   1; aartREV[19][ 3] =   6;
	aartREV[ 0][ 4] = 439; aartREV[ 1][ 4] =  92; aartREV[ 2][ 4] = 128; aartREV[ 3][ 4] =   1; aartREV[ 4][ 4] =   0;
	aartREV[ 5][ 4] =  70; aartREV[ 6][ 4] =   1; aartREV[ 7][ 4] =  48; aartREV[ 8][ 4] = 124; aartREV[ 9][ 4] = 104;
	aartREV[10][ 4] = 110; aartREV[11][ 4] =  16; aartREV[12][ 4] = 156; aartREV[13][ 4] =  70; aartREV[14][ 4] =   1;
	aartREV[15][ 4] =  75; aartREV[16][ 4] = 117; aartREV[17][ 4] =  55; aartREV[18][ 4] = 131; aartREV[19][ 4] = 295;
	aartREV[ 0][ 5] =  32; aartREV[ 1][ 5] = 221; aartREV[ 2][ 5] = 236; aartREV[ 3][ 5] =  78; aartREV[ 4][ 5] =  70;
	aartREV[ 5][ 5] =   0; aartREV[ 6][ 5] = 372; aartREV[ 7][ 5] =  18; aartREV[ 8][ 5] = 387; aartREV[ 9][ 5] =  33;
	aartREV[10][ 5] =  54; aartREV[11][ 5] = 309; aartREV[12][ 5] = 158; aartREV[13][ 5] =   1; aartREV[14][ 5] =  68;
	aartREV[15][ 5] = 225; aartREV[16][ 5] = 146; aartREV[17][ 5] =  10; aartREV[18][ 5] =  45; aartREV[19][ 5] =  36;
	aartREV[ 0][ 6] =  81; aartREV[ 1][ 6] =  10; aartREV[ 2][ 6] =  79; aartREV[ 3][ 6] = 542; aartREV[ 4][ 6] =   1;
	aartREV[ 5][ 6] = 372; aartREV[ 6][ 6] =   0; aartREV[ 7][ 6] =  70; aartREV[ 8][ 6] =  34; aartREV[ 9][ 6] =   1;
	aartREV[10][ 6] =  21; aartREV[11][ 6] = 141; aartREV[12][ 6] =   1; aartREV[13][ 6] =   1; aartREV[14][ 6] =  52;
	aartREV[15][ 6] =  95; aartREV[16][ 6] =  82; aartREV[17][ 6] =  17; aartREV[18][ 6] =   1; aartREV[19][ 6] =  35;
	aartREV[ 0][ 7] = 135; aartREV[ 1][ 7] =  41; aartREV[ 2][ 7] =  94; aartREV[ 3][ 7] =  61; aartREV[ 4][ 7] =  48;
	aartREV[ 5][ 7] =  18; aartREV[ 6][ 7] =  70; aartREV[ 7][ 7] =   0; aartREV[ 8][ 7] =  68; aartREV[ 9][ 7] =   1;
	aartREV[10][ 7] =   3; aartREV[11][ 7] =  30; aartREV[12][ 7] =  37; aartREV[13][ 7] =   7; aartREV[14][ 7] =  17;
	aartREV[15][ 7] = 152; aartREV[16][ 7] =   7; aartREV[17][ 7] =  23; aartREV[18][ 7] =  21; aartREV[19][ 7] =   3;
	aartREV[ 0][ 8] =  30; aartREV[ 1][ 8] =  90; aartREV[ 2][ 8] = 320; aartREV[ 3][ 8] =  91; aartREV[ 4][ 8] = 124;
	aartREV[ 5][ 8] = 387; aartREV[ 6][ 8] =  34; aartREV[ 7][ 8] =  68; aartREV[ 8][ 8] =   0; aartREV[ 9][ 8] =  34;
	aartREV[10][ 8] =  51; aartREV[11][ 8] =  76; aartREV[12][ 8] = 116; aartREV[13][ 8] = 141; aartREV[14][ 8] =  44;
	aartREV[15][ 8] = 183; aartREV[16][ 8] =  49; aartREV[17][ 8] =  48; aartREV[18][ 8] = 307; aartREV[19][ 8] =   1;
	aartREV[ 0][ 9] =   1; aartREV[ 1][ 9] =  24; aartREV[ 2][ 9] =  35; aartREV[ 3][ 9] =   1; aartREV[ 4][ 9] = 104;
	aartREV[ 5][ 9] =  33; aartREV[ 6][ 9] =   1; aartREV[ 7][ 9] =   1; aartREV[ 8][ 9] =  34; aartREV[ 9][ 9] =   0;
	aartREV[10][ 9] = 385; aartREV[11][ 9] =  34; aartREV[12][ 9] = 375; aartREV[13][ 9] =  64; aartREV[14][ 9] =  10;
	aartREV[15][ 9] =   4; aartREV[16][ 9] =  72; aartREV[17][ 9] =  39; aartREV[18][ 9] =  26; aartREV[19][ 9] =1048;
	aartREV[ 0][10] =  45; aartREV[ 1][10] =  18; aartREV[ 2][10] =  15; aartREV[ 3][10] =   5; aartREV[ 4][10] = 110;
	aartREV[ 5][10] =  54; aartREV[ 6][10] =  21; aartREV[ 7][10] =   3; aartREV[ 8][10] =  51; aartREV[ 9][10] = 385;
	aartREV[10][10] =   0; aartREV[11][10] =  23; aartREV[12][10] = 581; aartREV[13][10] = 179; aartREV[14][10] =  22;
	aartREV[15][10] =  24; aartREV[16][10] =  25; aartREV[17][10] =  47; aartREV[18][10] =  64; aartREV[19][10] = 112;
	aartREV[ 0][11] =  38; aartREV[ 1][11] = 593; aartREV[ 2][11] = 123; aartREV[ 3][11] =  20; aartREV[ 4][11] =  16;
	aartREV[ 5][11] = 309; aartREV[ 6][11] = 141; aartREV[ 7][11] =  30; aartREV[ 8][11] =  76; aartREV[ 9][11] =  34;
	aartREV[10][11] =  23; aartREV[11][11] =   0; aartREV[12][11] = 134; aartREV[13][11] =  14; aartREV[14][11] =  43;
	aartREV[15][11] =  77; aartREV[16][11] = 110; aartREV[17][11] =   6; aartREV[18][11] =   1; aartREV[19][11] =  19;
	aartREV[ 0][12] = 235; aartREV[ 1][12] =  57; aartREV[ 2][12] =   1; aartREV[ 3][12] =   1; aartREV[ 4][12] = 156;
	aartREV[ 5][12] = 158; aartREV[ 6][12] =   1; aartREV[ 7][12] =  37; aartREV[ 8][12] = 116; aartREV[ 9][12] = 375;
	aartREV[10][12] = 581; aartREV[11][12] = 134; aartREV[12][12] =   0; aartREV[13][12] = 247; aartREV[14][12] =   1;
	aartREV[15][12] =   1; aartREV[16][12] = 131; aartREV[17][12] = 111; aartREV[18][12] =  74; aartREV[19][12] = 236;
	aartREV[ 0][13] =   1; aartREV[ 1][13] =   7; aartREV[ 2][13] =  49; aartREV[ 3][13] =   1; aartREV[ 4][13] =  70;
	aartREV[ 5][13] =   1; aartREV[ 6][13] =   1; aartREV[ 7][13] =   7; aartREV[ 8][13] = 141; aartREV[ 9][13] =  64;
	aartREV[10][13] = 179; aartREV[11][13] =  14; aartREV[12][13] = 247; aartREV[13][13] =   0; aartREV[14][13] =  11;
	aartREV[15][13] =  20; aartREV[16][13] =  69; aartREV[17][13] = 182; aartREV[18][13] =1017; aartREV[19][13] =  92;
	aartREV[ 0][14] =  97; aartREV[ 1][14] =  24; aartREV[ 2][14] =  33; aartREV[ 3][14] =  55; aartREV[ 4][14] =   1;
	aartREV[ 5][14] =  68; aartREV[ 6][14] =  52; aartREV[ 7][14] =  17; aartREV[ 8][14] =  44; aartREV[ 9][14] =  10;
	aartREV[10][14] =  22; aartREV[11][14] =  43; aartREV[12][14] =   1; aartREV[13][14] =  11; aartREV[14][14] =   0;
	aartREV[15][14] = 134; aartREV[16][14] =  62; aartREV[17][14] =   9; aartREV[18][14] =  14; aartREV[19][14] =  25;
	aartREV[ 0][15] = 460; aartREV[ 1][15] = 102; aartREV[ 2][15] = 294; aartREV[ 3][15] = 136; aartREV[ 4][15] =  75;
	aartREV[ 5][15] = 225; aartREV[ 6][15] =  95; aartREV[ 7][15] = 152; aartREV[ 8][15] = 183; aartREV[ 9][15] =   4;
	aartREV[10][15] =  24; aartREV[11][15] =  77; aartREV[12][15] =   1; aartREV[13][15] =  20; aartREV[14][15] = 134;
	aartREV[15][15] =   0; aartREV[16][15] = 671; aartREV[17][15] =  14; aartREV[18][15] =  31; aartREV[19][15] =  39;
	aartREV[ 0][16] = 258; aartREV[ 1][16] =  64; aartREV[ 2][16] = 148; aartREV[ 3][16] =  55; aartREV[ 4][16] = 117;
	aartREV[ 5][16] = 146; aartREV[ 6][16] =  82; aartREV[ 7][16] =   7; aartREV[ 8][16] =  49; aartREV[ 9][16] =  72;
	aartREV[10][16] =  25; aartREV[11][16] = 110; aartREV[12][16] = 131; aartREV[13][16] =  69; aartREV[14][16] =  62;
	aartREV[15][16] = 671; aartREV[16][16] =   0; aartREV[17][16] =   1; aartREV[18][16] =  34; aartREV[19][16] = 196;
	aartREV[ 0][17] =   5; aartREV[ 1][17] =  13; aartREV[ 2][17] =  16; aartREV[ 3][17] =   1; aartREV[ 4][17] =  55;
	aartREV[ 5][17] =  10; aartREV[ 6][17] =  17; aartREV[ 7][17] =  23; aartREV[ 8][17] =  48; aartREV[ 9][17] =  39;
	aartREV[10][17] =  47; aartREV[11][17] =   6; aartREV[12][17] = 111; aartREV[13][17] = 182; aartREV[14][17] =   9;
	aartREV[15][17] =  14; aartREV[16][17] =   1; aartREV[17][17] =   0; aartREV[18][17] = 176; aartREV[19][17] =  26;
	aartREV[ 0][18] =  55; aartREV[ 1][18] =  47; aartREV[ 2][18] =  28; aartREV[ 3][18] =   1; aartREV[ 4][18] = 131;
	aartREV[ 5][18] =  45; aartREV[ 6][18] =   1; aartREV[ 7][18] =  21; aartREV[ 8][18] = 307; aartREV[ 9][18] =  26;
	aartREV[10][18] =  64; aartREV[11][18] =   1; aartREV[12][18] =  74; aartREV[13][18] =1017; aartREV[14][18] =  14;
	aartREV[15][18] =  31; aartREV[16][18] =  34; aartREV[17][18] = 176; aartREV[18][18] =   0; aartREV[19][18] =  59;
	aartREV[ 0][19] = 197; aartREV[ 1][19] =  29; aartREV[ 2][19] =  21; aartREV[ 3][19] =   6; aartREV[ 4][19] = 295;
	aartREV[ 5][19] =  36; aartREV[ 6][19] =  35; aartREV[ 7][19] =   3; aartREV[ 8][19] =   1; aartREV[ 9][19] =1048;
	aartREV[10][19] = 112; aartREV[11][19] =  19; aartREV[12][19] = 236; aartREV[13][19] =  92; aartREV[14][19] =  25;
	aartREV[15][19] =  39; aartREV[16][19] = 196; aartREV[17][19] =  26; aartREV[18][19] =  59; aartREV[19][19] =   0;
	rtrevPi[ 0] = 0.0646;
	rtrevPi[ 1] = 0.0453;
	rtrevPi[ 2] = 0.0376;
	rtrevPi[ 3] = 0.0422;
	rtrevPi[ 4] = 0.0114;
	rtrevPi[ 5] = 0.0606;
	rtrevPi[ 6] = 0.0607;
	rtrevPi[ 7] = 0.0639;
	rtrevPi[ 8] = 0.0273;
	rtrevPi[ 9] = 0.0679;
	rtrevPi[10] = 0.1018;
	rtrevPi[11] = 0.0751;
	rtrevPi[12] = 0.0150;
	rtrevPi[13] = 0.0287;
	rtrevPi[14] = 0.0681;
	rtrevPi[15] = 0.0488;
	rtrevPi[16] = 0.0622;
	rtrevPi[17] = 0.0251;
	rtrevPi[18] = 0.0318;
	rtrevPi[19] = 0.0619;
	
	/* wag */
	aaWAG[ 0][ 0] = 0.0000000; aaWAG[ 1][ 0] = 0.5515710; aaWAG[ 2][ 0] = 0.5098480; aaWAG[ 3][ 0] = 0.7389980; aaWAG[ 4][ 0] = 1.0270400; 
	aaWAG[ 5][ 0] = 0.9085980; aaWAG[ 6][ 0] = 1.5828500; aaWAG[ 7][ 0] = 1.4167200; aaWAG[ 8][ 0] = 0.3169540; aaWAG[ 9][ 0] = 0.1933350; 
	aaWAG[10][ 0] = 0.3979150; aaWAG[11][ 0] = 0.9062650; aaWAG[12][ 0] = 0.8934960; aaWAG[13][ 0] = 0.2104940; aaWAG[14][ 0] = 1.4385500; 
	aaWAG[15][ 0] = 3.3707900; aaWAG[16][ 0] = 2.1211100; aaWAG[17][ 0] = 0.1131330; aaWAG[18][ 0] = 0.2407350; aaWAG[19][ 0] = 2.0060100;
	aaWAG[ 0][ 1] = 0.5515710; aaWAG[ 1][ 1] = 0.0000000; aaWAG[ 2][ 1] = 0.6353460; aaWAG[ 3][ 1] = 0.1473040; aaWAG[ 4][ 1] = 0.5281910;  
	aaWAG[ 5][ 1] = 3.0355000; aaWAG[ 6][ 1] = 0.4391570; aaWAG[ 7][ 1] = 0.5846650; aaWAG[ 8][ 1] = 2.1371500; aaWAG[ 9][ 1] = 0.1869790;  
	aaWAG[10][ 1] = 0.4976710; aaWAG[11][ 1] = 5.3514200; aaWAG[12][ 1] = 0.6831620; aaWAG[13][ 1] = 0.1027110; aaWAG[14][ 1] = 0.6794890;  
	aaWAG[15][ 1] = 1.2241900; aaWAG[16][ 1] = 0.5544130; aaWAG[17][ 1] = 1.1639200; aaWAG[18][ 1] = 0.3815330; aaWAG[19][ 1] = 0.2518490;
	aaWAG[ 0][ 2] = 0.5098480; aaWAG[ 1][ 2] = 0.6353460; aaWAG[ 2][ 2] = 0.0000000; aaWAG[ 3][ 2] = 5.4294200; aaWAG[ 4][ 2] = 0.2652560;  
	aaWAG[ 5][ 2] = 1.5436400; aaWAG[ 6][ 2] = 0.9471980; aaWAG[ 7][ 2] = 1.1255600; aaWAG[ 8][ 2] = 3.9562900; aaWAG[ 9][ 2] = 0.5542360;  
	aaWAG[10][ 2] = 0.1315280; aaWAG[11][ 2] = 3.0120100; aaWAG[12][ 2] = 0.1982210; aaWAG[13][ 2] = 0.0961621; aaWAG[14][ 2] = 0.1950810;  
	aaWAG[15][ 2] = 3.9742300; aaWAG[16][ 2] = 2.0300600; aaWAG[17][ 2] = 0.0719167; aaWAG[18][ 2] = 1.0860000; aaWAG[19][ 2] = 0.1962460;
	aaWAG[ 0][ 3] = 0.7389980; aaWAG[ 1][ 3] = 0.1473040; aaWAG[ 2][ 3] = 5.4294200; aaWAG[ 3][ 3] = 0.0000000; aaWAG[ 4][ 3] = 0.0302949;  
	aaWAG[ 5][ 3] = 0.6167830; aaWAG[ 6][ 3] = 6.1741600; aaWAG[ 7][ 3] = 0.8655840; aaWAG[ 8][ 3] = 0.9306760; aaWAG[ 9][ 3] = 0.0394370;  
	aaWAG[10][ 3] = 0.0848047; aaWAG[11][ 3] = 0.4798550; aaWAG[12][ 3] = 0.1037540; aaWAG[13][ 3] = 0.0467304; aaWAG[14][ 3] = 0.4239840;  
	aaWAG[15][ 3] = 1.0717600; aaWAG[16][ 3] = 0.3748660; aaWAG[17][ 3] = 0.1297670; aaWAG[18][ 3] = 0.3257110; aaWAG[19][ 3] = 0.1523350;
	aaWAG[ 0][ 4] = 1.0270400; aaWAG[ 1][ 4] = 0.5281910; aaWAG[ 2][ 4] = 0.2652560; aaWAG[ 3][ 4] = 0.0302949; aaWAG[ 4][ 4] = 0.0000000;  
	aaWAG[ 5][ 4] = 0.0988179; aaWAG[ 6][ 4] = 0.0213520; aaWAG[ 7][ 4] = 0.3066740; aaWAG[ 8][ 4] = 0.2489720; aaWAG[ 9][ 4] = 0.1701350;  
	aaWAG[10][ 4] = 0.3842870; aaWAG[11][ 4] = 0.0740339; aaWAG[12][ 4] = 0.3904820; aaWAG[13][ 4] = 0.3980200; aaWAG[14][ 4] = 0.1094040;  
	aaWAG[15][ 4] = 1.4076600; aaWAG[16][ 4] = 0.5129840; aaWAG[17][ 4] = 0.7170700; aaWAG[18][ 4] = 0.5438330; aaWAG[19][ 4] = 1.0021400;
	aaWAG[ 0][ 5] = 0.9085980; aaWAG[ 1][ 5] = 3.0355000; aaWAG[ 2][ 5] = 1.5436400; aaWAG[ 3][ 5] = 0.6167830; aaWAG[ 4][ 5] = 0.0988179;  
	aaWAG[ 5][ 5] = 0.0000000; aaWAG[ 6][ 5] = 5.4694700; aaWAG[ 7][ 5] = 0.3300520; aaWAG[ 8][ 5] = 4.2941100; aaWAG[ 9][ 5] = 0.1139170;  
	aaWAG[10][ 5] = 0.8694890; aaWAG[11][ 5] = 3.8949000; aaWAG[12][ 5] = 1.5452600; aaWAG[13][ 5] = 0.0999208; aaWAG[14][ 5] = 0.9333720;  
	aaWAG[15][ 5] = 1.0288700; aaWAG[16][ 5] = 0.8579280; aaWAG[17][ 5] = 0.2157370; aaWAG[18][ 5] = 0.2277100; aaWAG[19][ 5] = 0.3012810;
	aaWAG[ 0][ 6] = 1.5828500; aaWAG[ 1][ 6] = 0.4391570; aaWAG[ 2][ 6] = 0.9471980; aaWAG[ 3][ 6] = 6.1741600; aaWAG[ 4][ 6] = 0.0213520;  
	aaWAG[ 5][ 6] = 5.4694700; aaWAG[ 6][ 6] = 0.0000000; aaWAG[ 7][ 6] = 0.5677170; aaWAG[ 8][ 6] = 0.5700250; aaWAG[ 9][ 6] = 0.1273950;  
	aaWAG[10][ 6] = 0.1542630; aaWAG[11][ 6] = 2.5844300; aaWAG[12][ 6] = 0.3151240; aaWAG[13][ 6] = 0.0811339; aaWAG[14][ 6] = 0.6823550;  
	aaWAG[15][ 6] = 0.7049390; aaWAG[16][ 6] = 0.8227650; aaWAG[17][ 6] = 0.1565570; aaWAG[18][ 6] = 0.1963030; aaWAG[19][ 6] = 0.5887310;
	aaWAG[ 0][ 7] = 1.4167200; aaWAG[ 1][ 7] = 0.5846650; aaWAG[ 2][ 7] = 1.1255600; aaWAG[ 3][ 7] = 0.8655840; aaWAG[ 4][ 7] = 0.3066740;  
	aaWAG[ 5][ 7] = 0.3300520; aaWAG[ 6][ 7] = 0.5677170; aaWAG[ 7][ 7] = 0.0000000; aaWAG[ 8][ 7] = 0.2494100; aaWAG[ 9][ 7] = 0.0304501;  
	aaWAG[10][ 7] = 0.0613037; aaWAG[11][ 7] = 0.3735580; aaWAG[12][ 7] = 0.1741000; aaWAG[13][ 7] = 0.0499310; aaWAG[14][ 7] = 0.2435700;  
	aaWAG[15][ 7] = 1.3418200; aaWAG[16][ 7] = 0.2258330; aaWAG[17][ 7] = 0.3369830; aaWAG[18][ 7] = 0.1036040; aaWAG[19][ 7] = 0.1872470;
	aaWAG[ 0][ 8] = 0.3169540; aaWAG[ 1][ 8] = 2.1371500; aaWAG[ 2][ 8] = 3.9562900; aaWAG[ 3][ 8] = 0.9306760; aaWAG[ 4][ 8] = 0.2489720;  
	aaWAG[ 5][ 8] = 4.2941100; aaWAG[ 6][ 8] = 0.5700250; aaWAG[ 7][ 8] = 0.2494100; aaWAG[ 8][ 8] = 0.0000000; aaWAG[ 9][ 8] = 0.1381900;  
	aaWAG[10][ 8] = 0.4994620; aaWAG[11][ 8] = 0.8904320; aaWAG[12][ 8] = 0.4041410; aaWAG[13][ 8] = 0.6793710; aaWAG[14][ 8] = 0.6961980;  
	aaWAG[15][ 8] = 0.7401690; aaWAG[16][ 8] = 0.4733070; aaWAG[17][ 8] = 0.2625690; aaWAG[18][ 8] = 3.8734400; aaWAG[19][ 8] = 0.1183580;
	aaWAG[ 0][ 9] = 0.1933350; aaWAG[ 1][ 9] = 0.1869790; aaWAG[ 2][ 9] = 0.5542360; aaWAG[ 3][ 9] = 0.0394370; aaWAG[ 4][ 9] = 0.1701350;  
	aaWAG[ 5][ 9] = 0.1139170; aaWAG[ 6][ 9] = 0.1273950; aaWAG[ 7][ 9] = 0.0304501; aaWAG[ 8][ 9] = 0.1381900; aaWAG[ 9][ 9] = 0.0000000;  
	aaWAG[10][ 9] = 3.1709700; aaWAG[11][ 9] = 0.3238320; aaWAG[12][ 9] = 4.2574600; aaWAG[13][ 9] = 1.0594700; aaWAG[14][ 9] = 0.0999288;  
	aaWAG[15][ 9] = 0.3194400; aaWAG[16][ 9] = 1.4581600; aaWAG[17][ 9] = 0.2124830; aaWAG[18][ 9] = 0.4201700; aaWAG[19][ 9] = 7.8213000;
	aaWAG[ 0][10] = 0.3979150; aaWAG[ 1][10] = 0.4976710; aaWAG[ 2][10] = 0.1315280; aaWAG[ 3][10] = 0.0848047; aaWAG[ 4][10] = 0.3842870;  
	aaWAG[ 5][10] = 0.8694890; aaWAG[ 6][10] = 0.1542630; aaWAG[ 7][10] = 0.0613037; aaWAG[ 8][10] = 0.4994620; aaWAG[ 9][10] = 3.1709700;  
	aaWAG[10][10] = 0.0000000; aaWAG[11][10] = 0.2575550; aaWAG[12][10] = 4.8540200; aaWAG[13][10] = 2.1151700; aaWAG[14][10] = 0.4158440;  
	aaWAG[15][10] = 0.3447390; aaWAG[16][10] = 0.3266220; aaWAG[17][10] = 0.6653090; aaWAG[18][10] = 0.3986180; aaWAG[19][10] = 1.8003400;
	aaWAG[ 0][11] = 0.9062650; aaWAG[ 1][11] = 5.3514200; aaWAG[ 2][11] = 3.0120100; aaWAG[ 3][11] = 0.4798550; aaWAG[ 4][11] = 0.0740339;  
	aaWAG[ 5][11] = 3.8949000; aaWAG[ 6][11] = 2.5844300; aaWAG[ 7][11] = 0.3735580; aaWAG[ 8][11] = 0.8904320; aaWAG[ 9][11] = 0.3238320;  
	aaWAG[10][11] = 0.2575550; aaWAG[11][11] = 0.0000000; aaWAG[12][11] = 0.9342760; aaWAG[13][11] = 0.0888360; aaWAG[14][11] = 0.5568960;  
	aaWAG[15][11] = 0.9671300; aaWAG[16][11] = 1.3869800; aaWAG[17][11] = 0.1375050; aaWAG[18][11] = 0.1332640; aaWAG[19][11] = 0.3054340;
	aaWAG[ 0][12] = 0.8934960; aaWAG[ 1][12] = 0.6831620; aaWAG[ 2][12] = 0.1982210; aaWAG[ 3][12] = 0.1037540; aaWAG[ 4][12] = 0.3904820;  
	aaWAG[ 5][12] = 1.5452600; aaWAG[ 6][12] = 0.3151240; aaWAG[ 7][12] = 0.1741000; aaWAG[ 8][12] = 0.4041410; aaWAG[ 9][12] = 4.2574600;  
	aaWAG[10][12] = 4.8540200; aaWAG[11][12] = 0.9342760; aaWAG[12][12] = 0.0000000; aaWAG[13][12] = 1.1906300; aaWAG[14][12] = 0.1713290;  
	aaWAG[15][12] = 0.4939050; aaWAG[16][12] = 1.5161200; aaWAG[17][12] = 0.5157060; aaWAG[18][12] = 0.4284370; aaWAG[19][12] = 2.0584500;
	aaWAG[ 0][13] = 0.2104940; aaWAG[ 1][13] = 0.1027110; aaWAG[ 2][13] = 0.0961621; aaWAG[ 3][13] = 0.0467304; aaWAG[ 4][13] = 0.3980200;  
	aaWAG[ 5][13] = 0.0999208; aaWAG[ 6][13] = 0.0811339; aaWAG[ 7][13] = 0.0499310; aaWAG[ 8][13] = 0.6793710; aaWAG[ 9][13] = 1.0594700;  
	aaWAG[10][13] = 2.1151700; aaWAG[11][13] = 0.0888360; aaWAG[12][13] = 1.1906300; aaWAG[13][13] = 0.0000000; aaWAG[14][13] = 0.1614440;  
	aaWAG[15][13] = 0.5459310; aaWAG[16][13] = 0.1719030; aaWAG[17][13] = 1.5296400; aaWAG[18][13] = 6.4542800; aaWAG[19][13] = 0.6498920;
	aaWAG[ 0][14] = 1.4385500; aaWAG[ 1][14] = 0.6794890; aaWAG[ 2][14] = 0.1950810; aaWAG[ 3][14] = 0.4239840; aaWAG[ 4][14] = 0.1094040;  
	aaWAG[ 5][14] = 0.9333720; aaWAG[ 6][14] = 0.6823550; aaWAG[ 7][14] = 0.2435700; aaWAG[ 8][14] = 0.6961980; aaWAG[ 9][14] = 0.0999288;  
	aaWAG[10][14] = 0.4158440; aaWAG[11][14] = 0.5568960; aaWAG[12][14] = 0.1713290; aaWAG[13][14] = 0.1614440; aaWAG[14][14] = 0.0000000;  
	aaWAG[15][14] = 1.6132800; aaWAG[16][14] = 0.7953840; aaWAG[17][14] = 0.1394050; aaWAG[18][14] = 0.2160460; aaWAG[19][14] = 0.3148870;
	aaWAG[ 0][15] = 3.3707900; aaWAG[ 1][15] = 1.2241900; aaWAG[ 2][15] = 3.9742300; aaWAG[ 3][15] = 1.0717600; aaWAG[ 4][15] = 1.4076600;  
	aaWAG[ 5][15] = 1.0288700; aaWAG[ 6][15] = 0.7049390; aaWAG[ 7][15] = 1.3418200; aaWAG[ 8][15] = 0.7401690; aaWAG[ 9][15] = 0.3194400;  
	aaWAG[10][15] = 0.3447390; aaWAG[11][15] = 0.9671300; aaWAG[12][15] = 0.4939050; aaWAG[13][15] = 0.5459310; aaWAG[14][15] = 1.6132800;  
	aaWAG[15][15] = 0.0000000; aaWAG[16][15] = 4.3780200; aaWAG[17][15] = 0.5237420; aaWAG[18][15] = 0.7869930; aaWAG[19][15] = 0.2327390;
	aaWAG[ 0][16] = 2.1211100; aaWAG[ 1][16] = 0.5544130; aaWAG[ 2][16] = 2.0300600; aaWAG[ 3][16] = 0.3748660; aaWAG[ 4][16] = 0.5129840;  
	aaWAG[ 5][16] = 0.8579280; aaWAG[ 6][16] = 0.8227650; aaWAG[ 7][16] = 0.2258330; aaWAG[ 8][16] = 0.4733070; aaWAG[ 9][16] = 1.4581600;  
	aaWAG[10][16] = 0.3266220; aaWAG[11][16] = 1.3869800; aaWAG[12][16] = 1.5161200; aaWAG[13][16] = 0.1719030; aaWAG[14][16] = 0.7953840;  
	aaWAG[15][16] = 4.3780200; aaWAG[16][16] = 0.0000000; aaWAG[17][16] = 0.1108640; aaWAG[18][16] = 0.2911480; aaWAG[19][16] = 1.3882300;
	aaWAG[ 0][17] = 0.1131330; aaWAG[ 1][17] = 1.1639200; aaWAG[ 2][17] = 0.0719167; aaWAG[ 3][17] = 0.1297670; aaWAG[ 4][17] = 0.7170700;  
	aaWAG[ 5][17] = 0.2157370; aaWAG[ 6][17] = 0.1565570; aaWAG[ 7][17] = 0.3369830; aaWAG[ 8][17] = 0.2625690; aaWAG[ 9][17] = 0.2124830;  
	aaWAG[10][17] = 0.6653090; aaWAG[11][17] = 0.1375050; aaWAG[12][17] = 0.5157060; aaWAG[13][17] = 1.5296400; aaWAG[14][17] = 0.1394050;  
	aaWAG[15][17] = 0.5237420; aaWAG[16][17] = 0.1108640; aaWAG[17][17] = 0.0000000; aaWAG[18][17] = 2.4853900; aaWAG[19][17] = 0.3653690;
	aaWAG[ 0][18] = 0.2407350; aaWAG[ 1][18] = 0.3815330; aaWAG[ 2][18] = 1.0860000; aaWAG[ 3][18] = 0.3257110; aaWAG[ 4][18] = 0.5438330;  
	aaWAG[ 5][18] = 0.2277100; aaWAG[ 6][18] = 0.1963030; aaWAG[ 7][18] = 0.1036040; aaWAG[ 8][18] = 3.8734400; aaWAG[ 9][18] = 0.4201700;  
	aaWAG[10][18] = 0.3986180; aaWAG[11][18] = 0.1332640; aaWAG[12][18] = 0.4284370; aaWAG[13][18] = 6.4542800; aaWAG[14][18] = 0.2160460;  
	aaWAG[15][18] = 0.7869930; aaWAG[16][18] = 0.2911480; aaWAG[17][18] = 2.4853900; aaWAG[18][18] = 0.0000000; aaWAG[19][18] = 0.3147300;
	aaWAG[ 0][19] = 2.0060100; aaWAG[ 1][19] = 0.2518490; aaWAG[ 2][19] = 0.1962460; aaWAG[ 3][19] = 0.1523350; aaWAG[ 4][19] = 1.0021400;  
	aaWAG[ 5][19] = 0.3012810; aaWAG[ 6][19] = 0.5887310; aaWAG[ 7][19] = 0.1872470; aaWAG[ 8][19] = 0.1183580; aaWAG[ 9][19] = 7.8213000;  
	aaWAG[10][19] = 1.8003400; aaWAG[11][19] = 0.3054340; aaWAG[12][19] = 2.0584500; aaWAG[13][19] = 0.6498920; aaWAG[14][19] = 0.3148870;  
	aaWAG[15][19] = 0.2327390; aaWAG[16][19] = 1.3882300; aaWAG[17][19] = 0.3653690; aaWAG[18][19] = 0.3147300; aaWAG[19][19] = 0.0000000;
	wagPi[ 0] = 0.08662790;
	wagPi[ 1] = 0.04397200;
	wagPi[ 2] = 0.03908940;
	wagPi[ 3] = 0.05704510;
	wagPi[ 4] = 0.01930780;
	wagPi[ 5] = 0.03672810;
	wagPi[ 6] = 0.05805890;
	wagPi[ 7] = 0.08325180;
	wagPi[ 8] = 0.02443130;
	wagPi[ 9] = 0.04846600;
	wagPi[10] = 0.08620970;
	wagPi[11] = 0.06202860;
	wagPi[12] = 0.01950273;
	wagPi[13] = 0.03843190;
	wagPi[14] = 0.04576310;
	wagPi[15] = 0.06951790;
	wagPi[16] = 0.06101270;
	wagPi[17] = 0.01438590;
	wagPi[18] = 0.03527420;
	wagPi[19] = 0.07089560;

	/* cpRev */
	aacpREV[ 0][ 0] =    0; aacpREV[ 0][ 1] =  105; aacpREV[ 0][ 2] =  227; aacpREV[ 0][ 3] =  175; aacpREV[ 0][ 4] =  669; 
	aacpREV[ 0][ 5] =  157; aacpREV[ 0][ 6] =  499; aacpREV[ 0][ 7] =  665; aacpREV[ 0][ 8] =   66; aacpREV[ 0][ 9] =  145; 
	aacpREV[ 0][10] =  197; aacpREV[ 0][11] =  236; aacpREV[ 0][12] =  185; aacpREV[ 0][13] =   68; aacpREV[ 0][14] =  490; 
	aacpREV[ 0][15] = 2440; aacpREV[ 0][16] = 1340; aacpREV[ 0][17] =   14; aacpREV[ 0][18] =   56; aacpREV[ 0][19] =  968; 
	aacpREV[ 1][ 0] =  105; aacpREV[ 1][ 1] =    0; aacpREV[ 1][ 2] =  357; aacpREV[ 1][ 3] =   43; aacpREV[ 1][ 4] =  823; 
	aacpREV[ 1][ 5] = 1745; aacpREV[ 1][ 6] =  152; aacpREV[ 1][ 7] =  243; aacpREV[ 1][ 8] =  715; aacpREV[ 1][ 9] =  136; 
	aacpREV[ 1][10] =  203; aacpREV[ 1][11] = 4482; aacpREV[ 1][12] =  125; aacpREV[ 1][13] =   53; aacpREV[ 1][14] =   87; 
	aacpREV[ 1][15] =  385; aacpREV[ 1][16] =  314; aacpREV[ 1][17] =  230; aacpREV[ 1][18] =  323; aacpREV[ 1][19] =   92; 
	aacpREV[ 2][ 0] =  227; aacpREV[ 2][ 1] =  357; aacpREV[ 2][ 2] =    0; aacpREV[ 2][ 3] = 4435; aacpREV[ 2][ 4] =  538; 
	aacpREV[ 2][ 5] =  768; aacpREV[ 2][ 6] = 1055; aacpREV[ 2][ 7] =  653; aacpREV[ 2][ 8] = 1405; aacpREV[ 2][ 9] =  168; 
	aacpREV[ 2][10] =  113; aacpREV[ 2][11] = 2430; aacpREV[ 2][12] =   61; aacpREV[ 2][13] =   97; aacpREV[ 2][14] =  173; 
	aacpREV[ 2][15] = 2085; aacpREV[ 2][16] = 1393; aacpREV[ 2][17] =   40; aacpREV[ 2][18] =  754; aacpREV[ 2][19] =   83; 
	aacpREV[ 3][ 0] =  175; aacpREV[ 3][ 1] =   43; aacpREV[ 3][ 2] = 4435; aacpREV[ 3][ 3] =    0; aacpREV[ 3][ 4] =   10; 
	aacpREV[ 3][ 5] =  400; aacpREV[ 3][ 6] = 3691; aacpREV[ 3][ 7] =  431; aacpREV[ 3][ 8] =  331; aacpREV[ 3][ 9] =   10; 
	aacpREV[ 3][10] =   10; aacpREV[ 3][11] =  412; aacpREV[ 3][12] =   47; aacpREV[ 3][13] =   22; aacpREV[ 3][14] =  170; 
	aacpREV[ 3][15] =  590; aacpREV[ 3][16] =  266; aacpREV[ 3][17] =   18; aacpREV[ 3][18] =  281; aacpREV[ 3][19] =   75; 
	aacpREV[ 4][ 0] =  669; aacpREV[ 4][ 1] =  823; aacpREV[ 4][ 2] =  538; aacpREV[ 4][ 3] =   10; aacpREV[ 4][ 4] =    0; 
	aacpREV[ 4][ 5] =   10; aacpREV[ 4][ 6] =   10; aacpREV[ 4][ 7] =  303; aacpREV[ 4][ 8] =  441; aacpREV[ 4][ 9] =  280; 
	aacpREV[ 4][10] =  396; aacpREV[ 4][11] =   48; aacpREV[ 4][12] =  159; aacpREV[ 4][13] =  726; aacpREV[ 4][14] =  285; 
	aacpREV[ 4][15] = 2331; aacpREV[ 4][16] =  576; aacpREV[ 4][17] =  435; aacpREV[ 4][18] = 1466; aacpREV[ 4][19] =  592; 
	aacpREV[ 5][ 0] =  157; aacpREV[ 5][ 1] = 1745; aacpREV[ 5][ 2] =  768; aacpREV[ 5][ 3] =  400; aacpREV[ 5][ 4] =   10; 
	aacpREV[ 5][ 5] =    0; aacpREV[ 5][ 6] = 3122; aacpREV[ 5][ 7] =  133; aacpREV[ 5][ 8] = 1269; aacpREV[ 5][ 9] =   92; 
	aacpREV[ 5][10] =  286; aacpREV[ 5][11] = 3313; aacpREV[ 5][12] =  202; aacpREV[ 5][13] =   10; aacpREV[ 5][14] =  323; 
	aacpREV[ 5][15] =  396; aacpREV[ 5][16] =  241; aacpREV[ 5][17] =   53; aacpREV[ 5][18] =  391; aacpREV[ 5][19] =   54; 
	aacpREV[ 6][ 0] =  499; aacpREV[ 6][ 1] =  152; aacpREV[ 6][ 2] = 1055; aacpREV[ 6][ 3] = 3691; aacpREV[ 6][ 4] =   10; 
	aacpREV[ 6][ 5] = 3122; aacpREV[ 6][ 6] =    0; aacpREV[ 6][ 7] =  379; aacpREV[ 6][ 8] =  162; aacpREV[ 6][ 9] =  148; 
	aacpREV[ 6][10] =   82; aacpREV[ 6][11] = 2629; aacpREV[ 6][12] =  113; aacpREV[ 6][13] =  145; aacpREV[ 6][14] =  185; 
	aacpREV[ 6][15] =  568; aacpREV[ 6][16] =  369; aacpREV[ 6][17] =   63; aacpREV[ 6][18] =  142; aacpREV[ 6][19] =  200; 
	aacpREV[ 7][ 0] =  665; aacpREV[ 7][ 1] =  243; aacpREV[ 7][ 2] =  653; aacpREV[ 7][ 3] =  431; aacpREV[ 7][ 4] =  303; 
	aacpREV[ 7][ 5] =  133; aacpREV[ 7][ 6] =  379; aacpREV[ 7][ 7] =    0; aacpREV[ 7][ 8] =   19; aacpREV[ 7][ 9] =   40; 
	aacpREV[ 7][10] =   20; aacpREV[ 7][11] =  263; aacpREV[ 7][12] =   21; aacpREV[ 7][13] =   25; aacpREV[ 7][14] =   28; 
	aacpREV[ 7][15] =  691; aacpREV[ 7][16] =   92; aacpREV[ 7][17] =   82; aacpREV[ 7][18] =   10; aacpREV[ 7][19] =   91; 
	aacpREV[ 8][ 0] =   66; aacpREV[ 8][ 1] =  715; aacpREV[ 8][ 2] = 1405; aacpREV[ 8][ 3] =  331; aacpREV[ 8][ 4] =  441; 
	aacpREV[ 8][ 5] = 1269; aacpREV[ 8][ 6] =  162; aacpREV[ 8][ 7] =   19; aacpREV[ 8][ 8] =    0; aacpREV[ 8][ 9] =   29; 
	aacpREV[ 8][10] =   66; aacpREV[ 8][11] =  305; aacpREV[ 8][12] =   10; aacpREV[ 8][13] =  127; aacpREV[ 8][14] =  152; 
	aacpREV[ 8][15] =  303; aacpREV[ 8][16] =   32; aacpREV[ 8][17] =   69; aacpREV[ 8][18] = 1971; aacpREV[ 8][19] =   25; 
	aacpREV[ 9][ 0] =  145; aacpREV[ 9][ 1] =  136; aacpREV[ 9][ 2] =  168; aacpREV[ 9][ 3] =   10; aacpREV[ 9][ 4] =  280; 
	aacpREV[ 9][ 5] =   92; aacpREV[ 9][ 6] =  148; aacpREV[ 9][ 7] =   40; aacpREV[ 9][ 8] =   29; aacpREV[ 9][ 9] =    0; 
	aacpREV[ 9][10] = 1745; aacpREV[ 9][11] =  345; aacpREV[ 9][12] = 1772; aacpREV[ 9][13] =  454; aacpREV[ 9][14] =  117; 
	aacpREV[ 9][15] =  216; aacpREV[ 9][16] = 1040; aacpREV[ 9][17] =   42; aacpREV[ 9][18] =   89; aacpREV[ 9][19] = 4797; 
	aacpREV[10][ 0] =  197; aacpREV[10][ 1] =  203; aacpREV[10][ 2] =  113; aacpREV[10][ 3] =   10; aacpREV[10][ 4] =  396; 
	aacpREV[10][ 5] =  286; aacpREV[10][ 6] =   82; aacpREV[10][ 7] =   20; aacpREV[10][ 8] =   66; aacpREV[10][ 9] = 1745; 
	aacpREV[10][10] =    0; aacpREV[10][11] =  218; aacpREV[10][12] = 1351; aacpREV[10][13] = 1268; aacpREV[10][14] =  219; 
	aacpREV[10][15] =  516; aacpREV[10][16] =  156; aacpREV[10][17] =  159; aacpREV[10][18] =  189; aacpREV[10][19] =  865; 
	aacpREV[11][ 0] =  236; aacpREV[11][ 1] = 4482; aacpREV[11][ 2] = 2430; aacpREV[11][ 3] =  412; aacpREV[11][ 4] =   48; 
	aacpREV[11][ 5] = 3313; aacpREV[11][ 6] = 2629; aacpREV[11][ 7] =  263; aacpREV[11][ 8] =  305; aacpREV[11][ 9] =  345; 
	aacpREV[11][10] =  218; aacpREV[11][11] =    0; aacpREV[11][12] =  193; aacpREV[11][13] =   72; aacpREV[11][14] =  302; 
	aacpREV[11][15] =  868; aacpREV[11][16] =  918; aacpREV[11][17] =   10; aacpREV[11][18] =  247; aacpREV[11][19] =  249; 
	aacpREV[12][ 0] =  185; aacpREV[12][ 1] =  125; aacpREV[12][ 2] =   61; aacpREV[12][ 3] =   47; aacpREV[12][ 4] =  159; 
	aacpREV[12][ 5] =  202; aacpREV[12][ 6] =  113; aacpREV[12][ 7] =   21; aacpREV[12][ 8] =   10; aacpREV[12][ 9] = 1772; 
	aacpREV[12][10] = 1351; aacpREV[12][11] =  193; aacpREV[12][12] =    0; aacpREV[12][13] =  327; aacpREV[12][14] =  100; 
	aacpREV[12][15] =   93; aacpREV[12][16] =  645; aacpREV[12][17] =   86; aacpREV[12][18] =  215; aacpREV[12][19] =  475; 
	aacpREV[13][ 0] =   68; aacpREV[13][ 1] =   53; aacpREV[13][ 2] =   97; aacpREV[13][ 3] =   22; aacpREV[13][ 4] =  726; 
	aacpREV[13][ 5] =   10; aacpREV[13][ 6] =  145; aacpREV[13][ 7] =   25; aacpREV[13][ 8] =  127; aacpREV[13][ 9] =  454; 
	aacpREV[13][10] = 1268; aacpREV[13][11] =   72; aacpREV[13][12] =  327; aacpREV[13][13] =    0; aacpREV[13][14] =   43; 
	aacpREV[13][15] =  487; aacpREV[13][16] =  148; aacpREV[13][17] =  468; aacpREV[13][18] = 2370; aacpREV[13][19] =  317; 
	aacpREV[14][ 0] =  490; aacpREV[14][ 1] =   87; aacpREV[14][ 2] =  173; aacpREV[14][ 3] =  170; aacpREV[14][ 4] =  285; 
	aacpREV[14][ 5] =  323; aacpREV[14][ 6] =  185; aacpREV[14][ 7] =   28; aacpREV[14][ 8] =  152; aacpREV[14][ 9] =  117; 
	aacpREV[14][10] =  219; aacpREV[14][11] =  302; aacpREV[14][12] =  100; aacpREV[14][13] =   43; aacpREV[14][14] =    0; 
	aacpREV[14][15] = 1202; aacpREV[14][16] =  260; aacpREV[14][17] =   49; aacpREV[14][18] =   97; aacpREV[14][19] =  122; 
	aacpREV[15][ 0] = 2440; aacpREV[15][ 1] =  385; aacpREV[15][ 2] = 2085; aacpREV[15][ 3] =  590; aacpREV[15][ 4] = 2331; 
	aacpREV[15][ 5] =  396; aacpREV[15][ 6] =  568; aacpREV[15][ 7] =  691; aacpREV[15][ 8] =  303; aacpREV[15][ 9] =  216; 
	aacpREV[15][10] =  516; aacpREV[15][11] =  868; aacpREV[15][12] =   93; aacpREV[15][13] =  487; aacpREV[15][14] = 1202; 
	aacpREV[15][15] =    0; aacpREV[15][16] = 2151; aacpREV[15][17] =   73; aacpREV[15][18] =  522; aacpREV[15][19] =  167; 
	aacpREV[16][ 0] = 1340; aacpREV[16][ 1] =  314; aacpREV[16][ 2] = 1393; aacpREV[16][ 3] =  266; aacpREV[16][ 4] =  576; 
	aacpREV[16][ 5] =  241; aacpREV[16][ 6] =  369; aacpREV[16][ 7] =   92; aacpREV[16][ 8] =   32; aacpREV[16][ 9] = 1040; 
	aacpREV[16][10] =  156; aacpREV[16][11] =  918; aacpREV[16][12] =  645; aacpREV[16][13] =  148; aacpREV[16][14] =  260; 
	aacpREV[16][15] = 2151; aacpREV[16][16] =    0; aacpREV[16][17] =   29; aacpREV[16][18] =   71; aacpREV[16][19] =  760; 
	aacpREV[17][ 0] =   14; aacpREV[17][ 1] =  230; aacpREV[17][ 2] =   40; aacpREV[17][ 3] =   18; aacpREV[17][ 4] =  435; 
	aacpREV[17][ 5] =   53; aacpREV[17][ 6] =   63; aacpREV[17][ 7] =   82; aacpREV[17][ 8] =   69; aacpREV[17][ 9] =   42; 
	aacpREV[17][10] =  159; aacpREV[17][11] =   10; aacpREV[17][12] =   86; aacpREV[17][13] =  468; aacpREV[17][14] =   49; 
	aacpREV[17][15] =   73; aacpREV[17][16] =   29; aacpREV[17][17] =    0; aacpREV[17][18] =  346; aacpREV[17][19] =   10; 
	aacpREV[18][ 0] =   56; aacpREV[18][ 1] =  323; aacpREV[18][ 2] =  754; aacpREV[18][ 3] =  281; aacpREV[18][ 4] = 1466; 
	aacpREV[18][ 5] =  391; aacpREV[18][ 6] =  142; aacpREV[18][ 7] =   10; aacpREV[18][ 8] = 1971; aacpREV[18][ 9] =   89; 
	aacpREV[18][10] =  189; aacpREV[18][11] =  247; aacpREV[18][12] =  215; aacpREV[18][13] = 2370; aacpREV[18][14] =   97; 
	aacpREV[18][15] =  522; aacpREV[18][16] =   71; aacpREV[18][17] =  346; aacpREV[18][18] =    0; aacpREV[18][19] =  119; 
	aacpREV[19][ 0] =  968; aacpREV[19][ 1] =   92; aacpREV[19][ 2] =   83; aacpREV[19][ 3] =   75; aacpREV[19][ 4] =  592; 
	aacpREV[19][ 5] =   54; aacpREV[19][ 6] =  200; aacpREV[19][ 7] =   91; aacpREV[19][ 8] =   25; aacpREV[19][ 9] = 4797; 
	aacpREV[19][10] =  865; aacpREV[19][11] =  249; aacpREV[19][12] =  475; aacpREV[19][13] =  317; aacpREV[19][14] =  122; 
	aacpREV[19][15] =  167; aacpREV[19][16] =  760; aacpREV[19][17] =   10; aacpREV[19][18] =  119; aacpREV[19][19] =    0; 

	cprevPi[0] = 0.076;
	cprevPi[1] = 0.062;
	cprevPi[2] = 0.041;
	cprevPi[3] = 0.037;
	cprevPi[4] = 0.009;
	cprevPi[5] = 0.038;
	cprevPi[6] = 0.049;
	cprevPi[7] = 0.084;
	cprevPi[8] = 0.025;
	cprevPi[9] = 0.081;
	cprevPi[10] = 0.101;
	cprevPi[11] = 0.050;
	cprevPi[12] = 0.022;
	cprevPi[13] = 0.051;
	cprevPi[14] = 0.043;
	cprevPi[15] = 0.062;
	cprevPi[16] = 0.054;
	cprevPi[17] = 0.018;
	cprevPi[18] = 0.031;
	cprevPi[19] = 0.066;
	
	/* VT model */
	aaVt[ 0][ 0] = 0.000000; aaVt[ 0][ 1] = 0.233108; aaVt[ 0][ 2] = 0.199097; aaVt[ 0][ 3] = 0.265145; aaVt[ 0][ 4] = 0.227333; 
	aaVt[ 0][ 5] = 0.310084; aaVt[ 0][ 6] = 0.567957; aaVt[ 0][ 7] = 0.876213; aaVt[ 0][ 8] = 0.078692; aaVt[ 0][ 9] = 0.222972; 
	aaVt[ 0][10] = 0.424630; aaVt[ 0][11] = 0.393245; aaVt[ 0][12] = 0.211550; aaVt[ 0][13] = 0.116646; aaVt[ 0][14] = 0.399143; 
	aaVt[ 0][15] = 1.817198; aaVt[ 0][16] = 0.877877; aaVt[ 0][17] = 0.030309; aaVt[ 0][18] = 0.087061; aaVt[ 0][19] = 1.230985; 
	aaVt[ 1][ 0] = 0.233108; aaVt[ 1][ 1] = 0.000000; aaVt[ 1][ 2] = 0.210797; aaVt[ 1][ 3] = 0.105191; aaVt[ 1][ 4] = 0.031726; 
	aaVt[ 1][ 5] = 0.493763; aaVt[ 1][ 6] = 0.255240; aaVt[ 1][ 7] = 0.156945; aaVt[ 1][ 8] = 0.213164; aaVt[ 1][ 9] = 0.081510; 
	aaVt[ 1][10] = 0.192364; aaVt[ 1][11] = 1.755838; aaVt[ 1][12] = 0.087930; aaVt[ 1][13] = 0.042569; aaVt[ 1][14] = 0.128480; 
	aaVt[ 1][15] = 0.292327; aaVt[ 1][16] = 0.204109; aaVt[ 1][17] = 0.046417; aaVt[ 1][18] = 0.097010; aaVt[ 1][19] = 0.113146; 
	aaVt[ 2][ 0] = 0.199097; aaVt[ 2][ 1] = 0.210797; aaVt[ 2][ 2] = 0.000000; aaVt[ 2][ 3] = 0.883422; aaVt[ 2][ 4] = 0.027495; 
	aaVt[ 2][ 5] = 0.275700; aaVt[ 2][ 6] = 0.270417; aaVt[ 2][ 7] = 0.362028; aaVt[ 2][ 8] = 0.290006; aaVt[ 2][ 9] = 0.087225; 
	aaVt[ 2][10] = 0.069245; aaVt[ 2][11] = 0.503060; aaVt[ 2][12] = 0.057420; aaVt[ 2][13] = 0.039769; aaVt[ 2][14] = 0.083956; 
	aaVt[ 2][15] = 0.847049; aaVt[ 2][16] = 0.471268; aaVt[ 2][17] = 0.010459; aaVt[ 2][18] = 0.093268; aaVt[ 2][19] = 0.049824; 
	aaVt[ 3][ 0] = 0.265145; aaVt[ 3][ 1] = 0.105191; aaVt[ 3][ 2] = 0.883422; aaVt[ 3][ 3] = 0.000000; aaVt[ 3][ 4] = 0.010313; 
	aaVt[ 3][ 5] = 0.205842; aaVt[ 3][ 6] = 1.599461; aaVt[ 3][ 7] = 0.311718; aaVt[ 3][ 8] = 0.134252; aaVt[ 3][ 9] = 0.011720; 
	aaVt[ 3][10] = 0.060863; aaVt[ 3][11] = 0.261101; aaVt[ 3][12] = 0.012182; aaVt[ 3][13] = 0.016577; aaVt[ 3][14] = 0.160063; 
	aaVt[ 3][15] = 0.461519; aaVt[ 3][16] = 0.178197; aaVt[ 3][17] = 0.011393; aaVt[ 3][18] = 0.051664; aaVt[ 3][19] = 0.048769; 
	aaVt[ 4][ 0] = 0.227333; aaVt[ 4][ 1] = 0.031726; aaVt[ 4][ 2] = 0.027495; aaVt[ 4][ 3] = 0.010313; aaVt[ 4][ 4] = 0.000000; 
	aaVt[ 4][ 5] = 0.004315; aaVt[ 4][ 6] = 0.005321; aaVt[ 4][ 7] = 0.050876; aaVt[ 4][ 8] = 0.016695; aaVt[ 4][ 9] = 0.046398; 
	aaVt[ 4][10] = 0.091709; aaVt[ 4][11] = 0.004067; aaVt[ 4][12] = 0.023690; aaVt[ 4][13] = 0.051127; aaVt[ 4][14] = 0.011137; 
	aaVt[ 4][15] = 0.175270; aaVt[ 4][16] = 0.079511; aaVt[ 4][17] = 0.007732; aaVt[ 4][18] = 0.042823; aaVt[ 4][19] = 0.163831; 
	aaVt[ 5][ 0] = 0.310084; aaVt[ 5][ 1] = 0.493763; aaVt[ 5][ 2] = 0.275700; aaVt[ 5][ 3] = 0.205842; aaVt[ 5][ 4] = 0.004315; 
	aaVt[ 5][ 5] = 0.000000; aaVt[ 5][ 6] = 0.960976; aaVt[ 5][ 7] = 0.128660; aaVt[ 5][ 8] = 0.315521; aaVt[ 5][ 9] = 0.054602; 
	aaVt[ 5][10] = 0.243530; aaVt[ 5][11] = 0.738208; aaVt[ 5][12] = 0.120801; aaVt[ 5][13] = 0.026235; aaVt[ 5][14] = 0.156570; 
	aaVt[ 5][15] = 0.358017; aaVt[ 5][16] = 0.248992; aaVt[ 5][17] = 0.021248; aaVt[ 5][18] = 0.062544; aaVt[ 5][19] = 0.112027; 
	aaVt[ 6][ 0] = 0.567957; aaVt[ 6][ 1] = 0.255240; aaVt[ 6][ 2] = 0.270417; aaVt[ 6][ 3] = 1.599461; aaVt[ 6][ 4] = 0.005321; 
	aaVt[ 6][ 5] = 0.960976; aaVt[ 6][ 6] = 0.000000; aaVt[ 6][ 7] = 0.250447; aaVt[ 6][ 8] = 0.104458; aaVt[ 6][ 9] = 0.046589; 
	aaVt[ 6][10] = 0.151924; aaVt[ 6][11] = 0.888630; aaVt[ 6][12] = 0.058643; aaVt[ 6][13] = 0.028168; aaVt[ 6][14] = 0.205134; 
	aaVt[ 6][15] = 0.406035; aaVt[ 6][16] = 0.321028; aaVt[ 6][17] = 0.018844; aaVt[ 6][18] = 0.055200; aaVt[ 6][19] = 0.205868; 
	aaVt[ 7][ 0] = 0.876213; aaVt[ 7][ 1] = 0.156945; aaVt[ 7][ 2] = 0.362028; aaVt[ 7][ 3] = 0.311718; aaVt[ 7][ 4] = 0.050876; 
	aaVt[ 7][ 5] = 0.128660; aaVt[ 7][ 6] = 0.250447; aaVt[ 7][ 7] = 0.000000; aaVt[ 7][ 8] = 0.058131; aaVt[ 7][ 9] = 0.051089; 
	aaVt[ 7][10] = 0.087056; aaVt[ 7][11] = 0.193243; aaVt[ 7][12] = 0.046560; aaVt[ 7][13] = 0.050143; aaVt[ 7][14] = 0.124492; 
	aaVt[ 7][15] = 0.612843; aaVt[ 7][16] = 0.136266; aaVt[ 7][17] = 0.023990; aaVt[ 7][18] = 0.037568; aaVt[ 7][19] = 0.082579; 
	aaVt[ 8][ 0] = 0.078692; aaVt[ 8][ 1] = 0.213164; aaVt[ 8][ 2] = 0.290006; aaVt[ 8][ 3] = 0.134252; aaVt[ 8][ 4] = 0.016695; 
	aaVt[ 8][ 5] = 0.315521; aaVt[ 8][ 6] = 0.104458; aaVt[ 8][ 7] = 0.058131; aaVt[ 8][ 8] = 0.000000; aaVt[ 8][ 9] = 0.020039; 
	aaVt[ 8][10] = 0.103552; aaVt[ 8][11] = 0.153323; aaVt[ 8][12] = 0.021157; aaVt[ 8][13] = 0.079807; aaVt[ 8][14] = 0.078892; 
	aaVt[ 8][15] = 0.167406; aaVt[ 8][16] = 0.101117; aaVt[ 8][17] = 0.020009; aaVt[ 8][18] = 0.286027; aaVt[ 8][19] = 0.068575; 
	aaVt[ 9][ 0] = 0.222972; aaVt[ 9][ 1] = 0.081510; aaVt[ 9][ 2] = 0.087225; aaVt[ 9][ 3] = 0.011720; aaVt[ 9][ 4] = 0.046398; 
	aaVt[ 9][ 5] = 0.054602; aaVt[ 9][ 6] = 0.046589; aaVt[ 9][ 7] = 0.051089; aaVt[ 9][ 8] = 0.020039; aaVt[ 9][ 9] = 0.000000; 
	aaVt[ 9][10] = 2.089890; aaVt[ 9][11] = 0.093181; aaVt[ 9][12] = 0.493845; aaVt[ 9][13] = 0.321020; aaVt[ 9][14] = 0.054797; 
	aaVt[ 9][15] = 0.081567; aaVt[ 9][16] = 0.376588; aaVt[ 9][17] = 0.034954; aaVt[ 9][18] = 0.086237; aaVt[ 9][19] = 3.654430; 
	aaVt[10][ 0] = 0.424630; aaVt[10][ 1] = 0.192364; aaVt[10][ 2] = 0.069245; aaVt[10][ 3] = 0.060863; aaVt[10][ 4] = 0.091709; 
	aaVt[10][ 5] = 0.243530; aaVt[10][ 6] = 0.151924; aaVt[10][ 7] = 0.087056; aaVt[10][ 8] = 0.103552; aaVt[10][ 9] = 2.089890; 
	aaVt[10][10] = 0.000000; aaVt[10][11] = 0.201204; aaVt[10][12] = 1.105667; aaVt[10][13] = 0.946499; aaVt[10][14] = 0.169784; 
	aaVt[10][15] = 0.214977; aaVt[10][16] = 0.243227; aaVt[10][17] = 0.083439; aaVt[10][18] = 0.189842; aaVt[10][19] = 1.337571; 
	aaVt[11][ 0] = 0.393245; aaVt[11][ 1] = 1.755838; aaVt[11][ 2] = 0.503060; aaVt[11][ 3] = 0.261101; aaVt[11][ 4] = 0.004067; 
	aaVt[11][ 5] = 0.738208; aaVt[11][ 6] = 0.888630; aaVt[11][ 7] = 0.193243; aaVt[11][ 8] = 0.153323; aaVt[11][ 9] = 0.093181; 
	aaVt[11][10] = 0.201204; aaVt[11][11] = 0.000000; aaVt[11][12] = 0.096474; aaVt[11][13] = 0.038261; aaVt[11][14] = 0.212302; 
	aaVt[11][15] = 0.400072; aaVt[11][16] = 0.446646; aaVt[11][17] = 0.023321; aaVt[11][18] = 0.068689; aaVt[11][19] = 0.144587; 
	aaVt[12][ 0] = 0.211550; aaVt[12][ 1] = 0.087930; aaVt[12][ 2] = 0.057420; aaVt[12][ 3] = 0.012182; aaVt[12][ 4] = 0.023690; 
	aaVt[12][ 5] = 0.120801; aaVt[12][ 6] = 0.058643; aaVt[12][ 7] = 0.046560; aaVt[12][ 8] = 0.021157; aaVt[12][ 9] = 0.493845; 
	aaVt[12][10] = 1.105667; aaVt[12][11] = 0.096474; aaVt[12][12] = 0.000000; aaVt[12][13] = 0.173052; aaVt[12][14] = 0.010363; 
	aaVt[12][15] = 0.090515; aaVt[12][16] = 0.184609; aaVt[12][17] = 0.022019; aaVt[12][18] = 0.073223; aaVt[12][19] = 0.307309; 
	aaVt[13][ 0] = 0.116646; aaVt[13][ 1] = 0.042569; aaVt[13][ 2] = 0.039769; aaVt[13][ 3] = 0.016577; aaVt[13][ 4] = 0.051127; 
	aaVt[13][ 5] = 0.026235; aaVt[13][ 6] = 0.028168; aaVt[13][ 7] = 0.050143; aaVt[13][ 8] = 0.079807; aaVt[13][ 9] = 0.321020; 
	aaVt[13][10] = 0.946499; aaVt[13][11] = 0.038261; aaVt[13][12] = 0.173052; aaVt[13][13] = 0.000000; aaVt[13][14] = 0.042564; 
	aaVt[13][15] = 0.138119; aaVt[13][16] = 0.085870; aaVt[13][17] = 0.128050; aaVt[13][18] = 0.898663; aaVt[13][19] = 0.247329; 
	aaVt[14][ 0] = 0.399143; aaVt[14][ 1] = 0.128480; aaVt[14][ 2] = 0.083956; aaVt[14][ 3] = 0.160063; aaVt[14][ 4] = 0.011137; 
	aaVt[14][ 5] = 0.156570; aaVt[14][ 6] = 0.205134; aaVt[14][ 7] = 0.124492; aaVt[14][ 8] = 0.078892; aaVt[14][ 9] = 0.054797; 
	aaVt[14][10] = 0.169784; aaVt[14][11] = 0.212302; aaVt[14][12] = 0.010363; aaVt[14][13] = 0.042564; aaVt[14][14] = 0.000000; 
	aaVt[14][15] = 0.430431; aaVt[14][16] = 0.207143; aaVt[14][17] = 0.014584; aaVt[14][18] = 0.032043; aaVt[14][19] = 0.129315; 
	aaVt[15][ 0] = 1.817198; aaVt[15][ 1] = 0.292327; aaVt[15][ 2] = 0.847049; aaVt[15][ 3] = 0.461519; aaVt[15][ 4] = 0.175270; 
	aaVt[15][ 5] = 0.358017; aaVt[15][ 6] = 0.406035; aaVt[15][ 7] = 0.612843; aaVt[15][ 8] = 0.167406; aaVt[15][ 9] = 0.081567; 
	aaVt[15][10] = 0.214977; aaVt[15][11] = 0.400072; aaVt[15][12] = 0.090515; aaVt[15][13] = 0.138119; aaVt[15][14] = 0.430431; 
	aaVt[15][15] = 0.000000; aaVt[15][16] = 1.767766; aaVt[15][17] = 0.035933; aaVt[15][18] = 0.121979; aaVt[15][19] = 0.127700; 
	aaVt[16][ 0] = 0.877877; aaVt[16][ 1] = 0.204109; aaVt[16][ 2] = 0.471268; aaVt[16][ 3] = 0.178197; aaVt[16][ 4] = 0.079511; 
	aaVt[16][ 5] = 0.248992; aaVt[16][ 6] = 0.321028; aaVt[16][ 7] = 0.136266; aaVt[16][ 8] = 0.101117; aaVt[16][ 9] = 0.376588; 
	aaVt[16][10] = 0.243227; aaVt[16][11] = 0.446646; aaVt[16][12] = 0.184609; aaVt[16][13] = 0.085870; aaVt[16][14] = 0.207143; 
	aaVt[16][15] = 1.767766; aaVt[16][16] = 0.000000; aaVt[16][17] = 0.020437; aaVt[16][18] = 0.094617; aaVt[16][19] = 0.740372; 
	aaVt[17][ 0] = 0.030309; aaVt[17][ 1] = 0.046417; aaVt[17][ 2] = 0.010459; aaVt[17][ 3] = 0.011393; aaVt[17][ 4] = 0.007732; 
	aaVt[17][ 5] = 0.021248; aaVt[17][ 6] = 0.018844; aaVt[17][ 7] = 0.023990; aaVt[17][ 8] = 0.020009; aaVt[17][ 9] = 0.034954; 
	aaVt[17][10] = 0.083439; aaVt[17][11] = 0.023321; aaVt[17][12] = 0.022019; aaVt[17][13] = 0.128050; aaVt[17][14] = 0.014584; 
	aaVt[17][15] = 0.035933; aaVt[17][16] = 0.020437; aaVt[17][17] = 0.000000; aaVt[17][18] = 0.124746; aaVt[17][19] = 0.022134; 
	aaVt[18][ 0] = 0.087061; aaVt[18][ 1] = 0.097010; aaVt[18][ 2] = 0.093268; aaVt[18][ 3] = 0.051664; aaVt[18][ 4] = 0.042823; 
	aaVt[18][ 5] = 0.062544; aaVt[18][ 6] = 0.055200; aaVt[18][ 7] = 0.037568; aaVt[18][ 8] = 0.286027; aaVt[18][ 9] = 0.086237; 
	aaVt[18][10] = 0.189842; aaVt[18][11] = 0.068689; aaVt[18][12] = 0.073223; aaVt[18][13] = 0.898663; aaVt[18][14] = 0.032043; 
	aaVt[18][15] = 0.121979; aaVt[18][16] = 0.094617; aaVt[18][17] = 0.124746; aaVt[18][18] = 0.000000; aaVt[18][19] = 0.125733; 
	aaVt[19][ 0] = 1.230985; aaVt[19][ 1] = 0.113146; aaVt[19][ 2] = 0.049824; aaVt[19][ 3] = 0.048769; aaVt[19][ 4] = 0.163831; 
	aaVt[19][ 5] = 0.112027; aaVt[19][ 6] = 0.205868; aaVt[19][ 7] = 0.082579; aaVt[19][ 8] = 0.068575; aaVt[19][ 9] = 3.654430; 
	aaVt[19][10] = 1.337571; aaVt[19][11] = 0.144587; aaVt[19][12] = 0.307309; aaVt[19][13] = 0.247329; aaVt[19][14] = 0.129315; 
	aaVt[19][15] = 0.127700; aaVt[19][16] = 0.740372; aaVt[19][17] = 0.022134; aaVt[19][18] = 0.125733; aaVt[19][19] = 0.000000; 

	vtPi[ 0] = 0.078837;
	vtPi[ 1] = 0.051238;
	vtPi[ 2] = 0.042313;
	vtPi[ 3] = 0.053066;
	vtPi[ 4] = 0.015175;
	vtPi[ 5] = 0.036713;
	vtPi[ 6] = 0.061924;
	vtPi[ 7] = 0.070852;
	vtPi[ 8] = 0.023082;
	vtPi[ 9] = 0.062056;
	vtPi[10] = 0.096371;
	vtPi[11] = 0.057324;
	vtPi[12] = 0.023771;
	vtPi[13] = 0.043296;
	vtPi[14] = 0.043911;
	vtPi[15] = 0.063403;
	vtPi[16] = 0.055897;
	vtPi[17] = 0.013272;
	vtPi[18] = 0.034399;
	vtPi[19] = 0.073101;
	
	/* Blosum62 */
	aaBlosum[ 0][ 0] = 0.000000000000; aaBlosum[ 0][ 1] = 0.735790389698; aaBlosum[ 0][ 2] = 0.485391055466; aaBlosum[ 0][ 3] = 0.543161820899; aaBlosum[ 0][ 4] = 1.459995310470; 
	aaBlosum[ 0][ 5] = 1.199705704602; aaBlosum[ 0][ 6] = 1.170949042800; aaBlosum[ 0][ 7] = 1.955883574960; aaBlosum[ 0][ 8] = 0.716241444998; aaBlosum[ 0][ 9] = 0.605899003687; 
	aaBlosum[ 0][10] = 0.800016530518; aaBlosum[ 0][11] = 1.295201266783; aaBlosum[ 0][12] = 1.253758266664; aaBlosum[ 0][13] = 0.492964679748; aaBlosum[ 0][14] = 1.173275900924; 
	aaBlosum[ 0][15] = 4.325092687057; aaBlosum[ 0][16] = 1.729178019485; aaBlosum[ 0][17] = 0.465839367725; aaBlosum[ 0][18] = 0.718206697586; aaBlosum[ 0][19] = 2.187774522005; 
	aaBlosum[ 1][ 0] = 0.735790389698; aaBlosum[ 1][ 1] = 0.000000000000; aaBlosum[ 1][ 2] = 1.297446705134; aaBlosum[ 1][ 3] = 0.500964408555; aaBlosum[ 1][ 4] = 0.227826574209; 
	aaBlosum[ 1][ 5] = 3.020833610064; aaBlosum[ 1][ 6] = 1.360574190420; aaBlosum[ 1][ 7] = 0.418763308518; aaBlosum[ 1][ 8] = 1.456141166336; aaBlosum[ 1][ 9] = 0.232036445142; 
	aaBlosum[ 1][10] = 0.622711669692; aaBlosum[ 1][11] = 5.411115141489; aaBlosum[ 1][12] = 0.983692987457; aaBlosum[ 1][13] = 0.371644693209; aaBlosum[ 1][14] = 0.448133661718; 
	aaBlosum[ 1][15] = 1.122783104210; aaBlosum[ 1][16] = 0.914665954563; aaBlosum[ 1][17] = 0.426382310122; aaBlosum[ 1][18] = 0.720517441216; aaBlosum[ 1][19] = 0.438388343772; 
	aaBlosum[ 2][ 0] = 0.485391055466; aaBlosum[ 2][ 1] = 1.297446705134; aaBlosum[ 2][ 2] = 0.000000000000; aaBlosum[ 2][ 3] = 3.180100048216; aaBlosum[ 2][ 4] = 0.397358949897; 
	aaBlosum[ 2][ 5] = 1.839216146992; aaBlosum[ 2][ 6] = 1.240488508640; aaBlosum[ 2][ 7] = 1.355872344485; aaBlosum[ 2][ 8] = 2.414501434208; aaBlosum[ 2][ 9] = 0.283017326278; 
	aaBlosum[ 2][10] = 0.211888159615; aaBlosum[ 2][11] = 1.593137043457; aaBlosum[ 2][12] = 0.648441278787; aaBlosum[ 2][13] = 0.354861249223; aaBlosum[ 2][14] = 0.494887043702; 
	aaBlosum[ 2][15] = 2.904101656456; aaBlosum[ 2][16] = 1.898173634533; aaBlosum[ 2][17] = 0.191482046247; aaBlosum[ 2][18] = 0.538222519037; aaBlosum[ 2][19] = 0.312858797993; 
	aaBlosum[ 3][ 0] = 0.543161820899; aaBlosum[ 3][ 1] = 0.500964408555; aaBlosum[ 3][ 2] = 3.180100048216; aaBlosum[ 3][ 3] = 0.000000000000; aaBlosum[ 3][ 4] = 0.240836614802; 
	aaBlosum[ 3][ 5] = 1.190945703396; aaBlosum[ 3][ 6] = 3.761625208368; aaBlosum[ 3][ 7] = 0.798473248968; aaBlosum[ 3][ 8] = 0.778142664022; aaBlosum[ 3][ 9] = 0.418555732462; 
	aaBlosum[ 3][10] = 0.218131577594; aaBlosum[ 3][11] = 1.032447924952; aaBlosum[ 3][12] = 0.222621897958; aaBlosum[ 3][13] = 0.281730694207; aaBlosum[ 3][14] = 0.730628272998; 
	aaBlosum[ 3][15] = 1.582754142065; aaBlosum[ 3][16] = 0.934187509431; aaBlosum[ 3][17] = 0.145345046279; aaBlosum[ 3][18] = 0.261422208965; aaBlosum[ 3][19] = 0.258129289418; 
	aaBlosum[ 4][ 0] = 1.459995310470; aaBlosum[ 4][ 1] = 0.227826574209; aaBlosum[ 4][ 2] = 0.397358949897; aaBlosum[ 4][ 3] = 0.240836614802; aaBlosum[ 4][ 4] = 0.000000000000; 
	aaBlosum[ 4][ 5] = 0.329801504630; aaBlosum[ 4][ 6] = 0.140748891814; aaBlosum[ 4][ 7] = 0.418203192284; aaBlosum[ 4][ 8] = 0.354058109831; aaBlosum[ 4][ 9] = 0.774894022794; 
	aaBlosum[ 4][10] = 0.831842640142; aaBlosum[ 4][11] = 0.285078800906; aaBlosum[ 4][12] = 0.767688823480; aaBlosum[ 4][13] = 0.441337471187; aaBlosum[ 4][14] = 0.356008498769; 
	aaBlosum[ 4][15] = 1.197188415094; aaBlosum[ 4][16] = 1.119831358516; aaBlosum[ 4][17] = 0.527664418872; aaBlosum[ 4][18] = 0.470237733696; aaBlosum[ 4][19] = 1.116352478606; 
	aaBlosum[ 5][ 0] = 1.199705704602; aaBlosum[ 5][ 1] = 3.020833610064; aaBlosum[ 5][ 2] = 1.839216146992; aaBlosum[ 5][ 3] = 1.190945703396; aaBlosum[ 5][ 4] = 0.329801504630; 
	aaBlosum[ 5][ 5] = 0.000000000000; aaBlosum[ 5][ 6] = 5.528919177928; aaBlosum[ 5][ 7] = 0.609846305383; aaBlosum[ 5][ 8] = 2.435341131140; aaBlosum[ 5][ 9] = 0.236202451204; 
	aaBlosum[ 5][10] = 0.580737093181; aaBlosum[ 5][11] = 3.945277674515; aaBlosum[ 5][12] = 2.494896077113; aaBlosum[ 5][13] = 0.144356959750; aaBlosum[ 5][14] = 0.858570575674; 
	aaBlosum[ 5][15] = 1.934870924596; aaBlosum[ 5][16] = 1.277480294596; aaBlosum[ 5][17] = 0.758653808642; aaBlosum[ 5][18] = 0.958989742850; aaBlosum[ 5][19] = 0.530785790125; 
	aaBlosum[ 6][ 0] = 1.170949042800; aaBlosum[ 6][ 1] = 1.360574190420; aaBlosum[ 6][ 2] = 1.240488508640; aaBlosum[ 6][ 3] = 3.761625208368; aaBlosum[ 6][ 4] = 0.140748891814; 
	aaBlosum[ 6][ 5] = 5.528919177928; aaBlosum[ 6][ 6] = 0.000000000000; aaBlosum[ 6][ 7] = 0.423579992176; aaBlosum[ 6][ 8] = 1.626891056982; aaBlosum[ 6][ 9] = 0.186848046932; 
	aaBlosum[ 6][10] = 0.372625175087; aaBlosum[ 6][11] = 2.802427151679; aaBlosum[ 6][12] = 0.555415397470; aaBlosum[ 6][13] = 0.291409084165; aaBlosum[ 6][14] = 0.926563934846; 
	aaBlosum[ 6][15] = 1.769893238937; aaBlosum[ 6][16] = 1.071097236007; aaBlosum[ 6][17] = 0.407635648938; aaBlosum[ 6][18] = 0.596719300346; aaBlosum[ 6][19] = 0.524253846338; 
	aaBlosum[ 7][ 0] = 1.955883574960; aaBlosum[ 7][ 1] = 0.418763308518; aaBlosum[ 7][ 2] = 1.355872344485; aaBlosum[ 7][ 3] = 0.798473248968; aaBlosum[ 7][ 4] = 0.418203192284; 
	aaBlosum[ 7][ 5] = 0.609846305383; aaBlosum[ 7][ 6] = 0.423579992176; aaBlosum[ 7][ 7] = 0.000000000000; aaBlosum[ 7][ 8] = 0.539859124954; aaBlosum[ 7][ 9] = 0.189296292376; 
	aaBlosum[ 7][10] = 0.217721159236; aaBlosum[ 7][11] = 0.752042440303; aaBlosum[ 7][12] = 0.459436173579; aaBlosum[ 7][13] = 0.368166464453; aaBlosum[ 7][14] = 0.504086599527; 
	aaBlosum[ 7][15] = 1.509326253224; aaBlosum[ 7][16] = 0.641436011405; aaBlosum[ 7][17] = 0.508358924638; aaBlosum[ 7][18] = 0.308055737035; aaBlosum[ 7][19] = 0.253340790190; 
	aaBlosum[ 8][ 0] = 0.716241444998; aaBlosum[ 8][ 1] = 1.456141166336; aaBlosum[ 8][ 2] = 2.414501434208; aaBlosum[ 8][ 3] = 0.778142664022; aaBlosum[ 8][ 4] = 0.354058109831; 
	aaBlosum[ 8][ 5] = 2.435341131140; aaBlosum[ 8][ 6] = 1.626891056982; aaBlosum[ 8][ 7] = 0.539859124954; aaBlosum[ 8][ 8] = 0.000000000000; aaBlosum[ 8][ 9] = 0.252718447885; 
	aaBlosum[ 8][10] = 0.348072209797; aaBlosum[ 8][11] = 1.022507035889; aaBlosum[ 8][12] = 0.984311525359; aaBlosum[ 8][13] = 0.714533703928; aaBlosum[ 8][14] = 0.527007339151; 
	aaBlosum[ 8][15] = 1.117029762910; aaBlosum[ 8][16] = 0.585407090225; aaBlosum[ 8][17] = 0.301248600780; aaBlosum[ 8][18] = 4.218953969389; aaBlosum[ 8][19] = 0.201555971750; 
	aaBlosum[ 9][ 0] = 0.605899003687; aaBlosum[ 9][ 1] = 0.232036445142; aaBlosum[ 9][ 2] = 0.283017326278; aaBlosum[ 9][ 3] = 0.418555732462; aaBlosum[ 9][ 4] = 0.774894022794; 
	aaBlosum[ 9][ 5] = 0.236202451204; aaBlosum[ 9][ 6] = 0.186848046932; aaBlosum[ 9][ 7] = 0.189296292376; aaBlosum[ 9][ 8] = 0.252718447885; aaBlosum[ 9][ 9] = 0.000000000000; 
	aaBlosum[ 9][10] = 3.890963773304; aaBlosum[ 9][11] = 0.406193586642; aaBlosum[ 9][12] = 3.364797763104; aaBlosum[ 9][13] = 1.517359325954; aaBlosum[ 9][14] = 0.388355409206; 
	aaBlosum[ 9][15] = 0.357544412460; aaBlosum[ 9][16] = 1.179091197260; aaBlosum[ 9][17] = 0.341985787540; aaBlosum[ 9][18] = 0.674617093228; aaBlosum[ 9][19] = 8.311839405458; 
	aaBlosum[10][ 0] = 0.800016530518; aaBlosum[10][ 1] = 0.622711669692; aaBlosum[10][ 2] = 0.211888159615; aaBlosum[10][ 3] = 0.218131577594; aaBlosum[10][ 4] = 0.831842640142; 
	aaBlosum[10][ 5] = 0.580737093181; aaBlosum[10][ 6] = 0.372625175087; aaBlosum[10][ 7] = 0.217721159236; aaBlosum[10][ 8] = 0.348072209797; aaBlosum[10][ 9] = 3.890963773304; 
	aaBlosum[10][10] = 0.000000000000; aaBlosum[10][11] = 0.445570274261; aaBlosum[10][12] = 6.030559379572; aaBlosum[10][13] = 2.064839703237; aaBlosum[10][14] = 0.374555687471; 
	aaBlosum[10][15] = 0.352969184527; aaBlosum[10][16] = 0.915259857694; aaBlosum[10][17] = 0.691474634600; aaBlosum[10][18] = 0.811245856323; aaBlosum[10][19] = 2.231405688913; 
	aaBlosum[11][ 0] = 1.295201266783; aaBlosum[11][ 1] = 5.411115141489; aaBlosum[11][ 2] = 1.593137043457; aaBlosum[11][ 3] = 1.032447924952; aaBlosum[11][ 4] = 0.285078800906; 
	aaBlosum[11][ 5] = 3.945277674515; aaBlosum[11][ 6] = 2.802427151679; aaBlosum[11][ 7] = 0.752042440303; aaBlosum[11][ 8] = 1.022507035889; aaBlosum[11][ 9] = 0.406193586642; 
	aaBlosum[11][10] = 0.445570274261; aaBlosum[11][11] = 0.000000000000; aaBlosum[11][12] = 1.073061184332; aaBlosum[11][13] = 0.266924750511; aaBlosum[11][14] = 1.047383450722; 
	aaBlosum[11][15] = 1.752165917819; aaBlosum[11][16] = 1.303875200799; aaBlosum[11][17] = 0.332243040634; aaBlosum[11][18] = 0.717993486900; aaBlosum[11][19] = 0.498138475304; 
	aaBlosum[12][ 0] = 1.253758266664; aaBlosum[12][ 1] = 0.983692987457; aaBlosum[12][ 2] = 0.648441278787; aaBlosum[12][ 3] = 0.222621897958; aaBlosum[12][ 4] = 0.767688823480; 
	aaBlosum[12][ 5] = 2.494896077113; aaBlosum[12][ 6] = 0.555415397470; aaBlosum[12][ 7] = 0.459436173579; aaBlosum[12][ 8] = 0.984311525359; aaBlosum[12][ 9] = 3.364797763104; 
	aaBlosum[12][10] = 6.030559379572; aaBlosum[12][11] = 1.073061184332; aaBlosum[12][12] = 0.000000000000; aaBlosum[12][13] = 1.773855168830; aaBlosum[12][14] = 0.454123625103; 
	aaBlosum[12][15] = 0.918723415746; aaBlosum[12][16] = 1.488548053722; aaBlosum[12][17] = 0.888101098152; aaBlosum[12][18] = 0.951682162246; aaBlosum[12][19] = 2.575850755315; 
	aaBlosum[13][ 0] = 0.492964679748; aaBlosum[13][ 1] = 0.371644693209; aaBlosum[13][ 2] = 0.354861249223; aaBlosum[13][ 3] = 0.281730694207; aaBlosum[13][ 4] = 0.441337471187; 
	aaBlosum[13][ 5] = 0.144356959750; aaBlosum[13][ 6] = 0.291409084165; aaBlosum[13][ 7] = 0.368166464453; aaBlosum[13][ 8] = 0.714533703928; aaBlosum[13][ 9] = 1.517359325954; 
	aaBlosum[13][10] = 2.064839703237; aaBlosum[13][11] = 0.266924750511; aaBlosum[13][12] = 1.773855168830; aaBlosum[13][13] = 0.000000000000; aaBlosum[13][14] = 0.233597909629; 
	aaBlosum[13][15] = 0.540027644824; aaBlosum[13][16] = 0.488206118793; aaBlosum[13][17] = 2.074324893497; aaBlosum[13][18] = 6.747260430801; aaBlosum[13][19] = 0.838119610178; 
	aaBlosum[14][ 0] = 1.173275900924; aaBlosum[14][ 1] = 0.448133661718; aaBlosum[14][ 2] = 0.494887043702; aaBlosum[14][ 3] = 0.730628272998; aaBlosum[14][ 4] = 0.356008498769; 
	aaBlosum[14][ 5] = 0.858570575674; aaBlosum[14][ 6] = 0.926563934846; aaBlosum[14][ 7] = 0.504086599527; aaBlosum[14][ 8] = 0.527007339151; aaBlosum[14][ 9] = 0.388355409206; 
	aaBlosum[14][10] = 0.374555687471; aaBlosum[14][11] = 1.047383450722; aaBlosum[14][12] = 0.454123625103; aaBlosum[14][13] = 0.233597909629; aaBlosum[14][14] = 0.000000000000; 
	aaBlosum[14][15] = 1.169129577716; aaBlosum[14][16] = 1.005451683149; aaBlosum[14][17] = 0.252214830027; aaBlosum[14][18] = 0.369405319355; aaBlosum[14][19] = 0.496908410676; 
	aaBlosum[15][ 0] = 4.325092687057; aaBlosum[15][ 1] = 1.122783104210; aaBlosum[15][ 2] = 2.904101656456; aaBlosum[15][ 3] = 1.582754142065; aaBlosum[15][ 4] = 1.197188415094; 
	aaBlosum[15][ 5] = 1.934870924596; aaBlosum[15][ 6] = 1.769893238937; aaBlosum[15][ 7] = 1.509326253224; aaBlosum[15][ 8] = 1.117029762910; aaBlosum[15][ 9] = 0.357544412460; 
	aaBlosum[15][10] = 0.352969184527; aaBlosum[15][11] = 1.752165917819; aaBlosum[15][12] = 0.918723415746; aaBlosum[15][13] = 0.540027644824; aaBlosum[15][14] = 1.169129577716; 
	aaBlosum[15][15] = 0.000000000000; aaBlosum[15][16] = 5.151556292270; aaBlosum[15][17] = 0.387925622098; aaBlosum[15][18] = 0.796751520761; aaBlosum[15][19] = 0.561925457442; 
	aaBlosum[16][ 0] = 1.729178019485; aaBlosum[16][ 1] = 0.914665954563; aaBlosum[16][ 2] = 1.898173634533; aaBlosum[16][ 3] = 0.934187509431; aaBlosum[16][ 4] = 1.119831358516; 
	aaBlosum[16][ 5] = 1.277480294596; aaBlosum[16][ 6] = 1.071097236007; aaBlosum[16][ 7] = 0.641436011405; aaBlosum[16][ 8] = 0.585407090225; aaBlosum[16][ 9] = 1.179091197260; 
	aaBlosum[16][10] = 0.915259857694; aaBlosum[16][11] = 1.303875200799; aaBlosum[16][12] = 1.488548053722; aaBlosum[16][13] = 0.488206118793; aaBlosum[16][14] = 1.005451683149; 
	aaBlosum[16][15] = 5.151556292270; aaBlosum[16][16] = 0.000000000000; aaBlosum[16][17] = 0.513128126891; aaBlosum[16][18] = 0.801010243199; aaBlosum[16][19] = 2.253074051176; 
	aaBlosum[17][ 0] = 0.465839367725; aaBlosum[17][ 1] = 0.426382310122; aaBlosum[17][ 2] = 0.191482046247; aaBlosum[17][ 3] = 0.145345046279; aaBlosum[17][ 4] = 0.527664418872; 
	aaBlosum[17][ 5] = 0.758653808642; aaBlosum[17][ 6] = 0.407635648938; aaBlosum[17][ 7] = 0.508358924638; aaBlosum[17][ 8] = 0.301248600780; aaBlosum[17][ 9] = 0.341985787540; 
	aaBlosum[17][10] = 0.691474634600; aaBlosum[17][11] = 0.332243040634; aaBlosum[17][12] = 0.888101098152; aaBlosum[17][13] = 2.074324893497; aaBlosum[17][14] = 0.252214830027; 
	aaBlosum[17][15] = 0.387925622098; aaBlosum[17][16] = 0.513128126891; aaBlosum[17][17] = 0.000000000000; aaBlosum[17][18] = 4.054419006558; aaBlosum[17][19] = 0.266508731426; 
	aaBlosum[18][ 0] = 0.718206697586; aaBlosum[18][ 1] = 0.720517441216; aaBlosum[18][ 2] = 0.538222519037; aaBlosum[18][ 3] = 0.261422208965; aaBlosum[18][ 4] = 0.470237733696; 
	aaBlosum[18][ 5] = 0.958989742850; aaBlosum[18][ 6] = 0.596719300346; aaBlosum[18][ 7] = 0.308055737035; aaBlosum[18][ 8] = 4.218953969389; aaBlosum[18][ 9] = 0.674617093228; 
	aaBlosum[18][10] = 0.811245856323; aaBlosum[18][11] = 0.717993486900; aaBlosum[18][12] = 0.951682162246; aaBlosum[18][13] = 6.747260430801; aaBlosum[18][14] = 0.369405319355; 
	aaBlosum[18][15] = 0.796751520761; aaBlosum[18][16] = 0.801010243199; aaBlosum[18][17] = 4.054419006558; aaBlosum[18][18] = 0.000000000000; aaBlosum[18][19] = 1.000000000000; 
	aaBlosum[19][ 0] = 2.187774522005; aaBlosum[19][ 1] = 0.438388343772; aaBlosum[19][ 2] = 0.312858797993; aaBlosum[19][ 3] = 0.258129289418; aaBlosum[19][ 4] = 1.116352478606; 
	aaBlosum[19][ 5] = 0.530785790125; aaBlosum[19][ 6] = 0.524253846338; aaBlosum[19][ 7] = 0.253340790190; aaBlosum[19][ 8] = 0.201555971750; aaBlosum[19][ 9] = 8.311839405458; 
	aaBlosum[19][10] = 2.231405688913; aaBlosum[19][11] = 0.498138475304; aaBlosum[19][12] = 2.575850755315; aaBlosum[19][13] = 0.838119610178; aaBlosum[19][14] = 0.496908410676; 
	aaBlosum[19][15] = 0.561925457442; aaBlosum[19][16] = 2.253074051176; aaBlosum[19][17] = 0.266508731426; aaBlosum[19][18] = 1.000000000000; aaBlosum[19][19] = 0.000000000000; 	

	blosPi[ 0] = 0.074; 
	blosPi[ 1] = 0.052; 
	blosPi[ 2] = 0.045; 
	blosPi[ 3] = 0.054;
	blosPi[ 4] = 0.025; 
	blosPi[ 5] = 0.034; 
	blosPi[ 6] = 0.054; 
	blosPi[ 7] = 0.074;
	blosPi[ 8] = 0.026; 
	blosPi[ 9] = 0.068; 
	blosPi[10] = 0.099; 
	blosPi[11] = 0.058;
	blosPi[12] = 0.025; 
	blosPi[13] = 0.047; 
	blosPi[14] = 0.039; 
	blosPi[15] = 0.057;
	blosPi[16] = 0.051; 
	blosPi[17] = 0.013; 
	blosPi[18] = 0.032; 
	blosPi[19] = 0.073;

	/* now, check that the matrices are symmetrical */
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aaJones[i][j] - aaJones[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: Jones model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aaDayhoff[i][j] - aaDayhoff[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: Dayhoff model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aaMtrev24[i][j] - aaMtrev24[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: mtrev24 model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aaMtmam[i][j] - aaMtmam[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: mtmam model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aartREV[i][j] - aartREV[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: aartREV model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aaWAG[i][j] - aaWAG[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: aaWAG model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aacpREV[i][j] - aacpREV[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: cpREV model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aaVt[i][j] - aaVt[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: Vt model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			diff = aaBlosum[i][j] - aaBlosum[j][i];
			if (diff < 0.0)
				diff = -diff;
			if (diff > 0.001)
				{
				MrBayesPrint ("%s   ERROR: Blosum model is not symmetrical.\n");
				return (ERROR);
				}
			}
		}
	
	/* rescale stationary frequencies, to make certain they sum to 1.0 */
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += jonesPi[i];
	for (i=0; i<20; i++)
		jonesPi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += dayhoffPi[i];
	for (i=0; i<20; i++)
		dayhoffPi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += mtrev24Pi[i];
	for (i=0; i<20; i++)
		mtrev24Pi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += mtmamPi[i];
	for (i=0; i<20; i++)
		mtmamPi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += rtrevPi[i];
	for (i=0; i<20; i++)
		rtrevPi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += wagPi[i];
	for (i=0; i<20; i++)
		wagPi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += cprevPi[i];
	for (i=0; i<20; i++)
		cprevPi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += vtPi[i];
	for (i=0; i<20; i++)
		vtPi[i] /= sum;
	sum = 0.0;
	for (i=0; i<20; i++)
		sum += blosPi[i];
	for (i=0; i<20; i++)
		blosPi[i] /= sum;
		
	/* multiply entries by amino acid frequencies */
	for (i=0; i<20; i++)
		{
		for (j=0; j<20; j++)
			{
			aaJones[i][j]   *= jonesPi[j];
			aaDayhoff[i][j] *= dayhoffPi[j];
			aaMtrev24[i][j] *= mtrev24Pi[j];
			aaMtmam[i][j]   *= mtmamPi[j];
			aartREV[i][j]   *= rtrevPi[j];
			aaWAG[i][j]     *= wagPi[j];
			aacpREV[i][j]   *= cprevPi[j];
			aaVt[i][j]      *= vtPi[j];
			aaBlosum[i][j] *= blosPi[j];
			}
		}
		
	/* rescale, so branch lengths are in terms of expected number of
	   amino acid substitutions per site */
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += jonesPi[i] * aaJones[i][j];
			scaler += jonesPi[j] * aaJones[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aaJones[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += dayhoffPi[i] * aaDayhoff[i][j];
			scaler += dayhoffPi[j] * aaDayhoff[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aaDayhoff[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += mtrev24Pi[i] * aaMtrev24[i][j];
			scaler += mtrev24Pi[j] * aaMtrev24[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aaMtrev24[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += mtmamPi[i] * aaMtmam[i][j];
			scaler += mtmamPi[j] * aaMtmam[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aaMtmam[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += rtrevPi[i] * aartREV[i][j];
			scaler += rtrevPi[j] * aartREV[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aartREV[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += wagPi[i] * aaWAG[i][j];
			scaler += wagPi[j] * aaWAG[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aaWAG[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += cprevPi[i] * aacpREV[i][j];
			scaler += cprevPi[j] * aacpREV[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aacpREV[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += vtPi[i] * aaVt[i][j];
			scaler += vtPi[j] * aaVt[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aaVt[i][j] *= scaler;
	scaler = 0.0;
	for (i=0; i<20; i++)
		{
		for (j=i+1; j<20; j++)
			{
			scaler += blosPi[i] * aaBlosum[i][j];
			scaler += blosPi[j] * aaBlosum[j][i];
			}
		}
	scaler = 1.0 / scaler;
	for (i=0; i<20; i++)
		for (j=0; j<20; j++)
			aaBlosum[i][j] *= scaler;
	
	/* set diagonal of matrix */
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aaJones[i][j];
			}
		aaJones[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aaDayhoff[i][j];
			}
		aaDayhoff[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aaMtrev24[i][j];
			}
		aaMtrev24[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aaMtmam[i][j];
			}
		aaMtmam[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aartREV[i][j];
			}
		aartREV[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aaWAG[i][j];
			}
		aaWAG[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aacpREV[i][j];
			}
		aacpREV[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aaVt[i][j];
			}
		aaVt[i][i] = -sum;
		}
	for (i=0; i<20; i++)
		{
		sum = 0.0;
		for (j=0; j<20; j++)
			{
			if (i != j)
				sum += aaBlosum[i][j];
			}
		aaBlosum[i][i] = -sum;
		}

#	if 0
	for (i=0; i<20; i++)
		{
		MrBayesPrint ("%s   ", spacer);
		for (j=0; j<20; j++)
			{
			if (aaJones[i][j] < 0.0)
				MrBayesPrint ("%1.3lf ", aaJones[i][j]);
			else
				MrBayesPrint (" %1.3lf ", aaJones[i][j]);
			}
		MrBayesPrint ("\n");
		}
#	endif

	return (NO_ERROR);
	
}





int SetLocalTaxa (void)

{

	int			i, j;
	
	/* free memory if allocated */
	if (memAllocs[ALLOC_LOCTAXANAMES] == YES)
		{
		free (localTaxonNames);
		localTaxonNames = NULL;
		memAllocs[ALLOC_LOCTAXANAMES] = NO;
		}
	if (memAllocs[ALLOC_LOCALTAXONCALIBRATION] == YES)
		{
		free (localTaxonCalibration);
		localTaxonCalibration = NULL;
		memAllocs[ALLOC_LOCALTAXONCALIBRATION] = NO;
		}
	
	/* count number of non-excluded taxa */
	numLocalTaxa = 0;
	for (i=0; i<numTaxa; i++)
		{
		if (taxaInfo[i].isDeleted == NO)
			numLocalTaxa++;
		}
		
	/* allocate memory */
	localTaxonNames = (char **)SafeCalloc((size_t) numLocalTaxa, sizeof(char *));
	if (!localTaxonNames)
		return (ERROR);
	memAllocs[ALLOC_LOCTAXANAMES] = YES;

	localTaxonCalibration = (Calibration **)SafeCalloc((size_t) numLocalTaxa, sizeof(Calibration *));
	if (!localTaxonCalibration)
		return (ERROR);
	memAllocs[ALLOC_LOCALTAXONCALIBRATION] = YES;
		
	/* point to names and calibrations of non-excluded taxa */
    localOutGroup = 0;
    for (i=j=0; i<numTaxa; i++)
		{
		if (taxaInfo[i].isDeleted == NO)
			{
            localTaxonNames[j] = taxaNames[i];
			localTaxonCalibration[j] = &tipCalibration[i];
            if (i == outGroupNum)
                localOutGroup = j;
            j++;
			}
		}

#	if 0
	/* show non-excluded taxa */
	for (i=0; i<numLocalTaxa; i++)
		MrBayesPrint ("%s   %4d %s\n", spacer, i+1, localTaxonNames[i]);
#	endif
		
	return (NO_ERROR);
	
}





/*----------------------------------------------------------------------------
|
|	SetModelDefaults: This function will set up model defaults in modelParams.
|		It will also initialize parameters and moves by calling SetUpAnalysis.
|
-----------------------------------------------------------------------------*/
int SetModelDefaults (void)

{

	int			j;

	MrBayesPrint ("%s   Setting model defaults\n", spacer);
	MrBayesPrint ("%s   Seed (for generating default start values) = %d\n", spacer, globalSeed);

	if (InitializeLinks () == ERROR)
		{
		MrBayesPrint ("%s   Problem initializing link table\n", spacer);
		return (ERROR);
		}

	/* Check that models are allocated */
	if (memAllocs[ALLOC_MODEL] == NO)
		{
		MrBayesPrint ("%s   Model not allocated in SetModelDefaults\n", spacer);
		return (ERROR);
		}

	/* model parameters */
	for (j=0; j<numCurrentDivisions; j++)
		{
		modelParams[j] = defaultModel;                      /* start with default settings */
		
		modelParams[j].dataType = DataType (j);             /* data type for partition                      */

		if (modelParams[j].dataType == STANDARD)		    /* set default ascertainment bias for partition */
			strcpy(modelParams[j].coding, "Variable"); 
		else if (modelParams[j].dataType == RESTRICTION)
			strcpy(modelParams[j].coding, "Noabsencesites");
		else
			strcpy(modelParams[j].coding, "All");

        SetCode (j);
		modelParams[j].nStates = NumStates (j);			    /* number of states for partition             */

        modelParams[j].activeConstraints = (int *) SafeCalloc(numDefinedConstraints, sizeof(int));  /* allocate space for active constraints (yes/no) */
		}

	return (NO_ERROR);

}





/*----------------------------------------------------------------------------
|
|	SetModelInfo: This function will set up model info using model
|		params
|
-----------------------------------------------------------------------------*/
int SetModelInfo (void)

{

	int				i, j, chn, ts;
	ModelParams		*mp;
	ModelInfo		*m;
	
	/* wipe all model settings */
	inferSiteRates = NO;
	inferAncStates = NO;
    inferSiteOmegas = NO;

	for (i=0; i<numCurrentDivisions; i++)
		{
		m = &modelSettings[i];

		/* make certain that we set this intentionally to "NO" so we 
		   calculate cijk information and calculate cond likes when needed */
		m->upDateCijk = YES;
        m->upDateCl = YES;
        m->upDateAll = YES;

		/* make certain that we start with a parsimony branch length of zero */
		for (j=0; j<MAX_CHAINS; j++)
			m->parsTreeLength[j*2] = m->parsTreeLength[j*2+1] = 0.0;

		m->tRatio = NULL;
		m->revMat = NULL;
		m->omega = NULL;
		m->stateFreq = NULL;
		m->shape = NULL;
		m->pInvar = NULL;
		m->correlation = NULL;
		m->switchRates = NULL;
		m->rateMult = NULL;
		m->topology = NULL;
		m->brlens = NULL;
		m->speciationRates = NULL;
		m->extinctionRates = NULL;
		m->popSize = NULL;
		m->aaModel = NULL;
		m->cppRate = NULL;
		m->cppEvents = NULL;
		m->cppMultDev = NULL;
		m->tk02var = NULL;
		m->tk02BranchRates = NULL;
		m->igrvar = NULL;
		m->igrBranchRates = NULL;
        m->clockRate = NULL;

		m->CondLikeDown = NULL;
		m->CondLikeRoot = NULL;
		m->CondLikeScaler = NULL;
		m->Likelihood = NULL;
		m->TiProbs = NULL;

		m->CondLikeUp = NULL;
		m->StateCode = NULL;
		m->PrintAncStates = NULL;
		m->PrintSiteRates = NULL;
		
		m->printPosSel = NO;
		m->printAncStates = NO;
		m->printSiteRates = NO;

		m->nStates = NULL;
		m->bsIndex = NULL;
		m->cType = NULL;
		m->tiIndex = NULL;

		m->gibbsGamma = NO;
		m->gibbsFreq = 0;

		m->parsimonyBasedMove = NO;

#if defined (BEAGLE_ENABLED)
        m->beagleInstance = -1;               /* beagle instance                              */
        m->logLikelihoods = NULL;             /* array of log likelihoods from Beagle         */
        m->inRates = NULL;                    /* array of category rates for Beagle           */
        m->branchLengths = NULL;              /* array of branch lengths for Beagle           */
        m->tiProbIndices = NULL;              /* array of trans prob indices for Beagle       */
        m->inWeights = NULL;                  /* array of weights for Beagle root likelihood  */
        m->bufferIndices = NULL;              /* array of partial indices for root likelihood */
        m->childBufferIndices = NULL;         /* array of child partial indices (unrooted)    */
        m->childTiProbIndices = NULL;         /* array of child ti prob indices (unrooted)    */
        m->cumulativeScaleIndices = NULL;     /* array of cumulative scale indices            */
#endif

        /* likelihood calculator flags */
        m->useSSE = NO;                       /* use SSE code for this partition?             */
        m->useBeagle = NO;                    /* use Beagle for this partition?               */

        /* set all memory pointers to NULL */
        m->parsSets = NULL;
        m->numParsSets = 0;
        m->parsNodeLens = NULL;
        m->numParsNodeLens = 0;

        m->condLikes = NULL;
        m->tiProbs = NULL;
        m->scalers = NULL;
        m->numCondLikes = 0;
        m->numTiProbs = 0;
        m->numScalers = 0;

        m->condLikeIndex = NULL;
        m->condLikeScratchIndex = NULL;
        m->tiProbsIndex = NULL;
        m->tiProbsScratchIndex = NULL;
        m->nodeScalerIndex = NULL;
        m->nodeScalerScratchIndex = NULL;
        m->siteScalerIndex = NULL;
        m->siteScalerScratchIndex = -1;

        m->cijks = NULL;
        m->nCijkParts = 0;
        m->cijkIndex = NULL;
        m->cijkScratchIndex = -1;
        }

	/* set state of all chains to zero */
	for (chn=0; chn<numGlobalChains; chn++)
		state[chn] = 0;

	/* fill in modelSettings info with some basic model characteristics */
	for (i=0; i<numCurrentDivisions; i++)
		{
		mp = &modelParams[i];
		m = &modelSettings[i];
		
		if (!strcmp(mp->nucModel,"Protein") && (mp->dataType == DNA || mp->dataType == RNA))
            m->dataType = PROTEIN;
        else
            m->dataType = mp->dataType;

		/* nuc model structure */
		if (!strcmp(mp->nucModel, "4by4"))
			m->nucModelId = NUCMODEL_4BY4;
		else if (!strcmp(mp->nucModel, "Doublet"))
			m->nucModelId = NUCMODEL_DOUBLET;
		else if (!strcmp(mp->nucModel, "Protein"))
            m->nucModelId = NUCMODEL_AA;
        else /* if (!strcmp(mp->nucModelId, "Codon")) */
			m->nucModelId = NUCMODEL_CODON;
			
		/* model nst */
		if (!strcmp(mp->nst, "1"))
			m->nst = 1;
		else if (!strcmp(mp->nst, "2"))
			m->nst = 2;
		else if (!strcmp(mp->nst, "6"))
			m->nst = 6;
		else
			m->nst = NST_MIXED;
			
		/* We set the aa model here. We have two options. First, the model
		   could be fixed, in which case mp->aaModel has been set. We then
		   go ahead and also set the model settings. Second, the model
		   could be mixed. In this case, the amino acid matrix is considered
		   a parameter, and we will deal with it below. It doesn't hurt
		   to set it here anyway (it will be overwritten later). */
		if (!strcmp(mp->aaModelPr, "Fixed"))
			{
			if (!strcmp(mp->aaModel, "Poisson"))
				m->aaModelId = AAMODEL_POISSON;
			else if (!strcmp(mp->aaModel, "Equalin"))
				m->aaModelId = AAMODEL_EQ;
			else if (!strcmp(mp->aaModel, "Jones"))
				m->aaModelId = AAMODEL_JONES;
			else if (!strcmp(mp->aaModel, "Dayhoff"))
				m->aaModelId = AAMODEL_DAY;
			else if (!strcmp(mp->aaModel, "Mtrev"))
				m->aaModelId = AAMODEL_MTREV;
			else if (!strcmp(mp->aaModel, "Mtmam"))
				m->aaModelId = AAMODEL_MTMAM;
			else if (!strcmp(mp->aaModel, "Wag"))
				m->aaModelId = AAMODEL_WAG;
			else if (!strcmp(mp->aaModel, "Rtrev"))
				m->aaModelId = AAMODEL_RTREV;
			else if (!strcmp(mp->aaModel, "Cprev"))
				m->aaModelId = AAMODEL_CPREV;
			else if (!strcmp(mp->aaModel, "Vt"))
				m->aaModelId = AAMODEL_VT;
			else if (!strcmp(mp->aaModel, "Blosum"))
				m->aaModelId = AAMODEL_BLOSUM;
			else if (!strcmp(mp->aaModel, "Gtr"))
				m->aaModelId = AAMODEL_GTR;
			else
				{
				MrBayesPrint ("%s   Uncertain amino acid model\n", spacer);
				return (ERROR);
				}
			}
		else
			m->aaModelId = -1;
			
		/* parsimony model? */
		if (!strcmp(mp->parsModel, "Yes"))
			m->parsModelId = YES;
		else
			m->parsModelId = NO;

		/* number of gamma categories */
		if (activeParams[P_SHAPE][i] > 0)
			m->numGammaCats = mp->numGammaCats;
		else
			m->numGammaCats = 1;

		/* number of beta categories */
		if (mp->dataType == STANDARD && !(AreDoublesEqual(mp->symBetaFix, -1.0, 0.00001) == YES && !strcmp(mp->symPiPr,"Fixed")))
			m->numBetaCats = mp->numBetaCats;
		else
			m->numBetaCats = 1;

		/* number of omega categories */
		if ((mp->dataType == DNA || mp->dataType == RNA) && (!strcmp(mp->omegaVar, "Ny98") || !strcmp(mp->omegaVar, "M3")) && !strcmp(mp->nucModel, "Codon"))
			{
			m->numOmegaCats = 3;
			m->numGammaCats = 1; /* if we are here, then we cannot have gamma or beta variation */
			m->numBetaCats = 1;
			}
		else if ((mp->dataType == DNA || mp->dataType == RNA) && !strcmp(mp->omegaVar, "M10") && !strcmp(mp->nucModel, "Codon"))
			{
			m->numOmegaCats = mp->numM10BetaCats + mp->numM10GammaCats;
			m->numGammaCats = 1; /* if we are here, then we cannot have gamma or beta variation */
			m->numBetaCats = 1;
			}
		else
			m->numOmegaCats = 1;

		/* number of transition matrices depends on numGammaCats, numBetaCats, and numOmegaCats */
		m->numTiCats = m->numGammaCats * m->numBetaCats * m->numOmegaCats;

		/* TODO: check that numStates and numModelStates are set
			appropriately for codon and doublet models */

		/* number of observable states */
		if (m->dataType == STANDARD)
			m->numStates = 0;	/* zero, meaining variable */
		else if (!strcmp(mp->nucModel,"Protein") && (mp->dataType == DNA || mp->dataType == RNA))
            m->numStates = 20;
        else
			m->numStates = mp->nStates;
		
		/* number of model states including hidden ones */
		if ((mp->dataType == DNA || mp->dataType == RNA) && !strcmp (mp->covarionModel, "Yes") && !strcmp(mp->nucModel, "4by4"))
			m->numModelStates = mp->nStates * 2;
		else if (mp->dataType == PROTEIN && !strcmp (mp->covarionModel, "Yes"))
			m->numModelStates = mp->nStates * 2;
		else if ((mp->dataType == DNA || mp->dataType == RNA) && !strcmp(mp->nucModel,"Protein") && !strcmp (mp->covarionModel, "Yes"))
			m->numModelStates = 20 * 2;
		else if (mp->dataType == CONTINUOUS)
			m->numModelStates = 0;
		else if (mp->dataType == STANDARD)
			{
			/* use max possible for now; we don't know what chars will be included */
			m->numModelStates = 10;
			}
		else
			m->numModelStates = m->numStates;
			
		/* Fill in some information for calculating cijk. We will use m->cijkLength to 
		   figure out if we need to diagonalize Q to calculate transition probabilities.
		   If cijkLength = 0, then we won't bother. We use cijkLength later in this function. */
		m->cijkLength = 0;
		m->nCijkParts = 1;
		if (m->dataType == PROTEIN)
			{
			ts = m->numModelStates;
			m->cijkLength = (ts * ts * ts) + (2 * ts);
			if (!strcmp (mp->covarionModel, "Yes"))
				{
				m->cijkLength *= m->numGammaCats;
				m->nCijkParts = m->numGammaCats;
				}
			}
		else if (m->dataType == STANDARD)
			{
			/* set to 0 for now, update in ProcessStdChars */
			m->nCijkParts = 0;
			}
		else if (m->dataType == DNA || m->dataType == RNA)
			{
			if (m->nucModelId == NUCMODEL_4BY4)
				{
				if (!strcmp (mp->covarionModel, "No") && m->nst != 6 && m->nst != 203)
					m->cijkLength = 0;
				else
					{
					ts = m->numModelStates;
					m->cijkLength = (ts * ts * ts) + (2 * ts);
					}
				if (!strcmp (mp->covarionModel, "Yes"))
					{
					m->cijkLength *= m->numGammaCats;
					m->nCijkParts = m->numGammaCats;
					}
				}
			else if (m->nucModelId == NUCMODEL_DOUBLET)
				{
				ts = m->numModelStates;
				m->cijkLength = (ts * ts * ts) + (2 * ts);
				}
			else if (m->nucModelId == NUCMODEL_CODON)
				{
				ts = m->numModelStates;
				m->cijkLength = (ts * ts * ts) + (2 * ts);
				m->cijkLength *= m->numOmegaCats;
				m->nCijkParts = m->numOmegaCats;
				}
			else
				{
				MrBayesPrint ("%s   ERROR: Something is wrong if you are here.\n");
				return ERROR;
				}
			}

		/* check if we should calculate ancestral states */
		if (!strcmp(mp->inferAncStates,"Yes"))
			{
			if (m->dataType == PROTEIN && !strcmp(mp->covarionModel, "No"))
				m->printAncStates = YES;
			else if (m->dataType == DNA || m->dataType == RNA)
				{
				if (!strcmp(mp->nucModel,"4by4") && !strcmp(mp->covarionModel, "No"))
					m->printAncStates = YES;
                if (!strcmp(mp->nucModel,"Doublet"))
                    m->printAncStates = YES;
                if (!strcmp(mp->nucModel,"Codon") && !strcmp(mp->omegaVar,"Equal"))
                    m->printAncStates = YES;
				}
			else if (m->dataType == STANDARD || m->dataType == RESTRICTION)
				m->printAncStates = YES;
			if (m->printAncStates == YES)
				inferAncStates = YES;
            else
                MrBayesPrint ("%s   Print out of ancestral states is not applicable for devision %d.\n",spacer,i);
			}

		/* check if we should calculate site rates */
		if (!strcmp(mp->inferSiteRates,"Yes"))
			{
			if (m->numGammaCats > 1)
				{
				m->printSiteRates = YES;
				inferSiteRates = YES;
				}
			}

		/* check if we should calculate positive selection */
		if (!strcmp(mp->inferPosSel, "Yes"))
			{
			if (m->numOmegaCats > 1)
				{
				m->printPosSel = YES;
				inferPosSel = YES;
				}
			}

		/* check if we should calculate site omegas */
		if (!strcmp(mp->inferSiteOmegas, "Yes"))
			{
			if (m->numOmegaCats > 1)
				{
				m->printSiteOmegas = YES;
				inferSiteOmegas = YES;
				}
			}
		/* check if we should use gibbs sampling of gamma (don't use it with pinvar or invgamma) */
		if (!strcmp(mp->useGibbs,"Yes") && m->numGammaCats > 1)
			{
			if (m->dataType == DNA || m->dataType == RNA || m->dataType == PROTEIN)
				{
				if (activeParams[P_CORREL][i] <= 0 && m->printSiteRates == NO && activeParams[P_PINVAR][i] <= 0)
					{
					m->gibbsGamma = YES;
					m->gibbsFreq = mp->gibbsFreq;
					}
				}
			}
		}

	return (NO_ERROR);	
}





/*-----------------------------------------------------------------
|
|	SetModelParams: Set up parameter structs for all model
|		parameters, including trees
|
|----------------------------------------------------------------*/
int SetModelParams (void)

{

	int			    c, i, j, k, n, n1, n2, *isPartTouched, numRelParts, nRelParts, areAllPartsParsimony,
                    nClockBrlens, nRelaxedBrlens, nCalibratedBrlens;
	char		    tempCodon[15], tempMult[15], *tempStr,temp[30];
    char static     *partString=NULL;/*mad static to avoid posible memory leak on return ERROR if it would be introduced later */ 
	Param		    *p;
	ModelParams     *mp;
    ModelInfo       *m;
	int             tempStrSize = 300;

	tempStr = (char *) SafeMalloc((size_t) (tempStrSize * sizeof(char)));
	isPartTouched = (int *) SafeMalloc ((size_t) (numCurrentDivisions * sizeof (int)));
	if (!tempStr || !isPartTouched)
		{
		MrBayesPrint ("%s   Problem allocating tempString (%d) or isPartTouched (%d)\n", spacer,
			tempStrSize * sizeof(char), numCurrentDivisions * sizeof(int));
		return (ERROR);
		}

#	if defined DEBUG_SETCHAINPARAMS
	/* only for debugging */
	MrBFlt		lnPriorRatio = 0.0, lnProposalRatio = 0.0;
#	endif

    	/* allocate space for parameters */
	if (memAllocs[ALLOC_PARAMS] == YES)
		{
	    for (i=0; i<numParams; i++)
            {
            SafeFree ((void **)&params[i].name);
            if (params[i].paramHeader)
                {
                free (params[i].paramHeader);
                params[i].paramHeader = NULL;
                }
            }
        free (params);
		free (relevantParts);
		params = NULL;
		relevantParts = NULL;
		memAllocs[ALLOC_PARAMS] = NO;
		}

	/* wipe all chain parameter information */
	numParams = 0;
	numTrees = 0;
	chainHasAdgamma = NO;

	/* figure out number of parameters */
	/* this relies on activeParams[j][i] being set to 1, 2, ..., numParams */
	/* which is taken care of in SetUpLinkTable () */
	nRelParts = 0;
	for (j=0; j<NUM_LINKED; j++)
		{
		for (i=0; i<numCurrentDivisions; i++)
			{
			if (activeParams[j][i] > numParams)
				numParams = activeParams[j][i];
			if (activeParams[j][i] > 0)
				nRelParts++;
			}
		}

	params = (Param *) SafeMalloc (numParams * sizeof(Param));
	relevantParts = (int *) SafeMalloc (nRelParts * sizeof(int));
	if (!params || !relevantParts)
		{
		MrBayesPrint ("%s   Problem allocating params and relevantParts\n", spacer);
		if (params)
			free (params);
		if (relevantParts)
			free (relevantParts);
        free (tempStr);
		return ERROR;
		}
	else
		memAllocs[ALLOC_PARAMS] = YES;

	/* fill in info on each parameter */
	nRelParts = 0;	/* now cumulative number of relevant partitions */
	for (k=0; k<numParams; k++)
		{
		p = &params[k];

		/* find affected partitions */
		numRelParts = 0;
		for (j=0; j<NUM_LINKED; j++)
			{
			for (i=0; i<numCurrentDivisions; i++)
				{
				if (activeParams[j][i] == k + 1)
					{
					numRelParts++;
					isPartTouched[i] = YES;
					}
				else
					isPartTouched[i] = NO;
				}
			if (numRelParts > 0)
				break;
			}        

		/* find pointer to modelParams and modelSettings of first relevant partition */
		/* this will be handy later on */
		for (i=0; i<numCurrentDivisions; i++)
			if (isPartTouched[i] == YES)
				break;
		mp = &modelParams[i];
        m  = &modelSettings[i];
		
        /* Set default min and max */
        p->min = p->max = NEG_INFINITY;

        /* Parameter nValues and nSubValues, which are needed for memory allocation
		   are calculated for each case in the code below. nSympi, however, is
		   only used for one special type of parameter and it therefore makes
		   sense to initialize it to 0 here. The same applies to hasBinaryStd
           and nIntValues. To be safe, we set all to 0 here. */
        p->nValues = 0;
        p->nSubValues = 0;
        p->nIntValues = 0;
        p->nSympi = 0;
        p->hasBinaryStd = NO;
		
		/* should this parameter be printed to a file? */
		p->printParam = NO;

		/* set print subparams to 0 */
		p->nPrintSubParams = 0;
		
		/* check constraints for tree parameter ? */
		p->checkConstraints = NO;

		/* set index number of parameter */
		p->index = k;
		
        /* set prior function to NULL */
        p->LnPriorRatio = NULL;

        /* set prior paramters to NULL */
        p->priorParams = NULL;

        /* set affectsLikelihood to NO */
        p->affectsLikelihood = NO;

		/* set cpp event pointers to NULL */
		p->nEvents = NULL;
		p->position = NULL;
		p->rateMult = NULL;

        /* set header and name to NULL */
        p->paramHeader = NULL;
        p->name = NULL;

        /* set up relevant partitions */
		p->nRelParts = numRelParts;
		p->relParts = relevantParts + nRelParts;
		nRelParts += numRelParts;
		for (i=n=0; i<numCurrentDivisions; i++)
            if (isPartTouched[i] == YES)
				p->relParts[n++] = i;

        /* get partition descriptor */
        SafeStrcat(&partString,"");
        FillRelPartsString (p, &partString);
            
        /* set up information for parameter */
		if (j == P_TRATIO)
			{
			/* Set up tratio ****************************************************************************************/
			p->paramType = P_TRATIO;
			p->nValues = 1;         /* we store only the ratio */
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].tRatio = p;
	
            p->paramTypeName = "Transition and transversion rates";
			SafeStrcat(&p->name, "Tratio");
            SafeStrcat(&p->name, partString);			

			/* find the parameter x prior type */
			if (!strcmp(mp->tRatioPr,"Beta"))
				{
				p->paramId = TRATIO_DIR;
				}
			else
				p->paramId = TRATIO_FIX;
				
			if (p->paramId != TRATIO_FIX)
				p->printParam = YES;
			if (!strcmp(mp->tratioFormat,"Ratio"))
				{
				/* report ti/tv ratio */
				SafeStrcat (&p->paramHeader,"kappa");
				SafeStrcat (&p->paramHeader, partString);
				}
			else
				{
				/* report prop. of ratesum (Dirichlet) */
				SafeStrcat (&p->paramHeader,"ti");
                SafeStrcat (&p->paramHeader, partString);
                SafeStrcat (&p->paramHeader, "\ttv");
                SafeStrcat (&p->paramHeader, partString);
				}
			}
        else if (j == P_REVMAT)
			{
			/* Set up revMat ****************************************************************************************/
			p->paramType = P_REVMAT;
			if (m->dataType == PROTEIN)
				p->nValues = 190;
			else
				p->nValues = 6;
			p->nSubValues = 0;
            if (!strcmp(mp->nst, "Mixed"))
                p->nIntValues = 6;
            else
                p->nIntValues = 0;
            p->min = 0.0;
            p->max = 1.0;       /* adjust later for REVMAT_MIX, see a few lines below */
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].revMat = p;

            p->paramTypeName = "Rates of reversible rate matrix";
			SafeStrcat(&p->name, "Revmat");
			SafeStrcat(&p->name, partString);			

			/* find the parameter x prior type */
			if (!strcmp(mp->nst, "Mixed"))
                {
                p->paramId = REVMAT_MIX;
                p->max = 1E6;   /* some large value */
                }
            else if (!strcmp(mp->revMatPr,"Dirichlet"))
				p->paramId = REVMAT_DIR;
			else
				p->paramId = REVMAT_FIX;

			if (p->paramId != REVMAT_FIX)
				p->printParam = YES;
			if (m->dataType == PROTEIN)
				{
				for (n1=0; n1<20; n1++)
					{
					for (n2=n1+1; n2<20; n2++)
						{
						if (n1==0 && n2==1)
                            {
                            sprintf (temp, "r(%c<->%c)", StateCode_AA(n1), StateCode_AA(n2));
							SafeStrcat (&p->paramHeader, temp);
							SafeStrcat (&p->paramHeader, partString);
                            }
						else
							{
                            sprintf (temp, "\tr(%c<->%c)", StateCode_AA(n1), StateCode_AA(n2));
							SafeStrcat (&p->paramHeader, temp);
							SafeStrcat (&p->paramHeader, partString);
							}
						}
					}
				}
			else
                {
				for (n1=0; n1<4; n1++)
					{
					for (n2=n1+1; n2<4; n2++)
						{
						if (n1==0 && n2==1)
                            {
                            sprintf (temp, "r(%c<->%c)", StateCode_NUC4(n1), StateCode_NUC4(n2));
							SafeStrcat (&p->paramHeader, temp);
							SafeStrcat (&p->paramHeader, partString);
                            }
						else
							{
                            sprintf (temp, "\tr(%c<->%c)", StateCode_NUC4(n1), StateCode_NUC4(n2));
							SafeStrcat (&p->paramHeader, temp);
							SafeStrcat (&p->paramHeader, partString);
							}
						}
					}
                if (p->paramId == REVMAT_MIX)
                    {
                    sprintf (temp, "\tgtrsubmodel");
					SafeStrcat (&p->paramHeader, temp);
					SafeStrcat (&p->paramHeader, partString);
                    sprintf (temp, "\tk_revmat");
					SafeStrcat (&p->paramHeader, temp);
					SafeStrcat (&p->paramHeader, partString);
                    }
				}
			}
		else if (j == P_OMEGA)
			{
			/* Set up omega *****************************************************************************************/
			p->paramType = P_OMEGA;
            p->min = 0.0;
            p->max = POS_INFINITY;
			if (!strcmp(mp->omegaVar, "M3"))
				{
				p->nValues = 3;         /* omega values */
				p->nSubValues = 6;      /* category frequencies plus cache */
				for (i=0; i<numCurrentDivisions; i++)
					if (isPartTouched[i] == YES)
						modelSettings[i].omega = p;
			
				/* find the parameter x prior type */
				if (     !strcmp(mp->m3omegapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_EF;
				else if (!strcmp(mp->m3omegapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_ED;
				else if (!strcmp(mp->m3omegapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_FF;
				else if (!strcmp(mp->m3omegapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_FD;

				if (p->paramId != OMEGA_FF)
					p->printParam = YES;

				SafeStrcat (&p->paramHeader, "omega(1)");
				SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tomega(2)");
				SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tomega(3)");
				SafeStrcat (&p->paramHeader, partString);
					
				SafeStrcat (&p->paramHeader, "\tpi(1)");
				SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tpi(2)");
				SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tpi(3)");
				SafeStrcat (&p->paramHeader, partString);
				}
			else if (!strcmp(mp->omegaVar, "M10"))
				{
				p->nValues = mp->numM10BetaCats + mp->numM10GammaCats;
				p->nSubValues = mp->numM10BetaCats + mp->numM10GammaCats + 8;
				for (i=0; i<numCurrentDivisions; i++)
					if (isPartTouched[i] == YES)
						modelSettings[i].omega = p;

				/* find the parameter x prior type */
				if (    !strcmp(mp->m10betapr, "Uniform")     && !strcmp(mp->m10gammapr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10UUB;
				else if (!strcmp(mp->m10betapr, "Uniform")     && !strcmp(mp->m10gammapr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10UUF;
				else if (!strcmp(mp->m10betapr, "Uniform")     && !strcmp(mp->m10gammapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10UEB;
				else if (!strcmp(mp->m10betapr, "Uniform")     && !strcmp(mp->m10gammapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10UEF;
				else if (!strcmp(mp->m10betapr, "Uniform")     && !strcmp(mp->m10gammapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10UFB;
				else if (!strcmp(mp->m10betapr, "Uniform")     && !strcmp(mp->m10gammapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10UFF;
				else if (!strcmp(mp->m10betapr, "Exponential") && !strcmp(mp->m10gammapr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10EUB;
				else if (!strcmp(mp->m10betapr, "Exponential") && !strcmp(mp->m10gammapr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10EUF;
				else if (!strcmp(mp->m10betapr, "Exponential") && !strcmp(mp->m10gammapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10EEB;
				else if (!strcmp(mp->m10betapr, "Exponential") && !strcmp(mp->m10gammapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10EEF;
				else if (!strcmp(mp->m10betapr, "Exponential") && !strcmp(mp->m10gammapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10EFB;
				else if (!strcmp(mp->m10betapr, "Exponential") && !strcmp(mp->m10gammapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10EFF;
				else if (!strcmp(mp->m10betapr, "Fixed")       && !strcmp(mp->m10gammapr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10FUB;
				else if (!strcmp(mp->m10betapr, "Fixed")       && !strcmp(mp->m10gammapr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10FUF;
				else if (!strcmp(mp->m10betapr, "Fixed")       && !strcmp(mp->m10gammapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10FEB;
				else if (!strcmp(mp->m10betapr, "Fixed")       && !strcmp(mp->m10gammapr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10FEF;
				else if (!strcmp(mp->m10betapr, "Fixed")       && !strcmp(mp->m10gammapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_10FFB;
				else if (!strcmp(mp->m10betapr, "Fixed")       && !strcmp(mp->m10gammapr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Fixed")    )
				    p->paramId = OMEGA_10FFF;

				if (p->paramId != OMEGA_10FFF)
					p->printParam = YES;
				for (i=0; i<p->nValues; i++)
					{
					if (i==0)
                        sprintf (temp, "omega(%d)", i+1);
                    else
                        sprintf (temp, "\tomega(%d)", i+1);
					SafeStrcat (&p->paramHeader, temp);
    				SafeStrcat (&p->paramHeader, partString);
					}
				SafeStrcat (&p->paramHeader, "\tbeta(alpha)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tbeta(beta)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tgamma(alpha)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tgamma(beta)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tpi(1)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tpi(2)");
                SafeStrcat (&p->paramHeader, partString);
				}
			else if (!strcmp(mp->omegaVar, "Ny98"))
				{
				p->nValues = 3;     /* omega values */
				p->nSubValues = 6;  /* omega category frequencies plus cache */
				for (i=0; i<numCurrentDivisions; i++)
					if (isPartTouched[i] == YES)
						modelSettings[i].omega = p;
			
				/* find the parameter x prior type */
				if (     !strcmp(mp->ny98omega1pr, "Beta")  && !strcmp(mp->ny98omega3pr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_BUD;
				else if (!strcmp(mp->ny98omega1pr, "Beta")  && !strcmp(mp->ny98omega3pr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_BUF;
				else if (!strcmp(mp->ny98omega1pr, "Beta")  && !strcmp(mp->ny98omega3pr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_BED;
				else if (!strcmp(mp->ny98omega1pr, "Beta")  && !strcmp(mp->ny98omega3pr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_BEF;
				else if (!strcmp(mp->ny98omega1pr, "Beta")  && !strcmp(mp->ny98omega3pr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_BFD;
				else if (!strcmp(mp->ny98omega1pr, "Beta")  && !strcmp(mp->ny98omega3pr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_BFF;
				else if (!strcmp(mp->ny98omega1pr, "Fixed") && !strcmp(mp->ny98omega3pr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_FUD;
				else if (!strcmp(mp->ny98omega1pr, "Fixed") && !strcmp(mp->ny98omega3pr, "Uniform")     && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_FUF;
				else if (!strcmp(mp->ny98omega1pr, "Fixed") && !strcmp(mp->ny98omega3pr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_FED;
				else if (!strcmp(mp->ny98omega1pr, "Fixed") && !strcmp(mp->ny98omega3pr, "Exponential") && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_FEF;
				else if (!strcmp(mp->ny98omega1pr, "Fixed") && !strcmp(mp->ny98omega3pr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Dirichlet"))
				    p->paramId = OMEGA_FFD;
				else if (!strcmp(mp->ny98omega1pr, "Fixed") && !strcmp(mp->ny98omega3pr, "Fixed")       && !strcmp(mp->codonCatFreqPr, "Fixed"))
				    p->paramId = OMEGA_FFF;
				
				if (p->paramId != OMEGA_FFF)
					p->printParam = YES;
				SafeStrcat (&p->paramHeader, "omega(-)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tomega(N)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tomega(+)");
                SafeStrcat (&p->paramHeader, partString);
					
				SafeStrcat (&p->paramHeader, "\tpi(-)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tpi(N)");
                SafeStrcat (&p->paramHeader, partString);
				SafeStrcat (&p->paramHeader, "\tpi(+)");
                SafeStrcat (&p->paramHeader, partString);
				}
			else
				{
				p->nValues = 1;
				p->nSubValues = 0;
				for (i=0; i<numCurrentDivisions; i++)
					if (isPartTouched[i] == YES)
						modelSettings[i].omega = p;

				/* find the parameter x prior type */
				if (!strcmp(mp->omegaPr,"Dirichlet"))
					p->paramId = OMEGA_DIR;
				else
					p->paramId = OMEGA_FIX;

				if (p->paramId != OMEGA_FIX)
					p->printParam = YES;
				SafeStrcat (&p->paramHeader, "omega");
                SafeStrcat (&p->paramHeader, partString);
				}
			p->paramTypeName = "Positive selection (omega) model";
			SafeStrcat(&p->name, "Omega");
			SafeStrcat(&p->name, partString);			
			}
		else if (j == P_PI)
			{
			/* Set up state frequencies *****************************************************************************/
			p->paramType = P_PI;
            p->min = 0.0;
            p->max = 1.0;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].stateFreq = p;

            if (mp->dataType == STANDARD)
				{
				p->paramTypeName = "Symmetric diricihlet/beta distribution alpha_i parameter";
				SafeStrcat(&p->name, "Alpha_symdir");
                /* boundaries for alpha_i */
                p->min = ETA;
                p->max = POS_INFINITY;
				}
			else
				{
				p->paramTypeName = "Stationary state frequencies";
				SafeStrcat(&p->name, "Pi");
				}
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			/* and the number of values and subvalues needed */
			if (mp->dataType == STANDARD)
				{
                /* find number of model states */
                m->numModelStates = 2;
                for (c=0; c<numChar; c++)
                    {
                    for (i=0; i<p->nRelParts; i++)
                        {
                        if (partitionId[c][partitionNum] == p->relParts[i] + 1 && charInfo[c].numStates > m->numModelStates)
                            m->numModelStates = charInfo[c].numStates;
                        }
                    }
                for (i=0; i<p->nRelParts; i++)
                    modelSettings[p->relParts[i]].numModelStates = m->numModelStates;

				/* symmetric hyperprior with only one variable (0 if equal) */
				p->nValues = 1;     /* set to 0 below for the SYMPI_EQUAL model */
				if (!strcmp(mp->symPiPr,"Uniform"))
					{
					if (m->numModelStates > 2) 
						p->paramId = SYMPI_UNI_MS;
					else
						p->paramId = SYMPI_UNI;
					}
				else if (!strcmp(mp->symPiPr,"Exponential"))
					{
					if (m->numModelStates > 2)
						p->paramId = SYMPI_EXP_MS;
					else
						p->paramId = SYMPI_EXP;
					}
				else if (!strcmp(mp->symPiPr,"Fixed"))
					{
					if (AreDoublesEqual(mp->symBetaFix, -1.0, 0.00001) == YES)
						{
						p->paramId = SYMPI_EQUAL;
						p->nValues = 0;
						}
					else
						{
						if (m->numModelStates > 2) 
							p->paramId = SYMPI_FIX_MS;
						else
							p->paramId = SYMPI_FIX;
						}
					}
				p->nSubValues = 0;	/* store state frequencies in p->stdStateFreqs */
				if (p->paramId == SYMPI_EXP || p->paramId == SYMPI_EXP_MS || p->paramId == SYMPI_UNI || p->paramId == SYMPI_UNI_MS)
					p->printParam = YES;
				SafeStrcat (&p->paramHeader, "alpha_symdir");
                SafeStrcat (&p->paramHeader, partString);
                /* further processing done in ProcessStdChars */
				}
			else
				{
				/* deal with all models except standard */
				/* no hyperprior or fixed to one value, set default to 0  */
				p->nValues = 0;
				/* one subvalue for each state */
			    p->nSubValues = mp->nStates;    /* mp->nStates is set to 20 if DNA || RNA && nucmodel==PROTEIN */
				if (!strcmp(mp->stateFreqPr, "Dirichlet"))
					{
					p->paramId = PI_DIR;
					p->nValues = mp->nStates;
					}
				else if (!strcmp(mp->stateFreqPr, "Fixed") && !strcmp(mp->stateFreqsFixType,"User"))
					p->paramId = PI_USER;
				else if (!strcmp(mp->stateFreqPr, "Fixed") && !strcmp(mp->stateFreqsFixType,"Empirical"))
					p->paramId = PI_EMPIRICAL;
				else if (!strcmp(mp->stateFreqPr, "Fixed") && !strcmp(mp->stateFreqsFixType,"Equal"))
					{
					p->paramId = PI_EQUAL;
					}
					
				if (m->dataType == PROTEIN)
					{
					if (!strcmp(mp->aaModelPr, "Fixed"))
						{
						if (!strcmp(mp->aaModel, "Poisson"))
							p->paramId = PI_EQUAL;
						else if (!strcmp(mp->aaModel, "Equalin") || !strcmp(mp->aaModel, "Gtr"))
							{
							/* p->paramId stays to what it was set to above */
							}
						else
							p->paramId = PI_FIXED;
						}
					else
						p->paramId = PI_FIXED;
					}
					
				if (p->paramId == PI_DIR)
					p->printParam = YES;
				if (m->dataType == DNA || m->dataType == RNA)
					{
					if (!strcmp(mp->nucModel, "4by4"))
						{
                        sprintf(temp, "pi(%c)", StateCode_NUC4(0));
                        SafeStrcat (&p->paramHeader,temp);
                        SafeStrcat (&p->paramHeader,partString);
						for (n1=1; n1<4; n1++)
                            {
                            sprintf(temp, "\tpi(%c)", StateCode_NUC4(n1));
                            SafeStrcat (&p->paramHeader,temp);
                            SafeStrcat (&p->paramHeader,partString);
                            }
						}
					else if (!strcmp(mp->nucModel, "Doublet"))
						{
                        State_DOUBLET(tempCodon,0);
                        sprintf(temp, "pi(%s)", tempCodon);
                        SafeStrcat (&p->paramHeader,temp);
                        SafeStrcat (&p->paramHeader,partString);
						for (n1=1; n1<16; n1++)
                            {
                            State_DOUBLET(tempCodon,n1);
                            sprintf(temp, "\tpi(%s)", tempCodon);
                            SafeStrcat (&p->paramHeader,temp);
                            SafeStrcat (&p->paramHeader,partString);
                            }
						}
					else if (!strcmp(mp->nucModel, "Codon"))
						{
						for (c=0; c<p->nSubValues; c++)
							{
							if (mp->codonNucs[c][0] == 0)
								strcpy (tempCodon, "pi(A");
							else if (mp->codonNucs[c][0] == 1)
								strcpy (tempCodon, "pi(C");
							else if (mp->codonNucs[c][0] == 2)
								strcpy (tempCodon, "pi(G");
							else
								strcpy (tempCodon, "pi(T");
							if (mp->codonNucs[c][1] == 0)
								strcat (tempCodon, "A");
							else if (mp->codonNucs[c][1] == 1)
								strcat (tempCodon, "C");
							else if (mp->codonNucs[c][1] == 2)
								strcat (tempCodon, "G");
							else
								strcat (tempCodon, "T");
							if (mp->codonNucs[c][2] == 0)
								strcat (tempCodon, "A)");
							else if (mp->codonNucs[c][2] == 1)
								strcat (tempCodon, "C)");
							else if (mp->codonNucs[c][2] == 2)
								strcat (tempCodon, "G)");
							else
								strcat (tempCodon, "T)");
							if (c == 0)
                                {
								SafeStrcat (&p->paramHeader, tempCodon);
                                SafeStrcat (&p->paramHeader, partString);
                                }
							else
								{
								SafeStrcat (&p->paramHeader, "\t");
								SafeStrcat (&p->paramHeader, tempCodon);
                                SafeStrcat (&p->paramHeader, partString);
								}
							}
						}
					}
				else if (m->dataType == PROTEIN)
					{
					if (FillRelPartsString (p, &partString) == YES)
						{
						SafeSprintf(&tempStr, &tempStrSize, "pi(Ala)%s\tpi(Arg)%s\tpi(Asn)%s\tpi(Asp)%s\tpi(Cys)%s\tpi(Gln)%s\tpi(Glu)%s\tpi(Gly)%s\tpi(His)%s\tpi(Ile)%s\tpi(Leu)%s\tpi(Lys)%s\tpi(Met)%s\tpi(Phe)%s\tpi(Pro)%s\tpi(Ser)%s\tpi(Thr)%s\tpi(Trp)%s\tpi(Tyr)%s\tpi(Val)%s",
						partString, partString, partString, partString, partString, partString, partString, partString, partString, partString,
						partString, partString, partString, partString, partString, partString, partString, partString, partString, partString);
						SafeStrcat (&p->paramHeader, tempStr);
						}
					else
						SafeStrcat (&p->paramHeader, "pi(Ala)\tpi(Arg)\tpi(Asn)\tpi(Asp)\tpi(Cys)\tpi(Gln)\tpi(Glu)\tpi(Gly)\tpi(His)\tpi(Ile)\tpi(Leu)\tpi(Lys)\tpi(Met)\tpi(Phe)\tpi(Pro)\tpi(Ser)\tpi(Thr)\tpi(Trp)\tpi(Tyr)\tpi(Val)");
					}
				else if (mp->dataType == RESTRICTION)
					{
					if (FillRelPartsString (p, &partString) == YES)
						{
						SafeSprintf(&tempStr, &tempStrSize, "pi(0)%s\tpi(1)%s", partString, partString);
						SafeStrcat (&p->paramHeader, tempStr);
						}
					else
						SafeStrcat (&p->paramHeader, "pi(0)\tpi(1)");
					}
				else
					{
					MrBayesPrint ("%s   Unknown data type in SetModelParams\n", spacer);
					}
				}
			}
		else if (j == P_SHAPE)
			{
			/* Set up shape parameter of gamma **********************************************************************/
			p->paramType = P_SHAPE;
			p->nValues = 1;
			p->nSubValues = mp->numGammaCats;
            p->min = 1E-6;
            p->max = 200;
            for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].shape = p;

            p->paramTypeName = "Shape of scaled gamma distribution of site rates";
			SafeStrcat(&p->name, "Alpha");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			mp = &modelParams[p->relParts[0]];
			if (!strcmp(mp->shapePr,"Uniform"))
				p->paramId = SHAPE_UNI;
			else if (!strcmp(mp->shapePr,"Exponential"))
				p->paramId = SHAPE_EXP;
			else
				p->paramId = SHAPE_FIX;
				
			if (p->paramId != SHAPE_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "alpha");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_PINVAR)
			{
			/* Set up proportion of invariable sites ****************************************************************/
			p->paramType = P_PINVAR;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = 1.0;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].pInvar = p;

            p->paramTypeName = "Proportion of invariable sites";
			SafeStrcat(&p->name, "Pinvar");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (!strcmp(mp->pInvarPr,"Uniform"))
				p->paramId = PINVAR_UNI;
			else
				p->paramId = PINVAR_FIX;
				
			if (p->paramId != PINVAR_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "pinvar");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_CORREL)
			{
			/* Set up correlation parameter of adgamma model ********************************************************/
			p->paramType = P_CORREL;
			p->nValues = 1;
			p->nSubValues = mp->numGammaCats * mp->numGammaCats;
            p->min = -1.0;
            p->max = 1.0;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].correlation = p;

            p->paramTypeName = "Autocorrelation of gamma distribution of site rates";
			SafeStrcat(&p->name, "Rho");
			SafeStrcat(&p->name, partString);
			chainHasAdgamma = YES;

			/* find the parameter x prior type */
			if (!strcmp(mp->adGammaCorPr,"Uniform"))
				p->paramId = CORREL_UNI;
			else
				p->paramId = CORREL_FIX;

			if (p->paramId != CORREL_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "rho");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_SWITCH)
			{
			/* Set up switchRates for covarion model ****************************************************************/
			p->paramType = P_SWITCH;
			p->nValues = 2;
			p->nSubValues = mp->numGammaCats * mp->numGammaCats;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].switchRates = p;

            p->paramTypeName = "Switch rates of covarion model";
			SafeStrcat(&p->name, "s_cov");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (!strcmp(mp->covSwitchPr,"Uniform"))
				p->paramId = SWITCH_UNI;
			else if (!strcmp(mp->covSwitchPr,"Exponential"))
				p->paramId = SWITCH_EXP;
			else
				p->paramId = SWITCH_FIX;
				
			if (p->paramId != SWITCH_FIX)
				p->printParam = YES;

            SafeStrcat (&p->paramHeader, "s(off->on)");
			SafeStrcat (&p->paramHeader, partString);
			SafeStrcat (&p->paramHeader, "\ts(on->off)");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_RATEMULT)
			{
			/* Set up rateMult for partition specific rates ***********************************************************/
			p->paramType = P_RATEMULT;
            if (!strcmp(mp->ratePr,"Fixed"))
                {
                p->nValues = 1;
                p->nSubValues = 0;
                }
            else
                {
			    p->nValues = p->nRelParts = numRelParts; /* keep scaled division rates in value                        */
                p->nSubValues = p->nValues * 2;          /* keep number of uncompressed chars for scaling in subValue  */
													     /* also keep Dirichlet prior parameters here		  		   */
                }
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].rateMult = p;

            p->paramTypeName = "Partition-specific rate multiplier";
			SafeStrcat(&p->name, "Ratemultiplier");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (p->nSubValues == 0)
				p->paramId = RATEMULT_FIX;
			else
				p->paramId = RATEMULT_DIR;

			if (p->paramId != RATEMULT_FIX)
				p->printParam = YES;
			for (i=0; i<numCurrentDivisions; i++)
				{
				if (isPartTouched[i] == YES)
					{
					sprintf (tempMult, "m{%d}", i+1);
					if (i == 0)
						SafeStrcat (&p->paramHeader, tempMult);
					else
						{
						SafeStrcat (&p->paramHeader, "\t");
						SafeStrcat (&p->paramHeader, tempMult);
						}
					}
				}
			}
		else if (j == P_GENETREERATE)
			{
			/* Set up rateMult for partition specific rates ***********************************************************/
			p->paramType = P_GENETREERATE;
		    p->nValues = p->nRelParts = numRelParts; /* keep scaled division rates in value                        */
            p->nSubValues = p->nValues * 2;          /* keep number of uncompressed chars for scaling in subValue  */
												     /* also keep Dirichlet prior parameters here		  		   */
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
                    modelSettings[i].geneTreeRateMult = p;

            p->paramTypeName = "Gene-specific rate multiplier";
			SafeStrcat(&p->name, "Generatemult");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			p->paramId = GENETREERATEMULT_DIR;

			p->printParam = YES;
			for (i=0; i<numCurrentDivisions; i++)
				{
				if (isPartTouched[i] == YES)
					{
					sprintf (tempMult, "g_m{%d}", i+1);
					if (i == 0)
						SafeStrcat (&p->paramHeader, tempMult);
					else
						{
						SafeStrcat (&p->paramHeader, "\t");
						SafeStrcat (&p->paramHeader, tempMult);
						}
					}
				}
			}
		else if (j == P_TOPOLOGY)
			{
			/* Set up topology **************************************************************************************/
			p->paramType = P_TOPOLOGY;
			p->nValues = 0;
			p->nSubValues = 0;
            p->min = NEG_INFINITY;  /* NA */
            p->max = NEG_INFINITY;  /* NA */
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].topology = p;

            p->paramTypeName = "Topology";
			SafeStrcat(&p->name, "Tau");
			SafeStrcat(&p->name, partString);
					
			/* check that the model is not parsimony for all of the relevant partitions */
			areAllPartsParsimony = YES;
			for (i=0; i<p->nRelParts; i++)
				{
				if (modelSettings[p->relParts[i]].parsModelId == NO)
					areAllPartsParsimony = NO;
				}

			if (areAllPartsParsimony == YES)
				p->paramTypeName = "Topology"; /*Why again we set it to "Topology" we did it unconditionally 1 lines before*/
			
			/* check if topology is clock or nonclock */
			nClockBrlens = 0;
			nCalibratedBrlens = 0;
			nRelaxedBrlens = 0;
			if (areAllPartsParsimony == NO)
				{
				for (i=0; i<p->nRelParts; i++)
					{
					if (!strcmp(modelParams[p->relParts[i]].brlensPr, "Clock"))
						{
						nClockBrlens++;
						if (strcmp(modelParams[p->relParts[i]].clockVarPr,"Strict") != 0)  /* not strict */
							nRelaxedBrlens++;
						if (!strcmp(modelParams[p->relParts[i]].nodeAgePr,"Calibrated"))
							nCalibratedBrlens++;
						}
					}
				}
			
			/* now find the parameter x prior type */
			if (areAllPartsParsimony == YES)
				{
				if (!strcmp(mp->topologyPr, "Uniform"))
					p->paramId = TOPOLOGY_PARSIMONY_UNIFORM;
				else if (!strcmp(mp->topologyPr,"Constraints"))
					p->paramId = TOPOLOGY_PARSIMONY_CONSTRAINED;
                else
                    p->paramId = TOPOLOGY_PARSIMONY_FIXED;
				/* For this case, we also need to set the brlens ptr of the relevant partitions
				   so that it points to the topology parameter, since the rest of the
				   program will try to access the tree through this pointer. In FillTreeParams,
				   we will make sure that a pure parsimony topology parameter contains a pointer
				   to the relevant tree (like a brlens parameter would normally) */
				for (i=0; i<p->nRelParts; i++)
					modelSettings[p->relParts[i]].brlens = p;
				}
			else
				{
				/* we assume for now that there is only one branch length; we will correct this
				   later in AllocateTreeParams if there are more than one set of branch lengths,
                   which is only possible for non-clock trees */
				if (!strcmp(mp->topologyPr, "Speciestree"))
					p->paramId = TOPOLOGY_SPECIESTREE;
				else if (!strcmp(mp->topologyPr, "Uniform") && nClockBrlens == 0)
					p->paramId = TOPOLOGY_NCL_UNIFORM_HOMO;
				else if (!strcmp(mp->topologyPr,"Constraints") && nClockBrlens == 0)
					p->paramId = TOPOLOGY_NCL_CONSTRAINED_HOMO;
				else if (!strcmp(mp->topologyPr,"Fixed") && nClockBrlens == 0)
					p->paramId = TOPOLOGY_NCL_FIXED_HOMO;
				/* all below have clock branch lengths */
				else if (!strcmp(mp->topologyPr, "Uniform") && nRelaxedBrlens == 0)
					{
					if (nCalibratedBrlens >= 1)
						p->paramId = TOPOLOGY_CCL_UNIFORM;
					else
						p->paramId = TOPOLOGY_CL_UNIFORM;
					}
				else if (!strcmp(mp->topologyPr,"Constraints") && nRelaxedBrlens == 0)
					{
					if (nCalibratedBrlens >= 1)
						p->paramId = TOPOLOGY_CCL_CONSTRAINED;
					else
						p->paramId = TOPOLOGY_CL_CONSTRAINED;
					}
				else if (!strcmp(mp->topologyPr,"Fixed") && nRelaxedBrlens == 0)
					{
					if (nCalibratedBrlens >= 1)
						p->paramId = TOPOLOGY_CCL_FIXED;
					else
						p->paramId = TOPOLOGY_CL_FIXED;
					}
				/* all below have relaxed clock branch lengths */
				else if (!strcmp(mp->topologyPr, "Uniform"))
					{
					if (nCalibratedBrlens >= 1)
						p->paramId = TOPOLOGY_RCCL_UNIFORM;
					else
						p->paramId = TOPOLOGY_RCL_UNIFORM;
					}
				else if (!strcmp(mp->topologyPr,"Constraints"))
					{
					if (nCalibratedBrlens >= 1)
						p->paramId = TOPOLOGY_RCCL_CONSTRAINED;
					else
						p->paramId = TOPOLOGY_RCL_CONSTRAINED;
					}
				else if (!strcmp(mp->topologyPr,"Fixed"))
					{
					if (nCalibratedBrlens >= 1)
						p->paramId = TOPOLOGY_RCCL_FIXED;
					else
						p->paramId = TOPOLOGY_RCL_FIXED;
					}
				}

			/* should we print the topology? */
			for (i=0; i<p->nRelParts; i++)
				if (strcmp(modelParams[p->relParts[i]].treeFormat,"Topology") != 0)
					break;
			if (i == p->nRelParts)
				p->printParam = YES;
			else
				p->printParam = NO;
			/* but always print parsimony topology */
			if (areAllPartsParsimony == YES)
				p->printParam = YES;
            /* and never print fixed topologies */
            if (p->paramId == TOPOLOGY_RCL_FIXED ||
                p->paramId == TOPOLOGY_RCCL_FIXED ||
                p->paramId == TOPOLOGY_CL_FIXED ||
                p->paramId == TOPOLOGY_CCL_FIXED ||
                p->paramId == TOPOLOGY_NCL_FIXED ||
                p->paramId == TOPOLOGY_PARSIMONY_FIXED)
                p->printParam = NO;
			}
		else if (j == P_BRLENS)
			{
			/* Set up branch lengths ********************************************************************************/
			p->paramType = P_BRLENS;
			p->nValues = 0;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].brlens = p;

            p->paramTypeName = "Branch lengths";
			SafeStrcat(&p->name, "V");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (modelSettings[p->relParts[0]].parsModelId == YES)
				p->paramId = BRLENS_PARSIMONY;
			else
				{
				if (!strcmp(mp->brlensPr, "Clock"))
					{
					if (!strcmp(mp->clockPr,"Uniform"))
					    p->paramId = BRLENS_CLOCK_UNI;
					else if (!strcmp(mp->clockPr,"Coalescence"))
						p->paramId = BRLENS_CLOCK_COAL;
					else if (!strcmp(mp->clockPr,"Birthdeath"))
						p->paramId = BRLENS_CLOCK_BD;
					else if (!strcmp(mp->clockPr,"Speciestreecoalescence"))
						p->paramId = BRLENS_CLOCK_SPCOAL;
                    else if (!strcmp(mp->clockPr,"Fixed"))
                        p->paramId = BRLENS_CLOCK_FIXED;
					}
				else if (!strcmp(mp->brlensPr, "Unconstrained"))
					{
					if (!strcmp(mp->unconstrainedPr,"Uniform"))
						p->paramId = BRLENS_UNI;
					else if (!strcmp(mp->unconstrainedPr,"Exponential"))
						p->paramId = BRLENS_EXP;
					}
				else if (!strcmp(mp->brlensPr,"Fixed"))
					{
					p->paramId = BRLENS_FIXED;
					}
				}

			/* should we print the branch lengths? */
			for (i=0; i<p->nRelParts; i++)
				if (strcmp(modelParams[p->relParts[i]].treeFormat,"Topology") != 0)
					break;
			if (i < p->nRelParts)
				p->printParam = YES;
			else
				p->printParam = NO;
			}
		else if (j == P_SPECIESTREE)
			{
			/* Set up species tree **************************************************************************************/
			p->paramType = P_SPECIESTREE;
			p->nValues = 0;
			p->nSubValues = 0;
            p->min = NEG_INFINITY;  /* NA */
            p->max = NEG_INFINITY;  /* NA */
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].speciesTree = p;

            p->paramTypeName = "Species tree";
			SafeStrcat(&p->name, "Spt");
			SafeStrcat(&p->name, partString);
					
			/* check that the model is not parsimony for all of the relevant partitions */
			areAllPartsParsimony = YES;
			for (i=0; i<p->nRelParts; i++)
				{
				if (modelSettings[p->relParts[i]].parsModelId == NO)
					areAllPartsParsimony = NO;
				}
			
			/* find the parameter x prior type */
            p->paramId = SPECIESTREE_UNIFORM;

			/* should we print the tree? */
            p->printParam = YES;
			}
		else if (j == P_SPECRATE)
			{
			/* Set up speciation rate ******************************************************************************/
			p->paramType = P_SPECRATE;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].speciationRates = p;

            p->paramTypeName = "Speciation rate";
			SafeStrcat(&p->name, "Lambda-mu");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (!strcmp(mp->speciationPr,"Uniform"))
				p->paramId = SPECRATE_UNI;
			else if (!strcmp(mp->speciationPr,"Exponential"))
				p->paramId = SPECRATE_EXP;
			else
				p->paramId = SPECRATE_FIX;

			if (p->paramId != SPECRATE_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "lambda-mu");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_EXTRATE)
			{
			/* Set up extinction rates ******************************************************************************/
			p->paramType = P_EXTRATE;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].extinctionRates = p;

            p->paramTypeName = "Extinction rate";
			SafeStrcat(&p->name, "Mu/lambda");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (!strcmp(mp->extinctionPr,"Beta"))
				p->paramId = EXTRATE_BETA;
			else
				p->paramId = EXTRATE_FIX;

			if (p->paramId != EXTRATE_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "mu/lambda");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_POPSIZE)
			{
			/* Set up population size *****************************************************************************************/
			p->paramType = P_POPSIZE;
			if (!strcmp(mp->topologyPr,"Speciestree") && !strcmp(mp->popVarPr, "Variable"))
                p->nValues = 2 * numSpecies - 1;
            else
                p->nValues = 1;
			p->nSubValues = 0;
            p->min = POS_MIN;
            p->max = POS_INFINITY;
            for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].popSize = p;

            p->paramTypeName = "Coalescent process population size parameter";
			SafeStrcat(&p->name, "Popsize");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (!strcmp(mp->popSizePr,"Uniform"))
				{
				p->paramId      = POPSIZE_UNI;
                p->LnPriorRatio = &LnProbRatioUniform;
                p->priorParams  = mp->popSizeUni;
                p->LnPriorProb  = &LnPriorProbUniform;
				}
			else if (!strcmp(mp->popSizePr,"Normal"))
				{
				p->paramId      = POPSIZE_NORMAL;
                p->LnPriorRatio = &LnProbRatioTruncatedNormal;
                p->priorParams  = mp->popSizeNormal;
                p->LnPriorProb  = &LnPriorProbTruncatedNormal;
				}
			else if (!strcmp(mp->popSizePr,"Lognormal"))
				{
				p->paramId      = POPSIZE_LOGNORMAL;
                p->LnPriorRatio = &LnProbRatioLognormal;
                p->priorParams  = mp->popSizeLognormal;
                p->LnPriorProb  = &LnPriorProbLognormal;
				}
			else if (!strcmp(mp->popSizePr,"Gamma"))
				{
				p->paramId      = POPSIZE_GAMMA;
                p->LnPriorRatio = &LnProbRatioGamma;
                p->priorParams  = mp->popSizeGamma;
                p->LnPriorProb  = &LnPriorProbGamma;
				}
			else
                {
				p->paramId      = POPSIZE_FIX;
                p->LnPriorRatio = NULL;
                p->priorParams  = &mp->popSizeFix;
                p->LnPriorProb  = &LnPriorProbFix;
                }

			if (p->paramId != POPSIZE_FIX && p->nValues == 1)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "Popsize");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_GROWTH)
			{
			/* Set up growth rate ************************************************************************************/
			p->paramType = P_GROWTH;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].growthRate = p;

            p->paramTypeName = "Population growth rate";
			SafeStrcat(&p->name, "R_pop");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (!strcmp(mp->growthPr,"Uniform"))
				p->paramId = GROWTH_UNI;
			else if (!strcmp(mp->growthPr,"Exponential"))
				p->paramId = GROWTH_EXP;
			else if (!strcmp(mp->growthPr,"Normal"))
				p->paramId = GROWTH_NORMAL;
			else
				p->paramId = GROWTH_FIX;

			if (p->paramId != GROWTH_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "growthRate");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_AAMODEL)
			{
			/* Set up aamodel *****************************************************************************************/
			p->paramType = P_AAMODEL;
			p->nValues = 1;
			p->nSubValues = 10;
            p->min = 0;
            p->max = 9;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].aaModel = p;

            p->paramTypeName = "Aminoacid model";
			SafeStrcat(&p->name, "Aamodel");
			SafeStrcat(&p->name, partString);

			/* find the parameter x prior type */
			if (!strcmp(mp->aaModelPr,"Mixed"))
				p->paramId = AAMODEL_MIX;
			else
				p->paramId = AAMODEL_FIX;

			if (p->paramId != AAMODEL_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "aamodel");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_CPPRATE)
			{
			/* Set up cpprate *****************************************************************************************/
			p->paramType = P_CPPRATE;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].cppRate = p;

            p->paramTypeName = "Rate of rate-multiplying compound Poisson process";
			SafeStrcat(&p->name, "Lambda_cpp");
			SafeStrcat(&p->name, partString);
					
			/* find the parameter x prior type */
			if (!strcmp(mp->cppRatePr,"Exponential"))
				p->paramId = CPPRATE_EXP;
			else
				p->paramId = CPPRATE_FIX;
			
			if (p->paramId != CPPRATE_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "cppRate");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_CPPMULTDEV)
			{
			/* Set up sigma of cpp rate multipliers *****************************************************************************************/
			p->paramType = P_CPPMULTDEV;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].cppMultDev = p;

            p->paramTypeName = "Standard deviation (log) of CPP rate multipliers";
			SafeStrcat(&p->name, "Sigma_cpp");
			SafeStrcat(&p->name, partString);
					
			/* find the parameter x prior type */
			if (!strcmp(mp->cppMultDevPr,"Fixed"))
				p->paramId = CPPMULTDEV_FIX;
			
			if (p->paramId != CPPMULTDEV_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "sigma_cpp");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_CPPEVENTS)
			{
			/* Set up cpp events parameter *****************************************************************************************/
			p->paramType = P_CPPEVENTS;
			p->nValues = 0;
			p->nSubValues = 2*numLocalTaxa;		/* keep effective branch lengths here (for all nodes to be on the safe side) */
            p->min = 1E-6;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].cppEvents = p;

            p->paramTypeName = "Events of rate-multiplying compound Poisson process";
			SafeStrcat(&p->name, "Cpp");
			SafeStrcat(&p->name, partString);
			
			/* find the parameter x prior type */
			p->paramId = CPPEVENTS;
			
			/* should we print values to .p file? */
			p->printParam = NO;
			
			SafeStrcat (&p->paramHeader, "cppEvents");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_TK02VAR)
			{
			/* Set up tk02 relaxed clock variance parameter *****************************************************************************************/
			p->paramType = P_TK02VAR;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].tk02var = p;

            p->paramTypeName = "Variance of lognormal distribution of branch rates";
			SafeStrcat(&p->name, "TK02var");
			SafeStrcat(&p->name, partString);
			
			/* find the parameter x prior type */
			if (!strcmp(mp->tk02varPr,"Uniform"))
				p->paramId = TK02VAR_UNI;
			else if (!strcmp(mp->tk02varPr,"Exponential"))
				p->paramId = TK02VAR_EXP;
			else
				p->paramId = TK02VAR_FIX;
			
			if (p->paramId != TK02VAR_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "tk02_var");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_TK02BRANCHRATES)
			{
			/* Set up tk02 relaxed clock rates parameter *****************************************************************************************/
			p->paramType = P_TK02BRANCHRATES;
			p->nValues = 2*numLocalTaxa;     /* use to hold the branch rates; we need one rate for the root */
			p->nSubValues = 2*numLocalTaxa;  /* use to hold the effective branch lengths */
            p->min = RATE_MIN;
            p->max = RATE_MAX;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].tk02BranchRates = p;
			
			p->paramTypeName = "Branch rates of tk02 relaxed clock";
			SafeStrcat(&p->name, "TK02branchrates");
			SafeStrcat(&p->name, partString);
			
			/* find the parameter x prior type */
			p->paramId = TK02BRANCHRATES;
			
			/* should we print values to .p file? */
			p->printParam = NO;

			SafeStrcat (&p->paramHeader, "tk02_branchrates");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_IGRVAR)
			{
			/* Set up igr relaxed clock scaled gamma shape parameter *****************************************************************************************/
			p->paramType = P_IGRVAR;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = 1E-6;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].igrvar = p;

            p->paramTypeName = "Variance increase of igr model branch lenths";
			SafeStrcat(&p->name, "Igrvar");
			SafeStrcat(&p->name, partString);
			
			/* find the parameter x prior type */
			if (!strcmp(mp->igrvarPr,"Uniform"))
				p->paramId = IGRVAR_UNI;
			else if (!strcmp(mp->igrvarPr,"Exponential"))
				p->paramId = IGRVAR_EXP;
			else
				p->paramId = IGRVAR_FIX;
			
			if (p->paramId != IGRVAR_FIX)
				p->printParam = YES;
			SafeStrcat (&p->paramHeader, "igrvar");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_IGRBRANCHLENS)
			{
			/* Set up igr relaxed clock rates parameter *****************************************************************************************/
			p->paramType = P_IGRBRANCHLENS;
			p->nValues = 2*numLocalTaxa;     /* use to hold the branch rates; we need one rate for the root */
			p->nSubValues = 2*numLocalTaxa;  /* use to hold the effective branch lengths */
            p->min = 0.0;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].igrBranchRates = p;
			
			p->paramTypeName = "Branch lengths of relaxed clock";
			SafeStrcat(&p->name, "Igrbranchlens");
			SafeStrcat(&p->name, partString);
			
			/* find the parameter x prior type */
			p->paramId = IGRBRANCHLENS;
			
			/* should we print values to .p file? */
			p->printParam = NO;

			SafeStrcat (&p->paramHeader, "ibr_branchlens");
			SafeStrcat (&p->paramHeader, partString);
			}
		else if (j == P_CLOCKRATE)
			{
			/* Set up clockRate ****************************************************************************************/
			p->paramType = P_CLOCKRATE;
			p->nValues = 1;
			p->nSubValues = 0;
            p->min = POS_MIN;
            p->max = POS_INFINITY;
			for (i=0; i<numCurrentDivisions; i++)
				if (isPartTouched[i] == YES)
					modelSettings[i].clockRate = p;
	
            p->paramTypeName = "Base rate of clock";
			SafeStrcat(&p->name, "Clockrate");
            SafeStrcat(&p->name, partString);			

            /* parameter does affect likelihoods */
            p->affectsLikelihood = YES;
            
            /* find the parameter x prior type */
			if (!strcmp(mp->clockRatePr,"Normal"))
				{
				p->paramId      = CLOCKRATE_NORMAL;
                p->LnPriorRatio = &LnProbRatioNormal;
                p->priorParams  = mp->clockRateNormal;
                p->LnPriorProb  = &LnPriorProbNormal;
				}
			else if (!strcmp(mp->clockRatePr,"Lognormal"))
				{
				p->paramId      = CLOCKRATE_LOGNORMAL;
                p->LnPriorRatio = &LnProbRatioLognormal;
                p->priorParams  = mp->clockRateLognormal;
                p->LnPriorProb  = &LnPriorProbLognormal;
				}
			else if (!strcmp(mp->clockRatePr,"Exponential"))
				{
				p->paramId      = CLOCKRATE_EXP;
                p->LnPriorRatio = &LnProbRatioExponential;
                p->priorParams  = &mp->clockRateExp;
                p->LnPriorProb  = &LnPriorProbExponential;
				}
			else if (!strcmp(mp->clockRatePr,"Gamma"))
				{
				p->paramId      = CLOCKRATE_GAMMA;
                p->LnPriorRatio = &LnProbRatioGamma;
                p->priorParams  = mp->clockRateGamma;
                p->LnPriorProb  = &LnPriorProbGamma;
				}
			else
                {
				p->paramId      = CLOCKRATE_FIX;
                p->LnPriorRatio = NULL;
                p->priorParams  = &mp->clockRateFix;
                p->LnPriorProb  = &LnPriorProbFix;
                }
				
			SafeStrcat (&p->paramHeader, "Clockrate");
			SafeStrcat (&p->paramHeader, partString);
			if (p->paramId != CLOCKRATE_FIX)
                p->printParam = YES;
			}
		}
	free (tempStr);
	free (isPartTouched);
    SafeFree ((void **)&partString);

	return (NO_ERROR);
}





/*----------------------------------------------------------------------------
|
|	SetMoves: This function will set up the applicable moves that could
|		potentially be used in updating the model parameters
|
-----------------------------------------------------------------------------*/
int SetMoves (void)

{
	
	int			i, j, k, moveIndex;
	Param		*param;
	
	/* free up previous moves if any */
	if (memAllocs[ALLOC_MOVES] == YES)
		{
		for (i=0; i<numApplicableMoves; i++)
			FreeMove (moves[i]);
		free (moves);
		moves = NULL;
		memAllocs[ALLOC_MOVES] = NO;
		}

	/* set up applicable moves                                   */
	/* each combination of moveType and param is a separate move */
	
	/* first count applicable moves */
	numApplicableMoves = 0;
	for (k=0; k<numParams; k++)
		{
		param = &params[k];
		for (i=0; i<NUM_MOVE_TYPES; i++)
			{
			if (moveTypes[i].level > userLevel)
				continue;
			if (moveTypes[i].isApplicable(param) == NO)
                continue;
            for (j=0; j<moveTypes[i].nApplicable; j++)
				if (moveTypes[i].applicableTo[j] == param->paramId)
					{
					numApplicableMoves++;
					break;
					}
			}
		}

	/* then allocate space for move pointers */
	moves = (MCMCMove **) SafeMalloc (numApplicableMoves * sizeof (MCMCMove *));
	if (!moves)
		{
		MrBayesPrint ("%s   Problem allocating moves\n", spacer);
		return (ERROR);
		}
	memAllocs[ALLOC_MOVES] = YES;

	/* finally allocate space for and set move defaults */
	moveIndex = 0;
	for (k=0; k<numParams; k++)
		{
		param = &params[k];
		for (i=0; i<NUM_MOVE_TYPES; i++)
			{	
			if (moveTypes[i].level > userLevel)
				continue;
			if (moveTypes[i].isApplicable(param) == NO)
                continue;
			for (j=0; j<moveTypes[i].nApplicable; j++)
				{
				if (moveTypes[i].applicableTo[j] == param->paramId)
					{
					if ((moves[moveIndex] = AllocateMove (&moveTypes[i], param)) == NULL)
						break;
					else
						{
						moves[moveIndex]->parm = param;
						moveIndex++;
						break;
						}
					}
				}
			}
		}

	if (moveIndex < numApplicableMoves)
		{
		for (i=0; i<moveIndex; i++)
			FreeMove (moves[i]);
		free (moves);
		memAllocs[ALLOC_MOVES] = NO;
		MrBayesPrint ("%s   Problem setting moves\n", spacer);
		return (ERROR);
		}

	return (NO_ERROR);

}





/** SetPopSizeParam: Set population size values for a species tree from an input tree */
int SetPopSizeParam (Param *param, int chn, int state, PolyTree *pt)

{
	int			i, j, k, nLongsNeeded;
	MrBFlt		*values;
	Tree		*speciesTree;
	PolyNode	*pp;
	TreeNode	*p=NULL;

    nLongsNeeded = 1 + (pt->nNodes - pt->nIntNodes - 1) / nBitsInALong;

    /* Get pointer to values to be set */
    values = GetParamVals (param, chn, state);

    /* Get species tree */
    speciesTree = GetTree (modelSettings[param->relParts[0]].speciesTree, chn, state);

    /* Set them based on index of matching partitions */
    AllocatePolyTreePartitions(pt);
    AllocateTreePartitions(speciesTree);
    for (i=0; i<pt->nNodes; i++)
        {
        pp = pt->allDownPass[i];
        for (j=0; j<speciesTree->nNodes-1; j++)
            {
            p = speciesTree->allDownPass[j];
            for (k=0; k<nLongsNeeded; k++)
                {
                if (pp->partition[k] != p->partition[k])
                    break;
                }
            if (k == nLongsNeeded)
                break;
            }
        if (j == speciesTree->nNodes - 1)
            {
            MrBayesPrint ("%s   Non-matching partitions when setting population size parameter", spacer);
            FreePolyTreePartitions(pt);
            FreeTreePartitions(speciesTree);
            return (ERROR);
            }
        values[p->index] = pt->popSize[pp->index];
    }

    FreePolyTreePartitions(pt);
    FreeTreePartitions(speciesTree);
    
    return (NO_ERROR);
}





/* SetRelaxedClockParam: set values for a relaxed clock param from an input tree */
int SetRelaxedClockParam (Param *param, int chn, int state, PolyTree *pt)

{
	int			i, j, k, *nEvents=NULL, *nEventsP=NULL, nLongsNeeded, isEventSet;
	MrBFlt		 *effectiveBranchLengthP=NULL, *tk02BranchRate=NULL, *igrBranchLen =NULL,
                **position=NULL, **rateMult=NULL, **positionP=NULL, **rateMultP=NULL,
                baseRate;
	Tree		*t;
	PolyNode	*pp;
	TreeNode	*p=NULL, *q;

    nLongsNeeded = 1 + (numLocalTaxa - 1) / nBitsInALong;

	/* set pointers to the right set of values */
    isEventSet = NO;
	if (param->paramType == P_CPPEVENTS)
		{
		/* find the right event set */
        for (i=0; i<pt->nESets; i++)
            {
		    if (!strcmp(param->name,pt->eSetName[i]))
			    break;
            }
		if (i == pt->nESets)
            {
		    for (i=0; i<pt->nBSets; i++)
    		    if (!strcmp(param->name,pt->bSetName[i]))
	    		    break;

            if (i == pt->nBSets)
			    return (NO_ERROR);
            else
                isEventSet = NO;
            }
        else
            isEventSet = YES;

        if (isEventSet == YES)
            {
            nEventsP  = pt->nEvents[i];
		    positionP = pt->position[i];
		    rateMultP = pt->rateMult[i];
            }
        else
            effectiveBranchLengthP = pt->effectiveBrLen[i];

        nEvents  = param->nEvents[2*chn+state];
        position = param->position[2*chn+state];
        rateMult = param->rateMult[2*chn+state];
		}
	else if (param->paramType == P_TK02BRANCHRATES)
		{
		/* find the right effective branch length set */
		for (i=0; i<pt->nBSets; i++)
    		if (!strcmp(param->name,pt->bSetName[i]))
	    		break;
		if (i == pt->nBSets)
			return (NO_ERROR);

        effectiveBranchLengthP = pt->effectiveBrLen[i];
		tk02BranchRate         = GetParamVals (param, chn, state);
		}
	else if (param->paramType == P_IGRBRANCHLENS)
		{
		/* find the right effective branch length set */
		for (i=0; i<pt->nBSets; i++)
		if (!strcmp(param->name,pt->bSetName[i]))
			break;
		if (i == pt->nBSets)
			return (NO_ERROR);

		effectiveBranchLengthP = pt->effectiveBrLen[i];
		igrBranchLen           = GetParamSubVals (param, chn, state);
		}

	t = GetTree (param, chn, state);
	AllocatePolyTreePartitions (pt);
	AllocateTreePartitions (t);
	
	for (i=pt->nNodes-1; i>=0; i--)
		{
		pp = pt->allDownPass[i];
		for (j=0; j<t->nNodes; j++)
			{
			p = t->allDownPass[j];
			for (k=0; k<nLongsNeeded; k++)
				if (p->partition[k] != pp->partition[k])
					break;
			if (k == nLongsNeeded)
				break;  /* match */
			}
		if (param->paramType == P_CPPEVENTS)
			{
			if (isEventSet == NO)
                {
                if (nEvents[p->index] != 1)
				    {
				    position[p->index] = (MrBFlt *) SafeRealloc ((void *) position[p->index], 1*sizeof(MrBFlt));
				    rateMult[p->index] = (MrBFlt *) SafeRealloc ((void *) rateMult[p->index], 1*sizeof(MrBFlt));
				    nEvents [p->index] = 1;
				    }
			    position[p->index][0] = 0.5;
                if (p->anc->anc == NULL)
			        rateMult[p->index][0] = 1.0;
                else
                    {
                    baseRate = 1.0;
	                q = p->anc;
	                while (q->anc != NULL)
		                {
		                baseRate *= rateMult[q->index][0];
		                q = q->anc;
		                }
			        rateMult[p->index][0] = 2.0 * effectiveBranchLengthP[pp->index] / (p->length * baseRate) - 1.0;
                    }
                }
            else
                {
                if (nEvents[p->index] != nEventsP[pp->index])
				    {
				    if (nEventsP[pp->index] == 0)
					    {
					    free (position[p->index]);
					    free (rateMult[p->index]);
					    }
				    else
					    {
					    position[p->index] = (MrBFlt *) SafeRealloc ((void *) position[p->index], nEventsP[pp->index]*sizeof(MrBFlt));
					    rateMult[p->index] = (MrBFlt *) SafeRealloc ((void *) rateMult[p->index], nEventsP[pp->index]*sizeof(MrBFlt));
					    }
				    nEvents[p->index] = nEventsP[pp->index];
				    }
			    for (j=0; j<nEventsP[pp->index]; j++)
				    {
				    position[p->index][j] = positionP[pp->index][j];
				    rateMult[p->index][j] = rateMultP[pp->index][j];
				    }
                }
			}
		else if (param->paramType == P_TK02BRANCHRATES)
			{
			if (p->anc->anc == NULL)
                tk02BranchRate[p->index] = 1.0;
            else
                tk02BranchRate[p->index] = 2.0 * effectiveBranchLengthP[pp->index] / p->length - tk02BranchRate[p->anc->index];
			}
		else if (param->paramType == P_IGRBRANCHLENS)
			{
			igrBranchLen[p->index] = effectiveBranchLengthP[pp->index];
			}
		}
    
    if (param->paramType == P_CPPEVENTS)
        {
        if (UpdateCppEvolLengths (param, t->root->left, chn) == ERROR)
            return (ERROR);
        }
    else if (param->paramType == P_TK02BRANCHRATES)
        {
        if (UpdateTK02EvolLengths (param, t, chn) == ERROR)
            return (ERROR);
        }
    else if (param->paramType == P_IGRBRANCHLENS)
        {
        if (UpdateIgrBranchRates (param, t, chn) == ERROR)
            return (ERROR);
        }

	FreePolyTreePartitions (pt);
	FreeTreePartitions (t);

	return (NO_ERROR);
}





/*------------------------------------------------------------------------
|
|	SetUpAnalysis: Set parameters and moves
|
------------------------------------------------------------------------*/
int SetUpAnalysis (SafeLong *seed)

{

    setUpAnalysisSuccess=NO;

    /* calculate number of characters and taxa */
	numLocalChar = NumNonExcludedChar ();

    /* we are checking later to make sure no partition is without characters */
	SetLocalTaxa ();
	if (numLocalTaxa <= 2)
		{
		MrBayesPrint ("%s   There must be at least two included taxa, now there is %s\n", spacer,
			numLocalTaxa == 0 ? "none" : "only one");
		return (ERROR);
		}

	/* calculate number of global chains */
	numGlobalChains = chainParams.numRuns * chainParams.numChains;

	/* Set up link table */
	if (SetUpLinkTable () == ERROR)
		return (ERROR);
	
	/* Check that the settings for doublet or codon models are correct. */
	if (CheckExpandedModels() == ERROR)
		return (ERROR);

	/* Set up model info */
	if (SetModelInfo() == ERROR) 
		return (ERROR);
	
	/* Calculate number of (uncompressed) characters for each division */
	if (GetNumDivisionChars() == ERROR)
		return (ERROR);

    /* Compress data and calculate some things needed for setting up params. */
	if (CompressData() == ERROR)
		return (ERROR);

	/* Add dummy characters, if needed. */
	if (AddDummyChars() == ERROR)
		return (ERROR);

	/* Set up parameters for the chain. */
	if (SetModelParams () == ERROR)
		return (ERROR);
	
	/* Allocate normal params */
	if (AllocateNormalParams () == ERROR)
		return (ERROR);
	
	/* Allocate tree params */
	if (AllocateTreeParams () == ERROR)
		return (ERROR);
	
    /* Set default number of trees for sumt to appropriate number */
    sumtParams.numTrees = numTrees;

	/* Fill in normal parameters */
	if (FillNormalParams (seed, 0, numGlobalChains) == ERROR) 
		return (ERROR);

	/* Process standard characters (calculates bsIndex, tiIndex, and more). */
	if (ProcessStdChars(seed) == ERROR)
		return (ERROR);

	
	/* Fill in trees */
	if (FillTreeParams (seed, 0, numGlobalChains) == ERROR)
		return (ERROR);
	
	/* Set the applicable moves that could be used by the chain. */
	if (SetMoves () == ERROR)
		return (ERROR);

    setUpAnalysisSuccess=YES;
	
    return (NO_ERROR);
	
}





int SetUpLinkTable (void)

{

	int 		i, j, k, m, paramCount, isApplicable1, isApplicable2,
				isFirst, isSame;
	int			*check, *modelId;

	check = (int *) SafeMalloc ((size_t) (2*numCurrentDivisions * sizeof (int)));
	if (!check)
		{
		MrBayesPrint ("%s   Problem allocating check (%d)\n", spacer,
			2*numCurrentDivisions * sizeof(int));
		return (ERROR);
		}
	modelId = check + numCurrentDivisions;

	for (j=0; j<NUM_LINKED; j++)
		for (i=0; i<numCurrentDivisions; i++)
			activeParams[j][i] = 0;
	
	if (numCurrentDivisions > 1)
		{
		paramCount = 0;
		for (j=0; j<NUM_LINKED; j++) /* loop over parameters */
			{
			isFirst = YES;
			for (i=0; i<numCurrentDivisions; i++)
				modelId[i] = 0;		
			for (i=0; i<numCurrentDivisions-1; i++) /* loop over partitions */
				{
				for (k=i+1; k<numCurrentDivisions; k++)
					{
					if (IsModelSame (j, i, k, &isApplicable1, &isApplicable2) == NO || linkTable[j][i] != linkTable[j][k])
						{
						/* we cannot link the parameters */
						if (isApplicable1 == NO)
							modelId[i] = -1;
						if (isApplicable2 == NO)
							modelId[k] = -1;
						if (isApplicable1 == YES)
							{
							if (isFirst == YES && modelId[i] == 0)
								{
								modelId[i] = ++paramCount;
								isFirst = NO;
								}
							else
								{
								if (modelId[i] == 0)
									modelId[i] = ++paramCount;
								}
							}
						if (modelId[k] == 0 && isApplicable2 == YES)
							modelId[k] = ++paramCount;
						}
					else
						{
						/* we can link the parameters */
						if (isFirst == YES)
							{
							if (modelId[i] == 0)
								modelId[i] = ++paramCount;
							isFirst = NO;
							}
						else
							{
							if (modelId[i] == 0)
								modelId[i] = ++paramCount;
							}
						modelId[k] = modelId[i];
						}
					}
				}
			for (i=0; i<numCurrentDivisions; i++)
				activeParams[j][i] = modelId[i];
			}
		}
	else
		{
		/* if we have only one partition, then we do things a bit differently */
		paramCount = 0;
		for (j=0; j<NUM_LINKED; j++) /* loop over parameters */
			{
			IsModelSame (j, 0, 0, &isApplicable1, &isApplicable2);
			if (isApplicable1 == YES)
				activeParams[j][0] = ++paramCount;
			else
				activeParams[j][0] = -1;
			}
		}
		
	/* Check that the same report format is specified for all partitions with the same rate multiplier */
	for (i=0; i<numCurrentDivisions; i++)
		check[i] = NO;
	for (i=0; i<numCurrentDivisions; i++)
		{
		m = activeParams[P_RATEMULT][i];
		if (m == -1 || check[i] == YES)
			continue;
		isSame = YES;
		for (j=i+1; j<numCurrentDivisions; j++)
			{
			if (activeParams[P_RATEMULT][j] == m)
				{
				check[i] = YES;
				if (strcmp(modelParams[i].ratemultFormat,modelParams[j].ratemultFormat)!= 0)
					{
					isSame = NO;
					strcpy (modelParams[j].ratemultFormat, modelParams[i].ratemultFormat);
					}
				}
			}
		if (isSame == NO)
			{
			MrBayesPrint ("%s   WARNING: Report format for ratemult (parameter %d) varies across partitions.\n", spacer);
			MrBayesPrint ("%s      MrBayes will use the format for the first partition, which is %s.\n", spacer, modelParams[i].ratemultFormat);
			}
		}
	   
	/* probably a good idea to clean up link table here */
	paramCount = 0;
	for (j=0; j<NUM_LINKED; j++)
		{
		for (i=0; i<numCurrentDivisions; i++)
			check[i] = NO;
		for (i=0; i<numCurrentDivisions; i++)
			{
			if (check[i] == NO && activeParams[j][i] > 0)
				{
				m = activeParams[j][i];
				paramCount++;
				for (k=i; k<numCurrentDivisions; k++)
					{
					if (check[k] == NO && activeParams[j][k] == m)
						{
						activeParams[j][k] = paramCount;
						check[k] = YES;
						}
					}
				}
			}
		}

	free (check);

	return (NO_ERROR);

}





/*------------------------------------------------------------------------
|
|	SetUpMoveTypes: Set up structs holding info on each move type
|
------------------------------------------------------------------------*/
void SetUpMoveTypes (void)

{
	
	/* Register the move type here when new move functions are added 
	   Remember to check that the number of move types does not exceed NUM_MOVE_TYPES
	   defined in mb.h.         */
	int			i;
	MoveType	*mt;

	/* reset move types */
	for (i=0; i<NUM_MOVE_TYPES; i++)
		{
		mt = &moveTypes[i];
		mt->level = DEVELOPER;
		mt->numTuningParams = 0;
		mt->minimum[0] = mt->minimum[1] = -1000000000.0;
		mt->maximum[0] = mt->maximum[1] =  1000000000.0;
		mt->tuningParam[0] = mt->tuningParam[1] = 0.0;
		mt->nApplicable = 0;
		mt->name = mt->tuningName[0] = mt->tuningName[1] = "";
		mt->paramName = "";
        mt->subParams = NO;
		mt->relProposalProb = 0.0;
		mt->parsimonyBased = NO;
        mt->isApplicable = &IsApplicable;
        mt->Autotune = NULL;
        mt->targetRate = -1.0;
		}

	/* Moves are in alphabetic order after parameter name, which matches the name of a move function if
       there is a separate move function for the parameter. See mcmc.h for declaration of move functions.
       Since 2010-10-04, some parameters use generalized move functions and do not have their own. */
    
	i = 0;

	/* Move_Aamodel */
	mt = &moveTypes[i++];
	mt->name = "Uniform random pick";
	mt->shortName = "Uniform";
	mt->applicableTo[0] = AAMODEL_MIX;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Aamodel;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_Adgamma */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = CORREL_UNI;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Adgamma;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.5;  /* window size */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 1.999;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_Beta */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = SYMPI_UNI;
	mt->applicableTo[1] = SYMPI_EXP;
	mt->applicableTo[2] = SYMPI_UNI_MS;
	mt->applicableTo[3] = SYMPI_EXP_MS;
	mt->nApplicable = 4;
	mt->moveFxn = &Move_Beta;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.5);  /* so-called lambda */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_BrLen */
	mt = &moveTypes[i++];
	mt->name = "Random brlen hit with multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";	
	mt->applicableTo[0] = BRLENS_UNI;
	mt->applicableTo[1] = BRLENS_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_BrLen;
	mt->relProposalProb = 20.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (2.0);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_ClockRateM */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = CLOCKRATE_NORMAL;
	mt->applicableTo[1] = CLOCKRATE_LOGNORMAL;
	mt->applicableTo[2] = CLOCKRATE_GAMMA;
	mt->applicableTo[3] = CLOCKRATE_EXP;
	mt->nApplicable = 4;
	mt->moveFxn = &Move_ClockRateM;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.5);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_Extinction */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = EXTRATE_BETA;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Extinction;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 1.0;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_ExtSPR */
	mt = &moveTypes[i++];
	mt->name = "Extending SPR";
	mt->shortName = "ExtSPR";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ExtSPR;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.5; /* extension probability */
	mt->tuningParam[1] = 2.0 * log (1.05); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_ExtSPRClock */
	mt = &moveTypes[i++];
	mt->name = "Extending SPR for clock trees";
	mt->shortName = "ExtSprClock";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->applicableTo[0] = TOPOLOGY_CL_UNIFORM;
	mt->applicableTo[1] = TOPOLOGY_CCL_UNIFORM;
	mt->applicableTo[2] = TOPOLOGY_CL_CONSTRAINED;
	mt->applicableTo[3] = TOPOLOGY_CCL_CONSTRAINED;
	mt->applicableTo[4] = TOPOLOGY_RCL_UNIFORM;
	mt->applicableTo[5] = TOPOLOGY_RCL_CONSTRAINED;
	mt->applicableTo[6] = TOPOLOGY_RCCL_UNIFORM;
	mt->applicableTo[7] = TOPOLOGY_RCCL_CONSTRAINED;
	mt->nApplicable = 8;
	mt->moveFxn = &Move_ExtSPRClock;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.5; /* extension probability */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_ExtSS */
	mt = &moveTypes[i++];
	mt->name = "Extending subtree swapper";
	mt->shortName = "ExtSS";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ExtSS;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.5; /* extension probability */
	mt->tuningParam[1] = 2.0 * log (1.05); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_ExtSSClock */
	mt = &moveTypes[i++];
	mt->name = "Extending subtree swapper";
	mt->shortName = "ExtSsClock";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->applicableTo[0] = TOPOLOGY_CL_UNIFORM;
	mt->applicableTo[1] = TOPOLOGY_CCL_UNIFORM;
	mt->applicableTo[2] = TOPOLOGY_CL_CONSTRAINED;
	mt->applicableTo[3] = TOPOLOGY_CCL_CONSTRAINED;
	mt->applicableTo[4] = TOPOLOGY_RCL_UNIFORM;
	mt->applicableTo[5] = TOPOLOGY_RCL_CONSTRAINED;
	mt->applicableTo[6] = TOPOLOGY_RCCL_UNIFORM;
	mt->applicableTo[7] = TOPOLOGY_RCCL_CONSTRAINED;
	mt->nApplicable = 8;
	mt->moveFxn = &Move_ExtSSClock;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.5; /* extension probability */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

    /* Move_ExtTBR */
	mt = &moveTypes[i++];
	mt->name = "Extending TBR";
	mt->shortName = "ExtTBR";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ExtTBR;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.5;  /* extension probability */
	mt->tuningParam[1] = 2.0 * log (1.05);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99;
	mt->minimum[1] = 0.00001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_ExtTBR1 */
	mt = &moveTypes[i++];
	mt->name = "Extending TBR variant 1";
	mt->shortName = "eTBR1";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ExtTBR1;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* extension probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_ExtTBR2 */
	mt = &moveTypes[i++];
	mt->name = "Extending TBR variant 2";
	mt->shortName = "eTBR2";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ExtTBR2;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* extension probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_ExtTBR3 */
	mt = &moveTypes[i++];
	mt->name = "Extending TBR variant 3";
	mt->shortName = "eTBR3";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ExtTBR3;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* extension probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_ExtTBR4 */
	mt = &moveTypes[i++];
	mt->name = "Extending TBR variant 4";
	mt->shortName = "eTBR4";
    mt->subParams = YES;
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ExtTBR4;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* extension probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_GammaShape_M */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = SHAPE_UNI;
	mt->applicableTo[1] = SHAPE_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_GammaShape_M;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.5);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_GeneRate_Dir */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = GENETREERATEMULT_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_GeneRate_Dir;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 1000.0; /* alphaPi */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneDirichlet;
    mt->targetRate = 0.25;

	/* Move_GeneTree1 */
	mt = &moveTypes[i++];
	mt->name = "Extending SPR move for gene trees in species trees";
	mt->shortName = "ExtSPRClockGS";
	mt->tuningName[0] = "Extension probability";
	mt->shortTuningName[0] = "prob";
	mt->applicableTo[0] = TOPOLOGY_SPECIESTREE;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_GeneTree1;
	mt->relProposalProb = 10.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.5;       /* Tuning parameter value */
	mt->minimum[0] = 0.00001;       /* Minimum value of tuning param */
	mt->maximum[0] = 10000000.0;    /* Maximum value of tuning param */
	mt->parsimonyBased = NO;        /* It does not use parsimony scores */
	mt->level = STANDARD_USER;

	/* Move_GeneTree2 */
	mt = &moveTypes[i++];
	mt->name = "NNI move for gene trees in species trees";
	mt->shortName = "NNIClockGS";
	mt->applicableTo[0] = TOPOLOGY_SPECIESTREE;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_GeneTree2;
	mt->relProposalProb = 10.0;
	mt->numTuningParams = 0;        /* No tuning parameters */
	mt->parsimonyBased = NO;        /* It does not use parsimony scores */
	mt->level = STANDARD_USER;

	/* Move_GeneTree3 */
	mt = &moveTypes[i++];
	mt->name = "Parsimony-biased SPR for gene trees in species trees";
	mt->shortName = "ParsSPRClockGS";
    mt->subParams = YES;
	mt->tuningName[0] = "parsimony warp factor";
	mt->shortTuningName[0] = "warp";
	mt->tuningName[1] = "reweighting probability";
	mt->shortTuningName[1] = "r";
	mt->applicableTo[0] = TOPOLOGY_SPECIESTREE;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_GeneTree3;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.1; /* warp */
	mt->tuningParam[1] = 0.05; /* upweight and downweight probability */
	mt->minimum[0] = 0.01;
	mt->maximum[0] = 10.0;
	mt->minimum[1] = 0.0;
	mt->maximum[1] = 0.30;
	mt->parsimonyBased = YES;
	mt->level = STANDARD_USER;


	/* Move_Growth */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = GROWTH_UNI;
	mt->applicableTo[1] = GROWTH_EXP;
	mt->applicableTo[2] = GROWTH_NORMAL;
	mt->nApplicable = 3;
	mt->moveFxn = &Move_Growth;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 5.0;  /* window size */
	mt->minimum[0] = -10000.0;
	mt->maximum[0] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_Local */
	mt = &moveTypes[i++];
	mt->name = "BAMBE's LOCAL";
	mt->shortName = "Local";
    mt->subParams = YES;
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_Local;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_LocalClock */
	mt = &moveTypes[i++];
	mt->name = "Modified LOCAL for clock trees";
	mt->shortName = "LocalClock";
    mt->subParams = YES;
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_CL_UNIFORM;
	mt->applicableTo[1] = TOPOLOGY_CCL_UNIFORM;
	mt->applicableTo[2] = TOPOLOGY_CL_CONSTRAINED;
	mt->applicableTo[3] = TOPOLOGY_CCL_CONSTRAINED;
	mt->nApplicable = 4;
	mt->moveFxn = &Move_LocalClock;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (2.0);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_NNI */
	mt = &moveTypes[i++];
	mt->name = "NNI move for parsimony trees";
	mt->shortName = "ParsNNI";
	mt->applicableTo[0] = TOPOLOGY_PARSIMONY_UNIFORM;
	mt->applicableTo[1] = TOPOLOGY_PARSIMONY_CONSTRAINED;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_NNI;
	mt->relProposalProb = 10.0;
	mt->numTuningParams = 0;
	mt->parsimonyBased = NO;	/* no extra parsimony scores are needed */
	mt->level = STANDARD_USER;

	/* Move_NNIClock */
	mt = &moveTypes[i++];
	mt->name = "NNI move for clock trees";
	mt->shortName = "NNIClock";
    mt->subParams = YES;
	mt->applicableTo[0] = TOPOLOGY_CL_UNIFORM;
	mt->applicableTo[1] = TOPOLOGY_CCL_UNIFORM;
	mt->applicableTo[2] = TOPOLOGY_CL_CONSTRAINED;
	mt->applicableTo[3] = TOPOLOGY_CCL_CONSTRAINED;
	mt->applicableTo[4] = TOPOLOGY_RCL_UNIFORM;
	mt->applicableTo[5] = TOPOLOGY_RCL_CONSTRAINED;
	mt->applicableTo[6] = TOPOLOGY_RCCL_UNIFORM;
	mt->applicableTo[7] = TOPOLOGY_RCCL_CONSTRAINED;
	mt->nApplicable = 8;
	mt->moveFxn = &Move_NNIClock;
	mt->relProposalProb = 10.0;
	mt->numTuningParams = 0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_NNI */
	mt = &moveTypes[i++];
	mt->name = "NNI move";
	mt->shortName = "NNI";
    mt->subParams = YES;
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_NNI;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 0;
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_NNI_Hetero */
	mt = &moveTypes[i++];
	mt->name = "NNI move for trees with independent brlens";
	mt->shortName = "MultNNI";
    mt->subParams = YES;
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HETERO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HETERO;
	mt->nApplicable = 2; /* 3; */
	mt->moveFxn = &Move_NNI_Hetero;
	mt->relProposalProb = 15.0;
	mt->numTuningParams = 0;
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_NodeSlider */
	mt = &moveTypes[i++];
	mt->name = "Node slider (uniform on possible positions)";
	mt->shortName = "Nodeslider";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = BRLENS_UNI;
	mt->applicableTo[1] = BRLENS_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_NodeSlider;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_NodeSliderClock */
	mt = &moveTypes[i++];
	mt->name = "Node depth window slider (clock-constrained)";
	mt->shortName = "NodesliderClock";
	mt->tuningName[0] = "Window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = BRLENS_CLOCK_UNI;
	mt->applicableTo[1] = BRLENS_CLOCK_COAL;
	mt->applicableTo[2] = BRLENS_CLOCK_BD;
	mt->nApplicable = 3;
	mt->moveFxn = &Move_NodeSliderClock;
	mt->relProposalProb = 20.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.05; /* window size */
	mt->minimum[0] = 0.000001;
	mt->maximum[0] = 1.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_NodeSliderGeneTree */
	mt = &moveTypes[i++];
	mt->name = "Node depth slider for gene trees";
	mt->shortName = "Nodeslider";
	mt->tuningName[0] = "Window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = BRLENS_CLOCK_SPCOAL;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_NodeSliderGeneTree;
	mt->relProposalProb = 20.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.05; /* window size */
	mt->minimum[0] = 0.00000001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_Omega */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = OMEGA_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Omega;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 1.0; /* sliding window size */
	mt->minimum[0] = 0.0;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_Omega_M */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = OMEGA_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Omega_M;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.5);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_OmegaBeta_M */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->paramName = "Omega_beta_M10";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = OMEGA_10UUB;
	mt->applicableTo[1] = OMEGA_10UUF;
	mt->applicableTo[2] = OMEGA_10UEB;
	mt->applicableTo[3] = OMEGA_10UEF;
	mt->applicableTo[4] = OMEGA_10UFB;
	mt->applicableTo[5] = OMEGA_10UFF;
	mt->applicableTo[6] = OMEGA_10EUB;
	mt->applicableTo[7] = OMEGA_10EUF;
	mt->applicableTo[8] = OMEGA_10EEB;
	mt->applicableTo[9] = OMEGA_10EEF;
	mt->applicableTo[10] = OMEGA_10EFB;
	mt->applicableTo[11] = OMEGA_10EFF;
	mt->nApplicable = 12;
	mt->moveFxn = &Move_OmegaBeta_M;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00000001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_OmegaCat */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->paramName = "Omega_pi";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = OMEGA_BUD;
	mt->applicableTo[1] = OMEGA_BED;
	mt->applicableTo[2] = OMEGA_BFD;
	mt->applicableTo[3] = OMEGA_FUD;
	mt->applicableTo[4] = OMEGA_FED;
	mt->applicableTo[5] = OMEGA_FFD;
	mt->applicableTo[6] = OMEGA_ED;
	mt->applicableTo[7] = OMEGA_FD;
	mt->applicableTo[8] = OMEGA_10UUB;
	mt->applicableTo[9] = OMEGA_10UEB;
	mt->applicableTo[10] = OMEGA_10UFB;
	mt->applicableTo[11] = OMEGA_10EUB;
	mt->applicableTo[12] = OMEGA_10EEB;
	mt->applicableTo[13] = OMEGA_10EFB;
	mt->applicableTo[14] = OMEGA_10FUB;
	mt->applicableTo[15] = OMEGA_10FEB;
	mt->applicableTo[16] = OMEGA_10FFB;
	mt->nApplicable = 17;
	mt->moveFxn = &Move_OmegaCat;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 300.0;  /* alpha-pi */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneDirichlet;
    mt->targetRate = 0.25;

	/* Move_OmegaGamma_M */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->paramName = "Omega_shape_M10";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = OMEGA_10UUB;
	mt->applicableTo[1] = OMEGA_10UUF;
	mt->applicableTo[2] = OMEGA_10UEB;
	mt->applicableTo[3] = OMEGA_10UEF;
	mt->applicableTo[4] = OMEGA_10EUB;
	mt->applicableTo[5] = OMEGA_10EUF;
	mt->applicableTo[6] = OMEGA_10EEB;
	mt->applicableTo[7] = OMEGA_10EEF;
	mt->applicableTo[8] = OMEGA_10FUB;
	mt->applicableTo[9] = OMEGA_10FUF;
	mt->applicableTo[10] = OMEGA_10FEB;
	mt->applicableTo[11] = OMEGA_10FEF;
	mt->nApplicable = 12;
	mt->moveFxn = &Move_OmegaGamma_M;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00000001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_OmegaM3 */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = OMEGA_ED;
	mt->applicableTo[1] = OMEGA_EF;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_OmegaM3;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.1;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 1.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_OmegaPur : Let it be here so that omega moves are listed in logical order!*/
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->paramName = "Omega_pur";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = OMEGA_BUD;
	mt->applicableTo[1] = OMEGA_BUF;
	mt->applicableTo[2] = OMEGA_BED;
	mt->applicableTo[3] = OMEGA_BEF;
	mt->applicableTo[4] = OMEGA_BFD;
	mt->applicableTo[5] = OMEGA_BFF;
	mt->nApplicable = 6;
	mt->moveFxn = &Move_OmegaPur;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.1;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 1.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_OmegaPos */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->paramName = "Omega_pos";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = OMEGA_BUD;
	mt->applicableTo[1] = OMEGA_BUF;
	mt->applicableTo[2] = OMEGA_BED;
	mt->applicableTo[3] = OMEGA_BEF;
	mt->applicableTo[4] = OMEGA_FUD;
	mt->applicableTo[5] = OMEGA_FUF;
	mt->applicableTo[6] = OMEGA_FED;
	mt->applicableTo[7] = OMEGA_FEF;
	mt->nApplicable = 8;
	mt->moveFxn = &Move_OmegaPos;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 1.0;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_ParsEraser1 */
	mt = &moveTypes[i++];
	mt->name = "Parsimony-biased eraser version 1";
	mt->shortName = "pEraser1";
    mt->subParams = YES;
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->tuningName[1] = "parsimony warp factor";
	mt->shortTuningName[1] = "warp";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_ParsEraser1;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.5; /* alphaPi */
	mt->tuningParam[1] = 0.1; /* warp */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000.0;
	mt->minimum[1] = 0.00001;
	mt->maximum[1] = 0.99999;
	mt->parsimonyBased = YES;
	mt->level = DEVELOPER;

	/* Move_ParsSPR */
	mt = &moveTypes[i++];
	mt->name = "Parsimony-biased SPR";
	mt->shortName = "ParsSPR";
    mt->subParams = YES;
	mt->tuningName[0] = "parsimony warp factor";
	mt->shortTuningName[0] = "warp";
	mt->tuningName[1] = "multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->tuningName[2] = "reweighting probability";
	mt->shortTuningName[2] = "r";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_ParsSPR;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 3;
	mt->tuningParam[0] = 0.1; /* warp */
	mt->tuningParam[1] = 2.0 * log (1.05); /* multiplier tuning parameter lambda */
	mt->tuningParam[2] = 0.05; /* upweight and downweight probability */
	mt->minimum[0] = 0.0;
	mt->maximum[0] = 1.0;
	mt->minimum[1] = 2.0 * log (0.001);
	mt->maximum[1] = 2.0 * log (1000.0);
	mt->minimum[2] = 0.0;
	mt->maximum[2] = 0.30;
	mt->parsimonyBased = YES;
	mt->level = STANDARD_USER;

	/* Move_ParsSPRClock */
	mt = &moveTypes[i++];
	mt->name = "Parsimony-biased SPR for clock trees";
	mt->shortName = "ParsSPRClock";
    mt->subParams = YES;
	mt->tuningName[0] = "parsimony warp factor";
	mt->shortTuningName[0] = "warp";
	mt->tuningName[1] = "reweighting probability";
	mt->shortTuningName[1] = "r";
	mt->applicableTo[0] = TOPOLOGY_CL_UNIFORM;
	mt->applicableTo[1] = TOPOLOGY_CCL_UNIFORM;
	mt->applicableTo[2] = TOPOLOGY_CL_CONSTRAINED;
	mt->applicableTo[3] = TOPOLOGY_CCL_CONSTRAINED;
	mt->applicableTo[4] = TOPOLOGY_RCL_UNIFORM;
	mt->applicableTo[5] = TOPOLOGY_RCL_CONSTRAINED;
	mt->applicableTo[6] = TOPOLOGY_RCCL_UNIFORM;
	mt->applicableTo[7] = TOPOLOGY_RCCL_CONSTRAINED;
	mt->nApplicable = 8;
	mt->moveFxn = &Move_ParsSPRClock;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.1; /* warp */
	mt->tuningParam[1] = 0.05; /* upweight and downweight probability */
	mt->minimum[0] = 0.01;
	mt->maximum[0] = 10.0;
	mt->minimum[1] = 0.0;
	mt->maximum[1] = 0.30;
	mt->parsimonyBased = YES;
	mt->level = STANDARD_USER;

	/* Move_Pinvar */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = PINVAR_UNI;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Pinvar;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.1;  /* window size */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 0.999;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_PopsizeM */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = POPSIZE_UNI;
	mt->applicableTo[1] = POPSIZE_LOGNORMAL;
	mt->applicableTo[2] = POPSIZE_NORMAL;
	mt->applicableTo[3] = POPSIZE_GAMMA;
	mt->nApplicable = 4;
	mt->moveFxn = &Move_PopSizeM;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log(1.5);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_RanSPR1 */
	mt = &moveTypes[i++];
	mt->name = "Random SPR version 1";
	mt->shortName = "rSPR1";
    mt->subParams = YES;
	mt->tuningName[0] = "Move probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_RanSPR1;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* move probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_RanSPR2 */
	mt = &moveTypes[i++];
	mt->name = "Random SPR version 2";
	mt->shortName = "rSPR2";
    mt->subParams = YES;
	mt->tuningName[0] = "Move probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_RanSPR2;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* move probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_RanSPR3 */
	mt = &moveTypes[i++];
	mt->name = "Random SPR version 3";
	mt->shortName = "rSPR3";
    mt->subParams = YES;
	mt->tuningName[0] = "Move probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_RanSPR3;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* move probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_RanSPR4 */
	mt = &moveTypes[i++];
	mt->name = "Random SPR version 4";
	mt->shortName = "rSPR4";
    mt->subParams = YES;
	mt->tuningName[0] = "Move probability";
	mt->shortTuningName[0] = "p_ext";
	mt->tuningName[1] = "Multiplier tuning parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->applicableTo[1] = TOPOLOGY_NCL_CONSTRAINED_HOMO;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_RanSPR4;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 0.8; /* move probability */
	mt->tuningParam[1] = 2.0 * log (1.6); /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 0.99999;
	mt->minimum[1] = 0.00000001;
	mt->maximum[1] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_RateMult_Dir */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = RATEMULT_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_RateMult_Dir;
	mt->relProposalProb = 0.5;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 50.0; /* alphaPi per site */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneDirichlet;
    mt->targetRate = 0.25;

	/* Move_RateMult_Slider */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = RATEMULT_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_RateMult_Slider;
	mt->relProposalProb = 0.5;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.05;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 1.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_Revmat_Dir */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = REVMAT_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Revmat_Dir;
	mt->relProposalProb = 0.5;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 100.0;  /* alphaPi per rate */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
	mt->Autotune = &AutotuneDirichlet;
	mt->targetRate = 0.25;

	/* Move_Revmat_DirMix */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = REVMAT_MIX;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Revmat_DirMix;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 100.0;  /* alphaPi per rate */
	mt->minimum[0] = 0.01;
	mt->maximum[0] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneDirichlet;
    mt->targetRate = 0.25;


	/* Move_Revmat_Slider */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = REVMAT_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Revmat_Slider;
	mt->relProposalProb = 0.5;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.15;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 1.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_Revmat_SplitMerge1 */
	mt = &moveTypes[i++];
	mt->name = "Split-merge move 1";
	mt->shortName = "Splitmerge1";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = REVMAT_MIX;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Revmat_SplitMerge1;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 10.0;  /* alphaPi per rate */
	mt->minimum[0] = 0.5;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
	mt->Autotune = &AutotuneDirichlet;
	mt->targetRate = 0.25;

	/* Move_Revmat_SplitMerge2 */
	mt = &moveTypes[i++];
	mt->name = "Split-merge move 2";
	mt->shortName = "Splitmerge2";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = REVMAT_MIX;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Revmat_SplitMerge2;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 10.0;  /* alphaPi per rate */
	mt->minimum[0] = 0.5;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
	mt->Autotune = &AutotuneDirichlet;
	mt->targetRate = 0.25;

	/* Move_Speciation */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = SPECRATE_UNI;
	mt->applicableTo[1] = SPECRATE_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_Speciation;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 1.0;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_Speciation_M */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = SPECRATE_UNI;
	mt->applicableTo[1] = SPECRATE_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_Speciation_M;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.5);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_SpeciesTree */
	mt = &moveTypes[i++];
	mt->name = "Species tree move";
	mt->shortName = "Distmatrixmove";
	mt->tuningName[0] = "Divider of rate of truncated exponential";
	mt->shortTuningName[0] = "lambdadiv";
	mt->applicableTo[0] = SPECIESTREE_UNIFORM;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_SpeciesTree;
	mt->relProposalProb = 10.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 1.2;       /* Default tuning parameter value */
	mt->minimum[0] = 0.00001;       /* Minimum value of tuning param */
	mt->maximum[0] = 10000000.0;    /* Maximum value of tuning param */
	mt->parsimonyBased = NO;        /* It does not use parsimony scores */
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier; /* Autotune this move as a mutliplier move (larger is more bold) */
    mt->targetRate = 0.25;              /* Target acceptance rate */

	/* Move_SPRClock */
	/* not correctly balanced yet !! */
	mt = &moveTypes[i++];
	mt->name = "Clock-constrained SPR";
	mt->shortName = "cSPR";
    mt->subParams = YES;
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->tuningName[1] = "Exponential parameter";
	mt->shortTuningName[1] = "lambda";
	mt->applicableTo[0] = TOPOLOGY_CL_UNIFORM;
	mt->applicableTo[1] = TOPOLOGY_CCL_UNIFORM;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_SPRClock;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 10.0;  /* alphaPi */
	mt->tuningParam[1] = 10.0;  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000.0;
	mt->minimum[1] = 0.00001;
	mt->maximum[1] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_Statefreqs */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = PI_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Statefreqs;
	mt->relProposalProb = 0.5;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 100.0; /* alphaPi per state */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneDirichlet;
    mt->targetRate = 0.25;

	/* Move_Statefreqs_Slider */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = PI_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Statefreqs_Slider;
	mt->relProposalProb = 0.5;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 0.20;  /* window size (change in proportions) */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 1.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
	mt->Autotune = &AutotuneSlider;
	mt->targetRate = 0.25;

	/* Move_StatefreqsSymDirMultistate */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->paramName = "Pi_symdir";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = SYMPI_FIX_MS;
	mt->applicableTo[1] = SYMPI_UNI_MS;
	mt->applicableTo[2] = SYMPI_EXP_MS;
	mt->nApplicable = 3;
	mt->moveFxn = &Move_StatefreqsSymDirMultistate;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 50.0; /* alphaPi */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneDirichlet;
    mt->targetRate = 0.25;

	/* Move_SwitchRate */
	mt = &moveTypes[i++];
	mt->name = "Sliding window";
	mt->shortName = "Slider";
	mt->tuningName[0] = "Sliding window size";
	mt->shortTuningName[0] = "delta";
	mt->applicableTo[0] = SWITCH_UNI;
	mt->applicableTo[1] = SWITCH_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_SwitchRate;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 1.0;  /* window size */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 1000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneSlider;
    mt->targetRate = 0.25;

	/* Move_SwitchRate_M */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = SWITCH_UNI;
	mt->applicableTo[1] = SWITCH_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_SwitchRate_M;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.5);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_Tratio_Dir */
	mt = &moveTypes[i++];
	mt->name = "Dirichlet proposal";
	mt->shortName = "Dirichlet";
	mt->tuningName[0] = "Dirichlet parameter";
	mt->shortTuningName[0] = "alpha";
	mt->applicableTo[0] = TRATIO_DIR;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_Tratio_Dir;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 50.0;  /* alphaPi */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneDirichlet;
    mt->targetRate = 0.25;

	/* Move_TreeStretch */
	mt = &moveTypes[i++];
	mt->name = "Tree stretch";
	mt->shortName = "TreeStretch";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = BRLENS_CLOCK_UNI;
	mt->applicableTo[1] = BRLENS_CLOCK_BD;
	mt->applicableTo[2] = BRLENS_CLOCK_COAL;
	mt->nApplicable = 3;
	mt->moveFxn = &Move_TreeStretch;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log(1.01); /* lambda */
	mt->minimum[0] = 0.00000001;
	mt->maximum[0] = 2.0 * log(2.0);
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

    	/***************************			Begin Code added by Jeremy Brown 		*************************/

	/* Move_TreeLen */
	mt = &moveTypes[i++];
	mt->name = "Whole treelength hit with multiplier";
	mt->shortName = "TLMultiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";	
	mt->applicableTo[0] = BRLENS_UNI;
	mt->applicableTo[1] = BRLENS_EXP;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_TreeLen;
	mt->relProposalProb = 2.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (2.0);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/***************************			End Code added by Jeremy Brown 		*************************/

	/* Move_UnrootedSlider */
	/* Non-clock version of the SPRclock; this move is not correctly balanced yet */
	mt = &moveTypes[i++];
	mt->name = "Branch slider";
	mt->shortName = "Bslider";
    mt->subParams = YES;
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda_m";
	mt->tuningName[1] = "Exponential parameter";
	mt->shortTuningName[1] = "lambda_e";
	mt->applicableTo[0] = TOPOLOGY_NCL_UNIFORM_HOMO;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_UnrootedSlider;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 2;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda multiplier*/
	mt->tuningParam[1] = 10.0;  /* lambda exponential */
	mt->minimum[0] = 0.00000001;
	mt->maximum[0] = 10000000.0;
	mt->minimum[1] = 0.00001;
	mt->maximum[1] = 10000.0;
	mt->parsimonyBased = NO;
	mt->level = DEVELOPER;

	/* Move_AddDeleteCPPEvent */
	mt = &moveTypes[i++];
	mt->name = "Random addition/deletion of CPP event";
	mt->shortName = "Add_delete";
	mt->applicableTo[0] = CPPEVENTS;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_AddDeleteCPPEvent;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_CPPEventPosition */
	mt = &moveTypes[i++];
	mt->name = "Random draw of CPP event position from prior";
	mt->shortName = "Prior_draw_pos";
	mt->applicableTo[0] = CPPEVENTS;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_CPPEventPosition;
	mt->relProposalProb = 2.0;
	mt->numTuningParams = 0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_CPPRate */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = CPPRATE_EXP;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_CPPRate;
	mt->relProposalProb = 2.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_CPPRateMultiplierMult */
	mt = &moveTypes[i++];
	mt->name = "Random CPP rate multiplier hit with multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = CPPEVENTS;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_CPPRateMultiplierMult;
	mt->relProposalProb = 0.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_CPPRateMultiplierRnd */
	mt = &moveTypes[i++];
	mt->name = "Random draw of CPP rate multiplier from prior";
	mt->shortName = "Prior_draw_mult";
	mt->applicableTo[0] = CPPEVENTS;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_CPPRateMultiplierRnd;
	mt->relProposalProb = 2.0;
	mt->numTuningParams = 0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;

	/* Move_Nu */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = TK02VAR_EXP;
	mt->applicableTo[1] = TK02VAR_UNI;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_Nu;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_TK02BranchRate */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = TK02BRANCHRATES;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_TK02BranchRate;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 10.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_IgrVar */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = IGRVAR_EXP;
	mt->applicableTo[1] = IGRVAR_UNI;
	mt->nApplicable = 2;
	mt->moveFxn = &Move_IgrVar;
	mt->relProposalProb = 1.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.1);  /* lambda */
	mt->minimum[0] = 0.00001;
	mt->maximum[0] = 10000000.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	/* Move_IgrBranchLen */
	mt = &moveTypes[i++];
	mt->name = "Multiplier";
	mt->shortName = "Multiplier";
	mt->tuningName[0] = "Multiplier tuning parameter";
	mt->shortTuningName[0] = "lambda";
	mt->applicableTo[0] = IGRBRANCHLENS;
	mt->nApplicable = 1;
	mt->moveFxn = &Move_IgrBranchLen;
	mt->relProposalProb = 5.0;
	mt->numTuningParams = 1;
	mt->tuningParam[0] = 2.0 * log (1.2);  /* lambda */
	mt->minimum[0] = 0.001;
	mt->maximum[0] = 100.0;
	mt->parsimonyBased = NO;
	mt->level = STANDARD_USER;
    mt->Autotune = &AutotuneMultiplier;
    mt->targetRate = 0.25;

	numMoveTypes = i;
	
}





/* ShowModel: Display model on screen */
int ShowModel (void)

{
	
	int			i, j, ns;

	MrBayesPrint ("%s   Model settings:\n\n", spacer);
	for (i=0; i<numCurrentDivisions; i++)
		{
		ns = 0;

		if (numCurrentDivisions > 1)
			MrBayesPrint ("%s      Settings for partition %d --\n", spacer, i+1);
        else
			MrBayesPrint ("%s      Data not partitioned --\n", spacer);
		
		if (modelParams[i].dataType == DNA)
			{
			MrBayesPrint ("%s         Datatype  = DNA\n", spacer);
			ns = 4;
			}
		else if (modelParams[i].dataType == RNA)
			{
			MrBayesPrint ("%s         Datatype  = RNA\n", spacer);
			ns = 4;
			}
		else if (modelParams[i].dataType == PROTEIN)
			{
			MrBayesPrint ("%s         Datatype  = Protein\n", spacer);
			ns = 20;
			}
		else if (modelParams[i].dataType == RESTRICTION)
			{
			MrBayesPrint ("%s         Datatype  = Restriction\n", spacer);
			ns = 2;
			}
		else if (modelParams[i].dataType == STANDARD)
			{
			MrBayesPrint ("%s         Datatype  = Standard\n", spacer);
			ns = 10;
			}
		else if (modelParams[i].dataType == CONTINUOUS)
			{
			MrBayesPrint ("%s         Datatype  = Continuous\n", spacer);
			}
			
		if (modelSettings[i].dataType == CONTINUOUS)
			{
			/* begin description of continuous models */
			  if (!strcmp(modelParams[i].brownCorPr, "Fixed") && AreDoublesEqual(modelParams[i].brownCorrFix, 0.0, ETA)==YES)
				MrBayesPrint ("%s         Model     = Independent Brownian motion\n", spacer);
			else
				MrBayesPrint ("%s         Model     = Correlated Brownian motion\n", spacer);
			/* end description of continuous models */
			}
		else
			{
			/* begin description of discrete models */
			if (!strcmp(modelParams[i].parsModel, "Yes"))
				{
				MrBayesPrint ("%s         Parsmodel = %s\n", spacer, modelParams[i].parsModel);
				}
			else
				{
				/* dna characters in this partition */
				if (modelSettings[i].dataType == DNA || modelSettings[i].dataType == RNA)
					{
					/* general form of the rate matrix */ 
					MrBayesPrint ("%s         Nucmodel  = %s\n", spacer, modelParams[i].nucModel);
				
					/* constraints on rates of substitution */
					MrBayesPrint ("%s         Nst       = %s\n", spacer, modelParams[i].nst);
					if (!strcmp(modelParams[i].nst, "2"))
						{
						if (!strcmp(modelParams[i].tRatioPr,"Beta"))
							{
							MrBayesPrint ("%s                     Transition and transversion  rates, expressed\n", spacer);
							MrBayesPrint ("%s                     as proportions of the rate sum, have a\n", spacer);
							MrBayesPrint ("%s                     Beta(%1.2lf,%1.2lf) prior\n", spacer, modelParams[i].tRatioDir[0], modelParams[i].tRatioDir[1]);
							}
						else
							{
							MrBayesPrint ("%s                     Transition/transversion rate ratio is fixed to %1.2lf.\n", spacer, modelParams[i].tRatioFix);
							}
						}
					else if (!strcmp(modelParams[i].nst, "6"))
						{
						if (!strcmp(modelParams[i].revMatPr,"Dirichlet"))
							{
							MrBayesPrint ("%s                     Substitution rates, expressed as proportions\n", spacer);
							MrBayesPrint ("%s                     of the rate sum, have a Dirichlet prior\n", spacer);
							MrBayesPrint ("%s                     (%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer,
								modelParams[i].revMatDir[0], modelParams[i].revMatDir[1], modelParams[i].revMatDir[2],
								modelParams[i].revMatDir[3], modelParams[i].revMatDir[4], modelParams[i].revMatDir[5]);
							}
						else
							{
							MrBayesPrint ("%s                     Substitution rates are fixed to be \n", spacer);
							MrBayesPrint ("%s                     (%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf).\n", spacer, modelParams[i].revMatFix[0], modelParams[i].revMatFix[1], modelParams[i].revMatFix[2], modelParams[i].revMatFix[3], modelParams[i].revMatFix[4], modelParams[i].revMatFix[5]);
							}
						}
					else if (!strcmp(modelParams[i].nst, "Mixed"))
						{
						if (!strcmp(modelParams[i].revMatPr,"Dirichlet"))
							{
							MrBayesPrint ("%s                     Substitution rates, expressed as proportions\n", spacer);
							MrBayesPrint ("%s                     of the rate sum, have a Dirichlet prior\n", spacer);
							MrBayesPrint ("%s                     (%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer,
								modelParams[i].revMatDir[0], modelParams[i].revMatDir[1], modelParams[i].revMatDir[2],
								modelParams[i].revMatDir[3], modelParams[i].revMatDir[4], modelParams[i].revMatDir[5]);
							}
						else
							{
							MrBayesPrint ("%s                     Substitution rates are fixed to be \n", spacer);
							MrBayesPrint ("%s                     (%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf).\n", spacer, modelParams[i].revMatFix[0], modelParams[i].revMatFix[1], modelParams[i].revMatFix[2], modelParams[i].revMatFix[3], modelParams[i].revMatFix[4], modelParams[i].revMatFix[5]);
							}
						}
					
					if (!strcmp(modelParams[i].nucModel,"Codon"))
						{
						/* what is the distribution on the nonsyn./syn. rate ratio */
						if (!strcmp(modelParams[i].omegaVar, "Equal"))
							{
							if (!strcmp(modelParams[i].omegaPr,"Dirichlet"))
								{
								MrBayesPrint ("%s                     Nonsynonymous and synonymous rates, expressed\n", spacer);
								MrBayesPrint ("%s                     as proportions of the rate sum, have a\n", spacer);
								MrBayesPrint ("%s                     Dirichlet(%1.2lf,%1.2lf) prior\n", spacer, modelParams[i].omegaDir[0], modelParams[i].omegaDir[1]);
								}
							else
								{
								MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio is fixed to %1.2lf.\n", spacer, modelParams[i].omegaFix);
								}
							}
						else if (!strcmp(modelParams[i].omegaVar, "Ny98"))
							{
							if (!strcmp(modelParams[i].ny98omega1pr, "Beta"))
								{
								MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for purifying selection\n", spacer);
								MrBayesPrint ("%s                     (class 1) has a Beta(%1.2lf,%1.2lf) on the interval (0,1).\n", spacer, modelParams[i].ny98omega1Beta[0], modelParams[i].ny98omega1Beta[1]);
								}
							else
								{
								MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for purifying selection\n", spacer);
								MrBayesPrint ("%s                     (class 1) is fixed to %1.2lf.\n", spacer, modelParams[i].ny98omega1Fixed);
								}
							MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for neutral selection\n", spacer);
							MrBayesPrint ("%s                     (class 2) is fixed to 1.0.\n", spacer);
							if (!strcmp(modelParams[i].ny98omega3pr, "Uniform"))
								{
								MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for positive selection\n", spacer);
								MrBayesPrint ("%s                     is uniformly distributed on the interval (%1.2lf,%1.2lf).\n", spacer, modelParams[i].ny98omega3Uni[0], modelParams[i].ny98omega3Uni[1]);
								}
							else if (!strcmp(modelParams[i].ny98omega3pr, "Exponential"))
								{
								MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for positive selection\n", spacer);
								MrBayesPrint ("%s                     is exponentially distributed with parameter (%1.2lf).\n", spacer, modelParams[i].ny98omega3Exp);
								}
							else
								{
								MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for positive \n", spacer);
								MrBayesPrint ("%s                     selection is fixed to %1.2lf.\n", spacer, modelParams[i].ny98omega3Fixed);
								}
							}
						else if (!strcmp(modelParams[i].omegaVar, "M3"))
							{
							if (!strcmp(modelParams[i].m3omegapr, "Exponential"))
								{
								MrBayesPrint ("%s                     Nonsynonymous and synonymous rates for the tree classes of\n", spacer);
								MrBayesPrint ("%s                     omega are exponentially distributed random variables.\n", spacer);
								}
							else if (!strcmp(modelParams[i].m3omegapr, "Fixed"))
								{
								MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for the three omega\n", spacer);
								MrBayesPrint ("%s                     are fixed to %1.2lf, %1.2lf, and %1.2lf.\n", spacer, modelParams[i].m3omegaFixed[0], modelParams[i].m3omegaFixed[1], modelParams[i].m3omegaFixed[2]);
								}
							}
						else if (!strcmp(modelParams[i].omegaVar, "M10"))
							{
							MrBayesPrint ("%s                     Nonsynonymous/synonymous rate ratio for purifying \n", spacer);
							MrBayesPrint ("%s                     selection (class 1) has a Beta(alpha1,beta1) on the \n", spacer);
							MrBayesPrint ("%s                     interval (0,1). Nonsynonymous/synonymous rate ratio \n", spacer);
							MrBayesPrint ("%s                     for positive selection (class 2) has an offset \n", spacer);
							MrBayesPrint ("%s                     Gamma(alpha2,beta2) on the interval (1,Infinity).\n", spacer);
							}
							
						/* genetic code that is used (if nucmodel=codon) */
						MrBayesPrint ("%s         Code      = %s\n", spacer, modelParams[i].geneticCode);
						
						}
						
					}
				/* amino acid characters in this partition */
				else if (modelSettings[i].dataType == PROTEIN)
					{
                    if (modelParams[i].dataType == DNA || modelParams[i].dataType == RNA)
    					MrBayesPrint ("%s         Nucmodel  = %s\n", spacer, modelParams[i].nucModel);
					/* constraints on rates of substitution in 20 X 20 matrix */
					if (!strcmp(modelParams[i].aaModelPr, "Mixed"))
						MrBayesPrint ("%s         Aamodel   = Mixture of models with fixed rate matrices\n", spacer);
					else
						MrBayesPrint ("%s         Aamodel   = %s\n", spacer, modelParams[i].aaModel);
					/* revmat rates */
					if (!strcmp(modelParams[i].aaModelPr, "Mixed"))
						MrBayesPrint ("%s                     Substitution rates come from the mixture of models\n", spacer);
					else if (!strcmp(modelParams[i].aaModelPr, "Fixed") && (!strcmp(modelParams[i].aaModel, "Poisson") ||
                                !strcmp(modelParams[i].aaModel, "Equalin")))
						MrBayesPrint ("%s                     Substitution rates are fixed to be equal\n", spacer);
					else if (!strcmp(modelParams[i].aaModelPr, "Fixed") && strcmp(modelParams[i].aaModel, "Gtr")!=0)
						MrBayesPrint ("%s                     Substitution rates are fixed to the %s rates\n", spacer, modelParams[i].aaModel);
					else if (!strcmp(modelParams[i].aaModelPr, "Fixed") && !strcmp(modelParams[i].aaModel, "Gtr"))
						{
						if (!strcmp(modelParams[i].aaRevMatPr,"Dirichlet"))
							{
							for (j=0; j<190; j++)
								if (AreDoublesEqual(modelParams[i].aaRevMatDir[0], modelParams[i].aaRevMatDir[j], 0.00001) == NO)
									break;
							if (j == 190)
								{
								MrBayesPrint ("%s                     Substitution rates have a Dirichlet(%1.2lf,%1.2lf,...) prior\n",
									spacer, modelParams[i].aaRevMatDir[0], modelParams[i].aaRevMatDir[0]);
								}
							else
								{
								MrBayesPrint ("%s                     Substitution rates have a Dirichlet(\n", spacer);
								for (j=0; j<190; j++)
									{
									if (j % 10 == 0)
										MrBayesPrint ("%s                        ", spacer);
									MrBayesPrint ("%1.2lf", modelParams[i].aaRevMatDir[j]);
									if (j == 189)
										MrBayesPrint (") prior\n");
									else if ((j+1) % 10 == 0)
										MrBayesPrint (",\n");
									else
										MrBayesPrint (",");
									}
								}
							}
						else /* if (!strcmp(modelParams[i].aaRevMatPr,"Fixed")) */
							{
							for (j=0; j<190; j++)
								if (AreDoublesEqual(modelParams[i].aaRevMatFix[0], modelParams[i].aaRevMatFix[j], 0.00001) == NO)
									break;
							if (j == 190)
								{
								MrBayesPrint ("%s                     Substitution rates are fixed to (%1.1lf,%1.1lf,...)\n",
									spacer, modelParams[i].aaRevMatFix[0], modelParams[i].aaRevMatFix[0]);
								}
							else
								{
								MrBayesPrint ("%s                     Substitution rates are fixed to (\n", spacer);
								for (j=0; j<190; j++)
									{
									if (j % 10 == 0)
										MrBayesPrint ("%s                        ", spacer);
									MrBayesPrint ("%1.1lf", modelParams[i].aaRevMatFix[j]);
									if (j == 189)
										MrBayesPrint (") prior\n");
									else if ((j+1) % 10 == 0)
										MrBayesPrint (",\n");
									else
										MrBayesPrint (",");
									}
								}
							}
						}
					}
				/* restriction site or morphological characters in this partition */
				else if (modelSettings[i].dataType == RESTRICTION || modelSettings[i].dataType == STANDARD)
					{
					/* what type of characters are sampled? */
					MrBayesPrint ("%s         Coding    = %s\n", spacer, modelParams[i].coding);
					}
					
				/* is there rate variation in a single site across the tree? */
				if (((modelSettings[i].dataType == DNA || modelSettings[i].dataType == RNA) && !strcmp(modelParams[i].nucModel, "4by4")) || modelSettings[i].dataType == PROTEIN)
					{
					/* do rates change on tree accoding to covarion model? */
					MrBayesPrint ("%s         Covarion  = %s\n", spacer, modelParams[i].covarionModel);
					if (!strcmp(modelParams[i].covarionModel, "Yes"))
						{
						/* distribution on switching parameters, if appropriate */
						if (!strcmp(modelParams[i].covSwitchPr,"Uniform"))
							{
							MrBayesPrint ("%s                     Switching rates have independent uniform dist-\n", spacer);
							MrBayesPrint ("%s                     ributions on the interval (%1.2lf,%1.2lf).\n", spacer, modelParams[i].covswitchUni[0], modelParams[i].covswitchUni[1]);
							}
						else if (!strcmp(modelParams[i].covSwitchPr,"Exponential"))
							{
							MrBayesPrint ("%s                     Switching rates have independent exponential\n", spacer);
							MrBayesPrint ("%s                     distributions with parameters (%1.2lf).\n", spacer, modelParams[i].covswitchExp);
							}
						else
							{
							MrBayesPrint ("%s                     Switching rates are fixed to %1.2lf and %1.2lf.\n", spacer, modelParams[i].covswitchFix[0], modelParams[i].covswitchFix[0]);
							}
						ns *= 2;
						}
					}

				/* now, let's deal with variation in omega */
				if ((modelParams[i].dataType == DNA || modelParams[i].dataType == RNA) && !strcmp(modelParams[i].nucModel,"Codon"))
					{
					MrBayesPrint ("%s         Omegavar  = %s\n", spacer, modelParams[i].omegaVar);
					if (!strcmp(modelParams[i].geneticCode, "Universal"))
						ns = 61;
					else if (!strcmp(modelParams[i].geneticCode, "Vertmt"))
						ns = 60;
					else if (!strcmp(modelParams[i].geneticCode, "Mycoplasma"))
						ns = 62;
					else if (!strcmp(modelParams[i].geneticCode, "Yeast"))
						ns = 62;
					else if (!strcmp(modelParams[i].geneticCode, "Ciliates"))
						ns = 63;
					else if (!strcmp(modelParams[i].geneticCode, "Metmt"))
						ns = 62;
					}

				/* what assumptions are made about the state frequencies? */
				if (modelParams[i].dataType != CONTINUOUS)
					{
					if (modelParams[i].dataType == STANDARD)
						MrBayesPrint ("%s         # States  = Variable, up to 10\n", spacer);
					else if (modelSettings[i].numStates != modelSettings[i].numModelStates)
						MrBayesPrint ("%s         # States  = %d (in the model)\n", spacer, modelSettings[i].numModelStates);
					else
						MrBayesPrint ("%s         # States  = %d\n", spacer, ns);
					if (modelSettings[i].dataType == STANDARD)
						{
						if (!strcmp(modelParams[i].symPiPr,"Fixed"))
						        { 
							  if (AreDoublesEqual(modelParams[i].symBetaFix, -1.0,ETA)==YES)
								MrBayesPrint ("%s                     State frequencies are fixed to be equal\n", spacer);
							else
								MrBayesPrint ("%s                     Symmetric Dirichlet is fixed to %1.2lf\n", spacer, modelParams[i].symBetaFix);
							}
						else if (!strcmp(modelParams[i].symPiPr,"Uniform"))
							{
							MrBayesPrint ("%s                     Symmetric Dirichlet has a Uniform(%1.2lf,%1.2lf) prior\n", spacer, modelParams[i].symBetaUni[0], modelParams[i].symBetaUni[1]);
							}
						else
							{
							MrBayesPrint ("%s                     Symmetric Dirichlet has a Exponential(%1.2lf) prior\n", spacer, modelParams[i].symBetaExp);
							}
						}
					else if (modelSettings[i].dataType == RESTRICTION)
						{
						/* distribution on state frequencies for restriction site model */
						if (!strcmp(modelParams[i].stateFreqPr,"Dirichlet"))
							{
							MrBayesPrint ("%s                     State frequencies have a Dirichlet (%1.2lf,%1.2lf) prior\n", spacer,
								modelParams[i].stateFreqsDir[0], modelParams[i].stateFreqsDir[1]);
							}
						else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"Equal"))
							{
							MrBayesPrint ("%s                     State frequencies are fixed to be equal\n", spacer);
							}
						else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"User"))
							{
							MrBayesPrint ("%s                     State frequencies have been fixed by the user\n", spacer);
							}
						else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"Empirical"))
							{
							MrBayesPrint ("%s                     State frequencies have been fixed to the empirical frequencies in the data\n", spacer);
							}
						}
					else if (modelSettings[i].dataType == PROTEIN)
						{
						/* distribution on state frequencies for aminoacid model */
						if (!strcmp(modelParams[i].aaModelPr, "Fixed") && (strcmp(modelParams[i].aaModel, "Equalin")==0 ||
							strcmp(modelParams[i].aaModel, "Gtr")==0))
							{
							if (!strcmp(modelParams[i].stateFreqPr,"Dirichlet"))
								{
								MrBayesPrint ("%s                     State frequencies have a Dirichlet prior\n", spacer);
								MrBayesPrint ("%s                     (%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,", spacer,
									modelParams[i].stateFreqsDir[0], modelParams[i].stateFreqsDir[1], modelParams[i].stateFreqsDir[2],
									modelParams[i].stateFreqsDir[3], modelParams[i].stateFreqsDir[4]);
								MrBayesPrint ("%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,\n",
									modelParams[i].stateFreqsDir[5], modelParams[i].stateFreqsDir[6], modelParams[i].stateFreqsDir[7],
									modelParams[i].stateFreqsDir[8], modelParams[i].stateFreqsDir[9]);
								MrBayesPrint ("%s                     %1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,", spacer,
									modelParams[i].stateFreqsDir[10], modelParams[i].stateFreqsDir[11], modelParams[i].stateFreqsDir[12],
									modelParams[i].stateFreqsDir[13], modelParams[i].stateFreqsDir[14]);
								MrBayesPrint ("%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n",
									modelParams[i].stateFreqsDir[15], modelParams[i].stateFreqsDir[16], modelParams[i].stateFreqsDir[17],
									modelParams[i].stateFreqsDir[18], modelParams[i].stateFreqsDir[19]);
								}
							else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"Equal"))
								{
								MrBayesPrint ("%s                     State frequencies are fixed to be equal\n", spacer);
								}
							else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"User"))
								{
								MrBayesPrint ("%s                     State frequencies have been fixed by the user\n", spacer);
								}
							else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"Empirical"))
								{
								MrBayesPrint ("%s                     State frequencies have been fixed to the empirical frequencies in the data\n", spacer);
								}
							}
						else if (!strcmp(modelParams[i].aaModelPr, "Fixed") && !strcmp(modelParams[i].aaModel, "Poisson"))
							{
							MrBayesPrint ("%s                     State frequencies are fixed to be equal\n", spacer);
							}
						else if (!strcmp(modelParams[i].aaModelPr, "Fixed") && strcmp(modelParams[i].aaModel, "Equalin") && strcmp(modelParams[i].aaModel, "Poisson"))
							{
							MrBayesPrint ("%s                     State frequencies are fixed to the %s frequencies\n", spacer, modelParams[i].aaModel);
							}
						else
							{
							MrBayesPrint ("%s                     State frequencies come from the mixture of models\n", spacer);
							}
						}
					else
						{
						/* distribution on state frequencies for all other models */
						if (!strcmp(modelParams[i].stateFreqPr,"Dirichlet"))
							{
							MrBayesPrint ("%s                     State frequencies have a Dirichlet prior\n", spacer);
							if (!strcmp(modelParams[i].nucModel, "Doublet"))
								{
								MrBayesPrint ("%s                     (%1.2lf,%1.2lf,%1.2lf,%1.2lf,\n", spacer,
									modelParams[i].stateFreqsDir[0], modelParams[i].stateFreqsDir[1], modelParams[i].stateFreqsDir[2],
									modelParams[i].stateFreqsDir[3]);
								MrBayesPrint ("%s                     %1.2lf,%1.2lf,%1.2lf,%1.2lf,\n", spacer,
									modelParams[i].stateFreqsDir[4], modelParams[i].stateFreqsDir[5], modelParams[i].stateFreqsDir[6],
									modelParams[i].stateFreqsDir[7]);
								MrBayesPrint ("%s                     %1.2lf,%1.2lf,%1.2lf,%1.2lf,\n", spacer,
									modelParams[i].stateFreqsDir[8], modelParams[i].stateFreqsDir[9], modelParams[i].stateFreqsDir[10],
									modelParams[i].stateFreqsDir[11]);
								MrBayesPrint ("%s                     %1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer,
									modelParams[i].stateFreqsDir[12], modelParams[i].stateFreqsDir[13], modelParams[i].stateFreqsDir[14],
									modelParams[i].stateFreqsDir[15]);
								}
							else if (!strcmp(modelParams[i].nucModel, "4by4"))
								{
								MrBayesPrint ("%s                     (%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer,
									modelParams[i].stateFreqsDir[0], modelParams[i].stateFreqsDir[1], modelParams[i].stateFreqsDir[2],
									modelParams[i].stateFreqsDir[3]);
								}
							}
						else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"Equal"))
							{
							MrBayesPrint ("%s                     State frequencies are fixed to be equal\n", spacer);
							}
						else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"User"))
							{
							MrBayesPrint ("%s                     State frequencies have been fixed by the user\n", spacer);
							}
						else if (!strcmp(modelParams[i].stateFreqPr,"Fixed") && !strcmp(modelParams[i].stateFreqsFixType,"Empirical"))
							{
							MrBayesPrint ("%s                     State frequencies have been fixed to the empirical frequencies in the data\n", spacer);
							}
						}
					}
				else
					MrBayesPrint ("%s         # States  = Infinity\n", spacer);

				/* now, let's deal with rate variation across sites */
				if (modelSettings[i].dataType != CONTINUOUS)
					{
					if (((modelSettings[i].dataType == DNA || modelSettings[i].dataType == RNA) && strcmp(modelParams[i].nucModel,"Codon")) ||
					      modelSettings[i].dataType == PROTEIN || modelSettings[i].dataType == RESTRICTION || modelSettings[i].dataType == STANDARD)
						{
						if (!strcmp(modelParams[i].covarionModel, "No"))
							MrBayesPrint ("%s         Rates     = %s\n", spacer, modelParams[i].ratesModel);
						else
							{
							if (!strcmp(modelParams[i].ratesModel, "Propinv"))
								MrBayesPrint ("%s         Rates     = Equal ", spacer);
							else if (!strcmp(modelParams[i].ratesModel, "Invgamma"))
								MrBayesPrint ("%s         Rates     = Gamma ", spacer);
							else
								MrBayesPrint ("%s         Rates     = %s ", spacer, modelParams[i].ratesModel);
							MrBayesPrint ("(+ Propinv induced by covarion model)\n");
							}
						
						if ((modelParams[i].dataType == RESTRICTION || modelParams[i].dataType == STANDARD) && !strcmp(modelParams[i].ratesModel, "Adgamma"))
							{
							
							}
						else
							{
							if ((!strcmp(modelParams[i].ratesModel, "Invgamma") || !strcmp(modelParams[i].ratesModel, "Gamma") || !strcmp(modelParams[i].ratesModel, "Adgamma")))
								{
								/* distribution on shape parameter, if appropriate */
								if (!strcmp(modelParams[i].shapePr,"Uniform"))
									{
									MrBayesPrint ("%s                     Gamma shape parameter is uniformly dist-\n", spacer);
									MrBayesPrint ("%s                     ributed on the interval (%1.2lf,%1.2lf).\n", spacer, modelParams[i].shapeUni[0], modelParams[i].shapeUni[1]);
									}
								else if (!strcmp(modelParams[i].shapePr,"Exponential"))
									{
									MrBayesPrint ("%s                     Gamma shape parameter is exponentially\n", spacer);
									MrBayesPrint ("%s                     distributed with parameter (%1.2lf).\n", spacer, modelParams[i].shapeExp);
									}
								else
									{
									MrBayesPrint ("%s                     Gamma shape parameter is fixed to %1.2lf.\n", spacer, modelParams[i].shapeFix);
									}
								}
							if ((!strcmp(modelParams[i].ratesModel, "Propinv") || !strcmp(modelParams[i].ratesModel, "Invgamma")) && !strcmp(modelParams[i].covarionModel, "No"))
								{
								/* distribution on pInvar parameter, if appropriate */
								if (!strcmp(modelParams[i].pInvarPr,"Uniform"))
									{
									MrBayesPrint ("%s                     Proportion of invariable sites is uniformly dist-\n", spacer);
									MrBayesPrint ("%s                     ributed on the interval (%1.2lf,%1.2lf).\n", spacer, modelParams[i].pInvarUni[0], modelParams[i].pInvarUni[1]);
									}
								else
									{
									MrBayesPrint ("%s                     Proportion of invariable sites is fixed to %1.2lf.\n", spacer, modelParams[i].pInvarFix);
									}
								}
							if (!strcmp(modelParams[i].ratesModel, "Adgamma"))
								{
								/* distribution on correlation parameter, if appropriate */
								if (!strcmp(modelParams[i].adGammaCorPr,"Uniform"))
									{
									MrBayesPrint ("%s                     Rate correlation parameter is uniformly dist-\n", spacer);
									MrBayesPrint ("%s                     ributed on the interval (%1.2lf,%1.2lf).\n", spacer, modelParams[i].corrUni[0], modelParams[i].corrUni[1]);
									}
								else
									{
									MrBayesPrint ("%s                     Rate correlation parameter is fixed to %1.2lf.\n", spacer, modelParams[i].corrFix);
									}
								}
							
							if (!strcmp(modelParams[i].ratesModel, "Gamma") || !strcmp(modelParams[i].ratesModel, "Invgamma") || !strcmp(modelParams[i].ratesModel, "Adgamma"))
								{
								/* how many categories is the continuous gamma approximated by? */
								MrBayesPrint ("%s                     Gamma distribution is approximated using %d categories.\n", spacer, modelParams[i].numGammaCats);
								if (!strcmp(modelParams[i].useGibbs,"Yes"))
									MrBayesPrint ("%s                     Rate categories sampled using Gibbs sampling.\n", spacer, modelParams[i].numGammaCats);
								else
									MrBayesPrint ("%s                     Likelihood summarized over all rate categories in each generation.\n", spacer, modelParams[i].numGammaCats);
								}
							}						
						}
					}
				}
			/* end description of discrete models */
			}

		if (i != numCurrentDivisions - 1)
			MrBayesPrint ("\n");
		
		}

	MrBayesPrint ("\n");
	ShowParameters (NO, NO, NO);
	
	return (NO_ERROR);
	
}




/*------------------------------------------------------------------------------
|
|   ShowMoves: Show applicable moves
|
------------------------------------------------------------------------------*/
int ShowMoves (int used)

{

	int				i, k, run, chain, chainIndex, areRunsSame, areChainsSame, numPrintedMoves;
	MCMCMove		*mv;
	
	chainIndex = 0;
	numPrintedMoves = 0;
	for (i=0; i<numApplicableMoves; i++)
		{
		mv = moves[i];
		
		for (k=0; k<numGlobalChains; k++)
			{
			if (mv->relProposalProb[k] > 0.000001)
				break;
			}

		if (k == numGlobalChains && used == YES)
			continue;

		if (k < numGlobalChains && used == NO)
			continue;

		numPrintedMoves++;
		
		/* print move number and name */
		MrBayesPrint ("%s   %4d -- Move        = %s\n", spacer, numPrintedMoves, mv->name);
		
		/* print move type */
		MrBayesPrint ("%s           Type        = %s\n", spacer, mv->moveType->name);

		/* print parameter */
		if (mv->parm->nSubParams > 0)
			MrBayesPrint ("%s           Parameters  = %s [param. %d] (%s)\n", spacer, mv->parm->name,
				mv->parm->index+1, mv->parm->paramTypeName);
		else
			MrBayesPrint ("%s           Parameter   = %s [param. %d] (%s)\n", spacer, mv->parm->name,
				mv->parm->index+1, mv->parm->paramTypeName);
		for (k=0; k<mv->parm->nSubParams; k++)
			MrBayesPrint ("%s                         %s [param. %d] (%s)\n", spacer, mv->parm->subParams[k]->name,
				mv->parm->subParams[k]->index+1, mv->parm->subParams[k]->paramTypeName);

		/* print tuning parameters */
		for (k=0; k<mv->moveType->numTuningParams; k++)
			{
			if (k==0)
				MrBayesPrint ("%s           Tuningparam = %s (%s)\n", spacer, mv->moveType->shortTuningName[k], mv->moveType->tuningName[k]);
			else
				MrBayesPrint ("%s                         %s (%s)\n", spacer, mv->moveType->shortTuningName[k], mv->moveType->tuningName[k]);
			}
		
		/* loop over tuning parameters */
		for (k=0; k<mv->moveType->numTuningParams; k++)
			{
			/* find if tuning parameters are different for different runs */
			areRunsSame = YES;
			for (run=1; run<chainParams.numRuns; run++)
				{
				for (chain=0; chain<chainParams.numChains; chain++)
					{
					chainIndex = run*chainParams.numChains + chain;
					if (AreDoublesEqual (mv->tuningParam[chainIndex][k], mv->tuningParam[chain][k], 0.000001) == NO)
						{
						areRunsSame = NO;
						break;
						}
					}
				if (areRunsSame == NO)
					break;
				}
		
			/* now print values */
			for (run=0; run<chainParams.numRuns; run++)
				{
				if (areRunsSame == YES && run >= 1)
					break;

				/* find out if chains are different within this run */
				areChainsSame = YES;
				for (chain=1; chain<chainParams.numChains; chain++)
					{
					chainIndex = run*chainParams.numChains + chain;
					if (AreDoublesEqual (mv->tuningParam[chainIndex][k], mv->tuningParam[chainIndex-chain][k],0.000001) == NO)
						{
						areChainsSame = NO;
						break;
						}
					}
				/* now we can print the values */
				for (chain=0; chain<chainParams.numChains; chain++)
					{
					chainIndex = run*chainParams.numChains + chain;
					if (areChainsSame == YES && chain >= 1)
						break;
					
					if (run == 0 && chain == 0)
						MrBayesPrint ("%s%22s = %1.3lf", spacer, mv->moveType->shortTuningName[k], mv->tuningParam[chainIndex][k]);
					else
						MrBayesPrint ("%s                         %1.3lf", spacer, mv->tuningParam[chainIndex][k]);

					if (areChainsSame == NO && areRunsSame == YES)
						MrBayesPrint ("  [chain %d]\n", chain+1);
					else if (areChainsSame == YES && areRunsSame == NO)
						MrBayesPrint ("  [run %d]\n", run+1);
					else if (areChainsSame == NO && areRunsSame == NO)
						MrBayesPrint ("  [run %d, chain %d]\n", run+1, chain+1);
					else
						MrBayesPrint ("\n");
					}
				}
			}	/* next tuning parameter */

		/* print target acceptance rate for autotuning */
		if (mv->moveType->targetRate > 0.0 && mv->moveType->targetRate < 1.0)
            {

		    /* first find out if the targets are different in different runs */			
		    areRunsSame = YES;
		    for (run=1; run<chainParams.numRuns; run++)
			    {
			    for (chain=0; chain<chainParams.numChains; chain++)
				    {
				    chainIndex = run*chainParams.numChains + chain;
				    if (AreDoublesEqual (mv->targetRate[chainIndex], mv->targetRate[chain], 0.000001) == NO)
					    {
					    areRunsSame = NO;
					    break;
					    }
				    }
			    if (areRunsSame == NO)
				    break;
			    }
    	
		    /* now print values */
		    for (run=0; run<chainParams.numRuns; run++)
			    {
			    if (areRunsSame == YES && run >= 1)
				    break;

			    /* find out if chains are different within this run */
			    areChainsSame = YES;
			    for (chain=1; chain<chainParams.numChains; chain++)
				    {
				    chainIndex = run*chainParams.numChains + chain;
				    if (AreDoublesEqual (mv->targetRate[chainIndex], mv->targetRate[chainIndex-chain], 0.000001) == NO)
					    {
					    areChainsSame = NO;
					    break;
					    }
				    }
			    /* now we can print the values */
			    for (chain=0; chain<chainParams.numChains; chain++)
				    {
				    chainIndex = run*chainParams.numChains + chain;
				    if (areChainsSame == YES && chain >= 1)
					    break;
    				
				    if (run == 0 && chain == 0)
					    MrBayesPrint ("%s           Targetrate  = %1.3lf", spacer, mv->targetRate[chainIndex]);
				    else
					    MrBayesPrint ("%s                         %1.3lf", spacer, mv->targetRate[chainIndex]);

				    if (areChainsSame == NO && areRunsSame == YES)
					    MrBayesPrint ("  [chain %d]\n", chain+1);
				    else if (areChainsSame == YES && areRunsSame == NO)
					    MrBayesPrint ("  [run %d]\n", run+1);
				    else if (areChainsSame == NO && areRunsSame == NO)
					    MrBayesPrint ("  [run %d, chain %d]\n", run+1, chain+1);
				    else
					    MrBayesPrint ("\n");
				    }
			    }
            }

        
        /* finally print the relative proposal probability */
		
		/* first find out if the probabilities are different in different runs */			
		areRunsSame = YES;
		for (run=1; run<chainParams.numRuns; run++)
			{
			for (chain=0; chain<chainParams.numChains; chain++)
				{
				chainIndex = run*chainParams.numChains + chain;
				if (AreDoublesEqual (mv->relProposalProb[chainIndex], mv->relProposalProb[chain], 0.000001) == NO)
					{
					areRunsSame = NO;
					break;
					}
				}
			if (areRunsSame == NO)
				break;
			}
	
		/* now print values */
		for (run=0; run<chainParams.numRuns; run++)
			{
			if (areRunsSame == YES && run >= 1)
				break;

			/* find out if chains are different within this run */
			areChainsSame = YES;
			for (chain=1; chain<chainParams.numChains; chain++)
				{
				chainIndex = run*chainParams.numChains + chain;
				if (AreDoublesEqual (mv->relProposalProb[chainIndex], mv->relProposalProb[chainIndex-chain], 0.000001) == NO)
					{
					areChainsSame = NO;
					break;
					}
				}
			/* now we can print the values */
			for (chain=0; chain<chainParams.numChains; chain++)
				{
				chainIndex = run*chainParams.numChains + chain;
				if (areChainsSame == YES && chain >= 1)
					break;
				
				if (run == 0 && chain == 0)
					MrBayesPrint ("%s           Rel. prob.  = %1.1lf", spacer, mv->relProposalProb[chainIndex]);
				else
					MrBayesPrint ("%s                         %1.1lf", spacer, mv->relProposalProb[chainIndex]);

				if (areChainsSame == NO && areRunsSame == YES)
					MrBayesPrint ("  [chain %d]\n", chain+1);
				else if (areChainsSame == YES && areRunsSame == NO)
					MrBayesPrint ("  [run %d]\n", run+1);
				else if (areChainsSame == NO && areRunsSame == NO)
					MrBayesPrint ("  [run %d, chain %d]\n", run+1, chain+1);
				else
					MrBayesPrint ("\n");
				}
			}
		MrBayesPrint ("\n");
        }	/* next move */
		
	if (numPrintedMoves == 0)
		{
		if (used == YES)
			{
			MrBayesPrint ("%s      No moves currently used for this analysis. All parameters\n", spacer);
			MrBayesPrint ("%s      will be fixed to their starting values.\n\n", spacer);
			}
		else
			{
			MrBayesPrint ("%s      No additional moves available for this model.\n\n", spacer);
			}
		}

	return (NO_ERROR);

}





/*------------------------------------------------------------------------------
|
|   ShowParameters: Show parameter table and parameter info
|
------------------------------------------------------------------------------*/
int ShowParameters (int showStartVals, int showMoves, int showAllAvailable)

{

	int				a, b, d, i, j, k, m, n, run, chain, shouldPrint, isSame, areRunsSame, areChainsSame, nValues,
					chainIndex, refIndex, numPrinted, numMovedChains, printedCol, screenWidth=100;
	Param			*p;
	Model			*mp;
    ModelInfo       *ms;
	MrBFlt			*value, *refValue, *subValue;
	MCMCMove		*mv;
	
	
	MrBayesPrint ("%s   Active parameters: \n\n", spacer);
	if (numCurrentDivisions > 1)
		{ 
		MrBayesPrint ("%s                       Partition(s)\n", spacer);
		MrBayesPrint ("%s      Parameters     ", spacer);
		for (i=0; i<numCurrentDivisions; i++)
			MrBayesPrint (" %2d", i+1);
		MrBayesPrint ("\n");
		MrBayesPrint ("%s      ---------------", spacer);
		for (i=0; i<numCurrentDivisions; i++)
			MrBayesPrint ("---");
		MrBayesPrint ("\n");
		}
	else
		{
		MrBayesPrint ("%s      Parameters\n", spacer);
		MrBayesPrint ("%s      ------------------\n", spacer);
		}
	for (j=0; j<NUM_LINKED; j++)
		{
		shouldPrint = NO;
		for (i=0; i<numCurrentDivisions; i++)
			{
			if (activeParams[j][i] == -1)
				{}
			else
				shouldPrint = YES;
			}
		if (shouldPrint == NO)
			continue;
		
		if (j == P_TRATIO)
			{
			MrBayesPrint ("%s      Tratio         ", spacer);
			}
		else if (j == P_REVMAT)
			{
			MrBayesPrint ("%s      Revmat         ", spacer);
			}
		else if (j == P_OMEGA)
			{
			MrBayesPrint ("%s      Omega          ", spacer);
			}
		else if (j == P_PI)
			{
			MrBayesPrint ("%s      Statefreq      ", spacer);
			}
		else if (j == P_SHAPE)
			{
			MrBayesPrint ("%s      Shape          ", spacer);
			}
		else if (j == P_PINVAR)
			{
			MrBayesPrint ("%s      Pinvar         ", spacer);
			}
		else if (j == P_CORREL)
			{
			MrBayesPrint ("%s      Correlation    ", spacer);
			}
		else if (j == P_SWITCH)
			{
			MrBayesPrint ("%s      Switchrates    ", spacer);
			}
		else if (j == P_RATEMULT)
			{
			MrBayesPrint ("%s      Ratemultiplier ", spacer);
			}
		else if (j == P_GENETREERATE)
			{
			MrBayesPrint ("%s      Generatemult   ", spacer);
			}
		else if (j == P_TOPOLOGY)
			{
			MrBayesPrint ("%s      Topology       ", spacer);
			}
		else if (j == P_BRLENS)
			{
			MrBayesPrint ("%s      Brlens         ", spacer);
			}
		else if (j == P_SPECRATE)
			{
			MrBayesPrint ("%s      Speciationrate ", spacer);
			}
		else if (j == P_EXTRATE)
			{
			MrBayesPrint ("%s      Extinctionrate ", spacer);
			}
		else if (j == P_POPSIZE)
			{
			MrBayesPrint ("%s      Popsize        ", spacer);
			}
		else if (j == P_GROWTH)
			{
			MrBayesPrint ("%s      Growthrate     ", spacer);
			} 
		else if (j == P_AAMODEL)
			{
			MrBayesPrint ("%s      Aamodel        ", spacer);
			}
		else if (j == P_BRCORR)
			{
			MrBayesPrint ("%s      Brownian corr. ", spacer);
			}
		else if (j == P_BRSIGMA)
			{
			MrBayesPrint ("%s      Brownian sigma ", spacer);
			}
		else if (j == P_CPPRATE)
			{
			MrBayesPrint ("%s      Cpprate        ", spacer);
			}
		else if (j == P_CPPMULTDEV)
			{
			MrBayesPrint ("%s      Cppmultdev     ", spacer);
			}
		else if (j == P_CPPEVENTS)
			{
			MrBayesPrint ("%s      Cppevents      ", spacer);
			}
		else if (j == P_TK02VAR)
			{
			MrBayesPrint ("%s      TK02var        ", spacer);
			}
		else if (j == P_TK02BRANCHRATES)
			{
			MrBayesPrint ("%s      TK02           ", spacer);
			}
		else if (j == P_IGRVAR)
			{
			MrBayesPrint ("%s      Igrvar         ", spacer);
			}
		else if (j == P_IGRBRANCHLENS)
			{
			MrBayesPrint ("%s      Igrbranchlens  ", spacer);
			}
		else if (j == P_CLOCKRATE)
			{
			MrBayesPrint ("%s      Clockrate      ", spacer);
			}
		else if (j == P_SPECIESTREE)
			{
			MrBayesPrint ("%s      Speciestree    ", spacer);
			}
        else
            {
            MrBayesPrint ("%s      ERROR: Someone forgot to name parameter type %d", spacer, j);
            return (ERROR);
            }
		
		for (i=0; i<numCurrentDivisions; i++)
			{
			if (activeParams[j][i] == -1)
				MrBayesPrint ("  .");
			else
				MrBayesPrint (" %2d", activeParams[j][i]);
			}
		MrBayesPrint ("\n");
		}
	if (numCurrentDivisions > 1)
		{ 
		MrBayesPrint ("%s      ---------------", spacer);
		for (i=0; i<numCurrentDivisions; i++)
			MrBayesPrint ("---");
		MrBayesPrint ("\n");
		}
	else
		{
		MrBayesPrint ("%s      ------------------\n", spacer);
		}
	
	MrBayesPrint ("\n");
	
	if (numCurrentDivisions > 1)
		MrBayesPrint ("%s      Parameters can be linked or unlinked across partitions using 'link' and 'unlink'\n\n", spacer);
	
	for (i=0; i<numParams; i++)
		{
		p = &params[i];
		j = p->paramType;
		
        mp = &modelParams[p->relParts[0]];
        ms = &modelSettings[p->relParts[0]];
		
		/* print parameter number and name */
		MrBayesPrint ("%s   %4d --  Parameter  = %s\n", spacer, i+1, p->name);
		MrBayesPrint ("%s            Type       = %s\n", spacer, p->paramTypeName);
		/* print prior */
		if (j == P_TRATIO)
			{
			if (!strcmp(mp->tRatioPr,"Beta"))
				MrBayesPrint ("%s            Prior      = Beta(%1.2lf,%1.2lf)\n", spacer, mp->tRatioDir[0], modelParams[i].tRatioDir[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->tRatioFix);
			}
		else if (j == P_REVMAT)
			{
			if (ms->numModelStates != 20)
                {
                if (!strcmp(mp->nst,"Mixed"))
                    {
                    MrBayesPrint ("%s            Prior      = All GTR submodels have equal probability\n", spacer); 
                    MrBayesPrint ("%s                         All revmat rates have a Symmetric Dirichlet(%1.2lf) prior\n", spacer, mp->revMatSymDir);
                    }
    			else if (!strcmp(mp->revMatPr,"Dirichlet"))
                    {
                    MrBayesPrint ("%s            Prior      = Dirichlet(%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer, 
				    mp->revMatDir[0], mp->revMatDir[1], mp->revMatDir[2],
				    mp->revMatDir[3], mp->revMatDir[4], mp->revMatDir[5]);
                    }
			    else
				    MrBayesPrint ("%s            Prior      = Fixed(%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf,%1.2lf)\n", spacer, 
				    mp->revMatFix[0], mp->revMatFix[1], mp->revMatFix[2],
				    mp->revMatFix[3], mp->revMatFix[4], mp->revMatFix[5]);
                }
            else if (ms->numModelStates == 20)
                {
    			if (!strcmp(mp->revMatPr,"Dirichlet"))
                    MrBayesPrint ("%s            Prior      = Dirichlet\n", spacer);
			    else
                    MrBayesPrint ("%s            Prior      = Fixed(user-specified)\n", spacer);
                }
			}
		else if (j == P_OMEGA)
			{
			if (!strcmp(mp->omegaVar,"Equal"))
				{
				if (!strcmp(mp->omegaPr,"Dirichlet"))
					MrBayesPrint ("%s            Prior      = Dirichlet(%1.2lf,%1.2lf)\n", spacer, mp->omegaDir[0], mp->omegaDir[1]);
				else
					MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->omegaFix);
				}
			else if (!strcmp(mp->omegaVar,"Ny98"))
				{
				if (!strcmp(mp->ny98omega1pr,"Beta"))
					MrBayesPrint ("%s            Prior      = Beta(%1.2lf,%1.2lf) on omega(1)\n", spacer, mp->ny98omega1Beta[0], mp->ny98omega1Beta[1]);
				else
					MrBayesPrint ("%s            Prior      = Fixed(%1.2lf) on omega(1)\n", spacer, mp->ny98omega1Fixed);
				MrBayesPrint ("%s                         Fixed(1.00) on omega(2)\n", spacer);
				if (!strcmp(mp->ny98omega3pr,"Uniform"))
					MrBayesPrint ("%s                         Uniform(%1.2lf,%1.2lf) on omega(3)\n", spacer, mp->ny98omega3Uni[0], mp->ny98omega3Uni[1]);
				else if (!strcmp(mp->ny98omega3pr,"Exponential"))
					MrBayesPrint ("%s                         Exponential(%1.2lf) on omega(3)\n", spacer, mp->ny98omega3Exp);
				else
					MrBayesPrint ("%s                         Fixed(%1.2lf) on omega(3)\n", spacer, mp->ny98omega3Fixed);
				if (!strcmp(mp->codonCatFreqPr,"Dirichlet"))
					MrBayesPrint ("%s                         Dirichlet(%1.2lf,%1.2lf,%1.2lf) on pi(-), pi(N), and pi(+)\n", spacer, mp->codonCatDir[0], mp->codonCatDir[1], mp->codonCatDir[2]);
				else
					MrBayesPrint ("%s                         Fixed(%1.2lf,%1.2lf,%1.2lf) on pi(-), pi(N), and pi(+)\n", spacer, mp->codonCatFreqFix[0], mp->codonCatFreqFix[1], mp->codonCatFreqFix[2]);
				}
			else if (!strcmp(mp->omegaVar,"M3"))
				{
				if (!strcmp(mp->m3omegapr,"Exponential"))
					MrBayesPrint ("%s                         dN1, dN2, dN3, and dS are all exponential random variables\n", spacer);
				else
					MrBayesPrint ("%s                         Fixed(%1.2lf,%1.2lf,%1.2lf) for the three selection categories\n", spacer, mp->m3omegaFixed[0], mp->m3omegaFixed[1], mp->m3omegaFixed[2]);
				if (!strcmp(mp->codonCatFreqPr,"Dirichlet"))
					MrBayesPrint ("%s                         Dirichlet(%1.2lf,%1.2lf,%1.2lf) on pi(1), pi(2), and pi(3)\n", spacer, mp->codonCatDir[0], mp->codonCatDir[1], mp->codonCatDir[2]);
				else
					MrBayesPrint ("%s                         Fixed(%1.2lf,%1.2lf,%1.2lf) on pi(1), pi(2), and pi(3)\n", spacer, mp->codonCatFreqFix[0], mp->codonCatFreqFix[1], mp->codonCatFreqFix[2]);
				}
			else if (!strcmp(mp->omegaVar,"M10"))
				{
				MrBayesPrint ("%s                         With probability pi(1), omega is drawn from a Beta(alpha1,beta1) \n", spacer);
				MrBayesPrint ("%s                         distribution and with probability pi(2), omega is drawn from\n", spacer);
				MrBayesPrint ("%s                         a Gamma(alpha2,beta2) distribution.\n", spacer);
				if (!strcmp(mp->m10betapr,"Uniform"))
					{
					MrBayesPrint ("%s                         The parameters of the beta distribution each follow \n", spacer);
					MrBayesPrint ("%s                         independent Uniform(%1.2lf,%1.2lf) priors\n", spacer, mp->m10betaUni[0], mp->m10betaUni[1]);
					}
				else if (!strcmp(mp->m10betapr,"Exponential"))
					{
					MrBayesPrint ("%s                         The parameters of the beta distribution each follow \n", spacer);
					MrBayesPrint ("%s                         independent Exponential(%1.2lf) priors\n", spacer, mp->m10betaExp);
					}
				else
					{
					MrBayesPrint ("%s                         The parameters of the beta distribution are fixed to \n", spacer);
					MrBayesPrint ("%s                         %1.2lf and %1.2lf\n", spacer, mp->m10betaFix[0], mp->m10betaFix[0]);
					}

				if (!strcmp(mp->m10gammapr,"Uniform"))
					{
					MrBayesPrint ("%s                         The parameters of the gamma distribution each follow  \n", spacer);
					MrBayesPrint ("%s                         independent Uniform(%1.2lf,%1.2lf) priors\n", spacer, mp->m10gammaUni[0], mp->m10gammaUni[1]);
					}
				else if (!strcmp(mp->m10gammapr,"Exponential"))
					{
					MrBayesPrint ("%s                         The parameters of the gamma distribution each follow \n", spacer);
					MrBayesPrint ("%s                         independent Exponential(%1.2lf) priors\n", spacer, mp->m10gammaExp);
					}
				else
					{
					MrBayesPrint ("%s                         The parameters of the gamma distribution are fixed to \n", spacer);
					MrBayesPrint ("%s                         %1.2lf and %1.2lf\n", spacer, mp->m10gammaFix[0], mp->m10gammaFix[0]);
					}

				if (!strcmp(mp->codonCatFreqPr,"Dirichlet"))
					MrBayesPrint ("%s                         Dirichlet(%1.2lf,%1.2lf) on pi(1) and pi(2)\n", spacer, mp->codonCatDir[0], mp->codonCatDir[1]);
				else
					MrBayesPrint ("%s                         Fixed(%1.2lf,%1.2lf) on pi(1) and pi(2)\n", spacer, mp->codonCatFreqFix[0], mp->codonCatFreqFix[1]);
				}
			}
		else if (j == P_PI)
			{
			if (ms->dataType == STANDARD)
				{
				if (!strcmp(mp->symPiPr, "Uniform"))
					MrBayesPrint ("%s            Prior      = Symmetric dirichlet with uniform(%1.2lf,%1.2lf) variance parameter\n", spacer, mp->symBetaUni[0], mp->symBetaUni[1]);
				else if (!strcmp(mp->symPiPr, "Exponential"))
					MrBayesPrint ("%s            Prior      = Symmetric dirichlet with exponential(%1.2lf) variance parameter\n", spacer, mp->symBetaExp);
				else
					{ /* mp->symBetaFix == -1 */
					  if (AreDoublesEqual(mp->symBetaFix, 1.0, ETA)==YES)
						MrBayesPrint ("%s            Prior      = State frequencies are equal\n", spacer);
					else
						MrBayesPrint ("%s            Prior      = Symmetric dirichlet with fixed(%1.2lf) variance parameter\n", spacer, mp->symBetaFix);
					}
				}
			else if (ms->dataType == PROTEIN)
				{
				if (!strcmp(mp->aaModelPr, "Fixed") && (!strcmp(mp->aaModel, "Equalin") || !strcmp(mp->aaModel, "Gtr")))
					{
					if (!strcmp(mp->stateFreqPr,"Dirichlet"))
						{
						MrBayesPrint ("%s            Prior      = Dirichlet\n", spacer);
						}
					else if (!strcmp(mp->stateFreqPr,"Fixed") && !strcmp(mp->stateFreqsFixType,"Equal"))
						{
						MrBayesPrint ("%s            Prior      = Fixed (equal frequencies)\n", spacer);
						}
					else if (!strcmp(mp->stateFreqPr,"Fixed") && !strcmp(mp->stateFreqsFixType,"User"))
						{
						MrBayesPrint ("%s            Prior      = Fixed (user-specified)\n", spacer);
						}
					else if (!strcmp(mp->stateFreqPr,"Fixed") && !strcmp(mp->stateFreqsFixType,"Empirical"))
						{
						MrBayesPrint ("%s            Prior      = Fixed (empirical frequencies)\n", spacer);
						}
					}
				else if (!strcmp(mp->aaModelPr, "Fixed") && !strcmp(mp->aaModel, "Poisson"))
					{
					MrBayesPrint ("%s            Prior      = Fixed (equal frequencies)\n", spacer);
					}
				else if (!strcmp(mp->aaModelPr, "Fixed") && strcmp(mp->aaModel, "Equalin") && strcmp(mp->aaModel, "Poisson"))
					{
					MrBayesPrint ("%s            Prior      = Fixed (%s frequencies)\n", spacer, mp->aaModel);
					}
				else
					{
					MrBayesPrint ("%s            Prior      = Fixed (from mixture of models)\n", spacer);
					}
				}
			else
				{
				if (!strcmp(mp->stateFreqPr,"Dirichlet"))
					MrBayesPrint ("%s            Prior      = Dirichlet\n", spacer);
				else
					MrBayesPrint ("%s            Prior      = Fixed\n", spacer);
				}
			}
		else if (j == P_SHAPE)
			{
			if (!strcmp(mp->shapePr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->shapeUni[0], mp->shapeUni[1]);
			else if (!strcmp(mp->shapePr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.2lf)\n", spacer, mp->shapeExp);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->shapeFix);
			}
		else if (j == P_PINVAR)
			{
			if (!strcmp(mp->pInvarPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->pInvarUni[0], mp->pInvarUni[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->pInvarFix);
			}
		else if (j == P_CORREL)
			{
			if (!strcmp(mp->adGammaCorPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->corrUni[0], mp->corrUni[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->corrFix);
			}
		else if (j == P_SWITCH)
			{
			if (!strcmp(mp->covSwitchPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->covswitchUni[0], mp->covswitchUni[1]);
			else if (!strcmp(mp->covSwitchPr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.2lf)\n", spacer, mp->covswitchExp);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf,%1.2lf)\n", spacer, mp->covswitchFix[0], mp->covswitchFix[1]);
			}
		else if (j == P_RATEMULT)
			{
            if (p->nValues == 1)
    			MrBayesPrint ("%s            Prior      = Fixed(1.0)\n", spacer);
            else
                {
			    MrBayesPrint ("%s            Prior      = Dirichlet(", spacer);
			    for (d=n=0; d<numCurrentDivisions; d++)
				    {
				    if (activeParams[j][d] == i+1)
					    n++;
				    }
			    for (d=m=0; d<numCurrentDivisions; d++)
				    {
				    if (activeParams[j][d] == i+1)
					    {
					    m++;
					    if (m < n)
						    MrBayesPrint ("%1.2lf,", modelParams[d].ratePrDir);
					    else
						    MrBayesPrint ("%1.2lf)\n", modelParams[d].ratePrDir);
					    }
				    }
                }
			}
		else if (j == P_GENETREERATE)
			{
		    MrBayesPrint ("%s            Prior      = Dirichlet(", spacer);
		    printedCol = strlen(spacer) + 25 + 10;
            for (n=0; n<numTrees-1; n++)
			    {
                if (printedCol + 5 > screenWidth)
                    {
                    MrBayesPrint("\n%s                                   ", spacer);
                    printedCol = strlen(spacer) + 25 + 10;
                    }
                if (n == numTrees-2)
    			    MrBayesPrint ("1.00)\n");
                else
    			    MrBayesPrint ("1.00,");
                printedCol += 5;
                }
			}
		else if (j == P_TOPOLOGY)
			{
			if (!strcmp(mp->topologyPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = All topologies equally probable a priori\n", spacer);
			else if (!strcmp(mp->topologyPr,"Speciestree"))
				MrBayesPrint ("%s            Prior      = Topology constrained to fold within species tree\n", spacer);
			else if (!strcmp(mp->topologyPr,"Constraints"))
				MrBayesPrint ("%s            Prior      = Prior on topologies obeys constraints\n", spacer);
			else
				MrBayesPrint ("%s            Prior      = Prior is fixed to the topology of user tree '%s'\n", spacer,
                                                    userTree[mp->topologyFix]->name);
			}
		else if (j == P_BRLENS)
			{
			if (!strcmp(mp->parsModel, "Yes"))
				MrBayesPrint ("%s            Prior      = Reconstructed using parsimony\n", spacer);
			else
				{
				if (!strcmp(mp->brlensPr,"Unconstrained"))
					{
					MrBayesPrint ("%s            Prior      = Unconstrained:%s", spacer, mp->unconstrainedPr);
					if (!strcmp(mp->unconstrainedPr, "Uniform"))
						MrBayesPrint ("(%1.1lf,%1.1lf)\n", mp->brlensUni[0], mp->brlensUni[1]);
					else
						MrBayesPrint ("(%1.1lf)\n", mp->brlensExp);
					}
				else if (!strcmp(mp->brlensPr, "Clock"))
					{
					MrBayesPrint ("%s            Prior      = Clock:%s\n", spacer, mp->clockPr);
					if (!strcmp(mp->clockPr,"Uniform") && !strcmp(mp->nodeAgePr,"Unconstrained"))
						{
						if (!strcmp(mp->treeAgePr, "Fixed"))
							MrBayesPrint ("%s                         Tree age is fixed to %1.3lf\n", spacer, mp->treeAgeFix);
						else if (!strcmp(mp->treeAgePr, "Gamma"))
							MrBayesPrint ("%s                         Tree age has a Gamma(%1.3lf,%1.3lf) distribution\n", spacer, mp->treeAgeGamma[0], mp->treeAgeGamma[1]);
						else
							MrBayesPrint ("%s                         Tree age has an Exponential(%1.3lf) distribution\n", spacer, mp->treeAgeExp);
						}
                    else if (!strcmp(mp->clockPr, "Birthdeath") && !strcmp(mp->nodeAgePr,"Unconstrained"))
                        {
						MrBayesPrint ("%s                         Tree age has a Uniform(0,infinity) distribution\n", spacer);
                        }
                    if (!strcmp(mp->nodeAgePr,"Calibrated"))
						{
						b = 0;
						for (a=0; a<numTaxa; a++)
							{
							if (taxaInfo[a].isDeleted == NO && tipCalibration[a].prior != unconstrained)
								b++;
							}
						for (a=0; a<numDefinedConstraints; a++)
							{
							if (mp->activeConstraints[a] == YES && nodeCalibration[a].prior != unconstrained)
								b++;
							}
						if (b > 1)
							MrBayesPrint ("%s                         Node depths are constrained by the following age constraints:\n", spacer);
						else
							MrBayesPrint ("%s                         Node depths are calibrated by the following age constraint:\n", spacer);
						for (a=0; a<numTaxa; a++)
							{
							if (taxaInfo[a].isDeleted == NO && tipCalibration[a].prior != unconstrained)
								{
								MrBayesPrint ("%s                         -- The age of terminal \"%s\" is %s\n", spacer, taxaNames[a],
									tipCalibration[a].name);
								}
							}
						for (a=b=0; a<numDefinedConstraints; a++)
							{
							if (mp->activeConstraints[a] == YES && nodeCalibration[a].prior != unconstrained)
								{
								MrBayesPrint ("%s                         -- The age of node '%s' is %s\n", spacer,
                                    constraintNames[a], nodeCalibration[a].name);
								for (k=0; k<numTaxa; k++)
                                    if (taxaInfo[k].isDeleted == NO && IsBitSet(k,definedConstraint[a]) == NO)
                                        break;
                                if (k == numTaxa)
                                    b = 1;          /* root is calibrated */
                                }
							}
                        if (b == 0) /* we need to use default calibration for root for uniform and birthdeath */
                            {
					        if (!strcmp(mp->clockPr,"Uniform"))
						        {
						        if (!strcmp(mp->treeAgePr, "Fixed"))
							        MrBayesPrint ("%s                         -- Tree age is fixed to %1.3lf\n", spacer, mp->treeAgeFix);
						        else if (!strcmp(mp->treeAgePr, "Gamma"))
							        MrBayesPrint ("%s                         -- Tree age has a Gamma(%1.3lf,%1.3lf) distribution\n", spacer, mp->treeAgeGamma[0], mp->treeAgeGamma[1]);
						        else
							        MrBayesPrint ("%s                         -- Tree age has an Exponential(%1.3lf) distribution\n", spacer, mp->treeAgeExp);
						        }
					        else if (!strcmp(mp->clockPr,"Birthdeath"))
						        {
						        MrBayesPrint ("%s                         -- Tree age has a Uniform(0,infinity) distribution\n", spacer);
						        }
                            }
						}
					else
						MrBayesPrint ("%s                         Node ages are not constrained\n", spacer);
					}
				else
					{
					assert(!strcmp(mp->brlensPr, "Fixed"));
					MrBayesPrint ("%s            Prior      = Fixed, branch lengths are fixed to the ones of the user tree '%s'\n", spacer,
                                                    userTree[mp->topologyFix]->name);
					}
				}
			}
		else if (j == P_SPECRATE)
			{
			if (!strcmp(mp->speciationPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->speciationUni[0], mp->speciationUni[1]);
			else if (!strcmp(mp->speciationPr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.2lf)\n", spacer, mp->speciationExp);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->speciationFix);
			}
		else if (j == P_EXTRATE)
			{
			if (!strcmp(mp->extinctionPr,"Beta"))
				MrBayesPrint ("%s            Prior      = Beta(%1.2lf,%1.2lf)\n", spacer, mp->extinctionBeta[0], mp->extinctionBeta[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->extinctionFix);
			}
		else if (j == P_POPSIZE)
			{
			if (!strcmp(mp->popSizePr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->popSizeUni[0], mp->popSizeUni[1]);
			else if (!strcmp(mp->popSizePr,"Lognormal"))
				MrBayesPrint ("%s            Prior      = Lognormal(%1.2lf,%1.2lf)\n", spacer, mp->popSizeLognormal[0], mp->popSizeLognormal[1]);
			else if (!strcmp(mp->popSizePr,"Normal"))
				MrBayesPrint ("%s            Prior      = Truncated Normal(%1.2lf,%1.2lf)\n", spacer, mp->popSizeNormal[0], mp->popSizeNormal[1]);
			else if (!strcmp(mp->popSizePr,"Gamma"))
				MrBayesPrint ("%s            Prior      = Gamma(%1.2lf,%1.2lf)\n", spacer, mp->popSizeGamma[0], mp->popSizeGamma[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->popSizeFix);
            if (!strcmp(mp->topologyPr,"Speciestree"))
                {
                if (!strcmp(mp->popVarPr,"Equal") || !strcmp(mp->popSizePr,"Fixed"))
				    MrBayesPrint ("%s                         Population size the same across species tree\n", spacer);
                else
				    MrBayesPrint ("%s                         Population size varies across branches in species tree\n", spacer);
                }
			}
		else if (j == P_GROWTH)
			{
			if (!strcmp(mp->growthPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->growthUni[0], mp->growthUni[1]);
			else if (!strcmp(mp->growthPr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.2lf)\n", spacer, mp->growthExp);
			else if (!strcmp(mp->growthPr,"Normal"))
				MrBayesPrint ("%s            Prior      = Normal(%1.2lf,%1.2lf)\n", spacer, mp->growthNorm[0], mp->growthNorm[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->growthFix);
			} 
		else if (j == P_AAMODEL)
			{
			if (!strcmp(mp->aaModelPr,"Mixed"))
				{
				/* check to see if you have a uniform prior */
				isSame = YES;
				for (a=0; a<9; a++)
					for (b=a+1; b<10; b++)
							/* mp->aaModelPrProbs[a] != mp->aaModelPrProbs[b] */
					if (AreDoublesEqual(mp->aaModelPrProbs[a], mp->aaModelPrProbs[b], ETA)==FALSE)
							isSame = NO;
				MrBayesPrint ("%s            Prior      = Poisson, Jones, Dayhoff, Mtrev, Mtmam, Wag, Rtrev,\n", spacer);
				if (isSame == YES)
					MrBayesPrint ("%s                         Cprev, Vt, and Blosum models have equal prior probability\n", spacer);
				else
					MrBayesPrint ("%s                         Cprev, Vt, and Blosum models have unequal prior probability\n", spacer);
				}
			else
				MrBayesPrint ("%s            Prior      = Fixed(%s)\n", spacer, mp->aaModel);
			}
		else if (j == P_BRCORR)
			{
			if (!strcmp(mp->brownCorPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->brownCorrUni[0], mp->brownCorrUni[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->brownCorrFix);
			}
		else if (j == P_BRSIGMA)
			{
			if (!strcmp(mp->brownScalesPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->brownScalesUni[0], mp->brownScalesUni[1]);
			else if (!strcmp(mp->brownScalesPr,"Gammamean"))
				MrBayesPrint ("%s            Prior      = Gamma Mean=<char. ave.> Var=%1.2lf\n", spacer, mp->brownScalesGammaMean);
			else if (!strcmp(mp->brownScalesPr,"Gamma"))
				MrBayesPrint ("%s            Prior      = Gamma Mean=%lf Var=%1.2lf\n", spacer, mp->brownScalesGamma[0], mp->brownScalesGamma[1]);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->brownScalesFix);
			}
		else if (j == P_CPPRATE)
			{
			if (!strcmp(mp->cppRatePr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.2lf)\n", spacer, mp->cppRateExp);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->cppRateFix);
			}
		else if (j == P_CPPMULTDEV)
			{
			if (!strcmp(mp->cppMultDevPr,"Fixed"))
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->cppMultDevFix);
			}
		else if (j == P_CPPEVENTS)
			{
			MrBayesPrint ("%s            Prior      = Poisson (%s) [Events]\n", spacer, modelSettings[p->relParts[0]].cppRate->name);
			MrBayesPrint ("%s                         Lognormal (0.00,%1.2lf) [Rate multipliers]\n", spacer, mp->cppMultDevFix);
			}
		else if (j == P_TK02VAR)
			{
			if (!strcmp(mp->tk02varPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->tk02varUni[0], mp->tk02varUni[1]);
			else if (!strcmp(mp->tk02varPr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.2lf)\n", spacer, mp->tk02varExp);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->tk02varFix);
			}
		else if (j == P_TK02BRANCHRATES)
			{
			MrBayesPrint ("%s            Prior      = LogNormal (expectation = r_0, variance = %s * v) \n", spacer, modelSettings[p->relParts[0]].tk02var->name);
			MrBayesPrint ("%s                            [r_0 is beginning rate of branch, v is branch length]\n", spacer);
			}
		else if (j == P_IGRVAR)
			{
			if (!strcmp(mp->igrvarPr,"Uniform"))
				MrBayesPrint ("%s            Prior      = Uniform(%1.2lf,%1.2lf)\n", spacer, mp->igrvarUni[0], mp->igrvarUni[1]);
			else if (!strcmp(mp->igrvarPr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.2lf)\n", spacer, mp->igrvarExp);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.2lf)\n", spacer, mp->igrvarFix);
			}
		else if (j == P_IGRBRANCHLENS)
			{
			MrBayesPrint ("%s            Prior      = Scaledgamma (variance = %s * v) \n", spacer, modelSettings[p->relParts[0]].igrvar->name);
			MrBayesPrint ("%s                            [where v is branch length]\n", spacer);
			}
		else if (j == P_CLOCKRATE)
			{
			if (!strcmp(mp->clockRatePr,"Normal"))
				MrBayesPrint ("%s            Prior      = Normal(%1.6lf,%1.6lf)\n", spacer, mp->clockRateNormal[0], mp->clockRateNormal[1]);
			else if (!strcmp(mp->clockRatePr,"Lognormal"))
				MrBayesPrint ("%s            Prior      = Lognormal(%1.6lf,%1.6lf)\n", spacer, mp->clockRateLognormal[0], mp->clockRateLognormal[1]);
			else if (!strcmp(mp->clockRatePr,"Gamma"))
				MrBayesPrint ("%s            Prior      = Gamma(%1.6lf,%1.6lf)\n", spacer, mp->clockRateGamma[0], mp->clockRateGamma[1]);
			else if (!strcmp(mp->clockRatePr,"Exponential"))
				MrBayesPrint ("%s            Prior      = Exponential(%1.6lf)\n", spacer, mp->clockRateExp);
			else
				MrBayesPrint ("%s            Prior      = Fixed(%1.6lf)\n", spacer, mp->clockRateFix);
			if (!strcmp(mp->clockVarPr,"Strict"))
				MrBayesPrint ("%s                         The clock rate is constant (strict clock)\n", spacer);
			else if (!strcmp(mp->clockVarPr,"Cpp"))
				MrBayesPrint ("%s                         The clock rate varies according to a CPP model\n", spacer);
			else if (!strcmp(mp->clockVarPr,"TK02"))
				MrBayesPrint ("%s                         The clock rate varies according to a Brownian motion model\n", spacer);
			else /* if (!strcmp(mp->clockVarPr,"Igr")) */
				MrBayesPrint ("%s                         The clock rate varies according to an independent scaled gamma (white noise) model\n", spacer);
			}
		else if (j == P_SPECIESTREE)
			{
			MrBayesPrint ("%s            Prior      = Uniform on topologies and branch lengths\n", spacer);
			}
				
		/* print partitions */
		if (numCurrentDivisions > 1)
			{
			if (p->nRelParts == 1)
				MrBayesPrint ("%s            Partition  = %d\n", spacer, p->relParts[0]+1);
			else if (p->nRelParts == 2)
				{
				MrBayesPrint ("%s            Partitions = %d and %d\n", spacer, p->relParts[0]+1, p->relParts[1]+1);					
				}
			else if (p->nRelParts == numCurrentDivisions)
				{
				MrBayesPrint ("%s            Partitions = All\n", spacer);					
				}			
			else /* if (p->nRelParts > 2) */
				{
				MrBayesPrint ("%s            Partitions = ", spacer);
				for (j=0; j<p->nRelParts; j++)
					{
					if (j == p->nRelParts - 2)
						MrBayesPrint ("%d, and ", p->relParts[j]+1);
					else if (j == p->nRelParts - 1)
						MrBayesPrint ("%d\n", p->relParts[j]+1);
					else
						MrBayesPrint ("%d, ", p->relParts[j]+1);
					}				
				}
			}

		/* show subparams */
		if (p->nSubParams > 0)
			{
			if (p->nSubParams == 1)
				MrBayesPrint ("%s            Subparam.  = %s\n", spacer, p->subParams[0]->name);
			else
				{
				printedCol = 0;
                for (k=0; k<p->nSubParams; k++)
					{
                    if (k == 0)
                        {
						MrBayesPrint ("%s            Subparams  = %s", spacer, p->subParams[k]->name);
                        printedCol = strlen(spacer) + 25 + strlen(p->subParams[k]->name);
                        }
                    else if (k == p->nSubParams - 1)
                        {
                        if (printedCol + 5 > screenWidth)
                            MrBayesPrint ("\n%s                         and ", spacer);
                        else if (printedCol + (int)(strlen(p->subParams[k]->name)) + 5 > screenWidth)
                            MrBayesPrint (" and \n%s                         ", spacer);
                        else
                            MrBayesPrint (" and ");
						MrBayesPrint ("%s\n", p->subParams[k]->name);
                        }
                    else
                        {
                        if (printedCol + (int)(strlen(p->subParams[k]->name)) + 2 > screenWidth)
                            {
                            MrBayesPrint (", \n%s                         ", spacer);
                            printedCol = strlen(spacer) + 25;
                            }
                        else
                            {
                            MrBayesPrint(", ");
                            printedCol += 2;
                            }
						MrBayesPrint ("%s", p->subParams[k]->name);
                        printedCol += strlen(p->subParams[k]->name);
                        }
					}
				}
			}

		/* show used moves */
		if (showMoves == YES && (p->printParam == YES || p->paramType == P_TOPOLOGY || p->paramType == P_BRLENS))
			{
			/* check if runs are same */
			areRunsSame = YES;
			for (run=1; run<chainParams.numRuns; run++)
				{
				for (chain=0; chain<chainParams.numChains; chain++)
					{
					for (k=0; k<numApplicableMoves; k++)
						{
						mv = moves[k];
						if (mv->parm != p)
							continue;
						chainIndex = run*chainParams.numChains + chain;
						refIndex = chain;
						if (AreDoublesEqual (mv->relProposalProb[chainIndex], mv->relProposalProb[refIndex], 0.000001) == NO)
							areRunsSame = NO;
						}
					}
				}

			for (run=0; run<chainParams.numRuns; run++)
				{
				if (run > 0 && areRunsSame == YES)
					continue;
				
				/* check if chains are same */
				areChainsSame = YES;
				for (chain=1; chain<chainParams.numChains; chain++)
					{
					for (k=0; k<numApplicableMoves; k++)
						{
						mv = moves[k];
						if (mv->parm != p)
							continue;
						chainIndex = run*chainParams.numChains + chain;
						refIndex = run*chainParams.numChains;
						if (AreDoublesEqual (mv->relProposalProb[chainIndex], mv->relProposalProb[refIndex], 0.000001) == NO)
							areChainsSame = NO;
						}
					}

				/* now print moves */
                for (chain=0; chain<chainParams.numChains; chain++)
					{
					if (chain > 0 && areChainsSame == YES)
						continue;
					if (run==0 && chain==0)
						MrBayesPrint ("%s            Moves      = ", spacer);
					else
						MrBayesPrint ("%s                         ", spacer);
					numPrinted = 0;
    				printedCol = strlen(spacer) + 25;
					for (k=0; k<numApplicableMoves; k++)
						{
						mv = moves[k];
						if (mv->parm != p)
							continue;
						chainIndex = run*chainParams.numChains + chain;
						if (mv->relProposalProb[chainIndex] <= 0.000001)
							continue;
						if (numPrinted == 0)
                            {
							MrBayesPrint ("%s <prob %.1f>", mv->name, mv->relProposalProb[chainIndex]);
                            printedCol += 9 + strlen(mv->name) + (int)(log10(mv->relProposalProb[chainIndex])) + 3;
                            }
						else
                            {
                            if (printedCol + 11 + (int)(strlen(mv->name)) + (int)(log10(mv->relProposalProb[chainIndex])) + 3 > screenWidth)
                                {
                                MrBayesPrint(", \n%s                         ", spacer);
                                printedCol = 25 + strlen(spacer);
                                }
                            else
                                {
                                MrBayesPrint(", ");
                                printedCol += 2;
                                }
							MrBayesPrint ("%s <prob %.1f>", mv->name, mv->relProposalProb[chainIndex]);
                            printedCol += (9 + (int)(strlen(mv->name)) + (int)(log10(mv->relProposalProb[chainIndex])) + 3);
                            }
						numPrinted++;
						}

					if (numPrinted == 0 && p->paramType == P_BRLENS)
						{
						for (k=0; k<numParams; k++)
							if (params[k].tree == p->tree)
								break;
						MrBayesPrint ("For moves, see param. '%s'", params[k].name);
						}
					else if (numPrinted == 0)
						MrBayesPrint ("WARNING! No moves -- param. fixed to startvals");

					if (areRunsSame == YES && areChainsSame == YES)
						MrBayesPrint ("\n");
					else if (areRunsSame == YES && areChainsSame == NO)
						MrBayesPrint (" [chain %d]\n", chain+1);
					else if (areRunsSame == NO && areChainsSame == YES)
						MrBayesPrint (" [run %d]\n", run+1);
					else /* if (areRunsSame == NO && areChainsSame == NO) */
							MrBayesPrint (" [run %d, chain %d]\n", run+1, chain+1);
					}
				}
			}
			
		/* show available moves */
		if (showAllAvailable == YES && (p->printParam == YES || p->paramType == P_TOPOLOGY || p->paramType == P_BRLENS))
			{
			/* loop over moves */
			numPrinted = 0;
            printedCol = 0;
			for (k=0; k<numApplicableMoves; k++)
				{
				mv = moves[k];
				if (mv->parm != p)
					continue;
				numMovedChains = 0;
				for (run=0; run<chainParams.numRuns; run++)
					{
					for (chain=0; chain<chainParams.numChains; chain++)
						{
						chainIndex = run*chainParams.numChains + chain;
						if (mv->relProposalProb[chainIndex] > 0.000001)
							numMovedChains++;
						}
					}
				if (numMovedChains == 0)
					{
					if (numPrinted == 0)
                        {
						MrBayesPrint ("%s            Not used   = ", spacer);
                        printedCol = strlen(spacer) + 25;
                        }
					else if (printedCol + 2 + (int)(strlen(mv->moveType->shortName)) > screenWidth)
                        {
					    MrBayesPrint (", \n%s                         ", spacer);
                        printedCol = strlen(spacer) + 25;
                        }
                    else
                        {
                        MrBayesPrint (", ");
                        printedCol += 2;
                        }
                    MrBayesPrint("%s", mv->moveType->shortName);
                    printedCol += strlen(mv->moveType->shortName);
					numPrinted++;
					}
				}
				if (numPrinted > 0)
					MrBayesPrint ("\n");
			}

		/* show startvals */
        if (showStartVals == YES && (p->printParam == YES || p->paramType == P_TOPOLOGY || p->paramType == P_BRLENS || p->paramType == P_SPECIESTREE || p->paramType == P_POPSIZE))
			{					
			if (p->paramType == P_TOPOLOGY || p->paramType == P_BRLENS || p->paramType == P_SPECIESTREE)
				{
				/* check if they are the same */
				areRunsSame = YES;
				if (p->paramType == P_TOPOLOGY)
					{
					for (run=1; run<chainParams.numRuns; run++)
						{
						for (chain=0; chain<chainParams.numChains; chain++)
							{
							if (AreTopologiesSame (GetTree (p, run*chainParams.numChains + chain, 0), GetTree (p, chain, 0)) == NO)
								{
								areRunsSame = NO;
								break;
								}
							}
						}
					}
				else if (p->paramType == P_BRLENS)
					{
					for (run=1; run<chainParams.numRuns; run++)
						{
						for (chain=0; chain<chainParams.numChains; chain++)
							{
							if (AreTreesSame (GetTree (p, run*chainParams.numChains + chain, 0), GetTree (p, chain, 0)) == NO)
								{
								areRunsSame = NO;
								break;
								}
							}
						}
					}
				else if (p->paramType == P_SPECIESTREE)
					{
					for (run=1; run<chainParams.numRuns; run++)
						{
						for (chain=0; chain<chainParams.numChains; chain++)
							{
							if (AreTreesSame (GetTree (p, run*chainParams.numChains + chain, 0), GetTree (p, chain, 0)) == NO)
								{
								areRunsSame = NO;
								break;
								}
							}
						}
					}
				
				/* print trees */
				for (run=0; run<chainParams.numRuns; run++)
					{
					if (run > 0 && areRunsSame == YES)
						break;
					for (chain=0; chain<chainParams.numChains; chain++)
						{
						if (run == 0 && chain == 0)
							MrBayesPrint ("%s            Startvals  = tree '%s'", spacer, GetTree (p, run*chainParams.numChains+chain, 0)->name);
						else
							MrBayesPrint ("%s                         tree '%s'", spacer, GetTree (p, run*chainParams.numChains+chain, 0)->name);
						if (chainParams.numChains > 1 && areRunsSame == YES)
							MrBayesPrint ("  [chain %d]", chain+1);
						else if (chainParams.numChains == 1 && areRunsSame == NO)
							MrBayesPrint ("  [run %d]", run+1);
						else if (areRunsSame == NO)
							MrBayesPrint ("  [run %d, chain %d]", run+1, chain+1);
						MrBayesPrint ("\n");
						}
					}				
				}	/* end topology and brlens parameters */

			else if (p->paramType == P_OMEGA && p->paramId != OMEGA_DIR && p->paramId != OMEGA_FIX &&
					 p->paramId != OMEGA_FFF && p->paramId != OMEGA_FF && p->paramId != OMEGA_10FFF)
				{
				/* we need to print values and subvalues for the category frequencies in a NY98-like model. */
				areRunsSame = YES;
				for (run=1; run<chainParams.numRuns; run++)
					{
					for (chain=0; chain<chainParams.numChains; chain++)
						{
						value = GetParamVals (p, run*chainParams.numRuns+chain, 0);
						refValue = GetParamVals (p, chain, 0);
						nValues = p->nValues;
						for (k=0; k<nValues; k++)
							{
							if (AreDoublesEqual (value[k], refValue[k], 0.000001) == NO)
								{
								areRunsSame = NO;
								break;
								}
							}
						value = GetParamSubVals (p, run*chainParams.numRuns+chain, 0);
						refValue = GetParamSubVals (p, chain, 0);
						if (!strcmp(modelParams[p->relParts[0]].omegaVar, "M10"))
							{
							value += (mp->numM10BetaCats + mp->numM10GammaCats);
							refValue += (mp->numM10BetaCats + mp->numM10GammaCats);
							for (k=0; k<4; k++)
								{
								if (AreDoublesEqual (value[k + 4], refValue[k + 4], 0.000001) == NO)
									{
									areRunsSame = NO;
									break;
									}
								}
							for (k=0; k<2; k++)
								{
								if (AreDoublesEqual (value[k], refValue[k], 0.000001) == NO)
									{
									areRunsSame = NO;
									break;
									}
								}							
							}
						else
							{
                            for (k=0; k<3; k++)
								{
                                if (AreDoublesEqual (value[k], refValue[k], 0.000001) == NO)
									{
									areRunsSame = NO;
									break;
									}
								}
							}
						if (areRunsSame == NO)
							break;
						}
					if (areRunsSame == NO)
						break;
					}
				
				/* now print values */
				for (run=0; run<chainParams.numRuns; run++)
					{
					for (chain=0; chain<chainParams.numChains; chain++)
						{
						value = GetParamVals (p, run*chainParams.numRuns+chain, 0);
						subValue = GetParamSubVals (p, run*chainParams.numRuns+chain, 0);
						nValues = p->nValues;						
						if (run == 0 && chain == 0)
							MrBayesPrint ("%s            Startvals  = (%1.3lf", spacer, value[0]);
						else
							MrBayesPrint ("%s                         (%1.3lf", spacer, value[0]);
						for (k=1; k<nValues; k++)
							{
							MrBayesPrint (",%1.3lf", value[k]);
							}

						if (!strcmp(modelParams[p->relParts[0]].omegaVar, "M10"))
							{
							for (k=0; k<4; k++)
								{
								MrBayesPrint (",%1.3lf", subValue[k + mp->numM10BetaCats+mp->numM10GammaCats+4]);
								}
							for (k=0; k<2; k++)
								{
								MrBayesPrint (",%1.3lf", subValue[k + mp->numM10BetaCats+mp->numM10GammaCats]);
								}					
							}
						else
							{
							for (k=0; k<3; k++)
								{
								MrBayesPrint (",%1.3lf", subValue[k]);
								}
							}		
						MrBayesPrint (")");
						if (chainParams.numChains > 1 && areRunsSame == YES)
							MrBayesPrint ("  [chain %d]\n", chain+1);
						else if (chainParams.numChains == 1 && areRunsSame == NO)
							MrBayesPrint ("  [run %d]\n", run+1);
						else if (areRunsSame == NO)
							MrBayesPrint ("  [run %d, chain %d]\n", run+1, chain+1);
						}
					}
				}
			else
				{
                /* run of the mill parameter */
                if( p->paramType == P_CLOCKRATE )
                    {
                     for (j=0; j<numGlobalChains; j++)
                        {
                        if( UpdateClockRate(-1.0, j) == ERROR)
                            {
                            MrBayesPrint ("%s            Warning: There is no appropriate clock rate that would satisfy all calibrated trees for run:%d chain%d. Some of calibration, trees or clockprior needs to be changed. ", spacer, j/chainParams.numChains, j%chainParams.numChains );
                            }
                        }
                    }
				areRunsSame = YES;
				for (run=1; run<chainParams.numRuns; run++)
					{
					for (chain=0; chain<chainParams.numChains; chain++)
						{
						if ((p->paramType == P_PI && modelParams[p->relParts[0]].dataType != STANDARD))
							{
							nValues = p->nSubValues;
							value = GetParamSubVals (p, run*chainParams.numChains+chain, 0);
							refValue = GetParamSubVals (p, chain, 0);
							}
						else
							{
							nValues = p->nValues;
							value = GetParamVals (p, run*chainParams.numChains+chain, 0);
							refValue = GetParamVals (p, chain, 0);
							}
						for (k=0; k<nValues; k++)
							{
							if (AreDoublesEqual(value[k], refValue[k], 0.000001) == NO)
								{
								areRunsSame = NO;
								break;
								}							
							}
						if (areRunsSame == NO)
							break;							
						}
					if (areRunsSame == NO)
						break;
					}

				/* print values */
				for (run=0; run<chainParams.numRuns; run++)
					{
					if (run > 0 && areRunsSame == YES)
						break;
					areChainsSame = YES;
					for (chain=1; chain<chainParams.numChains; chain++)
						{
						if ((p->paramType == P_PI && modelParams[p->relParts[0]].dataType != STANDARD))
							{
							nValues = p->nSubValues;
							value = GetParamSubVals (p, run*chainParams.numChains+chain, 0);
							refValue = GetParamSubVals (p, run*chainParams.numChains, 0);
							}
						else
							{
							nValues = p->nValues;
							value = GetParamVals (p, run*chainParams.numChains+chain, 0);
							refValue = GetParamVals (p, run*chainParams.numChains, 0);
							}
						for (k=0; k<nValues; k++)
							{
							if (AreDoublesEqual(value[k], refValue[k], 0.000001) == NO)
								{
								areChainsSame = NO;
								break;
								}							
							}
						}
					for (chain=0; chain<chainParams.numChains; chain++)
						{
						if (areChainsSame == YES && chain > 0)
							continue;

						if ((p->paramType == P_PI && modelParams[p->relParts[0]].dataType != STANDARD))
							{
							nValues = p->nSubValues;
							value = GetParamSubVals (p, run*chainParams.numChains+chain, 0);
							}
						else
							{
							nValues = p->nValues;
							value = GetParamVals (p, run*chainParams.numChains+chain, 0);
							}

						if (run == 0 && chain == 0)
							MrBayesPrint ("%s            Startvals  = (%1.3lf", spacer, value[0]);
						else
							MrBayesPrint ("%s                         (%1.3lf", spacer, value[0]);
						
						for (k=1; k<nValues; k++)
                            {
                            if (k%10==0)
    							MrBayesPrint (",\n%s                          %1.3lf", spacer, value[k]);
							else
                                MrBayesPrint (",%1.3lf", value[k]);
                            }
						MrBayesPrint (")");
						
						if (areChainsSame == YES && areRunsSame == NO)
							MrBayesPrint ("  [run %d]", run+1);
						else if (areChainsSame == NO && areRunsSame == YES)
							MrBayesPrint ("  [chain %d]", chain+1);
						else if (areChainsSame == NO && areRunsSame == NO)
							MrBayesPrint ("  [run %d, chain %d]", run+1, chain+1);
						MrBayesPrint ("\n");
						}	
					}
				}
			}	/* end print start values */

		MrBayesPrint ("\n");
		}	/* next parameter */

	return (NO_ERROR);

}





int Unlink (void)

{
	
	return (NO_ERROR);

}





/* UpdateTK02EvolLengths: update branch lengths for tk02 model */
int UpdateTK02EvolLengths(Param *param, Tree *t, int chain)
{
    int         i;
    MrBFlt      *tk02Rate, *brlens;
    TreeNode    *p;
    
    tk02Rate = GetParamVals (param, chain, state[chain]);
	brlens = GetParamSubVals (param, chain, state[chain]);
	for (i=0; i<t->nNodes-2; i++)
		{
		p = t->allDownPass[i];
		brlens[p->index] = p->length * (tk02Rate[p->index] + tk02Rate[p->anc->index]) / 2.0;
		}

    return (NO_ERROR);
}





/*-------------------------------------------------
|
|	UpdateCppEvolLengths: Recalculate effective 
|      evolutionary lengths and set update flags
|      for Cpp relaxed clock model
|
--------------------------------------------------*/
int UpdateCppEvolLengths (Param *param, TreeNode *p, int chain)
{
	int			i, *nEvents;
	TreeNode	*q;
	MrBFlt		baseRate = 1.0, **pos, **rateMult, *evolLength;

	i = 2*chain + state[chain];
	nEvents = param->nEvents[i];
	pos = param->position[i];
	rateMult = param->rateMult[i];
	evolLength = GetParamSubVals (param, chain, state[chain]);
	
	q = p->anc;
	while (q->anc != NULL)
		{
		for (i=0; i<nEvents[q->index]; i++)
			baseRate *= rateMult[q->index][i];
		q = q->anc;
		}

	if (UpdateCppEvolLength (nEvents, pos, rateMult, evolLength, p, baseRate)==ERROR)
        return (ERROR);

    return(NO_ERROR);
}




/*-------------------------------------------------
|
|	UpdateClockRate:    Update clockRate of the given chain. Above all it will enforce fixed clockrate prior if it is set. Eroor will be returned if fixed clockrate prior may not be respected.  
|   @param clockRate    is the new clockRate to setup. Clock rate value could be set as positive, 0.0 or negative value. 
|                       The function does the fallowing depending on one of this three values:
|                        positive    - check that this 'positive' value is suitable rate. At the end re-enforce(update) the 'positive' value as clock rate on all trees. 
|                        0.0         - check if current rate is suitable, if not update it with minimal suitable value. At the end re-enforce(update) the resulting clock rate on all trees. 
|                        negative    - check if current rate is suitable, if not update it with minimal suitable value. At the end re-enforce(update) the resulting clock rate ONLY if clock rate was changed 
|   @return             ERROR if clockRate can not be set up, NO_ERROR otherwise. 
|
--------------------------------------------------*/
int UpdateClockRate(MrBFlt clockRate, int chain)
{

    int i, updateTrees;
    MrBFlt      *clockRatep;
    Tree        *t, *t_calibrated=NULL;
    MrBFlt      mintmp,maxtmp,minClockRate,maxClockRate;   

    clockRatep=NULL;
    minClockRate = 0.0;
    maxClockRate = MRBFLT_MAX;

    for (i=0; i<numTrees; i++)
        {
        t = GetTreeFromIndex(i, chain, 0);
        if (t->isCalibrated == NO)
            continue;

        if( clockRatep == NULL )
            {
            clockRatep = GetParamVals(modelSettings[t->relParts[0]].clockRate, chain, 0);
            t_calibrated = t;
            assert(clockRatep);
            }

        findAllowedClockrate (t, &mintmp, &maxtmp );

        if( minClockRate < mintmp )
            minClockRate = mintmp;

        if( maxClockRate > maxtmp )
            maxClockRate = maxtmp;

        }
        /* clock rate is the same for all trees of a given chain*/
    if( clockRatep != NULL)
        {
        if( minClockRate > maxClockRate)
            {
            MrBayesPrint ("%s   ERROR: Calibrated trees require uncomatable clockrates for run:%d chain:%d.\n", spacer, chain/chainParams.numChains, chain%chainParams.numChains);
            *clockRatep=0;
            return (ERROR);
            }
        

        if (!strcmp(modelParams[t_calibrated->relParts[0]].clockRatePr, "Fixed"))
            {
            if( clockRate < 0.0 && AreDoublesEqual (*clockRatep, modelParams[t_calibrated->relParts[0]].clockRateFix, 0.0001) == YES )
                {
                updateTrees = NO;
                }
            else
                {
                updateTrees = YES;
                }
            *clockRatep = modelParams[t_calibrated->relParts[0]].clockRateFix;
            if((*clockRatep < minClockRate && AreDoublesEqual (*clockRatep, minClockRate, 0.0001) == NO) || (*clockRatep > maxClockRate && AreDoublesEqual (*clockRatep, maxClockRate, 0.0001) == NO) )
                {
                MrBayesPrint ("%s   ERROR: Calibrated trees require clockrate in range from %f to %f, while clockrate prior is fixed to:%f for run:%d chain:%d.\n", spacer, minClockRate, minClockRate, *clockRatep, chain/chainParams.numChains, chain%chainParams.numChains);
                *clockRatep=0;
                return (ERROR);
                }
            if( clockRate > 0.0 )
                {
                if ( AreDoublesEqual (*clockRatep, clockRate, 0.0001) == NO )
                    {
                    MrBayesPrint ("%s   ERROR: Requested clockrate:%f does not match fixed clockrate prior :%f.\n", spacer, clockRate, *clockRatep);
                    *clockRatep=0;
                    return (ERROR);
                    }
                }
            }
        else
            {/*clock prior is not fixed*/
            updateTrees = YES;
            if( clockRate > 0.0 )
                {
                *clockRatep = clockRate;
                if((*clockRatep < minClockRate && AreDoublesEqual (*clockRatep, minClockRate, 0.0001) == NO) || (*clockRatep > maxClockRate && AreDoublesEqual (*clockRatep, maxClockRate, 0.0001) == NO) )
                    {
                    MrBayesPrint ("%s   ERROR: Calibrated trees require clockrate in range from %f to %f, while requested clockrate is:%f for run:%d chain:%d.\n", spacer, minClockRate, minClockRate, clockRate, chain/chainParams.numChains, chain%chainParams.numChains);
                    *clockRatep=0;
                    return (ERROR);
                    }
                }
            else if ( clockRate == 0.0 ) 
                {
                if((*clockRatep < minClockRate && AreDoublesEqual (*clockRatep, minClockRate, 0.0001) == NO) || (*clockRatep > maxClockRate && AreDoublesEqual (*clockRatep, maxClockRate, 0.0001) == NO) )
                    {
                    *clockRatep = minClockRate;
                    }
                }
            else// if ( clockRate < 0.0 ) 
                {
                if((*clockRatep < minClockRate && AreDoublesEqual (*clockRatep, minClockRate, 0.0001) == NO) || (*clockRatep > maxClockRate && AreDoublesEqual (*clockRatep, maxClockRate, 0.0001) == NO) )
                    {
                    *clockRatep = minClockRate;
                    }
                else
                    {
                    updateTrees = NO;
                    }
                }
            }

        
        if(updateTrees == YES)
            {
            for (i=0; i<numTrees; i++)
                {
                t = GetTreeFromIndex(i, chain, 0);
                if (t->isCalibrated == NO)
                    continue;
                UpdateTreeWithClockrate (t,*clockRatep);
                }
            }
        }

return (NO_ERROR);
}





/*----------------------------------------------
|
|	UpdateCppEvolLength: Recursive function to
|      update evolLength of one node for Cpp
|      relaxed clock model
|
-----------------------------------------------*/
int UpdateCppEvolLength (int *nEvents, MrBFlt **pos, MrBFlt **rateMult, MrBFlt *evolLength, TreeNode *p, MrBFlt baseRate)

{
	int		i;
	MrBFlt	endRate;

	if (p != NULL)
		{
#ifndef NDEBUG
        if (baseRate < POS_MIN || baseRate > POS_INFINITY)
            {
            printf("baseRate out of bounds (%.15e for node %d\n", baseRate, p->index);
            return (ERROR);
            }
#endif
        p->upDateTi = YES;
		p->upDateCl = YES;
		if (nEvents[p->index] == 0)
			{
			evolLength[p->index] = p->length * baseRate;
			}
		else
			{
			/* note that event positions are from the top of the branch (more recent)
			   to the bottom of the branch (older) */
			/* The algorithm below successively multiplies in the more basal rate multipliers,
			   starting from the top of the branch. The length of the branch is first assumed
			   to be 1.0; at the end we multiply the result with the true length of the branch. */
			evolLength[p->index] = pos[p->index][0] * rateMult[p->index][0];
			for (i=1; i<nEvents[p->index]; i++)
				{
				evolLength[p->index] += (pos[p->index][i] - pos[p->index][i-1]);
				evolLength[p->index] *= rateMult[p->index][i];
				}
			evolLength[p->index] += (1.0 - pos[p->index][nEvents[p->index]-1]);
			evolLength[p->index] *= baseRate;
			evolLength[p->index] *= p->length;
			}

		/* calculate end rate; we can do this in any order */
		endRate = baseRate;
		for (i=0; i<nEvents[p->index]; i++)
			endRate *= rateMult[p->index][i];

#ifndef NDEBUG
        if (endRate < POS_MIN || endRate > POS_INFINITY)
            {
			printf ("endRate out of bounds (%.15e for node %d)\n", endRate, p->index);
            return (ERROR);
            }
		if (p->anc != NULL && p->anc->anc != NULL && (evolLength[p->index] < POS_MIN || evolLength[p->index] > POS_INFINITY))
            {
			printf ("Effective branch length out of bounds (%.15e for node %d)\n", evolLength[p->index], p->index);
            return (ERROR);
            }
#endif
        /* call left and right descendants */
		if (UpdateCppEvolLength (nEvents, pos, rateMult, evolLength, p->left, endRate)==ERROR)
            return (ERROR);
		if (UpdateCppEvolLength (nEvents, pos, rateMult, evolLength, p->right, endRate)==ERROR)
            return (ERROR);
		}

    return (NO_ERROR);
}





/* UpdateIgrBranchRates: update branch rates for igr model */
int UpdateIgrBranchRates(Param *param, Tree *t, int chain)
{
    int         i;
    MrBFlt      *igrRate, *brlens;
    TreeNode    *p;
    
    igrRate = GetParamVals (param, chain, state[chain]);
	brlens = GetParamSubVals (param, chain, state[chain]);
	for (i=0; i<t->nNodes-2; i++)
		{
		p = t->allDownPass[i];
		igrRate[p->index] = brlens[p->index] / p->length;
		}

    return (NO_ERROR);
}

