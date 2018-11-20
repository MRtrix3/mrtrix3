/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
          case MGH_TAG_OLD_COLORTABLE:         return "MGH_TAG_OLD_COLORTABLE";
          case MGH_TAG_OLD_USEREALRAS:         return "MGH_TAG_OLD_USEREALRAS";
          case MGH_TAG_CMDLINE:                return "MGH_TAG_CMDLINE";
          case MGH_TAG_USEREALRAS:             return "MGH_TAG_USEREALRAS";
          case MGH_TAG_COLORTABLE:             return "MGH_TAG_COLORTABLE";

          case MGH_TAG_GCAMORPH_GEOM:          return "MGH_TAG_GCAMORPH_GEOM";
          case MGH_TAG_GCAMORPH_TYPE:          return "MGH_TAG_GCAMORPH_TYPE";
          case MGH_TAG_GCAMORPH_LABELS:        return "MGH_TAG_GCAMORPH_LABELS";

          case MGH_TAG_OLD_SURF_GEOM:          return "MGH_TAG_OLD_SURF_GEOM";
          case MGH_TAG_SURF_GEOM:              return "MGH_TAG_SURF_GEOM";

          case MGH_TAG_OLD_MGH_XFORM:          return "MGH_TAG_OLD_MGH_XFORM";
          case MGH_TAG_MGH_XFORM:              return "MGH_TAG_MGH_XFORM";
          case MGH_TAG_GROUP_AVG_SURFACE_AREA: return "MGH_TAG_GROUP_AVG_SURFACE_AREA";
          case MGH_TAG_AUTO_ALIGN:             return "MGH_TAG_AUTO_ALIGN";

          case MGH_TAG_SCALAR_DOUBLE:          return "MGH_TAG_SCALAR_DOUBLE";
          case MGH_TAG_PEDIR:                  return "MGH_TAG_PEDIR";
          case MGH_TAG_MRI_FRAME:              return "MGH_TAG_MRI_FRAME";
          case MGH_TAG_FIELDSTRENGTH:          return "MGH_TAG_FIELDSTRENGTH";

          default: break;
        }
        return "MGH_TAG_" + str(tag);
      }



      int32_t string_to_tag_ID (const std::string& key)
      {
        if (key.compare (0, 8, "MGH_TAG_") == 0) {

          auto id = key.substr (8);

          if (id == "OLD_COLORTABLE") return MGH_TAG_OLD_COLORTABLE;
          if (id == "OLD_USEREALRAS") return MGH_TAG_OLD_USEREALRAS;
          if (id == "CMDLINE") return MGH_TAG_CMDLINE;
          if (id == "USEREALRAS") return MGH_TAG_USEREALRAS;
          if (id == "COLORTABLE") return MGH_TAG_COLORTABLE;

          if (id == "GCAMORPH_GEOM") return MGH_TAG_GCAMORPH_GEOM;
          if (id == "GCAMORPH_TYPE") return MGH_TAG_GCAMORPH_TYPE;
          if (id == "GCAMORPH_LABELS") return MGH_TAG_GCAMORPH_LABELS;

          if (id == "OLD_SURF_GEOM") return MGH_TAG_OLD_SURF_GEOM;
          if (id == "SURF_GEOM") return MGH_TAG_SURF_GEOM;

          if (id == "OLD_MGH_XFORM") return MGH_TAG_OLD_MGH_XFORM;
          if (id == "MGH_XFORM") return MGH_TAG_MGH_XFORM;
          if (id == "GROUP_AVG_SURFACE_AREA") return MGH_TAG_GROUP_AVG_SURFACE_AREA;
          if (id == "AUTO_ALIGN") return MGH_TAG_AUTO_ALIGN;

          if (id == "SCALAR_DOUBLE") return MGH_TAG_SCALAR_DOUBLE;
          if (id == "PEDIR") return MGH_TAG_PEDIR;
          if (id == "MRI_FRAME") return MGH_TAG_MRI_FRAME;
          if (id == "FIELDSTRENGTH") return MGH_TAG_FIELDSTRENGTH;
        }

        return 0;
      }




    }
  }
}


