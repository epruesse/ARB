// -----------------------------------------------------------------------------
//	Include-Dateien
// -----------------------------------------------------------------------------

#include <cstring>
#include <fstream>
#include <iostream>

#include "a3_matrix.hxx"

using std::cout;

// -----------------------------------------------------------------------------
void A3Matrix::Init ( int xlen,
                      int ylen,
						  int del )
// -----------------------------------------------------------------------------
{
	width  = xlen;
	height = ylen;
	free   = !!del;
	matrix = new vp [width * height];

	memset(matrix,0,width * height * sizeof(vp));
}

// -----------------------------------------------------------------------------
	A3Matrix::A3Matrix ( int len,
						 int del )
// -----------------------------------------------------------------------------
{
	Init(len,len,del);
}

// -----------------------------------------------------------------------------
	A3Matrix::A3Matrix ( int xlen,
						 int ylen,
						 int del )
// -----------------------------------------------------------------------------
{
	Init(xlen,ylen,del);
}

// -----------------------------------------------------------------------------
	A3Matrix::A3Matrix ( A3Matrix &other )
// -----------------------------------------------------------------------------
{
	Init(other.width,other.height,other.free);
	
	memcpy(matrix,other.matrix,width * height * sizeof(vp));
}

// -----------------------------------------------------------------------------
	A3Matrix::~A3Matrix ( void )
// -----------------------------------------------------------------------------
{
	if (free) Clear();

	delete matrix;
}

// -----------------------------------------------------------------------------
	int A3Matrix::Set ( int xpos,
					    int ypos,
					    vp  val )
// -----------------------------------------------------------------------------
{
	int error = 0;
	
	if ((xpos < 0)      ||
		(xpos >= width) ||
		(ypos < 0)      ||
		(ypos >= height)) error = 1;
	else
	{
		int p = ypos * width + xpos;

		if (free && matrix[p]) delete matrix[p];
		
		matrix[p] = val;
	}
	
	return error;
}
	
// -----------------------------------------------------------------------------
	vp A3Matrix::Get ( int xpos,
					   int ypos )
// -----------------------------------------------------------------------------
{
	vp val = NULL;
	
	if ((xpos >= 0)    &&
		(xpos < width) &&
		(ypos >= 0)    &&
		(ypos < height))
	{
		int p = ypos * width + xpos;

		val = matrix[p];
	}
	
	return val;
}

// -----------------------------------------------------------------------------
	void A3Matrix::Clear ( void )
// -----------------------------------------------------------------------------
{
	int y = height;
		
	while (y--)
	{
		int x = width,
		    l = y * width;
			
		while (x--)
		{
			int p = l + x;
			
			if (free && matrix[p]) delete matrix[p];

			matrix[p] = NULL;
		}
	}
}
	
// -----------------------------------------------------------------------------
	void A3Matrix::Dump	( dumpfunc edump )
// -----------------------------------------------------------------------------
{
	int y = 0;
		
	while (y < height)
	{
		int x = 0,
		    l = y * width;
			
		while (x < width)
		{
			int p = l + x;
			
			if (edump) edump(matrix[p]);
			else cout << " " << (int)matrix[p];

			x++;
		}

		cout << "\n";

		y++;
	}
}
