//  ==================================================================== //
//                                                                       //
//    File      : config_parser.h                                        //
//    Purpose   : reads config files                                     //
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

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

// format of config files:
//
// # comment
// key = value
// # comment
// key = value
//
// # comment
// key = value
//


#ifndef _CPP_MAP
#include <map>
#endif
#ifndef _CPP_CSTDIO
#include <cstdio>
#endif
#ifndef _CPP_STRING
#include <string>
#endif

#define MAXCONFIGLINESIZE 200

namespace {

    class ConfigParser {
        typedef std::map<std::string, std::string> ConfigMap;

        ConfigMap entries;
        std::string    filename;
        GB_ERROR  error;

        ConfigParser(const ConfigParser& other); // copying not allowed
        ConfigParser& operator = (const ConfigParser& other); // assignment not allowed

        static char *unwhite(char *s) {
            while (s[0] == ' ') ++s;
            char *e = strchr(s, 0)-1;

            while (e>s && isspace(e[0])) --e;
            if (e<s) e = s;

            e[isspace(e[0]) ? 0 : 1] = 0;

            return s;
        }

    public:

        ConfigParser(const std::string& filename_)
            : filename(filename_)
            , error(0)
        {
            FILE *in = fopen(filename.c_str(), "rt");
            if (!in) {
                error = GBS_global_string("Can't open config '%s'", filename.c_str());
            }
            else {
                char buffer[MAXCONFIGLINESIZE+1];
                int  lineno = 0;
                while (!error && fgets(buffer, MAXCONFIGLINESIZE, in) != 0) {
                    ++lineno;
                    char *content = unwhite(buffer);
                    if (content[0] && content[0] != '#') { // skip empty and comment lines
                        char *key, *value;
                        error = splitText(content, '=', key, value);
                        if (!error && value[0] == 0) {
                            error = "content missing behind '='";
                        }

                        if (!error) {
                            ConfigMap::const_iterator found = entries.find(key);
                            if (found == entries.end()) {
                                // fprintf(stderr, "adding value '%s' at key '%s'\n", value, key);
                                entries[key] = value;
                            }
                            else {
                                error = GBS_global_string("entry '%s' redefined", key);
                            }
                        }

                        if (error) error = makeError(lineno, error);
                    }
                }
                fclose(in);
            }
        }

        GB_ERROR getError() { return error; }

        GB_ERROR makeError(const std::string& forEntry, const char *msg) {
            return GBS_global_string("%s (at entry '%s' in %s)", msg, forEntry.c_str(), filename.c_str());
        }
        GB_ERROR makeError(int lineno, const char *msg) {
            return GBS_global_string("%s (at line #%i in %s)", msg, lineno, filename.c_str());
        }
        GB_ERROR makeError(const char *msg) {
            return GBS_global_string("%s (in %s)", msg, filename.c_str());
        }

        static GB_ERROR splitText(char *text, char separator, char*& lhs, char*& rhs) {
            text      = unwhite(text);
            char *sep = strchr(text, separator);
            if (!sep) return GBS_global_string("'%c' expected in '%s'", separator, text);

            sep[0] = 0;
            lhs    = unwhite(text);
            rhs    = unwhite(sep+1);

            return 0;
        }

        const std::string *getValue(const std::string& key, GB_ERROR& err) {
            ConfigMap::const_iterator found = entries.find(key);
            if (found == entries.end()) {
                err = makeError(GBS_global_string("Entry '%s' expected", key.c_str()));
                return 0;
            }

            return &(found->second);
        }
    };

    // --------------------------------------------------------------------------------

    class ConfigBase {
        ConfigBase(const ConfigBase& other); // copying not allowed
        ConfigBase& operator = (const ConfigBase& other); // assignment not allowed

    protected:

        ConfigParser parser;
        GB_ERROR     error;

        GB_ERROR parse_double(const char *s, double& d) {
            if (s[0] == 0) return "floating point number expected";

            char *end = 0;
            d         = strtod(s, &end);
            if (end[0] != 0) {
                return GBS_global_string("Unexpected '%s' behind floating point number", end);
            }
            return 0;
        }

        GB_ERROR check_int_range(int value, int min_value, int max_value) {
            if (value<min_value || value>max_value) {
                return GBS_global_string("%i outside allowed range [%i .. %i]", value, min_value, max_value);
            }
            return 0;
        }
        GB_ERROR check_double_range(double value, double min_value, double max_value) {
            if (value<min_value || value>max_value) {
                return GBS_global_string("%f outside allowed range [%f .. %f]", value, min_value, max_value);
            }
            return 0;
        }
        GB_ERROR check_bool_range(int value) {
            if (value<0 || value>1) {
                return GBS_global_string("%i is not boolean (has to be 0 or 1).", value);
            }
            return 0;
        }

        void parseInt(const std::string& key, int& value) {
            const std::string *val = parser.getValue(key, error);
            if (val) value = atoi(val->c_str());
        }

        void parseInt_checked(const std::string& key, int& value, int min_value, int max_value) {
            parseInt(key, value);
            if (!error) {
                error            = check_int_range(value, min_value, max_value);
                if (error) error = parser.makeError(key, error);
            }
        }

        void parseIntRange(const std::string& key, int& low, int& high) {
            const std::string *val = parser.getValue(key, error);
            if (val) {
                char *range = strdup(val->c_str());
                char *lhs, *rhs;

                error             = ConfigParser::splitText(range, ',', lhs, rhs);
                if (!error)  {
                    low  = atoi(lhs);
                    high = atoi(rhs);
                    if (low>high) {
                        error = GBS_global_string("Invalid range (%i has to be smaller than %i)", low, high);
                    }
                }

                free(range);

                if (error) error = parser.makeError(key, error);
            }
        }

        void parseIntRange_checked(const std::string& key, int& low, int& high, int min_value, int max_value) {
            parseIntRange(key, low, high);
            if (!error) {
                error             = check_int_range(low, min_value, max_value);
                if (!error) error = check_int_range(high, min_value, max_value);
                if (error) error  = parser.makeError(key, error);
            }
        }

        void parseBool(const std::string& key, bool& boolean) {
            int b;
            parseInt(key, b);
            if (!error) {
                error            = check_bool_range(b);
                if (error) error = parser.makeError(key, error);
                else boolean     = static_cast<bool>(b);
            }
        }

        void parseDouble(const std::string& key, double& value) {
            const std::string *val = parser.getValue(key, error);
            if (val) {
                error = parse_double(val->c_str(), value);
            }
        }

        void parseDouble_checked(const std::string& key, double& value, double min_value, double max_value) {
            parseDouble(key, value);
            if (!error) {
                error            = check_double_range(value, min_value, max_value);
                if (error) error = parser.makeError(key, error);
            }
        }

        void parseDoubleRange(const std::string& key, double& low, double& high) {
            const std::string *val = parser.getValue(key, error);
            if (val) {
                char *range = strdup(val->c_str());
                char *lhs, *rhs;

                error             = ConfigParser::splitText(range, ',', lhs, rhs);
                if (!error) error = parse_double(lhs, low);
                if (!error) error = parse_double(rhs, high);
                if (!error && low>high) {
                    error = GBS_global_string("Invalid range (%f has to be smaller than %f)", low, high);
                }

                free(range);

                if (error) error = parser.makeError(key, error);
            }
        }

        void parseDoubleRange_checked(const std::string& key, double& low, double& high, double min_value, double max_value) {
            parseDoubleRange(key, low, high);
            if (!error) {
                error             = check_double_range(low, min_value, max_value);
                if (!error) error = check_double_range(high, min_value, max_value);
                if (error) error  = parser.makeError(key, error);
            }
        }

    public:
        ConfigBase(const std::string &filename)
            : parser(filename)
            , error(0)
        {}
        virtual ~ConfigBase() {}

        GB_ERROR getError() const { return error; }
    };
}

#undef MAXCONFIGLINESIZE


#else
#error config_parser.h included twice
#endif // CONFIG_PARSER_H

