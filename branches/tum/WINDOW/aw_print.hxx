#ifndef aw_print_hxx_included
#define aw_print_hxx_included


class AW_device_print: public AW_device {
	public:
	FILE *out;

	// ********* real public
	AW_device_print(AW_common *commoni);
	void	init(void);
	const char *open(const char *path);
	void	close(void);

	AW_DEVICE_TYPE type(void); 
	int	line(int gc, AW_pos x0,AW_pos y0, AW_pos x1,AW_pos y1, AW_bitset filter, AW_CL cd1, AW_CL cd2);
	int	text(int gc, const char *string,AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2,long opt_strlen);
	int	box(int gc, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filter, AW_CL cd1, AW_CL cd2);
	int	circle(int gc, AW_pos x0,AW_pos y0,AW_pos width,AW_pos heigth, AW_bitset filter, AW_CL cd1, AW_CL cd2);
	int	filled_area(int gc, int npoints, AW_pos *points, AW_bitset filteri, AW_CL cd1, AW_CL cd2);
};


#endif
