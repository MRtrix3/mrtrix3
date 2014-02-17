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


