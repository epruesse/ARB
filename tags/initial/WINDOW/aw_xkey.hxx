#ifndef aw_xkey_hxx_included
#define aw_xkey_hxx_included

struct awxkeymap_struct {
	int	xmod;
	int	xkey;
	const char	*xstr;
	AW_key_mod	awmod;
	AW_key_code	awkey;
	char	*awstr;
	};

void aw_install_xkeys(Display *display);
struct awxkeymap_struct *aw_xkey_2_awkey(XKeyEvent *xkeyevent);


#endif
