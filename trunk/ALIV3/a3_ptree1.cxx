// -----------------------------------------------------------------------------
//	Include-Dateien
// -----------------------------------------------------------------------------

#include <iostream>
#include <cstdlib>

#include "a3_basen.h"
#include "a3_ptree.hxx"

using std::cout;
using std::hex;
using std::dec;

// -----------------------------------------------------------------------------
static void CountAppearances ( str  seq,
								   int *pos,
								   int  num,
								   int  off,
								   int *anz )
// -----------------------------------------------------------------------------
{
	if (!num)
	{
		int base;
		
		while ((base = *seq++) != 0)
		{
			int index = BIndex[base];

			if ((index >= 0) &&
				(index < BASENPUR))	anz[index]++;
		}
	}
	else
	{
		while (num--)
		{
			int base = seq[pos[num] + off];
			
			if (base == 0) anz[BASEN] = 1;
			else		   anz[BIndex[base]]++;
		}
	}
}

// -----------------------------------------------------------------------------
	static void FindAppearances ( int *list,
								  int  base,
								  int  anz,
								  str  seq,
								  int *pos,
								  int  num,
								  int  off )
// -----------------------------------------------------------------------------
{
	int next  = 0,
	    count = 0;

	if (!num)
	{
		num = strlen(seq);

		while ((count < num) && (next < anz))
		{
			if (BIndex[seq[count]] == base) list[next++] = count;
				
			count++;
		}
	}
	else
		while ((count < num) && (next < anz))
		{
			if (base == BASEN)
			{
				if (!seq[pos[count]+ off]) list[next++] = pos[count];
			}
			else
			{
				if (BIndex[seq[pos[count]+ off]] == base) list[next++] = pos[count];
			}

			count++;
		}
}

// -----------------------------------------------------------------------------
//	Konstruktor zum Erstellen eines Positionsbaumes aus einer Sequenz
// -----------------------------------------------------------------------------
	PtNode::PtNode ( str  seq,
					 int *pos,
					 int  num,
					 int  rec )
// -----------------------------------------------------------------------------
{
	if (!seq) mask = (Mask)0, position = NULL, next = NULL;
	else
	{
		int anz[BASEN + 1],
			base;

		mask 	 = (Mask)0;
		position = NULL;
		next	 = NULL;
		
		memset(anz,0,(BASEN + 1) * sizeof(int));
		
		CountAppearances(seq,pos,num,rec,anz);

		if (anz[ADENIN] > 0)
			if (anz[ADENIN] > 1)	mask = (Mask)(mask | N_ADENIN);
			else					mask = (Mask)(mask | P_ADENIN);

		if (anz[CYTOSIN] > 0)
			if (anz[CYTOSIN] > 1)	mask = (Mask)(mask | N_CYTOSIN);
			else					mask = (Mask)(mask | P_CYTOSIN);

		if (anz[GUANIN] > 0)
			if (anz[GUANIN] > 1)	mask = (Mask)(mask | N_GUANIN);
			else					mask = (Mask)(mask | P_GUANIN);

		if (anz[URACIL] > 0)
			if (anz[URACIL] > 1)	mask = (Mask)(mask | N_URACIL);
			else					mask = (Mask)(mask | P_URACIL);

		if (anz[ONE] > 0)
			if (anz[ONE] > 1)		mask = (Mask)(mask | N_ONE);
			else					mask = (Mask)(mask | P_ONE);

		if (anz[ANY] > 0)
			if (anz[ANY] > 1)		mask = (Mask)(mask | N_ANY);
			else					mask = (Mask)(mask | P_ANY);

		if (anz[BASEN])				mask = (Mask)(mask | P_FINISH);
		
		{
			int panz = NumOfPos(),
			    nanz = NumOfNext();

			if (panz) position = new int 	 [panz];
			if (nanz) next	   = new PtNode* [nanz];
		}

		for (base = 0;base < (BASEN + 1);base++)
		{
			switch (anz[base])
			{
				case 0:		break;
				case 1:
				{
					int  list,
					     which = base;
					Mask tmp = P_ADENIN;
					
					while (which--) tmp = (Mask)(tmp * 2);
					
					FindAppearances(&list,base,1,seq,pos,num,rec);

					position[IndexOfPos(tmp)] = list;

					break;
				}
				default:
				{
					int    *list  = new int [anz[base]],
						    which = base,
						    index;
					Mask    tmp   = N_ADENIN;
					PtNode *ptn;
						
					while (which--) tmp = (Mask)( tmp * 2);
						
					FindAppearances(list,base,anz[base],seq,pos,num,rec);

					index = IndexOfNext(tmp);
					ptn   = new PtNode(seq,list,anz[base],rec + 1);
					next[index] = ptn;

					delete list;
					
					break;
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
//	Kopierkonstruktor
// -----------------------------------------------------------------------------
	PtNode::PtNode ( PtNode &node )
// -----------------------------------------------------------------------------
{
	cout << "\nKopierkonstruktor(PtNode)\n";
	
	mask = node.mask;
	
	if (position)
	{
		int anz = node.NumOfPos();
		
		position = new int [anz];

		memcpy(position,node.position,anz * sizeof(int));
	}
	
	if (next)
	{
		int anz = node.NumOfNext();
		
		next = new PtNode* [anz];
		
		while (anz--)
		    if (node.next[anz]) next[anz] = new PtNode(*node.next[anz]);
		    else				next[anz] = NULL;
	}
}

// -----------------------------------------------------------------------------
//	Destruktor
// -----------------------------------------------------------------------------
	PtNode::~PtNode ( void )
// -----------------------------------------------------------------------------
{
	if (position) delete position;
	
	if (next)
	{
		int anz = NumOfNext();
		
		while (anz--) if (next[anz]) delete next[anz];
		
		delete next;
	}
}

// -----------------------------------------------------------------------------
//	Anzahl der vorhandenen Positionen
// -----------------------------------------------------------------------------
	int PtNode::NumOfPos ( void )
// -----------------------------------------------------------------------------
{
	int anz  = 0,
		test = P_ADENIN;
		
	while (test <= P_FINISH)
	{
		if (mask & test) anz++;
			
		test *= 2;
	}

	return anz;
}

// -----------------------------------------------------------------------------
//	Positionsindex einer Base
// -----------------------------------------------------------------------------
	int PtNode::IndexOfPos ( Mask base )
// -----------------------------------------------------------------------------
{
	int index = INVALID;
	
	if ((base <= P_FINISH) && (mask & base))
	{
		int test = P_ADENIN;
		
		index = 0;
		
		while (test <= base)
		{
			if (mask & test) index++;
			
			test *= 2;
		}

		index--;
	}

	return index;
}

// -----------------------------------------------------------------------------
//	Anzahl der vorhandenen Verzweigungen
// -----------------------------------------------------------------------------
	int PtNode::NumOfNext ( void )
// -----------------------------------------------------------------------------
{
	int anz  = 0,
		test = N_ADENIN;
		
	while (test < (N_ANY * 2))
	{
		if (mask & test) anz++;
			
		test *= 2;
	}

	return anz;
}

// -----------------------------------------------------------------------------
//	Verzweigungsindex einer Base
// -----------------------------------------------------------------------------
	int PtNode::IndexOfNext ( Mask base )
// -----------------------------------------------------------------------------
{
	int index = INVALID;
	
	if ((base >= N_ADENIN) && (base <= N_ANY) && (mask & base))
	{
		int test = N_ADENIN;
		
		index = 0;
		
		while (test <= base)
		{
			if (mask & test) index++;
			
			test *= 2;
		}

		index--;
	}

	return index;
}

// -----------------------------------------------------------------------------
//	Ausgabefunktion fuer ein Knoten eines Positionsbaumes
// -----------------------------------------------------------------------------
	void PtNode::Dump ( void )
// -----------------------------------------------------------------------------
{
	cout << "\nnode: " << this;
	cout << "\nmask = 0x" << hex << mask << "\n";
	
	if (position)
	{
		int anz = NumOfPos(),
		    pos = 0;
		
		cout << "position[" << dec << anz << "] =";
		
		while (pos < anz) cout << " " << position[pos++];

		cout << "\n";
	}

	if (next)
	{
		int anz = NumOfNext(),
		    pos = 0;
		
		cout << "next[" << dec << anz << "]:";
		
		while (pos < anz) cout << " " << next[pos++];

		cout << "\n";

		pos = 0;
		
		while (pos < anz) next[pos++]->Dump();
	}
}

// -----------------------------------------------------------------------------
//	Ausgabefunktion fuer einen Positionsbaum
// -----------------------------------------------------------------------------
	void PtNode::Show ( int  rec,
					    int *where )
// -----------------------------------------------------------------------------
{
	int  panz   = NumOfPos(),
	     nanz   = NumOfNext();
	str  prefix = new char [rec * 3];

	prefix[0] = 0;
	
	if (rec)
	{
		int count = 0;
		
		prefix[0] = 0;
		
		while (count < rec)
			if (where[count++]) strcat(prefix,"|  ");
			else				strcat(prefix,"   ");
	}
	
	if (panz)
	{
		Base base  = ADENIN;
		Mask which = P_ADENIN;
		
		while (which <= P_FINISH)
		{
			int index = IndexOfPos(which);

			if ((index != INVALID) && (index < panz))
			{
				if (base == BASEN)
				{
					if (!nanz && (index == (panz - 1)))
						cout << prefix << "\\- # " << position[index] << "\n";
					else
						cout << prefix << "|- # " << position[index] << "\n";
				}
				else
				{
					if (!nanz && (index == (panz - 1)))
						cout << prefix << "\\- " << (char)BCharacter[base],
						cout << " " << position[index] << "\n";
					else
						cout << prefix << "|- " << (char)BCharacter[base],
						cout << " " << position[index] << "\n";
				}
			}
			
			which = (Mask)(which * 2);
			base  = (Base)(base  + 1);
		}
	}

	if (nanz)
	{
		Base  base     = ADENIN;
		Mask  which    = N_ADENIN;
		int  *wherenew = new int [rec + 1];

		memcpy(wherenew,where,rec * sizeof(int));

		while (which <= N_ANY)
		{
			int index = IndexOfNext(which);

			if ((index != INVALID) && (index < nanz))
			{
				if (base == BASEN)
				{
					if (index == (nanz - 1))
						cout << prefix << "\\- #\n",
						wherenew[rec] = 0;
					else
						cout << prefix << "|- #\n",
						wherenew[rec] = 1;
				}
				else
				{
					if (index == (nanz - 1))
						cout << prefix << "\\- " << (char)BCharacter[base] << "\n",
						wherenew[rec] = 0;
					else
						cout << prefix << "|- " << (char)BCharacter[base] << "\n",
						wherenew[rec] = 1;
				}
				
				next[index]->Show(rec + 1,wherenew);
			}
			
			which = (Mask)(which * 2);
			base  = (Base)(base  + 1);
		}

		delete wherenew;
	}

	delete prefix;
}
