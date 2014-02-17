#ifndef __math_cauchy_h__
#define __math_cauchy_h__

namespace MR
{
  namespace Math
  {

    inline float cauchy (float x, float s)
    {
      x /= s;
      return (1.0 / (1.0 + x*x));
    }

  }
}

#endif


