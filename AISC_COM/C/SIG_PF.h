/*   File      : SIG_PF.h
 *   Purpose   : wrapper to include or define stuff needed for SIG_PF
 *   Time-stamp: <Tue Aug/13/2002 18:39 MET Coder@ReallySoft.de>
 */

#if defined(SUN5_ECGS)
# include <sys/iso/signal_iso.h>
#else
# if defined(SUN4) || defined(SUN5)
#  if defined(__cplusplus) && !defined(SUN5_ECGS)
#   include <sysent.h>	/* c++ only for sun (used for shutdown) */
#  else
#   define SIG_PF void (*)()
#  endif
# else
#  define SIG_PF void (*)(int)
# endif
#endif



