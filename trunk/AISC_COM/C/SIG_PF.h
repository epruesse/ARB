/*   File      : SIG_PF.h
 *   Purpose   : wrapper to include or define stuff needed for SIG_PF
 *   Time-stamp: <Thu Aug/22/2002 19:06 MET Coder@ReallySoft.de>
 */

#if defined(SUN5_ECGS)
# include <sys/iso/signal_iso.h>
#elseif defined (LINUX)
# include <sysent.h>
#elseif defined(SUN4) || defined(SUN5)
# if defined(__cplusplus)
#  include <sysent.h>	/* c++ only for sun (used for shutdown) */
# else
#  define SIG_PF void (*)()
# endif
#else
# define SIG_PF void (*)(int)
#endif




