// -----------------------------------------------------------------------------
//	Include-Dateien
// -----------------------------------------------------------------------------

#include <cstring>
#include <fstream>
#include <iostream>

#include "a3_ali.hxx"

using std::cout;

// -----------------------------------------------------------------------------
int psolcmp ( const void *a,
				  const void *b )
// -----------------------------------------------------------------------------
{
	int res  = 0,
		diff = (*(PSolution**)a)->score - (*(PSolution**)b)->score;
	
	if      (diff < 0) res = -1;
	else if (diff > 0) res =  1;

	return res;
}

// -----------------------------------------------------------------------------
	void psoldump ( vp val )
// -----------------------------------------------------------------------------
{
	PSolution *psol = (PSolution*)val;

	if (psol)
	{
		cout << psol->score << "\n";
	
		psol->match.Dump(hmatchdump);
	}
}

// -----------------------------------------------------------------------------
	void PSolution::Dump ( void )
// -----------------------------------------------------------------------------
{
	psoldump((vp)this);
}

// -----------------------------------------------------------------------------
	Aligner::Aligner ( void ) : postree(), prealigned(), helix()
// -----------------------------------------------------------------------------
{
	prealigned.Sort(intcmp);
}

// -----------------------------------------------------------------------------
	Aligner::Aligner ( str     seq,
					   str     hel,
					   str     kon,
					   DArray &pre ) : postree(seq),
									   prealigned(pre),
									   helix(hel,kon,
											 ((PSolution*)pre[pre.Elements() - 1])->match)
// -----------------------------------------------------------------------------
{
}

// -----------------------------------------------------------------------------
	void Aligner::Set ( str	    seq,
					    str     hel,
					    str     kon,
					    DArray &pre )
// -----------------------------------------------------------------------------
{
	postree.Set(seq);

	prealigned.Clear();
	prealigned = pre;
}

// -----------------------------------------------------------------------------
	DArray &Aligner::Align ( void )
// -----------------------------------------------------------------------------
{
	DArray *array = new DArray;
	int	   anz    = prealigned.Elements();

	helix.Set(((PSolution*)prealigned[anz - 1])->match);

	{
		DArray &tmp = helix.Helices(1,10);
			
		delete &tmp;
	}
	
	return *array;
}

// -----------------------------------------------------------------------------
	void Aligner::Dump ( void )
// -----------------------------------------------------------------------------
{
	postree.Show(2);
	prealigned.Dump(psoldump);
	helix.Dump(1);
}
