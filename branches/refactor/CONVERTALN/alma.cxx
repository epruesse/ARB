#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "convert.h"
#include "global.h"

extern int warning_out;

/* --------------------------------------------------------------
 *   Function init_alma().
 *       Init. ALMA data.
 */
void init_alma()
{
    Freespace(&(data.alma.id));
    Freespace(&(data.alma.filename));
    Freespace(&(data.alma.sequence));
    data.alma.format = UNKNOWN;
    data.alma.defgap = '-';
    data.alma.num_of_sequence = 0;
    /* init. nbrf too */
    Freespace(&(data.nbrf.id));
    Freespace(&(data.nbrf.description));
}

/* ----------------------------------------------------------------
 *   Function alma_to_macke().
 *       Convert from ALMA format to Macke format.
 */
void alma_to_macke(char *inf, char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);
    
    int indi, total_num;

    init();
    /* macke format seq irrelevant header */
    macke_out_header(ofp);
    for (indi = 0; indi < 3; indi++) {
        FILE_BUFFER_rewind(ifp);
        init_seq_data();
        init_macke();
        init_alma();
        while (alma_in(ifp) != EOF) {
            data.numofseq++;
            if (atom()) {
                /* convert from alma form to macke form */
                switch (indi) {
                    case 0:
                        /* output seq display format */
                        macke_out0(ofp, ALMA);
                        break;
                    case 1:
                        /* output seq information */
                        macke_out1(ofp);
                        break;
                    case 2:
                        /* output seq data */
                        macke_out2(ofp);
                        break;
                    default:;
                }
            }
            else
                throw_error(47, "Conversion from alma to macke fails");
            init_alma();
            init_macke();
        }
        total_num = data.numofseq;
        if (indi == 0) {
            fprintf(ofp, "#-\n");
            /* no warning messages for next loop */
            warning_out = 0;
        }
    }
    warning_out = 1;            /* resume warning messages */

#ifdef log
    fprintf(stderr, "Total %d sequences have been processed\n", total_num);
#endif
}

/* ----------------------------------------------------------------
 *   Function alma_to_genbank().
 *       Convert from ALMA format to GenBank format.
 */
void alma_to_genbank(char *inf, char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    init();
    init_seq_data();
    init_macke();
    init_genbank();
    init_alma();
    while (alma_in(ifp) != EOF) {
        data.numofseq++;
        if (atom() && mtog()) {
            genbank_out(ofp);
            init_alma();
            init_macke();
            init_genbank();
        }
    }

#ifdef log
    fprintf(stderr, "Total %d sequences have been processed\n", data.numofseq);
#endif
}

/* ----------------------------------------------------------------
 *   Function alma_in().
 *       Read in one ALMA sequence data.
 */
char alma_in(FILE_BUFFER fp)
{
    char eoen, *eof, line[LINENUM];
    char key[TOKENNUM], *format;
    int indi, len, index;

    /* end-of-entry; set to be 'y' after reaching end of entry */
    format = NULL;
    eoen = ' ';
    for (eof = Fgetline(line, LINENUM, fp); eof != NULL && eoen != 'y';) {
        if (Lenstr(line) <= 1) {
            eof = Fgetline(line, LINENUM, fp);
            continue;           /* skip empty line */
        }
        index = alma_key_word(line, 0, key, TOKENNUM);
        eoen = 'n';
        if (str_equal(key, "NXT ENTRY")) {
            /* do nothing, just indicate beginning of entry */
        }
        else if (str_equal(key, "ENTRY ID")) {
            alma_one_entry(line, index, &(data.alma.id));
            for (indi = 0, len = Lenstr(data.alma.id); indi < len; indi++)
                if (data.alma.id[indi] == ' ')
                    data.alma.id[indi] = '_';
        }
        else if (str_equal(key, "SEQUENCE")) {
            alma_one_entry(line, index, &(data.alma.filename));
        }
        else if (str_equal(key, "FORMAT")) {
            alma_one_entry(line, index, &format);
            if (str_equal(format, "NBRF"))
                data.alma.format = NBRF;
            else if (str_equal(format, "UWGCG") || str_equal(format, "GCG"))
                data.alma.format = GCG;
            else if (str_equal(format, "EMBL"))
                data.alma.format = EMBL;
            else if (str_equal(format, "STADEN"))
                data.alma.format = STADEN;
            else {
                throw_errorf(49, "Unidentified file format %s in ALMA format", format);
            }
            Freespace(&(format));
        }
        else if (str_equal(key, "DEFGAP")) {
            /* skip white spaces */
            for (; line[index] != '[' && line[index] != '\n' && line[index] != '\0'; index++) ;
            if (line[index] == '[')
                data.alma.defgap = line[index + 1];
        }
        else if (str_equal(key, "GAPS")) {
            eof = alma_in_gaps(fp);
            eoen = 'y';
        }
        else if (str_equal(key, "END FILE")) {
            return (EOF);
        }
        if (eoen != 'y')
            eof = Fgetline(line, LINENUM, fp);
    }
    if (eof == NULL)
        return (EOF);
    else
        return (EOF + 1);
}

/* ----------------------------------------------------------------
 *   Function alma_key_word().
 *       Get the key_word from line beginning at index.
 */
int alma_key_word(char *line, int index, char *key, int length)
{
    int indi, indj;

    if (line == NULL) {
        key[0] = '\0';
        return (index);
    }
    for (indi = index, indj = 0; (index - indi) < length && line[indi] != '>' && line[indi] != '\n' && line[indi] != '\0'; indi++, indj++)
        key[indj] = line[indi];
    key[indj] = '\0';
    if (line[indi] == '>')
        return (indi + 1);
    else
        return (indi);
}

/* --------------------------------------------------------------
 *   Function alma_one_entry().
 *       Read in one ALMA entry lines.
 */
void alma_one_entry(char *line, int index, char **datastring)
{
    int indi, length;

    index = Skip_white_space(line, index);

    for (indi = index, length = Lenstr(line + index) + index; indi < length; indi++)
        if (line[indi] == '\n')
            line[indi] = '\0';

    replace_entry(datastring, line + index);
}

/* -------------------------------------------------------------
 *   Function alma_in_gaps().
 *       Get sequence data and merge with gaps(alignment).
 */
char *alma_in_gaps(FILE_BUFFER fp) {
    int   return_num, gaps, residues, count = 0;
    int   indi, numofseq, bases_not_matching;
    char  line[LINENUM], gap_chars[LINENUM];
    char *eof;

    alma_in_sequence();

    numofseq = 0;

    bases_not_matching = 0;
    /* alignment, merger with gaps information */
    {
        while (1) {
            char *gotLine = Fgetline(line, LINENUM, fp);

            if (!gotLine)
                break;

            return_num = sscanf(line, "%d %d %s", &gaps, &residues, gap_chars);
            if (return_num == 0 || gaps == -1) {
                FILE_BUFFER_back(fp, line);
                break;
            }

            data.alma.sequence = (char *)Reallocspace(data.alma.sequence, (unsigned)(sizeof(char) * (numofseq + gaps + 1)));

            for (indi = 0; indi < gaps; indi++)
                data.alma.sequence[numofseq + indi] = gap_chars[1];

            numofseq += gaps;

            if (residues > (data.seq_length - count))
                bases_not_matching = 1;

            data.alma.sequence = (char *)Reallocspace(data.alma.sequence, (unsigned)(sizeof(char) * (numofseq + residues + 1)));

            /* fill with gap if seq data is not enough as required */
            for (indi = 0; indi < residues; indi++) {
                if (count >= data.seq_length)
                    data.alma.sequence[numofseq + indi] = gap_chars[1];
                else
                    data.alma.sequence[numofseq + indi] = data.sequence[count++];
            }

            numofseq += residues;

        }
    }

    if (bases_not_matching) {
        warningf(142, "Bases number in ALMA file is larger than that in data file %s", data.alma.filename);
    }

    if (count < data.seq_length) {
        warningf(143, "Bases number in ALMA file is less than that in data file %s", data.alma.filename);

        /* Append the rest of the data at the end */
        data.alma.sequence = (char *)Reallocspace(data.alma.sequence, (unsigned)(sizeof(char) * (numofseq + data.seq_length - count + 1)));

        for (indi = 0; count < data.seq_length; indi++)
            data.alma.sequence[numofseq++] = data.sequence[count++];
    }
    data.alma.sequence[numofseq] = '\0';

    /* update sequence data */
    if (numofseq > data.max) {
        data.max = numofseq;
        data.sequence = (char *)Reallocspace(data.sequence, (unsigned)(sizeof(char) * data.max));
    }

    data.seq_length = numofseq;

    for (indi = 0; indi < numofseq; indi++)
        data.sequence[indi] = data.alma.sequence[indi];

    data.alma.num_of_sequence = numofseq;

    if (return_num != 0) {
        /* skip END ENTRY> line */
        if (Fgetline(line, LINENUM, fp) == NULL)
            eof = NULL;
        else
            eof = line;
    }
    else
        eof = NULL;

    return (eof);
}

/* -------------------------------------------------------------
 *   Function alma_in_sequence().
 *       Read in sequence data.
 */
void alma_in_sequence() {
    FILE        *IFP = open_input_or_die(data.alma.filename);
    FILE_BUFFER  ifp = create_FILE_BUFFER(data.alma.filename, IFP);

    if (data.alma.format == NBRF) {
        nbrf_in(ifp);
    }
    else if (data.alma.format == GCG) {
        gcg_in(ifp);
    }
    else if (data.alma.format == EMBL) {
        init_embl();
        embl_in(ifp);
    }
    else if (data.alma.format == STADEN) {
        staden_in(ifp);
    }
    else {
        throw_errorf(50, "Unidentified file format %d in ALMA file", data.alma.format);
    }
    destroy_FILE_BUFFER(ifp);
}

/* --------------------------------------------------------------
 *   Function nbrf_in().
 *       Read in nbrf data.
 */
void nbrf_in(FILE_BUFFER fp)
{
    int length, index, reach_end;
    char line[LINENUM], temp[TOKENNUM], *eof;

    if ((eof = Fgetline(line, LINENUM, fp)) == NULL)
        throw_error(52, "Cannot find id line in NBRF file");
    Cpystr(temp, line + 4);
    length = Lenstr(temp);
    if (temp[length - 1] == '\n')
        temp[length - 1] = '\0';

    replace_entry(&(data.nbrf.id), temp);

    if ((eof = Fgetline(line, LINENUM, fp)) == NULL)
        throw_error(54, "Cannot find description line in NBRF file");

    replace_entry(&(data.nbrf.description), line);

    /* read in sequence data */
    data.seq_length = 0;
    for (eof = Fgetline(line, LINENUM, fp), reach_end = 'n'; eof != NULL && reach_end != 'y'; eof = Fgetline(line, LINENUM, fp)) {
        for (index = 0; line[index] != '\n' && line[index] != '\0'; index++) {
            if (line[index] == '*') {
                reach_end = 'y';
                continue;
            }
            if (line[index] != ' ' && data.seq_length >= data.max) {
                data.max += 100;

                data.sequence = (char *)Reallocspace(data.sequence, (unsigned)(sizeof(char) * data.max));
            }
            if (line[index] != ' ')
                data.sequence[data.seq_length++]
                    = line[index];
        }                       /* scan through each line */
        data.sequence[data.seq_length] = '\0';
    }                           /* for each line */
}

/* ----------------------------------------------------------------
 *   Function gcg_in().
 *       Read in one data entry in UWGCG format.
 */
void gcg_in(FILE_BUFFER fp)
{
    char line[LINENUM];

    while (Fgetline(line, LINENUM, fp)) {
        char *two_dots = strstr(line, "..");

        if (two_dots) {
            FILE_BUFFER_back(fp, two_dots + 2);
            genbank_origin(line, fp);
        }
    }
}

/* ----------------------------------------------------------------
 *   Function staden_in().
 *       Read in sequence data in STADEN format.
 */
void staden_in(FILE_BUFFER fp)
{
    char line[LINENUM];

    int len, start, indi;

    data.seq_length = 0;
    while (Fgetline(line, LINENUM, fp) != NULL) {
        /* skip empty line */
        if ((len = Lenstr(line)) <= 1)
            continue;

        start = data.seq_length;
        data.seq_length += len;
        line[len - 1] = '\0';

        if (data.seq_length > data.max) {
            data.max += (data.seq_length + 100);
            data.sequence = (char *)Reallocspace(data.sequence, (unsigned)(sizeof(char) * data.max));
        }

        for (indi = 0; indi < len; indi++)
            data.sequence[start + indi] = line[indi];
    }
}

/* -------------------------------------------------------------
 *   Function atom().
 *       Convert one ALMA entry to Macke entry.
 */
int atom() {
    if (data.alma.format == NBRF) {
        data.macke.numofrem = 1;
        data.macke.remarks = (char **)Reallocspace((char *)data.macke.remarks, (unsigned)(sizeof(char *)));
        data.macke.remarks[0]
            = (char *)Dupstr(data.nbrf.description);
    }
    else if (data.alma.format == EMBL) {
        etom();
    }
    else if (data.alma.format == GCG) {
    }
    else if (data.alma.format == STADEN) {
    }
    else {
        throw_error(53, "Unidentified format type in ALMA file");
    }

    replace_entry(&(data.macke.seqabbr), data.alma.id);

    return (1);
}

/* -------------------------------------------------------------
 *   Function embl_to_alma().
 *       Convert from EMBL to ALMA.
 */
void embl_to_alma(char *inf, char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    init();
    init_embl();
    init_alma();
    alma_out_header(ofp);

    while (embl_in(ifp) != EOF) {
        if (data.numofseq > 0)
            fprintf(ofp, "\n");
        data.numofseq++;
        if (etoa()) {
            FILE *outfile = alma_out(ofp, EMBL);

            embl_out(outfile);
            fclose(outfile);
        }
        init_embl();
        init_alma();
    }

    fprintf(ofp, "END FILE>\n");

#ifdef log
    fprintf(stderr, "Total %d sequences have been processed\n", data.numofseq);
#endif

    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* ------------------------------------------------------------
 *   Function genbank_to_alma().
 *       Convert from GenBank to ALMA.
 */
void genbank_to_alma(char *inf, char *outf) {
    FILE        *IFP = open_input_or_die(inf);
    FILE_BUFFER  ifp = create_FILE_BUFFER(inf, IFP);
    FILE        *ofp = open_output_or_die(outf);

    init();
    init_genbank();
    init_embl();
    init_alma();
    alma_out_header(ofp);

    while (genbank_in(ifp) != EOF) {
        if (data.numofseq > 0)
            fprintf(ofp, "\n");
        data.numofseq++;
        if (gtoe() && etoa()) {
            FILE *outfile = alma_out(ofp, EMBL);

            embl_out(outfile);
            fclose(outfile);
        }
        init_genbank();
        init_embl();
        init_alma();
    }

    fprintf(ofp, "END FILE>\n");

#ifdef log
    fprintf(stderr, "Total %d sequences have been processed\n", data.numofseq);
#endif

    destroy_FILE_BUFFER(ifp);
    fclose(ofp);
}

/* -------------------------------------------------------------
 *   Function macke_to_alma().
 *       Convert from MACKE to ALMA.
 */
void macke_to_alma(char *inf, char *outf) {
    FILE        *IFP1 = open_input_or_die(inf);
    FILE        *IFP2 = open_input_or_die(inf);
    FILE        *IFP3 = open_input_or_die(inf);
    FILE_BUFFER  ifp1 = create_FILE_BUFFER(inf, IFP1);
    FILE_BUFFER  ifp2 = create_FILE_BUFFER(inf, IFP2);
    FILE_BUFFER  ifp3 = create_FILE_BUFFER(inf, IFP3);
    FILE        *ofp  = open_output_or_die(outf);

    init();
    init_macke();
    init_genbank();
    init_embl();
    init_alma();
    alma_out_header(ofp);
    while (macke_in(ifp1, ifp2, ifp3) != EOF) {
        if (data.numofseq > 0)
            fprintf(ofp, "\n");
        data.numofseq++;
        if (mtog() && gtoe() && partial_mtoe() && etoa()) {
            FILE *outfile = alma_out(ofp, EMBL);

            embl_out(outfile);
            fclose(outfile);
        }
        init_macke();
        init_genbank();
        init_embl();
        init_alma();
    }

    fprintf(ofp, "END FILE>\n");

#ifdef log
    fprintf(stderr, "Total %d sequences have been processed\n", data.numofseq);
#endif

    destroy_FILE_BUFFER(ifp1);
    destroy_FILE_BUFFER(ifp2);
    destroy_FILE_BUFFER(ifp3);
    fclose(ofp);
}

/* -------------------------------------------------------------
 *   Function etoa().
 *       Convert from EMBL to ALMA format.
 */
int etoa()
{
    char temp[TOKENNUM], t1[TOKENNUM], t2[TOKENNUM], t3[TOKENNUM];

    embl_key_word(data.embl.id, 0, temp, TOKENNUM);
    if (Lenstr(data.embl.dr) > 1) {
        /* get short_id from DR line if there is RDP def. */
        Cpystr(t3, "dummy");
        sscanf(data.embl.dr, "%s %s %s", t1, t2, t3);
        if (str_equal(t1, "RDP;")) {
            if (!str_equal(t3, "dummy")) {
                Cpystr(temp, t3);
            }
            else
                Cpystr(temp, t2);
            temp[Lenstr(temp) - 1] = '\0';      /* remove '.' */
        }
    }
    replace_entry(&(data.alma.id), temp);

    return (1);
}

/* -------------------------------------------------------------
 *   Function alma_out_header().
 *       Output alma format header.
 */
void alma_out_header(FILE * fp)
{
    fprintf(fp, "SCREEN WIDTH>   80\n");
    fprintf(fp, "ID COLUMN>    1\n");
    fprintf(fp, "LINK COLUMN>   10\n");
    fprintf(fp, "INFO LINE>   24\n");
    fprintf(fp, "ACTIVE WINDOW>    1    1\n");
    fprintf(fp, "SEQUENCE WINDOW>    1    1    1   23   12   80\n");
    fprintf(fp, "SCROLL DISTANCE>    1    1   50\n");
    fprintf(fp, "SEQ NUMBERS>    1    1    0    0\n");
    fprintf(fp, "CURSOR SCREEN>    1    1   11   54\n");
    fprintf(fp, "CURSOR ALIGNMENT>    1    1    9  143\n");
    fprintf(fp, "LINK ACTIVE>    2\n");
    fprintf(fp, "SCROLL>    2\n");
    fprintf(fp, "MULTI WINDOWS>F\n");
    fprintf(fp, "COLOURS>F\n");
    fprintf(fp, "UPDATE INT#>T\n");
    fprintf(fp, "HOMOLOGY>F\n");
    fprintf(fp, "PRINT>\n");
    fprintf(fp, "FILE_A>\n");
    fprintf(fp, "FILE_B>\n");
    fprintf(fp, "FILE_C>\n");
    fprintf(fp, "FILE_D>\n");
    fprintf(fp, "FILE_E>\n");
    fprintf(fp, "ALIGNMENT TITLE>\n");
    fprintf(fp, "PRINT TITLE>    0\n");
    fprintf(fp, "PRINT DEVICE>    0\n");
    fprintf(fp, "SPACERS TOP>    0\n");
    fprintf(fp, "FONT>    0\n");
    fprintf(fp, "FONT SIZE>    0\n");
    fprintf(fp, "PAGE WIDTH>    0\n");
    fprintf(fp, "PRINT FORMAT>    0\n");
    fprintf(fp, "PAGE LENGTH>    0\n");
    fprintf(fp, "SKIP LINES>    0\n");
    fprintf(fp, "COLOURING>    0\n");
    fprintf(fp, "COLOUR ACTIVE>    0    0    0    0    0    0    0    0    0    0\n");
    fprintf(fp, "RESIDUE B(LINK)>\n");
    fprintf(fp, "RESIDUE R(EVERSE)>\n");
    fprintf(fp, "RESIDUE I(NCREASE)>\n");
    fprintf(fp, "RESIDUE B+R>\n");
    fprintf(fp, "RESIDUE B+I>\n");
    fprintf(fp, "RESIDUE R+I>\n");
    fprintf(fp, "RESIDUE B+R+I>\n");
    fprintf(fp, "HOMOEF_A>    0\n");
    fprintf(fp, "HOMOEF_B>    0\n");
    fprintf(fp, "HOMOEF_C>    0\n");
    fprintf(fp, "HOMOEF_D>    0\n");
    fprintf(fp, "HOMOEF_E>    0\n");
    fprintf(fp, "HOMOEF_F>    0\n");
    fprintf(fp, "HOMOEF_G>    0\n");
    fprintf(fp, "HOMOEF_H>    0\n");
    fprintf(fp, "HOMOEF_I>    0\n");
    fprintf(fp, "HOMOEF_J>    0\n");
    fprintf(fp, "HOMOEF_K>    0\n");
    fprintf(fp, "HOMOEF_L>    0\n");
    fprintf(fp, "HOMOEF_M>    0\n");
    fprintf(fp, "HOMOEF_N>    0\n");
    fprintf(fp, "HOMOEF_O>    0\n");
    fprintf(fp, "HOMOEF_P>    0\n");
    fprintf(fp, "HOMOEF_Q>    0\n");
    fprintf(fp, "HOMOEF_R>    0\n");
    fprintf(fp, "HOMOEF_S>    0\n");
    fprintf(fp, "HOMOEF_T>    0\n");
}

static const char *alma_get_extension_for(int format_type) {
    if (format_type == EMBL)       return ".EMBL";
    if (format_type == NBRF)       return ".NBRF";
    if (format_type == GCG)        return ".UWGCG";
    if (format_type == STADEN)     return ".STADEN";

    throw_error(57, "Unknown format type when writing ALMA format");
}
static const char *alma_get_format_id(int format_type) {
    return alma_get_extension_for(format_type)+1; 
}

/* -------------------------------------------------------------
 *   Function alma_out().
 *       Output one alma entry.
 */
FILE *alma_out(FILE * fp, int format) {
    int indi, len;
    char filename[TOKENNUM];
    FILE *outfile;

    Cpystr(filename, data.alma.id);
    for (indi = 0, len = Lenstr(filename); indi < len; indi++)
        if (!isalnum(filename[indi]))
            filename[indi] = '_';

    Catstr(filename, alma_get_extension_for(format));

    outfile = alma_out_entry_header(fp, data.alma.id, filename, format);
    alma_out_gaps(fp);

    return outfile;
}

/* --------------------------------------------------------------
 *  Function alma_out_entry_header().
 *      Output one ALMA entry header.
 */
FILE *alma_out_entry_header(FILE * fp, char *entry_id, char *filename, int format_type) {
    fprintf(fp, "NXT ENTRY>S\n");
    fprintf(fp, "ENTRY ID>%s\n", entry_id);

    if (file_exists(filename)) warningf(55, "file %s gets overwritten.", filename);
    FILE *outfile = open_output_or_die(filename);

    fprintf(fp, "SEQUENCE>%s\n", filename);

    const char *format_name = alma_get_format_id(format_type);
    fprintf(fp, "FORMAT>%s\n", format_name);

    fprintf(fp, "ACCEPT>ALL\n");
    fprintf(fp, "DEFGAP>[-]\n");
    fprintf(fp, "PARAMS>1\n");
    fprintf(fp, "GAPS>\n");

    return outfile;
}

/* --------------------------------------------------------------
 *  Function alma_out_gaps().
 *       Output gaps information of one ALMA entry.
 */
void alma_out_gaps(FILE * fp)
{
    int indi, index, residue, gnum, rnum, tempcount;

    gnum = rnum = 0;

    if (data.sequence[0] == '.' || data.sequence[0] == '-' || data.sequence[0] == '~')
        residue = 0;
    else
        residue = 1;

    tempcount = data.seq_length;
    for (indi = index = 0; indi < tempcount; indi++) {
        if (data.sequence[indi] == '.' || data.sequence[indi] == '-' || data.sequence[indi] == '~') {
            if (residue) {
                fprintf(fp, " %4d %4d [-]\n", gnum, rnum);
                gnum = rnum = residue = 0;
            }
            gnum++;
            data.seq_length--;
        }
        else {
            residue = 1;
            rnum++;
            data.sequence[index++]
                = data.sequence[indi];
        }
    }
    data.sequence[index] = '\0';
    fprintf(fp, " %4d %4d [-]\n", gnum, rnum);
    fprintf(fp, "   -1   -1 [-]\n");
    fprintf(fp, "END ENTRY>\n");
}
