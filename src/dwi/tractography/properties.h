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

