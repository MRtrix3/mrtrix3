/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __math_hermite_h__
#define __math_hermite_h__

#include <limits>

namespace MR
{
  namespace Math
  {

    template <typename T> class Hermite
    {
      public:
        typedef T value_type;

        Hermite (value_type tension = 0.0) : t (T (0.5) *tension) { }

        void set (value_type position) {
          value_type p2 = position*position;
          value_type p3 = position*p2;
          w[0] = (T (0.5)-t) * (T (2.0) *p2  - p3 - position);
          w[1] = T (1.0) + (T (1.5) +t) *p3 - (T (2.5) +t) *p2;
          w[2] = (T (2.0) +T (2.0) *t) *p2 + (T (0.5)-t) *position - (T (1.5) +t) *p3;
          w[3] = (T (0.5)-t) * (p3 - p2);
        }

        value_type coef (size_t i) const {
          return (w[i]);
        }

        template <class S>
        S value (const S* vals) const {
          return (value (vals[0], vals[1], vals[2], vals[3]));
        }

        template <class S>
        S value (const S& a, const S& b, const S& c, const S& d) const {
          return (w[0]*a + w[1]*b + w[2]*c + w[3]*d);
        }

      private:
        value_type w[4];
        value_type t;



    };

  }
}

#endif
