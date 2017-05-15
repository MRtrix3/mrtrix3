/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <cctype>

#include "header.h"
#include "file/ofstream.h"
#include "file/mgh.h"
#include "file/nifti1_utils.h"

namespace MR
{
  namespace File
  {
    namespace MGH
    {



      std::string tag_ID_to_string (const int32_t tag)
      {
        switch (tag) {
          case 1: return "MGH_TAG_OLD_COLORTABLE";
          case 2: return "MGH_TAG_OLD_USEREALRAS";
          case 3: return "MGH_TAG_CMDLINE";
          case 4: return "MGH_TAG_USEREALRAS";
          case 5: return "MGH_TAG_COLORTABLE";

          case 10: return "MGH_TAG_GCAMORPH_GEOM";
          case 11: return "MGH_TAG_GCAMORPH_TYPE";
          case 12: return "MGH_TAG_GCAMORPH_LABELS";

          case 20: return "MGH_TAG_OLD_SURF_GEOM";
          case 21: return "MGH_TAG_SURF_GEOM";

          case 30: return "MGH_TAG_OLD_MGH_XFORM";
          case 31: return "MGH_TAG_MGH_XFORM";
          case 32: return "MGH_TAG_GROUP_AVG_SURFACE_AREA";
          case 33: return "MGH_TAG_AUTO_ALIGN";

          case 40: return "MGH_TAG_SCALAR_DOUBLE";
          case 41: return "MGH_TAG_PEDIR";
          case 42: return "MGH_TAG_MRI_FRAME";
          case 43: return "MGH_TAG_FIELDSTRENGTH";

          default: break;
        }
        return "MGH_TAG_" + str(tag);
      }



      int32_t string_to_tag_ID (const std::string& key)
      {
        if (key.compare (0, 8, "MGH_TAG_") == 0) {

          auto id = key.substr (8);

          if (id == "OLD_COLORTABLE") return 1;
          if (id == "OLD_USEREALRAS") return 2;
          if (id == "CMDLINE") return 3;
          if (id == "USEREALRAS") return 4;
          if (id == "COLORTABLE") return 5;

          if (id == "GCAMORPH_GEOM") return 10;
          if (id == "GCAMORPH_TYPE") return 11;
          if (id == "GCAMORPH_LABELS") return 12;

          if (id == "OLD_SURF_GEOM") return 20;
          if (id == "SURF_GEOM") return 21;

          if (id == "OLD_MGH_XFORM") return 30;
          if (id == "MGH_XFORM") return 31;
          if (id == "GROUP_AVG_SURFACE_AREA") return 32;
          if (id == "AUTO_ALIGN") return 33;

          if (id == "SCALAR_DOUBLE") return 40;
          if (id == "PEDIR") return 41;
          if (id == "MRI_FRAME") return 42;
          if (id == "FIELDSTRENGTH") return 43;
        }

        return 0;
      }




    }
  }
}


