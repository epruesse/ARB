//  ==================================================================== //
//                                                                       //
//    File      : read_config.h                                          //
//    Purpose   : reads and provides probe_parameters.conf               //
//    Time-stamp: <Fri Oct/01/2004 20:27 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in October 2003          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef READ_CONFIG_H
#define READ_CONFIG_H

#ifndef CONFIG_PARSER_H
#include <config_parser.h>
#endif

namespace {

    class Config : public ConfigBase {
        probe_config_data& data;

        void parseBondQuadruple(const std::string& key, double *bond_array) {
            const std::string *val = parser.getValue(key, error);
            if (val) {
                char *range = strdup(val->c_str());
                char *b1, *b2, *b3, *b4;

                error             = ConfigParser::splitText(range, ',', b1, b2);
                if (!error) error = ConfigParser::splitText(b2, ',', b2, b3);
                if (!error) error = ConfigParser::splitText(b3, ',', b3, b4);

                if (!error) error = parse_double(b1, bond_array[0]);
                if (!error) error = parse_double(b2, bond_array[1]);
                if (!error) error = parse_double(b3, bond_array[2]);
                if (!error) error = parse_double(b4, bond_array[3]);

                free(range);

                for (int i=0; i<4 && !error; ++i) {
                    error = check_double_range(bond_array[i], 0.0, 9.99);
                }

                if (error) error = parser.makeError(key, error);
            }
        }

    public:

        Config(const std::string &filename, probe_config_data& write_to_data)
            : ConfigBase(filename)
            , data(write_to_data)
        {
            memset(&data, 0, sizeof(data));
            error = parser.getError();
            if (!error) parseDoubleRange_checked("temperature", data.min_temperature, data.max_temperature, 0.0, 100.0);
            if (!error) parseDoubleRange_checked("gccontent", data.min_gccontent, data.max_gccontent, 0.0, 100.0);
            if (!error) parseIntRange_checked("ecolipos", data.ecoli_min_pos, data.ecoli_max_pos, 0, 10000);
            
            if (!error) parseBool("weighted_mismatches", data.use_weighted_mismatches);

            if (!error) parseBondQuadruple("bonds_Ax", data.bonds);
            if (!error) parseBondQuadruple("bonds_Cx", data.bonds+4);
            if (!error) parseBondQuadruple("bonds_Gx", data.bonds+8);
            if (!error) parseBondQuadruple("bonds_Tx", data.bonds+12);

            if (!error) parseInt_checked("maxhairpinbonds", data.max_hairpin_bonds, 0, 20);
            if (!error) parseDouble_checked("split", data.split, 0.0, 1.0);
            if (!error) parseDouble_checked("edge_misval", data.dtedge, 0.0, 1.0);
            if (!error) parseDouble_checked("single_misval", data.dt, 0.0, 1.0);
        }

    };

}


#else
#error read_config.h included twice
#endif // READ_CONFIG_H

