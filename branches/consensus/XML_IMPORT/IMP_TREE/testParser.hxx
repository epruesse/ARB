#include <string.h>
#include <fstream.h>
#include <stdlib.h>
#include "MySAXHandler.hxx"

class StrX 
{
private :
    char*   fLocalForm;

public :
    StrX(const XMLCh* const toTranscode) {
        // Call the private transcoding method
        fLocalForm = XMLString::transcode(toTranscode);
    }

    ~StrX() {
        XMLString::release(&fLocalForm);
    }

    //  Getter methods
    const char* localForm() const  {
        return fLocalForm;
    }
};

inline ostream& operator<<(ostream& target, const StrX& toDump)
{
    target << toDump.localForm();
    return target;
}
