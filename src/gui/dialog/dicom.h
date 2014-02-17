#ifndef __dialog_dicom_h__
#define __dialog_dicom_h__

#include "gui/opengl/gl.h"
#include "file/dicom/tree.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      using namespace MR::File::Dicom;

      std::vector< RefPtr<Series> > select_dicom (const Tree& tree);

    }
  }
}

#endif


