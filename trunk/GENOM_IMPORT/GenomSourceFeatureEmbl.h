#include <map>
#include <vector>
#include <string>
//#include <GenomUtilities.h>


class GenomSourceFeatureEmbl
{
	private:
    std::map<std::string,std::string> qualifiers;
		std::vector<std::string> row_lines;
		bool prepared;
		int actual_qualifier;
		long source_begin;
		long source_end;
    std::map<std::string,std::string>::const_iterator ki;

	public:
		GenomSourceFeatureEmbl(void);
		~GenomSourceFeatureEmbl(void);

		const std::string* getQualifier(void);
		std::string* getValue(const std::string*);

		void updateContain(std::string&);
		void prepareSource(void);   //	parses source feature of a feature table
		long getBegin(void){return source_begin;}
		long getEnd(void){return source_end;}
};

