
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ali_misc.hxx"
#include "ali_global.hxx"

#define EXCLUSIVE_FLAG_DEFAULT             1
#define MARK_FAMILY_FLAG_DEFAULT           0
#define MARK_EXTENSION_FLAG_DEFAULT        0
#define FIND_FAMILY_MODE_DEFAULT           1
#define MAX_FAMILY_SIZE_DEFAULT            10
#define MIN_FAMILY_SIZE_DEFAULT            5
#define MIN_WEIGHT_DEFAULT                 0.7
#define EXT_MAX_WEIGHT_DEFAULT             0.2
#define MULTI_GAP_FACTOR_DEFAULT           0.1
#define INSERT_FACTOR_DEFAULT              2.0;
#define MULTI_INSERT_FACTOR_DEFAULT        0.5;
#define COST_LOW_DEFAULT                   0.25
#define COST_MIDDLE_DEFAULT                0.5
#define COST_HIGH_DEFAULT                  0.8
#define MAX_COST_OF_SUB_PERCENT_DEFAULT    0.5
#define MAX_COST_OF_HELIX                  2.0
#define ERROR_COUNT_DEFAULT                2
#define MAX_NUMBER_OF_MAPS_DEFAULT         10 /* 100 */
#define MAX_NUMBER_OF_MAPS_ALIGNER_DEFAULT 2
#define INTERVALL_BORDER_DEFAULT           5
#define INTERVALL_CENTER_DEFAULT           5

/*
 * ACHTUNG: muss noch durch parameter belegbar sein
 */
#define MATCHES_MIN_DEFAULT          1000
#define PERCENT_MIN_DEFAULT          0.75
#define FAM_LIST_MAX_DEFAULT         5
#define EXT_LIST_MAX_DEFAULT         10
#define USE_SPECIFIED_FAMILY_DEFAULT 0


double default_substitute_matrix[5][5] = {
    /* a    c    g    u    -          */
    {0.0, 3.0, 1.0, 3.0, 5.0},  /* a */
    {3.0, 0.0, 3.0, 1.0, 5.0},  /* c */
    {1.0, 3.0, 0.0, 3.0, 5.0},  /* g */
    {3.0, 1.0, 3.0, 0.0, 5.0},  /* u */
    {5.0, 5.0, 5.0, 5.0, 0.0}   /* - */
};

double default_binding_matrix[5][5] = {
    /* a    c    g    u    -          */
    {9.9, 9.9, 2.0, 0.9, 9.9},  /* a */
    {9.9, 9.9, 0.6, 9.9, 9.9},  /* c */
    {2.0, 0.6, 5.0, 1.1, 9.9},  /* g */
    {0.9, 9.9, 1.1, 9.9, 9.9},  /* u */
    {9.9, 9.9, 9.9, 9.9, 0.0}   /* - */
};


void ALI_GLOBAL::init(int *argc, char *argv[])
{
    int kill, i, h, j, ret;
    char *pos;
    struct arb_params *params;
    float fl;

    params = arb_trace_argv(argc,argv);

    prog_name = argv[0];
    species_name = params->species_name;
    default_file = params->default_file;
    db_server = params->db_server;

    /*
     * Set the defaults
     */

    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++) {
            prof_context.substitute_matrix[i][j] = default_substitute_matrix[i][j];
            prof_context.binding_matrix[i][j] = default_binding_matrix[i][j];
        }

    cost_low = COST_LOW_DEFAULT;
    cost_middle = COST_MIDDLE_DEFAULT;
    cost_high = COST_HIGH_DEFAULT;
    preali_context.max_number_of_maps = MAX_NUMBER_OF_MAPS_DEFAULT;
    preali_context.max_number_of_maps_aligner = 
        MAX_NUMBER_OF_MAPS_ALIGNER_DEFAULT;
    preali_context.intervall_border = INTERVALL_BORDER_DEFAULT;
    preali_context.intervall_center = INTERVALL_CENTER_DEFAULT;
    preali_context.max_cost_of_sub_percent = MAX_COST_OF_SUB_PERCENT_DEFAULT;
    preali_context.max_cost_of_helix = MAX_COST_OF_HELIX;
    preali_context.error_count = ERROR_COUNT_DEFAULT;
    mark_species_flag = 0;

    prof_context.find_family_mode = FIND_FAMILY_MODE_DEFAULT;
    prof_context.exclusive_flag = EXCLUSIVE_FLAG_DEFAULT;
    prof_context.mark_family_flag = MARK_FAMILY_FLAG_DEFAULT;
    prof_context.mark_family_extension_flag = MARK_EXTENSION_FLAG_DEFAULT;
    prof_context.max_family_size = MAX_FAMILY_SIZE_DEFAULT;
    prof_context.min_family_size = MIN_FAMILY_SIZE_DEFAULT;
    prof_context.min_weight = MIN_WEIGHT_DEFAULT;
    prof_context.ext_max_weight = EXT_MAX_WEIGHT_DEFAULT;
    prof_context.multi_gap_factor = MULTI_GAP_FACTOR_DEFAULT;
    prof_context.insert_factor = INSERT_FACTOR_DEFAULT;
    prof_context.multi_insert_factor = MULTI_INSERT_FACTOR_DEFAULT;

    pt_context.matches_min = MATCHES_MIN_DEFAULT;
    pt_context.percent_min = PERCENT_MIN_DEFAULT;
    pt_context.fam_list_max = FAM_LIST_MAX_DEFAULT;
    pt_context.ext_list_max = EXT_LIST_MAX_DEFAULT;
    pt_context.use_specified_family = USE_SPECIFIED_FAMILY_DEFAULT;

    /*
     * evaluate the parameters
     */

    for (i = 1; i < *argc;) {
        kill = 0;
        if (strcmp("-nx",argv[i]) == 0 && kill == 0) {
            prof_context.exclusive_flag = 0;
            kill = i;
        }
        if (strncmp("-f",argv[i],2) == 0 && kill == 0) {
            pt_context.use_specified_family = strdup(argv[i] + 2);
            kill = i;
        }
        if (strcmp("-ms",argv[i]) == 0 && kill == 0) {
            mark_species_flag = 1;
            kill = i;
        }
        if (strcmp("-mf",argv[i]) == 0 && kill == 0) {
            prof_context.mark_family_flag = 1;
            kill = i;
        }
        if (strcmp("-mfe",argv[i]) == 0 && kill == 0) {
            prof_context.mark_family_extension_flag = 1;
            kill = i;
        }
        if (strncmp("-mgf",argv[i],4) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 4;
            ret = sscanf(pos,"%f",&prof_context.multi_gap_factor);
            if (ret != 1) {
                ali_warning("Wrong format for -mgf");
                break;
            }
        }
        if (strncmp("-if",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%f",&prof_context.insert_factor);
            if (ret != 1) {
                ali_warning("Wrong format for -if");
                break;
            }
        }
        if (strncmp("-mif",argv[i],4) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 4;
            ret = sscanf(pos,"%f",&prof_context.multi_insert_factor);
            if (ret != 1) {
                ali_warning("Wrong format for -mif");
                break;
            }
        }
        if (strcmp("-m",argv[i]) == 0 && kill == 0) {
            mark_species_flag = 1;
            prof_context.mark_family_flag = 1;
            kill = i;
        }
        if (strncmp("-msub",argv[i],5) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 5;
            for (h = 0; h < 5; h++)
                for (j = 0; j < 5; j++) {
                    ret = sscanf(pos,"%f",&fl);
                    if (ret != 1) {
                        ali_warning("wrong format for -msub");
                        break;
                    }
                    else
                        prof_context.substitute_matrix[h][j] = (double) fl;
                    pos = strchr(pos,',');
                    if ((h != 4 || j != 4) && pos == 0) {
                        ali_warning("Not enought values for -msub");
                        break;
                    }
                    pos++;
                }
        }
        if (strncmp("-mbind",argv[i],6) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 6;
            for (h = 0; h < 5; h++)
                for (j = 0; j < 5; j++) {
                    ret = sscanf(pos,"%f",&fl);
                    if (ret != 1) {
                        ali_warning("Wrong format for -mbind");
                        break;
                    }
                    else
                        prof_context.binding_matrix[h][j] = (double) fl;
                    pos = strchr(pos,',');
                    if ((h != 4 || j != 4) && pos == 0) {
                        ali_warning("Not enought values for -mbind");
                        break;
                    }
                    pos++;
                }
        }
        if (strncmp("-maxf",argv[i],5) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 5;
            ret = sscanf(pos,"%d",&prof_context.max_family_size);
            if (ret != 1) {
                ali_warning("Wrong format for -maxf");
                break;
            }
        }
        if (strncmp("-minf",argv[i],5) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 5;
            ret = sscanf(pos,"%d",&prof_context.min_family_size);
            if (ret != 1) {
                ali_warning("Wrong format for -minf");
                break;
            }
        }
        if (strncmp("-minw",argv[i],5) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 5;
            ret = sscanf(pos,"%f",&prof_context.min_weight);
            if (ret != 1) {
                ali_warning("Wrong format for -minw");
                break;
            }
        }
        if (strncmp("-maxew",argv[i],6) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 6;
            ret = sscanf(pos,"%f",&prof_context.ext_max_weight);
            if (ret != 1) {
                ali_warning("Wrong format for -minw");
                break;
            }
        }

        /* 
         * ACHTUNG: Unused BEGIN
         */
        if (strncmp("-cl",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%f",&cost_low);
            if (ret != 1) {
                ali_warning("Wrong format for -cl");
                break;
            }
        }
        if (strncmp("-cm",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%f",&cost_middle);
            if (ret != 1) {
                ali_warning("Wrong format for -cm");
                break;
            }
        }
        if (strncmp("-ch",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%f",&cost_high);
            if (ret != 1) {
                ali_warning("Wrong format for -ch");
                break;
            }
        }
        /*
         * ACHTUNG: Unused END
         */

        if (strncmp("-csub",argv[i],5) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 5;
            ret = sscanf(pos,"%f",&preali_context.max_cost_of_sub_percent);
            if (ret != 1) {
                ali_warning("Wrong format for -csub");
                break;
            }
        }
        if (strncmp("-chel",argv[i],5) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 5;
            ret = sscanf(pos,"%f",&preali_context.max_cost_of_helix);
            if (ret != 1) {
                ali_warning("Wrong format for -chel");
                break;
            }
        }
        if (strncmp("-mma",argv[i],4) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 4;
            ret = sscanf(pos,"%ld",&preali_context.max_number_of_maps_aligner);
            if (ret != 1) {
                ali_warning("Wrong format for -mma");
                break;
            }
        }
        if (strncmp("-mm",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%ld",&preali_context.max_number_of_maps);
            if (ret != 1) {
                ali_warning("Wrong format for -mm");
                break;
            }
        }
        if (strncmp("-ec",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%ld",&preali_context.error_count);
            if (ret != 1) {
                ali_warning("Wrong format for -ec");
                break;
            }
        }
        if (strncmp("-ib",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%d",&preali_context.intervall_border);
            if (ret != 1) {
                ali_warning("Wrong format for -ib");
                break;
            }
        }
        if (strncmp("-ic",argv[i],3) == 0 && kill == 0) {
            kill = i;
            pos = argv[i] + 3;
            ret = sscanf(pos,"%d",&preali_context.intervall_center);
            if (ret != 1) {
                ali_warning("Wrong format for -ic");
                break;
            }
        }

        if (kill > 0) {
            for (i++; i < *argc; i++)
                argv[i-1] = argv[i];
            (*argc)--;
            i = kill;
        }
        else
            i++;
    }

    /*
     * Check for consistency
     */

    if (prof_context.min_family_size > prof_context.max_family_size) {
        ali_warning("minf <= maxf");
    }

    if (prof_context.ext_max_weight < 0 || prof_context.ext_max_weight > 1.0) {
        ali_warning("0 <= maxew <= 1.0");
    }

    if (prof_context.min_weight < 0 || prof_context.min_weight > 1.0) {
        ali_warning("0 <=  minw <= 1.0");
    }

    if (cost_low > cost_middle || cost_middle > cost_high ||
        cost_low < 0 || cost_high > 1.0) {
        ali_warning("0 <= cl <= cm <= ch <= 1.0");
    }

    /*
     * Open Database and Pt server
     */

    ali_message("Connecting to Database server");
    if (arbdb.open(db_server) != 0) {
        ali_error("Can't connect to Database server");
    }
    ali_message("Connection established");
    prof_context.arbdb = &arbdb;

    pt_context.servername = params->pt_server;
    pt_context.gb_main = arbdb.gb_main;

    pt = new ALI_PT(&pt_context);

    prof_context.pt = pt;
}
