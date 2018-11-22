/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "gui/mrview/tool/odf/model.h"

#include "header.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        size_t ODF_Model::add_items (const vector<std::string>& list,
                                     const odf_type_t type,
                                     const bool colour_by_direction,
                                     const bool hide_negative_lobes,
                                     const float scale)
        {
          vector<std::unique_ptr<MR::Header>> hlist;
          for (size_t i = 0; i < list.size(); ++i) {
            try {
              auto header = make_unique<MR::Header> (MR::Header::open (list[i]));
              switch (type) {
                case odf_type_t::SH:
                  Math::SH::check (*header);
                  break;
                case odf_type_t::TENSOR:
                  if (header->ndim() != 4)
                    throw Exception ("Image \"" + Path::basename (header->name()) + "\" is not 4D; not a tensor image");
                  if (header->size(3) != 6)
                    throw Exception ("Image \"" + Path::basename (header->name()) + "\" does not contain 6 volumes; not a tensor image");
                  break;
                case odf_type_t::DIXEL:
                  if (header->ndim() != 4)
                    throw Exception ("Image \"" + Path::basename (header->name()) + "\" is not 4D; cannot contain direction amplitudes");
                  break;
              }
              hlist.push_back (std::move (header));
            }
            catch (Exception& E) {
              E.display();
            }
          }

          if (hlist.size()) {
            beginInsertRows (QModelIndex(), items.size(), items.size()+hlist.size());
            for (size_t i = 0; i < hlist.size(); ++i)
              items.push_back (make_unique<ODF_Item> (std::move (*hlist[i]), type, scale, hide_negative_lobes, colour_by_direction));
            endInsertRows();
          }

          return hlist.size();
        }



      }
    }
  }
}





