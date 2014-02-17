#ifndef __file_key_value_h__
#define __file_key_value_h__

#include <fstream>
#include "mrtrix.h"

namespace MR
{
  namespace File
  {

    class KeyValue
    {
      public:
        KeyValue () { }
        KeyValue (const std::string& file, const char* first_line = NULL) {
          open (file, first_line);
        }

        void  open (const std::string& file, const char* first_line = NULL);
        bool  next ();
        void  close () {
          in.close();
        }

        const std::string& key () const throw ()   {
          return (K);
        }
        const std::string& value () const throw () {
          return (V);
        }
        const std::string& name () const throw ()  {
          return (filename);
        }

      protected:
        std::string K, V, filename;
        std::ifstream in;
    };

  }
}

#endif

