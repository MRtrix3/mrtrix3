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
