/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAGenom.h"
#include "GAGenomUtilities.h"

using namespace std;

gellisary::GAGenom::GAGenom(string * fname)
{
	prepared = false;
	file_name = *fname;
	complete_file = false;
	error_number = 2;
	sequence_length = 0;
    sequence_a = 0;
	sequence_c = 0;
	sequence_g = 0;
	sequence_t = 0;
	sequence_other = 0;
	error_message = "The FlatFile is incomplete!";
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

int gellisary::GAGenom::getErrorNumber()
{
	return error_number;
}

string * gellisary::GAGenom::getErrorMessage()
{
	return &error_message;
}

string * gellisary::GAGenom::getOrganismClassificationAsOneString()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	organism_classification_as_one_string = GAGenomUtilities::toOneString(&organism_classification,true);
	return &organism_classification_as_one_string;
}

bool gellisary::GAGenom::isFileComplete()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return complete_file;;
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
	key_words_as_string = GAGenomUtilities::toOneString(&key_words,true);
	return &key_words_as_string;
}

string * gellisary::GAGenom::getCommentAsOneString()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	free_text_comment_as_one_string = GAGenomUtilities::toOneString(&free_text_comment,true);
	return &free_text_comment_as_one_string;
}

vector<string> * gellisary::GAGenom::getKeyWords()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &key_words;
}

int gellisary::GAGenom::getSequenceLength()
{
	if(!prepared)
    {
        parseFlatFile();
    }
    return sequence_length;
}

int gellisary::GAGenom::getSequenceA()
{
	if(!prepared)
    {
        parseFlatFile();
    }
    return sequence_a;
}

int gellisary::GAGenom::getSequenceC()
{
	if(!prepared)
    {
        parseFlatFile();
    }
    return sequence_c;
}

int gellisary::GAGenom::getSequenceG()
{
	if(!prepared)
    {
        parseFlatFile();
    }
    return sequence_g;
}

int gellisary::GAGenom::getSequenceT()
{
	if(!prepared)
    {
        parseFlatFile();
    }
    return sequence_t;
}

int gellisary::GAGenom::getSequenceOther()
{
	if(!prepared)
    {
        parseFlatFile();
    }
    return sequence_other;
}

vector<string> * gellisary::GAGenom::getComment()
{
	if(!prepared)
	{
		parseFlatFile();
	}
	return &free_text_comment;
}
