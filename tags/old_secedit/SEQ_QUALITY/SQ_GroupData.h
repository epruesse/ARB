//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupData.h                                         //
//    Purpose   : Classes to store global information about sequences    //
//    Time-stamp: <Wed Mar/21/2007 00:16 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July 2003 - February 2004                  //
//  Coded by Kai Bader (baderk@in.tum.de) in 2007                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //


#ifndef SQ_GROUPDATA_H
#define SQ_GROUPDATA_H

#ifndef _CPP_CSTDDEF
# include <cstddef>
#endif
#ifndef _CPP_IOSTREAM
# include <iostream>
#endif
#ifndef _MEMORY_H
#include <memory.h>
#endif

#ifndef ARB_ASSERT_H
# include <arb_assert.h>
#endif
#define seq_assert(bed) arb_assert(bed)


typedef struct {
    double conformity;
    double deviation;
}
consensus_result;


class SQ_GroupData {
public:
    SQ_GroupData();
    virtual ~SQ_GroupData();
    virtual SQ_GroupData& operator = ( const SQ_GroupData& other )    = 0;
    virtual SQ_GroupData *clone() const                             = 0;

    void         SQ_set_avg_bases ( int bases ) {
        avg_bases += bases;
    }
    int          SQ_get_avg_bases() const {
        return ( avg_bases/nr_sequences );
    }
    void         SQ_set_avg_gc ( double gc ) {
        gc_prop += gc;
    }
    double       SQ_get_avg_gc() const {
        return ( gc_prop/nr_sequences );
    }
    void         SQ_count_sequences() {
        nr_sequences++;
    }
    int          SQ_get_nr_sequences() const {
        return nr_sequences;
    }
    bool         SQ_is_initialized() const {
        return initialized;
    }

    virtual void   SQ_init_consensus ( int size )                              = 0;
    virtual int    SQ_print_on_screen()                                        = 0;
    virtual consensus_result SQ_calc_consensus ( const char *sequence ) const  = 0;
    virtual void   SQ_add_sequence ( const char *sequence )                    = 0;
    virtual void   SQ_add ( const SQ_GroupData& other )                        = 0;

    int getSize() const {
        return size;
    }

protected:
    int  size;
    int  avg_bases;
    int  nr_sequences;
    double  gc_prop;
    bool initialized;
};


template <int I>
class Int {
    Int ( const Int& other );   // copying not allowed

public:
    int i[I];

    int size() const {
        return I;
    }

    Int() {
        memset ( i, 0, I*sizeof ( int ) );
    }

    Int& operator += ( const Int& other ) {
        const int *otheri = other.i;
        for ( int j = 0; j<I; ++j ) {
            i[j] += otheri[j];
        }
        return *this;
    }

    Int& operator = ( const Int& other ) {
        memcpy ( i, other.i, I*sizeof ( int ) );
        return *this;
    }
};


template <int I>
class SQ_GroupData_Impl : public SQ_GroupData {
    SQ_GroupData_Impl ( const SQ_GroupData_Impl& other );

public:
    SQ_GroupData_Impl() {
        consensus = 0;
    }

    virtual ~SQ_GroupData_Impl();

    SQ_GroupData_Impl& operator= ( const SQ_GroupData_Impl& other ) {
        seq_assert ( other.size>0 && other.initialized );
        if ( !initialized ) SQ_init_consensus ( other.size );
        seq_assert ( size==other.size );
        avg_bases = other.avg_bases;
        gc_prop = other.gc_prop;
        for ( int s=0; s<size; ++s ) {
            consensus[s] = other.consensus[s];
        }
        nr_sequences = other.nr_sequences;
        return *this;
    }

    void SQ_init_consensus ( int size );
    int  SQ_print_on_screen();
    void SQ_add_column ( int col );
    void SQ_add ( const SQ_GroupData& other ); // add's other to this

protected:
    Int<I> *consensus;
};


class SQ_GroupData_RNA: public SQ_GroupData_Impl<6> {
    SQ_GroupData_RNA ( const SQ_GroupData_RNA& other );         // copying not allowed
public:
    SQ_GroupData_RNA() {}

    SQ_GroupData_RNA *clone() const {
        return new SQ_GroupData_RNA;
    }
    SQ_GroupData_RNA& operator= ( const SQ_GroupData& other ) {
        return static_cast<SQ_GroupData_RNA&> ( SQ_GroupData_Impl<6>::operator=
                                                ( static_cast<const SQ_GroupData_Impl<6>& > ( other ) ) );
    }

    consensus_result SQ_calc_consensus ( const char *sequence ) const;
    void   SQ_add_sequence ( const char *sequence );
protected:
    static int class_counter;
};


class SQ_GroupData_PRO: public SQ_GroupData_Impl<20> {
    SQ_GroupData_PRO ( const SQ_GroupData_PRO& other );         // copying not allowed
public:
    SQ_GroupData_PRO() {}

    SQ_GroupData_PRO *clone() const {
        return new SQ_GroupData_PRO;
    }
    SQ_GroupData_PRO& operator= ( const SQ_GroupData& other ) {
        return static_cast<SQ_GroupData_PRO&> ( SQ_GroupData_Impl<20>::operator=
                                                ( static_cast<const SQ_GroupData_Impl<20>& > ( other ) ) );
    }

    consensus_result SQ_calc_consensus ( const char *sequence ) const;
    void   SQ_add_sequence ( const char *sequence );
};


// -----------------------
//      implementation
// -----------------------


template <int I>
SQ_GroupData_Impl<I>::~SQ_GroupData_Impl() {
    delete [] consensus;
}


template <int I>
void SQ_GroupData_Impl<I>::SQ_init_consensus ( int size_ ) {
    seq_assert ( !initialized );

    size        = size_;
    consensus   = new Int<I>[size];
    initialized = true;
}


template <int I>
void SQ_GroupData_Impl<I>::SQ_add ( const SQ_GroupData& other_base ) {
    const SQ_GroupData_Impl<I>& other = dynamic_cast<const SQ_GroupData_Impl<I>&> ( other_base );
    seq_assert ( size==other.size );
    for ( int i = 0; i<size; ++i ) {
        consensus[i] += other.consensus[i];
    }
    nr_sequences+=other.nr_sequences;
    avg_bases+=other.avg_bases;
    gc_prop += other.gc_prop;
}


template <int I>
int SQ_GroupData_Impl<I>::SQ_print_on_screen() {
    for ( int i=0; i < size; i++ ) {
        for ( int j = 0; j<I; j++ ) {
            std::cout << consensus[i].i[j];
        }
    }
    return ( 0 );
}

#else
#error SQ_GroupData.h included twice
#endif
