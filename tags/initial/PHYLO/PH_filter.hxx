class PH_filter {
	public:
	char	*filter;	// 0 1
	long	filter_len;
	long	real_len;	// how many 1
	long	update;
	long    *options_vector;   // options used to calculate current filter
//	float   *markerline;       // line to create filter (according to options_vector)
	char 	*init(char *filter, char *zerobases, long size);
	char	*init(long size);
	
	PH_filter::PH_filter(void);
	~PH_filter(void);
	float *calculate_column_homology(void);
};	

