#ifndef GAGENOMIMPORT_H
#define GAGENOMIMPORT_H

#include "../GENOM_IMPORT/GAGenomReferenceEmbl.h"
#include "../GENOM_IMPORT/GAGenomFeatureTableEmbl.h"
#include "../GENOM_IMPORT/GAGenomGeneEmbl.h"
#include "../GENOM_IMPORT/GAGenomGeneLocationEmbl.h"
#include "../GENOM_IMPORT/GAGenomFeatureTableSourceEmbl.h"

#ifndef _CPP_STRING
#include <string>
#endif

#include <arbdbt.h>

namespace gellisary {

class GAGenomImport{
public:
	static GB_ERROR executeQuery(GBDATA *, const char *,const char *);
	static void writeReferenceEmbl(GBDATA *, gellisary::GAGenomReferenceEmbl *);
	static void writeFeatureTableEmbl(GBDATA *, gellisary::GAGenomFeatureTableEmbl *);
	static void writeGeneEmbl(GBDATA *, gellisary::GAGenomGeneEmbl *);
	static bool writeLocationEmbl(GBDATA *, gellisary::GAGenomGeneLocationEmbl *, std::vector<int> *);
	static void writeByte(GBDATA *, std::string *, int);
	static void writeSourceEmbl(GBDATA *, gellisary::GAGenomFeatureTableSourceEmbl *);
	static void writeString(GBDATA *, std::string *, std::string *);
	static void writeInteger(GBDATA *, std::string *, int);
	static void writeGenomSequence(GBDATA *, std::string *, const char *);
};

};

#endif // GAGENOMIMPORT_H
