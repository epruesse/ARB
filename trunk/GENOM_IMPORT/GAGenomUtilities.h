#ifndef GAGENOMUTILITIES_H
#define GAGENOMUTILITIES_H

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_VECTOR
#include <vector>
#endif

namespace gellisary{

class GAGenomUtilities{
public:
	static void eliminateFeatureTableSignInEmbl(std::string *, std::string *);
	static void eliminateSign(std::string *, char);
	static std::string toOneString(std::vector<std::string> *, bool);
	static void onlyOneDelimerChar(std::string *, char);
	static std::vector<std::string> findAndSeparateWordsBy(std::string *, char, bool);
	static void replaceByString(std::string *, std::string *, std::string *);
	static std::vector<std::string> findAndSeparateWordsByString(std::string *, std::string *, bool);
	static std::vector<std::string> findAndSeparateWordsByChar(std::string *, char, bool);
	static void trimString(std::string *);
	static void trimStringByChar(std::string *, char);
	static int stringToInteger(std::string *);
	static std::string integerToString(int);
	static std::vector<int> parseSourceLocation(std::string *);
	static std::string generateGeneID(std::string *, int);
	static bool isNewGene(std::string *);
	static bool isSource(std::string *);
//	static std::string generateGeneID(std::string *, int);
	static void replaceByWhiteSpaceCleanly(std::string *, std::string *);
	static void preparePropertyString(std::string *, std::string *, std::string *);
};

};

#endif // GAGENOMUTILITIES_H
