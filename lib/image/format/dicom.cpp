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


#include "file/path.h"
#include "file/config.h"
#include "get_set.h"
#include "file/dicom/mapper.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/tree.h"
#include "image/format/list.h"
#include "image/header.h"
#include "image/handler/base.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      RefPtr<Handler::Base> DICOM::read (Header& H) const
      {
        if (!Path::is_dir (H.name())) 
          return RefPtr<Handler::Base>();

        File::Dicom::Tree dicom;

        dicom.read (H.name());
        dicom.sort();

        std::vector< RefPtr<File::Dicom::Series> > series = File::Dicom::select_func (dicom);
        if (series.empty()) 
          throw Exception ("no DICOM series selected");

        return dicom_to_mapper (H, series);
      }


      bool DICOM::check (Header& H, size_t num_axes) const
      {
        return false;
      }

      RefPtr<Handler::Base> DICOM::create (Header& H) const
      {
        assert (0);
        return RefPtr<Handler::Base>();
      }


    }
  }
}
