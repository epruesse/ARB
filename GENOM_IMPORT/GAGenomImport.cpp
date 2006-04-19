/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 * Copyright 2004 - Lehrstuhl fuer Mikrobiologie - TU Muenchen
 */
#ifndef _GLIBCXX_CSTDLIB
#include <cstdlib>
#endif

#ifndef _GLIBCXX_IOSTREAM
#include <iostream>
#endif

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#ifndef GAFILE_H
#include "GAFile.h"
#endif

#ifndef GASOURCEFILESWITCHER_H
#include "GASourceFileSwitcher.h"
#endif

#ifndef GALOGGER_H
#include "GALogger.h"
#endif

#ifndef GAGENBANK_H
#include "GAGenBank.h"
#endif

#ifndef GADDBJ_H
#include "GADDBJ.h"
#endif

#ifndef GAEMBL_H
#include "GAEmbl.h"
#endif

#ifndef arbdbt_h_included
#include <arbdbt.h>
#endif

#ifndef awt_hxx_included
#include <awt.hxx>
#endif

#ifndef AW_RENAME_HXX
#include <AW_rename.hxx>
#endif

#ifndef ADGENE_H
#include <adGene.h>
#endif

GB_ERROR GI_importGenomeFile(GBDATA * gb_main, const char * file_name, const char * ali_name)
{
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
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ)
    {
    	gellisary::GADDBJ ddbj(logger,arb,flat_file_name,true);
    	ddbj.parse();
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ_WITHOUT_HEADER)
    {
		gellisary::GADDBJ ddbj(logger,arb,flat_file_name,false);
    	ddbj.parse();
    }
    else if(format == gellisary::GASourceFileSwitcher::GENBANK)
    {
		gellisary::GAGenBank genbank(logger,arb,flat_file_name);
    	genbank.parse();
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
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ)
    {
    	gellisary::GADDBJ ddbj(arb,flat_file_name,true);
    	ddbj.parse();
    }
    else if(format == gellisary::GASourceFileSwitcher::DDBJ_WITHOUT_HEADER)
    {
		gellisary::GADDBJ ddbj(arb,flat_file_name,false);
    	ddbj.parse();
    }
    else if(format == gellisary::GASourceFileSwitcher::GENBANK)
    {
		gellisary::GAGenBank genbank(arb,flat_file_name);
    	genbank.parse();
    }
    else if(format == gellisary::GASourceFileSwitcher::UNKNOWN)
    {
    }
  #endif
    return NULL;
}

