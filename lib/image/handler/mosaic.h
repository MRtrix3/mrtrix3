#ifndef __image_handler_mosaic_h__
#define __image_handler_mosaic_h__

#include "image/handler/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace Image
  {

    namespace Handler
    {

      class Mosaic : public Base
      {
        public:
          Mosaic (Header& header, size_t mosaic_xdim, size_t mosaic_ydim, size_t slice_xdim, size_t slice_ydim, size_t nslices) :
            Base (header), m_xdim (mosaic_xdim), m_ydim (mosaic_ydim),
            xdim (slice_xdim), ydim (slice_ydim), slices (nslices) { 
            segsize = header.dim(0) * header.dim(1) * header.dim(2);
            }
          ~Mosaic () { close(); }

        protected:
          size_t m_xdim, m_ydim, xdim, ydim, slices;

          virtual void load ();
      };

    }
  }
}

#endif


