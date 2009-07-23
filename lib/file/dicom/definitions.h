/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __file_dicom_dict_h__
#define __file_dicom_dict_h__

#define VR_OB 0x4F42U
#define VR_OW 0x4F57U
#define VR_OF 0x4F46U
#define VR_SQ 0x5351U
#define VR_UN 0x554EU
#define VR_AE 0x4145U
#define VR_AS 0x4153U
#define VR_AT 0x4154U
#define VR_CS 0x4353U
#define VR_DA 0x4441U
#define VR_DS 0x4453U
#define VR_DT 0x4454U
#define VR_FD 0x4644U
#define VR_FL 0x464CU
#define VR_IS 0x4953U
#define VR_LO 0x4C4FU
#define VR_LT 0x4C54U
#define VR_PN 0x504EU
#define VR_SH 0x5348U
#define VR_SL 0x534CU
#define VR_SS 0x5353U
#define VR_ST 0x5354U
#define VR_TM 0x544DU
#define VR_UI 0x5549U
#define VR_UL 0x554CU
#define VR_US 0x5553U
#define VR_UT 0x5554U

#define LENGTH_UNDEFINED           0xFFFFFFFFUL

#define GROUP_BYTE_ORDER                0x0002U
#define GROUP_BYTE_ORDER_SWAPPED        0x0200U
#define GROUP_SEQUENCE                  0xFFFEU 
#define GROUP_DATA                      0x7FE0U 

#define ELEMENT_TRANSFER_SYNTAX_UID     0x0010U
#define ELEMENT_SEQUENCE_ITEM           0xE000U
#define ELEMENT_SEQUENCE_DELIMITATION_ITEM  0xE0DDU
#define ELEMENT_DATA                    0x0010U



namespace MR {
  namespace File {
    namespace Dicom {

      inline std::string format_date (const std::string& date)
      {
        if (date.empty()) return (date);
        return (date.substr(6,2) + "/" + date.substr(4,2) + "/" + date.substr(0,4));
      }



      inline std::string format_time (const std::string& time)
      {
        if (time.empty()) return (time);
        return (time.substr(0,2) + ":" + time.substr(2,2) + ":" + time.substr(4,2));
      }



      inline std::string format_ID (const std::string& ID)
      {
        if (ID.empty()) return (ID);
        return ("(" + ID + ")");
      }


    }
  }
}


#endif
