#include <GenomEmbl.h>

class GenomArbConnection
{
  private:
    GenomEmbl embl;
    int typeOfDatabase = 0;
  public:
    GenomArbConnection(void);
    ~GenomArbConnection(void);
    setEmblFlatFile(GenomEmbl&);
    
};