/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <arbdb.h>
#include "GAFile.h"
#include "GASourceFileSwitcher.h"
#include "GAGenBank.h"
#include "GADDBJ.h"
#include "GAEmbl.h"
#include <awt.hxx>

GB_ERROR GI_importGenomeFile(GBDATA * gb_main, const char * file_name, const char * ali_name)
{
	GB_ERROR my_error = NULL;
  #if defined(DEBUG)
    gellisary::GALogger logger;
    gellisary::GAARB arb(gb_main,ali_name);
    gellisary::GASourceFileSwitcher switcher(file_name);
    int format = switcher.make_a_decision();
    std::string flat_file_name(file_name);
    if(format == gellisary::GASourceFileSwitcher::EMBL)
    {
    	gellisary::GAEmbl embl(logger,arb,flat_file_name);
    	embl.parse();
    	if(embl.has_messages())
    	{
    		my_error = embl.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ)
    {
    	gellisary::GADDBJ ddbj(logger,arb,flat_file_name,true);
    	ddbj.parse();
    	if(ddbj.has_messages())
    	{
    		my_error = ddbj.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ_WITHOUT_HEADER)
    {
		gellisary::GADDBJ ddbj(logger,arb,flat_file_name,false);
    	ddbj.parse();
    	if(ddbj.has_messages())
    	{
    		my_error = ddbj.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::GENBANK)
    {
		gellisary::GAGenBank genbank(logger,arb,flat_file_name);
    	genbank.parse();
    	if(genbank.has_messages())
    	{
    		my_error = genbank.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::UNKNOWN)
    {
    }
    if(logger.isLogFileOpen())
    {
    	logger.closeLogFile();
    }
  #else
  	gellisary::GAARB arb(gb_main,ali_name);
    gellisary::GASourceFileSwitcher switcher(file_name);
    int format = switcher.make_a_decision();
    std::string flat_file_name(file_name);
    if(format == gellisary::GASourceFileSwitcher::EMBL)
    {
    	gellisary::GAEmbl embl(arb,flat_file_name);
    	embl.parse();
    	if(embl.has_messages())
    	{
    		my_error = embl.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ)
    {
    	gellisary::GADDBJ ddbj(arb,flat_file_name,true);
    	ddbj.parse();
    	if(ddbj.has_messages())
    	{
    		my_error = ddbj.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ_WITHOUT_HEADER)
    {
		gellisary::GADDBJ ddbj(arb,flat_file_name,false);
    	ddbj.parse();
    	if(ddbj.has_messages())
    	{
    		my_error = ddbj.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::GENBANK)
    {
		gellisary::GAGenBank genbank(arb,flat_file_name);
    	genbank.parse();
    	if(genbank.has_messages())
    	{
    		my_error = genbank.get_message().c_str();
    	}
    }
    else if(format == gellisary::GASourceFileSwitcher::UNKNOWN)
    {
    }
  #endif
    return my_error;
}

