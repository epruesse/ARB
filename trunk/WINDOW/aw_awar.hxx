#ifndef aw_awar_hxx_included
#define aw_awar_hxx_included


#define AW_INSERT_BUTTON_IN_AWAR_LIST(vs,cd1,widget,type,aww) { AW_widget_list_for_variable *awibsldummy = \
	new AW_widget_list_for_variable(vs,cd1,(int*)widget,type,aww);awibsldummy = awibsldummy;};
#define AWAR_EPS 0.00000001

typedef enum {
	AW_WIDGET_INPUT_FIELD,
	AW_WIDGET_TEXT_FIELD,
	AW_WIDGET_LABEL_FIELD,
	AW_WIDGET_CHOICE_MENU,
	AW_WIDGET_TOGGLE_FIELD,
	AW_WIDGET_SELECTION_LIST,
	AW_WIDGET_TOGGLE
} AW_widget_type;

/*************************************************************************/
struct AW_widget_list_for_variable {

	AW_widget_list_for_variable( AW_awar *vs, AW_CL cd1, int *widgeti, AW_widget_type type, AW_window *awi );

	AW_CL				cd;
	AW_awar				*awar;
	int				*widget;
	AW_widget_type			widget_type;
	AW_window			*aw;
	AW_widget_list_for_variable	*next;
};



/*************************************************************************/
struct AW_var_callback {
    AW_var_callback( void (*vc_cb)(AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 );
    AW_var_callback( void (*vc_cb)(AW_root*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2, AW_var_callback *nexti );
    
    void 			(*value_changed_cb)(AW_root*,AW_CL,AW_CL);
    AW_CL			value_changed_cb_cd1;
    AW_CL			value_changed_cb_cd2;
    AW_var_callback		*next;
    void run_callback(AW_root *root);
};


struct AW_variable_struct {
	AW_variable_struct( AW_VARIABLE_TYPE var_type, const char *var_name, const char *var_value, double var_double_value, long *var_adress, AW_default default_file, AW_root *root );

	AW_VARIABLE_TYPE		variable_type;

	long				*variable_mem_pntr;
	union {
		struct {
			float		min,max;
		} f;
		char *srt;
	} pp;

#ifdef GB_INCLUDED
	GBDATA				*gb_var;
	GBDATA				*gb_origin;
#else
	void				*gb_var;
	void				*gb_origin;
#endif
	AW_root				*value_changed_cb_root;
	AW_var_callback			*callback_list;
	AW_var_callback			*last_of_callback_list;

	AW_widget_list_for_variable	*widget_list;
	AW_widget_list_for_variable	*last_of_widget_list;

	char	*update();		/* update all widgets */
	char	*map(GBDATA *dest);	/* map to new address */
	void	get(char **p_string);
	void	get(long *p_int);
	void	get(double *p_double);
	void	get(float *p_float);
	char	*get_as(void);
	char	*set(char *aw_string);
	char	*set(long aw_int);
	char	*set(double aw_double);
	char	*set_as(char *aw_value);
	char	*toggle_toggle();
	void	touch(void);
};

void aw_update_awar_window_geometry(AW_root *awr);


#endif
