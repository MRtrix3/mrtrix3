/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef MODE

// place #include files in here:
#include "gui/mrview/mode/slice.h"
#include "gui/mrview/mode/ortho.h"
#include "gui/mrview/mode/volume.h"
#include "gui/mrview/mode/lightbox.h"

#else 

/* Provide a corresponding line for each mode here:
The first argument is the class name, as defined in the corresponding header.
The second argument is the identifier to select this mode with the -mode option.
The third argument is the name of the mode as displayed in the menu. 
The fourth argument is a brief description of the mode, to be displayed in a tooltip.

Use the MODE_OPTION variant if your mode supplies its own command-line options. */

MODE_OPTION (Slice, slice, Single slice, Single slice display)
MODE (Ortho, ortho, Ortho view, Composite axial-coronal-sagittal display)
MODE (Volume, volume, Volume render, Volumetric render)
MODE (LightBox, lightbox, Light box, Light box display)

#endif

