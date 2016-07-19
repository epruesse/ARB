#include <MultiProbe.hxx>
#include <cmath>

enum Bitpos {
    bit1 = 1,
    bit2 = 2,
    bit3 = 4,
    bit4 = 8,
    bit5 = 16,
    bit6 = 32,
    bit7 = 64,
    bit8 = 128
};

Bitvector::Bitvector(int bits)
{
    int i;

    num_of_bits = bits;
    len =  (bits%8) ? (bits/8+1) : (bits/8);
    vector = new char[len];

    for (i=0; i<len; i++)
        vector[i] = vector[i] & 0;
}

Bitvector::~Bitvector()
{
    delete [] vector;
}


int Bitvector::gen_id()
{
    // generiere eine int Nummer fuer die Farbe aus dem Bitvector
    long num=0;
    for (int i=0; i<num_of_bits; i++)
        num += (int)(.5 + pow(2, i)*readbit(i));
    return num;
}
Bitvector* Bitvector::merge(Bitvector* x)
{
    int lthis, lx, lback;
    int i;                              // Zaehler

    lthis = num_of_bits;
    lx    = x->get_num_of_bits();
    lback = (lthis>lx) ? lthis : lx;
    Bitvector* back = new Bitvector(lback);

    for (i=0; i<lback; i++)
        if (this->readbit(i) || x->readbit(i))
            back->setbit (i);

    return back;
}

int Bitvector::subset(Bitvector* Obermenge)
{
    char* vector2 = Obermenge->get_vector();

    for (int i=0; i<len; i++)
    {
        if ((vector[i] & vector2[i]) != vector[i])
            return 0;
    }
    return 1;
}


void Bitvector::rshift()
{
    long gemerkt=0;

    if (readbit(num_of_bits-1))
        gemerkt=1;

    for (int i=len-1; i>-1;  i--)
    {
        vector[i] = vector[i] << 1;
        if (readbit(8*i-1))
            setbit(8*i);
    }
    if (gemerkt == 1)
        setbit(0);
}

void Bitvector::print()
{
    int i;
    printf("Bitvektor:   (");
    for (i=0; i<num_of_bits; i++)
        printf("%d", readbit(i));
    printf(")\n");
}

int Bitvector::setbit(int pos)
{
    int byte, idx, bitcode;
    if (pos > num_of_bits)
        return -1;
    byte = pos/8;
    idx = pos - byte*8;
    bitcode = (int) pow(2, idx);
    vector[byte] = vector[byte] | bitcode;
    return 0;
}

int Bitvector::delbit(int pos)
{
    int byte, idx, bitcode;
    if (pos > num_of_bits)
        return -1;
    byte = pos/8;
    idx = pos - byte*8;
    bitcode = (int) pow(2, idx);
    if (readbit(pos))
        vector[byte] = vector[byte] ^ bitcode;
    return 0;
}

int Bitvector::readbit(int pos)
{
    int byte, idx, bitcode;
    if (pos > num_of_bits)
        return 0;
    byte = pos/8;
    idx = pos - byte*8;
    bitcode = (int) pow(2, idx);
    if (vector[byte] & bitcode)
        return 1;
    else
        return 0;
}

