/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAGenomReference.h"
#include "GAGenomUtilities.h"

using namespace std;
using namespace gellisary;

gellisary::GAGenomReference::GAGenomReference()
{
    prepared = false;
    reference_number = -1;
}

string * gellisary::GAGenomReference::getTitle()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_title;
}

string * gellisary::GAGenomReference::getComment()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_comment;
}

string * gellisary::GAGenomReference::getLocation()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_location;
}

vector<string> * gellisary::GAGenomReference::getAuthors()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_authors;
}

int gellisary::GAGenomReference::getNumber()
{
    if(!prepared)
    {
        parse();
    }
    return reference_number;
}

vector<int> * gellisary::GAGenomReference::getPositionBegin()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_position_begin;
}

vector<int> * gellisary::GAGenomReference::getPositionEnd()
{
    if(!prepared)
    {
        parse();
    }
    return &reference_position_end;
}

string * gellisary::GAGenomReference::getAuthorsAsString()
{
    if(!prepared)
    {
        parse();
    }
    reference_authors_as_string = GAGenomUtilities::toOneString(&reference_authors,true);
    return &reference_authors_as_string;
}
