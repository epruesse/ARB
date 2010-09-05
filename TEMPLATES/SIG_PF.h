/*   File      : SIG_PF.h
 *   Purpose   : wrapper to include or define stuff needed for SIG_PF
 */

#if defined(SUN5_ECGS)
# include <sys/iso/signal_iso.h>
#else
# if defined(LINUX)
#  include <signal.h>
#  ifndef SIG_PF
#   define SIG_PF __sighandler_t
#  endif
# else
#  if defined(SUN4) || defined(SUN5)
#   if defined(__cplusplus)
#    include <sysent.h>         /* c++ only for sun (used for shutdown) */
#   else
#    define SIG_PF void (*)()
#   endif
#  else
#   define SIG_PF  void (*)(int)
#  endif
# endif
#endif




