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

MODE_OPTION (Slice, slice, single slice, display a single slice)
MODE (Ortho, ortho, OrthoView, axial-coronal-sagittal display)
MODE (Volume, volume, Volume, volumetric render)
MODE (LightBox, lightbox, LightBox, light-box display)

#endif

