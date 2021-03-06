#ifndef PS_BITSET_HXX
#define PS_BITSET_HXX

#ifndef __SET__
#include <set>
#endif
#ifndef PS_FILEBUFFER_HXX
#include "ps_filebuffer.hxx"
#endif

class PS_BitSet : virtual Noncopyable {
protected:
    unsigned char *data;

    bool bias;      // preset value for newly allocated memory
    long max_index; // max. used index
    long capacity;  // number of allocated bits

    PS_BitSet(const bool _bias, const long _max_index, const long _capacity)
        : data(NULL),
          bias(_bias),
          max_index(_max_index),
          capacity(_capacity)
    {}

public:
    typedef set<long> IndexSet;

            long getTrueIndices(IndexSet &_index_set, const long _fill_index);
            long getFalseIndices(IndexSet &_index_set, const long _fill_index);
            long getCountOfTrues(const long _fill_index = -1);
    
            long getMaxUsedIndex() const { return max_index; }
            long getCapacity() const { return capacity; }

    virtual bool Get(const long _index);
    virtual bool Set(const long _index, const bool _value);

    virtual void setTrue(const long _index);
    virtual void setFalse(const long _index);

            void invert();
            void x_or(const PS_BitSet *_other_set);

            void print(FILE *out, const bool _header, const long _fill_index);
            bool save(PS_FileBuffer *_file);
            bool load(PS_FileBuffer *_file, const long _fill_index);

            bool copy(const PS_BitSet *_other_bitset);
    virtual bool reserve(const long _capacity);

    explicit PS_BitSet(const bool _bias, const long _capacity) : bias(_bias), max_index(-1), capacity(0) {
        data = 0;
        reserve(_capacity);
    }

    explicit PS_BitSet(PS_FileBuffer *_file, const long _fill_index = -1) : bias(false), max_index(-1), capacity(0) {
        data = 0;
        load(_file, _fill_index);
    }

    virtual ~PS_BitSet() {
        if (data) free(data);
    }
};

//  ############################################################
//  # PS_BitSet_Fast
//  ############################################################
class PS_BitSet_Fast : public PS_BitSet {
private:

    PS_BitSet_Fast(const PS_BitSet_Fast&);
    PS_BitSet_Fast();

public:

    bool Get(const long _index) OVERRIDE;
    bool Set(const long _index, const bool _value) OVERRIDE;

    void setTrue(const long _index) OVERRIDE;
    void setFalse(const long _index) OVERRIDE;

    bool reserve(const long _capacity) OVERRIDE;

    explicit PS_BitSet_Fast(PS_FileBuffer *_file, const long _fill_index = -1) : PS_BitSet(false, -1, 0) {
        data = 0;
        load(_file, _fill_index);
    }

    explicit PS_BitSet_Fast(bool _bias, long _capacity) : PS_BitSet(_bias, _capacity) {
        // we just altered member functions so we don't do anything here
        // but call correct base class constructor to prevent
        // the call of PS_BitSet() without parameters
    }
};


long PS_BitSet::getFalseIndices(PS_BitSet::IndexSet &_index_set, const long _fill_index = -1) {
    _index_set.clear();
    // get indices of falses from bitset up to max_index
    long index      = 0;
    long byte_index = 0;
    while (index<=max_index) {
        unsigned char byte = data[byte_index++];                                      // get a data byte
        for (int i = 0; i<8 && index<=max_index; ++i) {
            if (!(byte&1)) _index_set.insert(index);
            ++index;
            byte >>= 1;
        }
    }
    ps_assert(index == max_index+1);
    // append indices [max_index+1 .. _fill_index] if bias is set to false
    if (!bias) {
        for (; (index <= _fill_index); ++index) {
            _index_set.insert(index);
        }
    }
    return _index_set.size();
}


long PS_BitSet::getTrueIndices(PS_BitSet::IndexSet &_index_set, const long _fill_index = -1) {
    _index_set.clear();
    // get indices of trues from bitset up to max_index
    long index      = 0;
    long byte_index = 0;
    while (index<=max_index) {
        unsigned char byte  = data[byte_index++];       // get a data byte
        for (int i = 0; i<8 && index<=max_index; ++i) {
            if (byte&1) _index_set.insert(index);
            ++index;
            byte >>= 1;
        }
    }
    ps_assert(index == max_index+1);
    // append indices [max_index+1 .. _max_index] if bias is set to true
    if (bias) {
        for (; (index <= _fill_index); ++index) {
            _index_set.insert(index);
        }
    }
    return _index_set.size();
}


long PS_BitSet::getCountOfTrues(const long _fill_index) {
    long count = 0;
    // get indices of trues from bitset up to max_index
    long index      = 0;
    long byte_index = 0;
    while (index<=max_index) {
        unsigned char byte  = data[byte_index++];       // get a data byte
        for (int i = 0; i<8 && index<=max_index; ++i) {
            if (byte&1) ++count;
            ++index;
            byte >>= 1;
        }
    }
    ps_assert(index == max_index+1);
    // append indices [max_index+1 .. _max_index] if bias is set to true
    if (bias && (_fill_index > max_index)) {
        count += _fill_index-max_index + 1;
    }
    return count;
}


bool PS_BitSet::Set(const long _index, const bool _value) {
    ps_assert(_index >= 0);
    reserve(_index);
    bool previous_value = (((data[_index/8] >> (_index % 8)) & 1) == 1);
    if (_value) {
        data[_index/8] |= 1 << (_index % 8);
    }
    else {
        data[_index/8] &= ~(1 << (_index % 8));
    }
    if (_index > max_index) max_index = _index;
    return previous_value;
}


void PS_BitSet::setTrue(const long _index) {
    ps_assert(_index >= 0);
    reserve(_index);
    data[_index/8] &= 1 << (_index % 8);
    if (_index > max_index) max_index = _index;
}


void PS_BitSet::setFalse(const long _index) {
    ps_assert(_index >= 0);
    reserve(_index);
    data[_index/8] &= ~(1 << (_index % 8));
    if (_index > max_index) max_index = _index;
}


bool PS_BitSet::Get(const long _index) {
    ps_assert(_index >= 0);
    reserve(_index);
    return (((data[_index/8] >> (_index % 8)) & 1) == 1);
}


bool PS_BitSet::copy(const PS_BitSet *_other_bitset) {
    bias = _other_bitset->bias;
    if (!reserve(_other_bitset->capacity)) return false;
    memcpy(data, _other_bitset->data, (capacity>>3));
    max_index = _other_bitset->max_index;
    return true;
}


bool PS_BitSet::reserve(const long _capacity) {
    unsigned char *new_data;
    long new_capacity_bytes = (_capacity/8)+1;
    long old_capacity_bytes = (capacity/8)+1;
    if (capacity > 0) {
        if (new_capacity_bytes <= old_capacity_bytes) return true;      // smaller or same size requested ?
    }
    new_capacity_bytes = ((new_capacity_bytes / 32)+1)*32;              // adjust requested size to bigger chunks
    new_data = (unsigned char *)malloc(new_capacity_bytes);             // get new memory
    if (new_data == 0) return false;
    memset(new_data, bias ? 0xFF : 0, new_capacity_bytes);              // set memory to bias value
    if (capacity > 0) memcpy(new_data, data, old_capacity_bytes);       // copy old values
    if (data) free(data);                                               // free old memory
    data     = new_data;                                                // store new pointer
    capacity = new_capacity_bytes*8;                                    // store new capacity
    return true;
}


void PS_BitSet::invert() {
    for (long i = 0; i < capacity/8; ++i) {
        data[i] = ~data[i];
    }
}


void PS_BitSet::x_or(const PS_BitSet *_other_set) {
    for (long i = 0; i < capacity/8; ++i) {
        data[i] ^= _other_set->data[i];
    }
}


void PS_BitSet::print(FILE *out, const bool _header = true, const long _fill_index = -1) {
    if (_header) fprintf(out, "PS_BitSet: bias(%1i) max_index(%6li) capacity(%6li) ", bias, max_index, capacity);
    for (long i = 0; i <= max_index; ++i) {
        fputc(Get(i) ? '+' : '_', out);
    }
    for (long i = max_index+1; i <= _fill_index; ++i) {
        fputc('.', out);
    }
    fprintf(out, " %li\n", max_index);
}


bool PS_BitSet::save(PS_FileBuffer *_file) {
    if (_file->isReadonly()) return false;
    // save max_index
    _file->put_long(max_index);
    // save bias
    _file->put_char((bias) ? '1' : '0');
    // save bitset
    long bytes = (max_index / 8)+1;
    long i = 0;
    while ((bytes-i) >= PS_FileBuffer::BUFFER_SIZE) {
        _file->put(&(data[i]), PS_FileBuffer::BUFFER_SIZE);
        i += PS_FileBuffer::BUFFER_SIZE;
    }
    _file->put(&(data[i]), (bytes-i));
    return true;
}


bool PS_BitSet::load(PS_FileBuffer *_file, const long _fill_index = -1) {
    // load max_index
    _file->get_long(max_index);
    // load bias
    bias = (_file->get_char() == '1');
    // initialize bitset
    capacity = 0;
    if (!reserve((max_index > _fill_index) ? max_index : _fill_index)) return false;
    // load bitset
    long bytes = (max_index / 8)+1;
    long i = 0;
    while ((bytes-i) >= PS_FileBuffer::BUFFER_SIZE) {
        _file->get(&(data[i]), PS_FileBuffer::BUFFER_SIZE);
        i += PS_FileBuffer::BUFFER_SIZE;
    }
    _file->get(&(data[i]), (bytes-i));
    return true;
}


bool PS_BitSet_Fast::Get(const long _index) {
    if (_index >= capacity) {
        printf("PS_BitSet_Fast::get( %li ) exceeds capacity %li\n", _index, capacity);
        ps_assert(0);
        return false;
    }
    if (_index > max_index) max_index = _index;
    return (((data[_index/8] >> (_index % 8)) & 1) == 1);
}


bool PS_BitSet_Fast::Set(const long _index, const bool _value) {
    if (_index >= capacity) {
        printf("PS_BitSet_Fast::set( %li,%1i ) exceeds capacity %li\n", _index, _value, capacity);
        ps_assert(0);
        return false;
    }
    bool previous_value = (((data[_index/8] >> (_index % 8)) & 1) == 1);
    if (_value) {
        data[_index/8] |= 1 << (_index % 8);
    }
    else {
        data[_index/8] &= ~(1 << (_index % 8));
    }
    if (_index > max_index) max_index = _index;
    return previous_value;
}


void PS_BitSet_Fast::setTrue(const long _index) {
    if (_index >= capacity) {
        printf("PS_BitSet_Fast::setTrue( %li ) exceeds capacity %li\n", _index, capacity);
        ps_assert(0);
        return;
    }
    data[_index/8] |= 1 << (_index % 8);
    if (_index > max_index) max_index = _index;
}


void PS_BitSet_Fast::setFalse(const long _index) {
    if (_index >= capacity) printf("PS_BitSet_Fast::setFalse( %li ) exceeds capacity %li\n", _index, capacity);
    data[_index/8] &= ~(1 << (_index % 8));
    if (_index > max_index) max_index = _index;
}


//   as i assume that a user of the _FAST variant knows how much
//   data will be stored in the set we don't adjust the given
//   capacity to bigger chunks as PS_BitSet::reserve does
bool PS_BitSet_Fast::reserve(const long _capacity) {
    unsigned char *new_data;
    long new_capacity_bytes = (_capacity/8)+1;
    long old_capacity_bytes = (capacity/8)+1;
    if (capacity > 0) {
        if (new_capacity_bytes <= old_capacity_bytes) return true;      // smaller or same size requested ?
    }
    new_data = (unsigned char *)malloc(new_capacity_bytes);             // get new memory
    if (new_data == 0) return false;
    memset(new_data, bias ? 0xFF : 0, new_capacity_bytes);              // set memory to bias value
    if (capacity > 0) memcpy(new_data, data, old_capacity_bytes);       // copy old values
    if (data) free(data);                                               // free old memory
    data     = new_data;                                                // store new pointer
    capacity = new_capacity_bytes*8;                                    // store new capacity
    return true;
}
#else
#error ps_bitset.hxx included twice
#endif
