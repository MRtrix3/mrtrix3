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
#ifndef TOOL

// place #include files in here:
#include "gui/mrview/tool/view.h"
#include "gui/mrview/tool/roi_editor/roi.h"
#include "gui/mrview/tool/overlay.h"
#include "gui/mrview/tool/odf/odf.h"
#include "gui/mrview/tool/fixel/fixel.h"
#include "gui/mrview/tool/screen_capture.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/mrview/tool/connectome/connectome.h"

#else

/* Provide a corresponding line for each mode here:
The first argument is the name of the corresponding Mode class.
The second argument is the name of the mode as displayed in the menu.
The third argument is the text to be shown in the menu tooltip. */

TOOL(View, View options, Adjust view settings)
TOOL(Overlay, Overlay, Overlay other images over the current image)
TOOL(ROI, ROI editor, View & edit regions of interest)
TOOL(Tractography, Tractography, Display tracks over the current image)
TOOL(ODF, ODF Display, Display orientation density functions)
TOOL(Fixel, Fixel Plot, Plot fixel images)
TOOL(Connectome, Connectome, Plot connectome properties)
TOOL(Capture, Screen capture, Capture the screen as a png file)

#endif



