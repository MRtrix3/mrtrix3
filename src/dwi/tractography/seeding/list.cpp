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


#include "dwi/tractography/seeding/list.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      void List::add (Base* const in)
      {
        if (seeders.size() && !(in->is_finite() == is_finite()))
          throw Exception ("Cannot use a combination of seed types where some are number-limited and some are not!");
        seeders.push_back (in);
        total_volume += in->vol();
        total_count += in->num();
      }



      void List::clear()
      {
        for (std::vector<Base*>::iterator i = seeders.begin(); i != seeders.end(); ++i) {
          delete *i;
          *i = NULL;
        }
        seeders.clear();
        total_volume = 0.0;
        total_count = 0.0;
      }



      bool List::get_seed (Point<float>& p, Point<float>& d)
      {

        if (is_finite()) {

          for (std::vector<Base*>::iterator i = seeders.begin(); i != seeders.end(); ++i) {
            if ((*i)->get_seed (p, d))
              return true;
          }
          p.invalidate();
          return false;

        } else {

          if (seeders.size() == 1)
            return seeders.front()->get_seed (p, d);

          do {
            float incrementer = 0.0;
            const float sample = rng.uniform() * total_volume;
            for (std::vector<Base*>::iterator i = seeders.begin(); i != seeders.end(); ++i) {
              if ((incrementer += (*i)->vol()) > sample)
                return (*i)->get_seed (p, d);
            }
          } while (1);

        }

      }




      }
    }
  }
}


