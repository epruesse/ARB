#ifndef GENOMLOCATION_H
#define GENOMLOCATION_H

#ifndef VECTOR
#include <vector>
#endif
#ifndef STRING
#include <string>
#endif


//typedef vector<GenomLocation> LocationVector;
// #include <vector>
// #include <string>
//#include <GenomLocation.h>

class GenomLocation
{
	private:
//		LocationVektor locations;
    vector<GenomLocation> locations;
		long value;

		bool range;				//	..
		bool complement;	//	complement(...)
		bool join;				//	join(...)
		bool order;				//	order(...)
		bool point;				//	.
		bool roof;				//	^
		bool smaller_begin;			//	<
		bool bigger_begin;			//	>
		bool smaller_end;			//	<
		bool bigger_end;			//	>
    bool normal;				//	Not Range, only single value
		int actual_value;
    bool pointer;
    string reference;

  //    GenomLocation(const GenomLocation&) {}
  // GenomLocation& operator=(const GenomLocation& other) { return *this; }
  static int tmp;
	public:
		GenomLocation()
    {
  		value = 0;
  		range = false;
  		complement = false;
  		join = false;
  		order = false;
  		point = false;
  		roof = false;
  		smaller_begin = false;
  		bigger_begin = false;
  		smaller_end = false;
  		bigger_end = false;
      normal = true;
  		actual_value = 0;
      pointer = false;
    }

		bool isSingleValue(void){return normal;}
		bool isRanged(void) {return range;}
		bool isJoin(void){return join;}
		bool isComplement(void){return complement;}
		bool isOrder(void){return order;}
		bool isRoof(void){return roof;}
		bool isPoint(void){return point;}
		bool isSmallerBegin(void){return smaller_begin;}
		bool isBiggerBegin(void){return bigger_begin;}
		bool isSmallerEnd(void){return smaller_end;}
		bool isBiggerEnd(void){return bigger_end;}
    bool isReference(void){return pointer;}

		/*void isSingleValue(bool b){ normal = b;}
		void isRanged(bool b){ range = b;}
		void isJoin(bool b){ join = b;}
		void isComplement(bool b){ complement = b;}
		void isOrder(bool b){ order = b;}
		void isRoof(bool b){ roof = b;}
		void isPoint(bool b){ point = b;}
		void isSmallerBegin(bool b){ smaller_begin = b;}
		void isBiggerBegin(bool b){ bigger_begin = b;}
		void isSmallerEnd(bool b){ smaller_end = b;}
		void isBiggerEnd(bool b){ bigger_end = b;}
    void isReference(bool b){ pointer = b;}*/
    void parseLocation(string&);

		void setSingleValue(long val){value = val;}
		long getSingleValue(void){return value;}
    void setReference(string& ref){ reference = ref;}
    string * getReference(void) { return &reference;}

//    LocationVector::iterator begin() { return locations.begin(); }
//    LocationVector::iterator end() { return locations.end(); }


//  GenomLocation gl;
//  for (LocationVector::iterator i = gl.begin(); i != gl.end(); ++i) {
  //      const GenomLocation& current = *i;

    //    if ((*i).isSingleValue()) { ... }
      //  if (current.isSingleValue()) { ... }
//    }

    bool hasMore()
    {
      if(actual_value < (int)locations.size())
      {
        return true;
      }
      else
      {
        return false;
      }
    }

		GenomLocation getNextValue(void)
		{
    	return locations[actual_value++];
		}

		bool hasMoreValues()
		{
			if(actual_value < (int)locations.size())
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		void setValue(const GenomLocation& nValue)
		{
			locations.push_back(nValue);
		}
};

#else
#error GenomLocation.h included twice
#endif
