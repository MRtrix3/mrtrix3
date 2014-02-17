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


#ifndef __dwi_tractography_properties_h__
#define __dwi_tractography_properties_h__

#include <map>
#include "dwi/tractography/roi.h"
#include "dwi/tractography/seeding/list.h"
#include "timer.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      class Properties : public std::map<std::string, std::string> {
        public:

          Properties () : timestamp_precision (20) {
            timestamp = Timer::current_time();
          }

          double timestamp;
          ROISet include, exclude, mask;
          Seeding::List seeds;
          std::vector<std::string> comments;
          std::multimap<std::string, std::string> roi;
          const size_t timestamp_precision;


          void  clear () { 
            timestamp = 0.0;
            std::map<std::string, std::string>::clear(); 
            seeds.clear();
            include.clear();
            exclude.clear();
            mask.clear();
            comments.clear(); 
            roi.clear();
          }

          template <typename T> void set (T& variable, const std::string& name) {
            if ((*this)[name].empty()) (*this)[name] = str (variable);
            else variable = to<T> ((*this)[name]);
          }

          void load_ROIs ();
      };





      inline std::ostream& operator<< (std::ostream& stream, const Properties& P)
      {
        stream << "seeds: " << P.seeds;
        stream << "include: " << P.include << ", exclude: " << P.exclude << ", mask: " << P.mask << ", dict: ";
        for (std::map<std::string, std::string>::const_iterator i = P.begin(); i != P.end(); ++i)
          stream << "[ " << i->first << ": " << i->second << " ], ";
        stream << "comments: ";
        for (std::vector<std::string>::const_iterator i = P.comments.begin(); i != P.comments.end(); ++i)
          stream << "\"" << *i << "\", ";
        stream << "timestamp: " << P.timestamp;
        return (stream);
      }




    }
  }
}

#endif

