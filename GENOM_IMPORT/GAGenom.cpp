#include "GAGenom.h"

using namespace std;

gellisary::GAGenom::GAGenom(string * fname)
{
	prepared = false;
	file_name = *fname;
}

string * gellisary::GAGenom::getIdentification()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &identification;
}

string * gellisary::GAGenom::getAccessionNumber()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &accession_number;
}

string * gellisary::GAGenom::getOrganismClassificationAsOneString()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &organism_classification_as_one_string;
}

string * gellisary::GAGenom::getSequenceVersion()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &sequence_version;
}

vector<string> * gellisary::GAGenom::getOrganismClassification()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &organism_classification;
}

string * gellisary::GAGenom::getDescription()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &description;
}

string * gellisary::GAGenom::getOrganism()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &organism_species;
}

string * gellisary::GAGenom::getContig()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &contig;
}

string * gellisary::GAGenom::getSequence()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &sequence;
}

string * gellisary::GAGenom::getKeyWordsAsString()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &key_words_as_string;
}

string * gellisary::GAGenom::getCommentAsOneString()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &free_text_commnt_as_one_string;
}

vector<string> * gellisary::GAGenom::getKeyWords()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &key_words;
}

vector<string> * gellisary::GAGenom::getComment()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &free_text_comment;
}
