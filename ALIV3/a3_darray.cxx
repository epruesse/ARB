// -----------------------------------------------------------------------------
//	Include-Dateien
// -----------------------------------------------------------------------------

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "a3_darray.hxx"

using std::cout;

// -----------------------------------------------------------------------------
void DArray::Init ( int num,
                    int inc,
                    int del )
// -----------------------------------------------------------------------------
{
	elements  = num;
	nextfree  = 0;
	increment = inc;
	free	  = del;
	array	  = new vp [elements];

	memset(array,0,elements * sizeof(vp));
}

// -----------------------------------------------------------------------------
	void DArray::Grow ( void )
// -----------------------------------------------------------------------------
{
	vp  *save = array;
	int  anz  = elements,
		 next = nextfree;
	
	Init(elements + increment,increment,free);

	nextfree = next;
	
	memcpy(array,save,anz * sizeof(vp));
	
	delete save;
}

// -----------------------------------------------------------------------------
	void DArray::Shrink ( int num )
// -----------------------------------------------------------------------------
{
	vp  *save = array;
	
	Init(num,increment,free);

	nextfree = num;
	
	memcpy(array,save,num * sizeof(vp));
	
	delete save;
}

// -----------------------------------------------------------------------------
	DArray::DArray ( void )
// -----------------------------------------------------------------------------
{
	Init(DARRAY_SIZE,DARRAY_INC,0);
}

// -----------------------------------------------------------------------------
	DArray::DArray ( int num )
// -----------------------------------------------------------------------------
{
	if (num > 0) Init(num,DARRAY_INC,0);
	else		 Init(DARRAY_SIZE,DARRAY_INC,0);
}

// -----------------------------------------------------------------------------
	DArray::DArray ( int num,
					 int inc,
					 int del )
// -----------------------------------------------------------------------------
{
	del = !!del;
	
	if (num > 0)
	{
		if (inc > 0) Init(num,inc,del);
		else		 Init(num,DARRAY_INC,del);
	}
	else
	{
		if (inc > 0) Init(DARRAY_SIZE,inc,del);
		else		 Init(DARRAY_SIZE,DARRAY_INC,del);
	}
}

// -----------------------------------------------------------------------------
	DArray::DArray ( DArray &other )
// -----------------------------------------------------------------------------
{
	Init(other.elements,other.increment,other.free);
	nextfree = other.nextfree;
	memcpy(array,other.array,elements * sizeof(vp));
}
	
// -----------------------------------------------------------------------------
	DArray::~DArray ( void )
// -----------------------------------------------------------------------------
{
	Clear();
}

// -----------------------------------------------------------------------------
	int DArray::Add ( vp elem )
// -----------------------------------------------------------------------------
{
	int index = nextfree;
	
	if (nextfree == elements) Grow();
	
	array[nextfree++] = elem;
	
	return index;
}

// -----------------------------------------------------------------------------
	int DArray::Set ( vp  elem,
					  int index )
// -----------------------------------------------------------------------------
{
	int pos = -1;
	
	if (index >= 0)
	{
		pos = index;
		
		while (pos >= elements) Grow();

		array[pos] = elem;
		
		nextfree = pos + 1;
	}
	
	return pos;
}

// -----------------------------------------------------------------------------
	int DArray::Del ( int index )
// -----------------------------------------------------------------------------
{
	int error = 0;

	if ((index < 0) || (index >= elements)) error = 1;
	else
	{
		if (free && array[index]) delete array[index];
		
		array[index] = NULL;
		
		if (nextfree == (index - 1)) nextfree = index;
	}
	
	return error;
}

// -----------------------------------------------------------------------------
	int DArray::Elements ( void )
// -----------------------------------------------------------------------------
{
	return elements;
}
	
// -----------------------------------------------------------------------------
	vp DArray::operator [] ( int index )
// -----------------------------------------------------------------------------
{
	vp elem = NULL;
	
	if ((index >= 0) && (index < elements)) elem = array[index];

	return elem;
}

// -----------------------------------------------------------------------------
	DArray &DArray::operator = ( DArray &other )
// -----------------------------------------------------------------------------
{
	if (this != &other)
	{
		Init(other.elements,other.increment,other.free);
		nextfree = other.nextfree;
		null	 = other.null;
		memcpy(array,other.array,elements * sizeof(vp));
	}

	return *this;
}

// -----------------------------------------------------------------------------
	void DArray::Sort ( cmpfunc cmp )
// -----------------------------------------------------------------------------
{
	if (elements > 1)
	{
		int count = 0,
			anz	  = elements;

		if (!null)
		{
			int sum = 0,
				tmp;
			
			for (tmp = 0;tmp < anz;tmp++) if (array[tmp]) sum++;
			
			if (sum != anz)
			{
				vp *help = new vp [sum];

				anz = elements = nextfree = sum;
				
				sum = 0;
				
				for (tmp = 0;tmp < anz;tmp++) if (array[tmp]) help[sum++] = array[tmp];

				delete array;
				
				array = help;
			}
		}
		
		qsort(array,anz,sizeof(vp),cmp);
		int source;
		int dest = 1;
		vp lastelem = array[0];
	
		for (source = 1; source < anz; source++) {
			if (array[source] != lastelem){
				array[dest++] = lastelem = array[source];
			}
		}
		anz = dest;
	
		if (anz < elements) Shrink(anz);
	}
}

// -----------------------------------------------------------------------------
	void DArray::Clear ( void )
// -----------------------------------------------------------------------------
{
	if (free) 
	{
		int count = elements;
		
		while (count--) if (array[count]) delete array[count];
	}

	delete array;

	array = NULL;
}

// -----------------------------------------------------------------------------
	void DArray::Dump ( dumpfunc edump )
// -----------------------------------------------------------------------------
{
	int count = 0;
	
	cout << "\nelements  = " << elements;
	cout << "\nnextfree  = " << nextfree;
	cout << "\nincrement = " << increment;
	
	cout << "\n\n";

	while (edump && (count < elements)) edump(array[count++]);

	cout << "\n\n";
}
