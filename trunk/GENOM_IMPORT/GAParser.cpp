/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#include "GAParser.h"

using namespace std;

gellisary::GAParser::GAParser()
{
    prepared = false;
}

void gellisary::GAParser::update(string * new_line)
{
    row_lines.push_back(*new_line);
    prepared = false;
}

// bool gellisary::GAParser::writeMessage(string * source_str)
// {
//  return true;
// }
