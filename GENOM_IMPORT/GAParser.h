#ifndef GAPARSER_H
#define GAPARSER_H

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

namespace gellisary{

class GAParser{
protected:
	std::vector<std::string> row_lines;
	bool prepared;
	
	void checkFields();
	
public:

	GAParser();
	virtual ~GAParser();
	virtual void parse() = 0;
	void update(std::string*);
};

};

#endif // GAPARSER_H
