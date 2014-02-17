#include "image/handler/base.h"
#include "image/header.h"

namespace MR
{
  namespace Image
  {
    namespace Handler
    {
      Base::Base (const Image::Header& header) : 
        name (header.name()), 
        datatype (header.datatype()),
        segsize (Image::voxel_count (header)),
        is_new (false),
        writable (false) { }


      Base::~Base ()
      {
        close();
      }


      void Base::open ()
      {
        if (addresses.size())
          return;

        load();
        DEBUG ("image \"" + name + "\" loaded");
      }



      void Base::close ()
      {
        if (addresses.empty())
          return;

        unload();
        DEBUG ("image \"" + name + "\" unloaded");
        addresses.clear();
      }

      void Base::unload() { }

    }
  }
}

