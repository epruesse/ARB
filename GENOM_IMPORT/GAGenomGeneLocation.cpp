/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#include "GAGenomGeneLocation.h"

using namespace std;

gellisary::GAGenomGeneLocation::GAGenomGeneLocation(string * new_location_string)
{
	range = false;
	complement = false;
	join = false;
	order = false;
	point = false;
	roof = false;
	single_value = true;
	collection = false;
	smaller_begin = false;
	smaller_end = false;
	bigger_begin = false;
	bigger_end = false;
	value = -1;
	location_as_string = *new_location_string;
}

bool gellisary::GAGenomGeneLocation::isSingleValue()
{
	if(!prepared)
	{
		parse();
	}
	return single_value;
}

bool gellisary::GAGenomGeneLocation::isRange()
{
	if(!prepared)
	{
		parse();
	}
	return range;
}

bool gellisary::GAGenomGeneLocation::isJoin()
{
	if(!prepared)
	{
		parse();
	}
	return join;
}

bool gellisary::GAGenomGeneLocation::isComplement()
{
	if(!prepared)
	{
		parse();
	}
	return complement;
}

bool gellisary::GAGenomGeneLocation::isOrder()
{
	if(!prepared)
	{
		parse();
	}
	return order;
}

bool gellisary::GAGenomGeneLocation::isRoof()
{
	if(!prepared)
	{
		parse();
	}
	return roof;
}

bool gellisary::GAGenomGeneLocation::isPoint()
{
	if(!prepared)
	{
		parse();
	}
	return point;
}

bool gellisary::GAGenomGeneLocation::isCollection()
{
	if(!prepared)
	{
		parse();
	}
	return collection;
}

bool gellisary::GAGenomGeneLocation::isSmallerBegin()
{
	if(!prepared)
	{
		parse();
	}
	return smaller_begin;
}

bool gellisary::GAGenomGeneLocation::isSmallerEnd()
{
	if(!prepared)
	{
		parse();
	}
	return smaller_end;
}

bool gellisary::GAGenomGeneLocation::isBiggerBegin()
{
	if(!prepared)
	{
		parse();
	}
	return bigger_begin;
}

bool gellisary::GAGenomGeneLocation::isBiggerEnd()
{
	if(!prepared)
	{
		parse();
	}
	return bigger_end;
}

void gellisary::GAGenomGeneLocation::setSingleValue(int new_value)
{
	value = new_value;
}

int gellisary::GAGenomGeneLocation::getSingleValue()
{
	if(!prepared)
	{
		parse();
	}
	return value;
}
