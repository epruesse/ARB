/*
 * Author : Artem Artemov
 * Mail : hagilis@web.de
 */
#ifndef GAPARSER_H
#define GAPARSER_H

#ifndef _CPP_STRING
#include <string>
#endif

#ifndef _CPP_VECTOR
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
	virtual ~GAParser(){}
	virtual void parse() = 0;
	void update(std::string*);
	// static bool writeMessage(std::string *);
};

};

#endif // GAPARSER_H
