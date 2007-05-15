//  ==================================================================== //
//                                                                       //
//    File      : dynamic_array.h                                        //
//    Time-stamp: <Thu Feb/05/2004 11:40 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#include <vector>


template <typename T>
class dynamic_array
{
    public:
        dynamic_array() {};
        dynamic_array ( int rows, int cols )
        {
            for ( int i=0; i<rows; ++i )
            {
                data_.push_back ( std::vector<T> ( cols ) );
            }
        }
        std::vector<T> & operator[] ( int i )
        {
            return data_[i];
        }
        const std::vector<T> & operator[] ( int i ) const
        {
            return data_[i];
        }

    private:
        std::vector<std::vector<T> > data_;
};
