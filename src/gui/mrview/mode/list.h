#ifndef MODE

// place #include files in here:
#include "gui/mrview/mode/mode2d.h"
#include "gui/mrview/mode/mode3d.h"
#include "gui/mrview/mode/ortho.h"
#include "gui/mrview/mode/volume.h"

#else 

/* Provide a corresponding line for each mode here:
The first argument is the class name, as defined in the corresponding header.
The second argument is the name of the mode as displayed to the user. 
The third argument is a brief description of the mode, to be displayed in a tooltip. */

MODE (Mode2D, 2D, display slices aligned with image axes)
MODE_OPTION (Mode3D, 3D reslice, display slices at an arbitrary angle)
MODE (Ortho, OrthoView, axial-coronal-sagittal display)
MODE (Volume, Volume, volumetric render)

#endif

