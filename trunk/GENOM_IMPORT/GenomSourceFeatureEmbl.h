#include <map>
#include <vector>
#include <string>
//#include <GenomUtilities.h>


class GenomSourceFeatureEmbl
{
	private:
    map<string,string> qualifiers;
		vector<string> row_lines;
		bool prepared;
		int actual_qualifier;
		long source_begin;
		long source_end;
    map<string,string>::const_iterator ki;

	public:
		GenomSourceFeatureEmbl(void);
		~GenomSourceFeatureEmbl(void);

		const string* getQualifier(void);
		string* getValue(const string*);

		void updateContain(string&);
		void prepareSource(void);   //	parses source feature of a feature table
		long getBegin(void){return source_begin;}
		long getEnd(void){return source_end;}
};

