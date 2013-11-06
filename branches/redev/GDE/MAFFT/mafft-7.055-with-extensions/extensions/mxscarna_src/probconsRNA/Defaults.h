/////////////////////////////////////////////////////////////////
// Defaults.h
//
// Default constants for use in PROBCONS.  The emission
// probabilities were computed using the program used to build
// the BLOSUM62 matrix from the BLOCKS 5.0 dataset.  Transition
// parameters were obtained via unsupervised EM training on the
// BALIBASE 2.0 benchmark alignment database.
/////////////////////////////////////////////////////////////////

#ifndef DEFAULTS_H
#define DEFAULTS_H

#include <string>

using namespace std;
/* Default */
namespace MXSCARNA {
/*
float initDistrib1Default[] = { 0.9588437676f, 0.0205782652f, 0.0205782652f };
float gapOpen1Default[] = { 0.0190259293f, 0.0190259293f };
float gapExtend1Default[] = { 0.3269913495f, 0.3269913495f };
*/

/* EMtrainingALL.txt*/
float initDistrib1Default[] = { 0.9234497547, 0.0385021642, 0.0385021642 };
float gapOpen1Default[] = { 0.0266662259, 0.0266662259 };
float gapExtend1Default[] = { 0.3849118352, 0.3849118352 };


float initDistrib2Default[] = { 0.9615409374f, 0.0000004538f, 0.0000004538f, 0.0192291681f, 0.0192291681f };
float gapOpen2Default[] = { 0.0082473317f, 0.0082473317f, 0.0107844425f, 0.0107844425f };
float gapExtend2Default[] = { 0.3210460842f, 0.3210460842f, 0.3298229277f, 0.3298229277f };

string alphabetDefault = "ACGUTN";

//float emitSingleDefault[6] = {
//   0.2174750715, 0.2573366761, 0.3005372882, 0.2233072966, 0.2233072966, 0.0004049665
//};

/* Default */
/*
float emitSingleDefault[6] = {
  0.2270790040f, 0.2422080040f, 0.2839320004f, 0.2464679927f, 0.2464679927f, 0.0003124650f 
};
*/

/* EMtrainingALL.txt */
float emitSingleDefault[6] = {
    0.2017124593, 0.2590311766, 0.2929603755, 0.2453189045, 0.2453189045, 0.0000873194 };

/* ACGUTN */
/* Default */
/*
float emitPairsDefault[6][6] = {
  { 0.1487240046f, 0.0184142999f, 0.0361397006f, 0.0238473993f, 0.0238473993f, 0.0000375308f },
  { 0.0184142999f, 0.1583919972f, 0.0275536999f, 0.0389291011f, 0.0389291011f, 0.0000815823f },
  { 0.0361397006f, 0.0275536999f, 0.1979320049f, 0.0244289003f, 0.0244289003f, 0.0000824765f },
  { 0.0238473993f, 0.0389291011f, 0.0244289003f, 0.1557479948f, 0.1557479948f, 0.0000743985f },
  { 0.0238473993f, 0.0389291011f, 0.0244289003f, 0.1557479948f, 0.1557479948f, 0.0000743985f },
  { 0.0000375308f, 0.0000815823f, 0.0000824765f, 0.0000743985f, 0.0000743985f, 0.0000263252f }
};
*/
/* EMtrainingALL.txt */
float emitPairsDefault[6][6] = {
    { 0.1659344733, 0.0298952684, 0.0543937907, 0.0344539173, 0.0344539173, 0.0000032761 },
    { 0.0298952684, 0.1817403436, 0.0415624641, 0.0589077808, 0.0589077808, 0.0000117011 },
    { 0.0543937907, 0.0415624641, 0.2342105955, 0.0410407558, 0.0410407558, 0.0000072893 },
    { 0.0344539173, 0.0589077808, 0.0410407558, 0.1578272283, 0.1578272283, 0.0000067871 },
    { 0.0344539173, 0.0589077808, 0.0410407558, 0.1578272283, 0.1578272283, 0.0000067871 },
    { 0.0344539173, 0.0589077808, 0.0410407558, 0.1578272283, 0.1578272283, 0.0000067871 },
//    { 0.0000032761, 0.0000117011, 0.0000072893, 0.0000067871, 0.0000067871, 0.0000000166 } 
};

  /*
float emitPairsDefault[6][6] = {
    {0.1731323451, 0.0378843173, 0.0656677559, 0.0450690985, 0.0450690985, 0.0000215275},
    {0.0378843173, 0.1611578614, 0.0492933467, 0.0651549697, 0.0651549697, 0.0000362353},
    {0.0656677559, 0.0492933467, 0.1937607974, 0.0464556068, 0.0464556068, 0.0000293904},
    {0.0450690985, 0.0651549697, 0.0464556068, 0.1622997671, 0.1622997671, 0.0000352637},
    {0.0450690985, 0.0651549697, 0.0464556068, 0.1622997671, 0.1622997671, 0.0000352637},
    {0.0000215275, 0.0000362353, 0.0000293904, 0.0000352637, 0.0000352637, 0.0000000000}
};
  */
}
#endif
