//  ==================================================================== //
//                                                                       //
//    File      : path_code.h                                            //
//    Purpose   : node-path en/decoding                                  //
//    Note      : include only once in each executable!!!                //
//    Time-stamp: <Tue Oct/05/2004 19:30 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in October 2003          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef PATH_CODE_H
#define PATH_CODE_H

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define pc_assert(cond) arb_assert(cond)

#define MAXNODEPATH 1024

    // Note: When changing encoding please adjust
    //       encodePath() in ./PROBE_WEB/CLIENT/TreeNode.java

    const char *encodePath(const char *path, int pathlen, GB_ERROR& error) {
        // path contains strings like 'LRLLRRL'
        // 'L' is interpreted as 0 ('R' as 1)
        // (paths containing 0 and 1 are ok as well)

        pc_assert(!error);
        pc_assert(strlen(path) == (unsigned)pathlen);

        if (!pathlen) return "0000"; // encoding for root node

        static char buffer[MAXNODEPATH+1];
        sprintf(buffer, "%04X", pathlen);
        int         idx = 4;

        while (pathlen>0 && !error) {
            int halfbyte = 0;
            for (int p = 0; p<4 && !error; ++p, --pathlen) {
                halfbyte <<= 1;
                if (pathlen>0) {
                    char c = toupper(*path++);
                    pc_assert(c);

                    if (c == 'R' || c == '1') halfbyte |= 1;
                    else if (c != 'L' && c != '0') {
                        error = GBS_global_string("Illegal character '%c' in path", c);
                    }
                }
            }

            pc_assert(halfbyte >= 0 && halfbyte <= 15);
            buffer[idx++] = "0123456789ABCDEF"[halfbyte];
        }

        if (error) return 0;

        buffer[idx] = 0;
        return buffer;
    }

    const char *decodeTable[256];
    bool  decodeTableInitialized = false;

    void initDecodeTable() {
        for (int i = 0; i<256; ++i) decodeTable[i] = 0;

        char decoded[5] = "LLLL";
        GB_ERROR error  = 0;

        for (int i = 0; i<16; ++i) {
            const char *enc = encodePath(decoded, 4, error);
            pc_assert(!error);

            decodeTable[(unsigned char)enc[4]] = strdup(decoded);

            for (int j = 3; j >= 0; --j) { // permutate decoded path
                if (decoded[j] == 'L') {
                    decoded[j] = 'R';
                    break;
                }
                decoded[j] = 'L';
            }
        }

        decodeTableInitialized = true;

#if defined(DEBUG) && 0
        printf("decodeTable:\n");
        for (int i = 0; i<256; ++i) {
            if (decodeTable[i]) {
                printf("decodeTable[%c]=%s\n", (char)i, decodeTable[i]);
            }
        }
#endif // DEBUG
    }

    const char *decodePath(const char *encodedPath, GB_ERROR& error) {
        // does the opposite of encodePath

        static char buffer[MAXNODEPATH+1];

        pc_assert(decodeTableInitialized);

        memcpy(buffer, encodedPath, 4);
        buffer[4] = 0;

        char *end  = 0;
        long  bits = strtoul(buffer, &end, 16);

        if (end == (buffer+4) && bits<MAXNODEPATH) {
            char       *bp = buffer;
            const char *pp = encodedPath+4;

            while (bits > 0) {
                const char *dec = decodeTable[(unsigned char)*pp++];
                if (!dec) {
                    error = GBS_global_string("Illegal char '%c' in path ('%s')", *pp, encodedPath);
                    return 0;
                }

                memcpy(bp, dec, 4);

                bp   += 4;
                bits -= 4;
            }

            if (bits < 0) { // too many chars written to decoded path
                bp += bits;
            }
            *bp = 0;

            error = 0;
            return buffer;
        }

        error = "cannot decode path";
        return 0;
    }



#else
#error path_code.h included twice
#endif // PATH_CODE_H

