#ifndef __wrap_r_h__
#define __wrap_r_h__

#ifdef MRTRIX_AS_R_LIBRARY
# ifdef WARN
#  undef WARN
# endif
# include <R.h>
# ifdef WARN
#  undef WARN
# endif
#endif

#endif
