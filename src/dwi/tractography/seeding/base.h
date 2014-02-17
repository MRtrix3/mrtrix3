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


#ifndef __dwi_tractography_seeding_base_h__
#define __dwi_tractography_seeding_base_h__


#include "file/path.h"
#include "point.h"

#include "image/loop.h"
#include "image/voxel.h"

#include "math/rng.h"

#include "thread/mutex.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




      template <typename T>
      uint32_t get_count (T& data)
      {
        typename T::voxel_type v (data);
        uint32_t count = 0;
        Image::Loop loop;
        for (loop.start (v); loop.ok(); loop.next (v)) {
          if (v.value())
            ++count;
        }
        return count;
      }


      template <typename T>
      float get_volume (T& data)
      {
        typename T::voxel_type v (data);
        float volume = 0;
        Image::Loop loop;
        for (loop.start (v); loop.ok(); loop.next (v))
          volume += v.value();
        return volume;
      }




      // Common interface for providing streamline seeds
      class Base {

        public:
          Base (const std::string& in, const Math::RNG& seed_rng, const std::string& desc) :
            volume (0.0),
            count (0),
            rng (seed_rng),
            type (desc),
            name (Path::basename (in)) { }

          virtual ~Base() { }

          float vol() const { return volume; }
          uint32_t num() const { return count; }
          bool is_finite() const { return count; }
          const std::string& get_type() const { return type; }
          const std::string& get_name() const { return name; }
          virtual bool get_seed (Point<float>& p) { throw Exception ("Calling empty virtual function Seeder_base::get_seed()!"); return false; }
          virtual bool get_seed (Point<float>& p, Point<float>& d) { return get_seed (p); }

          friend inline std::ostream& operator<< (std::ostream& stream, const Base& B) {
            stream << B.name;
            return (stream);
          }


        protected:
          // Finite seeds are defined by the number of seeds; non-limited are defined by volume
          float volume;
          uint32_t count;
          // These are not used by all possible seed classes, but it's easier to have them within the base class anyway
          Math::RNG rng;
          Thread::Mutex mutex;
          const std::string type; // Text describing the type of seed this is

        private:
          const std::string name; // Could be an image path, or spherical coordinates

      };





      }
    }
  }
}

#endif

