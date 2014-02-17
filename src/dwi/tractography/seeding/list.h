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


#ifndef __dwi_tractography_seeding_list_h__
#define __dwi_tractography_seeding_list_h__


#include "math/rng.h"

#include "dwi/tractography/seeding/base.h"

#include <vector>



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      class List
      {

        public:
          List() :
            total_volume (0.0),
            total_count (0) { }

          ~List()
          {
            for (std::vector<Base*>::iterator i = seeders.begin(); i != seeders.end(); ++i) {
              delete *i;
              *i = NULL;
            }
          }


          void add (Base* const in);
          void clear();
          bool get_seed (Point<float>& p, Point<float>& d);


          size_t num_seeds() const { return seeders.size(); }
          const Base* operator[] (const size_t n) const { return seeders[n]; }
          bool is_finite() const { return total_count; }
          uint32_t get_total_count() const { return total_count; }
          const Math::RNG& get_rng() const { return rng; }


          friend inline std::ostream& operator<< (std::ostream& stream, const List& S) {
            if (S.seeders.empty())
              return stream;
            std::vector<Base*>::const_iterator i = S.seeders.begin();
            stream << **i;
            for (++i; i != S.seeders.end(); ++i)
              stream << ", " << **i;
            return (stream);
          }


        private:
          std::vector<Base*> seeders;
          Math::RNG rng;
          float total_volume;
          uint32_t total_count;

      };





      }
    }
  }
}

#endif

