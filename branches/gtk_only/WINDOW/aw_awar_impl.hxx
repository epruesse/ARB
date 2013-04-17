class AW_awar_int : public AW_awar {
  long min_value;
  long max_value;
  long default_value;
  long value;
  std::vector<long*> target_variables;
public:
  AW_awar_int(const char *var_name, long var_value, AW_default default_file, AW_root *root);
  ~AW_awar_int();
  GB_TYPES get_type() const OVERRIDE { return GB_INT; } 
  const char* get_type_name() const { return "AW_awar_int"; }
  void     do_update() OVERRIDE;
  AW_awar* set_minmax(float min, float max) OVERRIDE;
  GB_ERROR  write_as_string(const char* para, bool touch=false) OVERRIDE;
  GB_ERROR  write_int(long para, bool touch=false) OVERRIDE;
  long     read_int() OVERRIDE;
  char*    read_as_string() OVERRIDE;
  AW_awar *add_target_var(long *pint) OVERRIDE;
  GB_ERROR toggle_toggle() OVERRIDE;
};
class AW_awar_float : public AW_awar {
  double min_value;
  double max_value;
  double default_value;
  double value;
  std::vector<float*> target_variables;
public:
  AW_awar_float(const char *var_name, double var_value, AW_default default_file, AW_root *root);
  ~AW_awar_float();
  GB_TYPES get_type() const OVERRIDE { return GB_FLOAT; }
  const char* get_type_name() const { return "AW_awar_float"; }
  void     do_update() OVERRIDE;
  AW_awar* set_minmax(float min, float max) OVERRIDE;
  GB_ERROR  write_as_string(const char* para, bool touch=false) OVERRIDE;
  GB_ERROR  write_float(double para, bool touch=false) OVERRIDE;
  double   read_float() OVERRIDE;
  char*    read_as_string() OVERRIDE;
  AW_awar *add_target_var(float *pfloat) OVERRIDE;
  GB_ERROR toggle_toggle() OVERRIDE;
};
class AW_awar_string : public AW_awar {
  char* srt_program;
  char* default_value;
  char* value;
  std::vector<char**> target_variables;
public:
  AW_awar_string(const char *var_name, const char* var_value, AW_default default_file, AW_root *root);
  ~AW_awar_string();
  GB_TYPES get_type() const OVERRIDE { return GB_STRING; }
  const char* get_type_name() const { return "AW_awar_string"; }
  void     do_update() OVERRIDE;
  AW_awar* set_srt(const char *srt) OVERRIDE;
  GB_ERROR  write_as_string(const char* para, bool touch=false) OVERRIDE;
  GB_ERROR  write_string(const char* para, bool touch=false) OVERRIDE;
  char*    read_string() OVERRIDE;
  const char* read_char_pntr() OVERRIDE;
  char*    read_as_string() OVERRIDE;
  AW_awar *add_target_var(char **ppchr) OVERRIDE;
  GB_ERROR toggle_toggle() OVERRIDE;
};

class AW_awar_pointer : public AW_awar {
  void *default_value;
  void *value;
public:
  AW_awar_pointer(const char *var_name, void* var_value, AW_default default_file, AW_root *root);
  ~AW_awar_pointer();
  GB_TYPES get_type() const OVERRIDE { return GB_POINTER; }
  const char* get_type_name() const { return "AW_awar_pointer"; }
  void     do_update() OVERRIDE;
  GB_ERROR  write_pointer(GBDATA* para, bool touch=false) OVERRIDE;
  GBDATA*    read_pointer() OVERRIDE;
};
