/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __gui_mrview_tool_tractogram_enums_h__
#define __gui_mrview_tool_tractogram_enums_h__
 
//#include "gui/mrview/displayable.h"
//#include "dwi/tractography/properties.h"
//#include "gui/mrview/tool/tractography/tractography.h"
//#include "gui/mrview/colourmap.h"


namespace MR
{

	namespace GUI
	{

		namespace MRView
		{
			namespace Tool
			{

				enum class TrackColourType { Direction, Ends, Manual, ScalarFile };
				enum class TrackThresholdType { None, UseColourFile, SeparateFile };
			}
		}
	}
}


#endif

