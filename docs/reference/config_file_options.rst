.. _config_file_options:

################
List of MRtrix3 configuration file options
################

*  **AmbientIntensity**
    *default: 0.6*

     The default intensity for the ambient light in OpenGL renders.

*  **AnalyseLeftToRight**
    *default: 0 (false)*

     A boolean value to indicate whether images in Analyse format should be assumed to be in LAS orientation (default) or RAS (when this is option is turned on).

*  **BValueScaling**
    *default: 1 (true)*

     Specifies whether the b-values should be scaled by the squared norm of the gradient vectors when loading a DW gradient scheme. This is commonly required to correctly interpret images acquired on scanners that nominally only allow a single b-value, as the common workaround is to scale the gradient vectors to modulate the actual b-value.

*  **BZeroThreshold**
    *default: 10.0*

     Specifies the b-value threshold for determining those image volumes that correspond to b=0.

*  **BackgroundColor**
    *default: 1,1,1 (white)*

     The default colour to use for the background in OpenGL panels, notably the SH viewer.

*  **ConnectomeEdgeAssociatedAlphaMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the transparency of edges connected to one selected node.

*  **ConnectomeEdgeAssociatedColour**
    *default: 0.0,0.0,0.0*

     The colour mixed in to edges connected to one currently selected node.

*  **ConnectomeEdgeAssociatedColourFade**
    *default: 0.5*

     The fraction of the colour of an edge connected to one selected node determined by the fixed colour.

*  **ConnectomeEdgeAssociatedSizeMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the size of edges connected to one selected node.

*  **ConnectomeEdgeOtherAlphaMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the transparency of edges not connected to any selected node.

*  **ConnectomeEdgeOtherColour**
    *default: 0.0,0.0,0.0*

     The colour mixed in to edges not connected to any currently selected node.

*  **ConnectomeEdgeOtherColourFade**
    *default: 0.75*

     The fraction of the colour of an edge not connected to any selected node determined by the fixed colour.

*  **ConnectomeEdgeOtherSizeMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the size of edges not connected to any selected node.

*  **ConnectomeEdgeOtherVisibilityOverride**
    *default: true*

     Whether or not to force invisibility of edges not connected to any selected node.

*  **ConnectomeEdgeSelectedAlphaMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the transparency of edges connected to two selected nodes.

*  **ConnectomeEdgeSelectedColour**
    *default: 0.9,0.9,1.0*

     The colour used to highlight the edges connected to two currently selected nodes.

*  **ConnectomeEdgeSelectedColourFade**
    *default: 0.5*

     The fraction of the colour of an edge connected to two selected nodes determined by the fixed selection highlight colour.

*  **ConnectomeEdgeSelectedSizeMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the size of edges connected to two selected nodes.

*  **ConnectomeEdgeSelectedVisibilityOverride**
    *default: false*

     Whether or not to force visibility of edges connected to two selected nodes.

*  **ConnectomeNodeAssociatedAlphaMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the transparency of nodes associated with a selected node.

*  **ConnectomeNodeAssociatedColour**
    *default: 0.0,0.0,0.0*

     The colour mixed in to those nodes associated with any selected node.

*  **ConnectomeNodeAssociatedColourFade**
    *default: 0.5*

     The fraction of the colour of an associated node determined by the fixed associated highlight colour.

*  **ConnectomeNodeAssociatedSizeMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the size of nodes associated with a selected node.

*  **ConnectomeNodeOtherAlphaMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the transparency of nodes not currently selected nor associated with a selected node.

*  **ConnectomeNodeOtherColour**
    *default: 0.0,0.0,0.0*

     The colour mixed in to those nodes currently not selected nor associated with any selected node.

*  **ConnectomeNodeOtherColourFade**
    *default: 0.75*

     The fraction of the colour of an unselected, non-associated node determined by the fixed not-selected highlight colour.

*  **ConnectomeNodeOtherSizeMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the size of nodes not currently selected nor associated with a selected node.

*  **ConnectomeNodeOtherVisibilityOverride**
    *default: false*

     Whether or not nodes are forced to be invisible when not selected or associated with any selected node.

*  **ConnectomeNodeSelectedAlphaMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the transparency of selected nodes.

*  **ConnectomeNodeSelectedColour**
    *default: 1.0,1.0,1.0*

     The colour used to highlight those nodes currently selected.

*  **ConnectomeNodeSelectedColourFade**
    *default: 0.75*

     The fraction of the colour of a selected node determined by the fixed selection highlight colour.

*  **ConnectomeNodeSelectedSizeMultiplier**
    *default: 1.0*

     The multiplicative factor to apply to the size of selected nodes.

*  **ConnectomeNodeSelectedVisibilityOverride**
    *default: true*

     Whether or not nodes are forced to be visible when selected.

*  **DiffuseIntensity**
    *default: 0.3*

     The default intensity for the diffuse light in OpenGL renders.

*  **FailOnWarn**
    *default: 0 (false)*

     A boolean value specifying whether MRtrix applications should abort as soon as any (otherwise non-fatal) warning is issued.

*  **HelpCommand**
    *default: less*

     The command to use to display each command's help page (leave empty to send directly to the terminal).

*  **IconSize**
    *default: 24*

     The size of the icons in the main MRView toolbar.

*  **ImageInterpolation**
    *default: true*

     Define default interplation setting for image and image overlay.

*  **ImageInterpolation**
    *default: true*

     Interpolation switched on in the main image

*  **InitialToolBarPosition**
    *default: top*

     The starting position of the MRView toolbar. Valid values are: top, bottom, left, right.

*  **LightPosition**
    *default: 1,1,3*

     The default position vector to use for the light in OpenGL renders.

*  **MRViewColourBarHeight**
    *default: 100*

     The height of the colourbar in MRView, in pixels.

*  **MRViewColourBarInset**
    *default: 20*

     How far away from the edge of the main window to place the colourbar in MRView, in pixels.

*  **MRViewColourBarPosition**
    *default: bottomright*

     The position of the colourbar within the main window in MRView. Valid values are: bottomleft, bottomright, topleft, topright.

*  **MRViewColourBarTextOffset**
    *default: 10*

     How far away from the colourbar to place the associated text, in pixels.

*  **MRViewColourBarWidth**
    *default: 20*

     The width of the colourbar in MRView, in pixels.

*  **MRViewColourHorizontalPadding**
    *default: 100*

     The width in pixels between horizontally adjacent colour bars.

*  **MRViewDockFloating**
    *default: 0 (false)*

     Whether MRView tools should start docked in the main window, or floating (detached from the main window).

*  **MRViewFocusModifierKey**
    *default: meta (cmd on MacOSX)*

     Modifier key to select focus mode in MRView. Valid choices include shift, alt, ctrl, meta (on MacOSX: shift, alt, ctrl, cmd).

*  **MRViewImageBackgroundColour**
    *default: 0,0,0 (black)*

     The default image background colour in the main MRView window.

*  **MRViewMaxNumColourmapRows**
    *default: 3*

     The maximal number of rows used to layout a collection of rendered colourbars Note, that all tool-specific colourbars will form a single collection.

*  **MRViewMoveModifierKey**
    *default: shift*

     Modifier key to select move mode in MRView. Valid choices include shift, alt, ctrl, meta (on MacOSX: shift, alt, ctrl, cmd).

*  **MRViewRotateModifierKey**
    *default: ctrl*

     Modifier key to select rotate mode in MRView. Valid choices include shift, alt, ctrl, meta (on MacOSX: shift, alt, ctrl, cmd).

*  **MRViewShowColourbar**
    *default: true*

     Colourbar shown in main image overlay

*  **MRViewShowComments**
    *default: true*

     Comments shown in main image overlay

*  **MRViewShowFocus**
    *default: true*

     Focus cross hair shown in main image

*  **MRViewShowOrientationLabel**
    *default: true*

     Anatomical orientation information shown in main image overlay

*  **MRViewShowVoxelInformation**
    *default: true*

     Voxel information shown in main image overlay

*  **MRViewToolFontSize**
    *default: 2 points less than the standard system font*

     The point size for the font to use in MRView tools.

*  **MRViewToolsColourBarPosition**
    *default: topright*

     The position of all visible tool colourbars within the main window in MRView. Valid values are: bottomleft, bottomright, topleft, topright.

*  **MSAA**
    *default: 0 (false)*

     How many samples to use for multi-sample anti-aliasing (to improve display quality).

*  **NIFTI.AllowBitwise**
    *default: 0 (false)*

     A boolean value to indicate whether bitwise storage of binary data is permitted (most 3rd party software packages don't support bitwise data). If false (the default), data will be stored using more widely supported unsigned 8-bit integers.

*  **NIFTI.AlwaysUseVer2**
    *default: 0 (false)*

     A boolean value to indicate whether NIfTI images should always be written in the new NIfTI-2 format. If false, images will be written in the older NIfTI-1 format by default, with the exception being files where the number of voxels along any axis exceeds the maximum permissible in that format (32767), in which case the output file will automatically switch to the NIfTI-2 format.

*  **NIfTI.AutoLoadJSON**
    *default: 0 (false)*

     A boolean value to indicate whether, when opening NIfTI images, any corresponding JSON file should be automatically loaded

*  **NIfTI.AutoSaveJSON**
    *default: 0 (false)*

     A boolean value to indicate whether, when writing NIfTI images, a corresponding JSON file should be automatically created in order to save any header entries that cannot be stored in the NIfTI header

*  **NeedOpenGLCoreProfile**
    *default: 1 (true)*

     Whether the creation of an OpenGL 3.3 context requires it to be a core profile (needed on newer versions of the ATI drivers on Linux, for instance).

*  **NumberOfThreads**
    *default: number of threads provided by hardware*

     Set the default number of CPU threads to use for multi-threading.

*  **NumberOfUndos**
    *default: 16*

     The number of undo operations permitted in the MRView ROI editor tool.

*  **ObjectColor**
    *default: 1,1,0 (yellow)*

     The default colour to use for objects (i.e. SH glyphs) when not colouring by direction.

*  **SparseDataInitialSize**
    *default: 16777216*

     Initial buffer size for data in MRtrix sparse image format file (in bytes).

*  **SpecularExponent**
    *default: 1*

     The default exponent for the specular light in OpenGL renders.

*  **SpecularIntensity**
    *default: 0.4*

     The default intensity for the specular light in OpenGL renders.

*  **TckgenEarlyExit**
    *default: 0 (false)*

     Specifies whether tckgen should be terminated prematurely in cases where it appears as though the target number of accepted streamlines is not going to be met.

*  **TerminalColor**
    *default: 1 (true)*

     A boolean value to indicate whether colours should be used in the terminal.

*  **TmpFileDir**
    *default: `/tmp` (on Unix), `.` (on Windows)*

     The prefix for temporary files (as used in pipelines). By default, these files get written to the current folder, which may cause performance issues when operating over distributed file systems. In this case, it may be better to specify `/tmp/` here.

*  **TmpFilePrefix**
    *default: `mrtrix-tmp-`*

     The prefix to use for the basename of temporary files. This will be used to generate a unique filename for the temporary file, by adding random characters to this prefix, followed by a suitable suffix (depending on file type). Note that this prefix can also be manipulated using the `MRTRIX_TMPFILE_PREFIX` environment variable, without editing the config file.

*  **ToolbarStyle**
    *default: 2*

     The style of the main toolbar buttons in MRView. See Qt's documentation for Qt::ToolButtonStyle.

*  **TrackWriterBufferSize**
    *default: 16777216*

     The size of the write-back buffer (in bytes) to use when writing track files. MRtrix will store the output tracks in a relatively large buffer to limit the number of write() calls, avoid associated issues such as file fragmentation.

*  **VSync**
    *default: 0 (false)*

     Whether the screen update should synchronise with the monitor's vertical refresh (to avoid tearing artefacts).

*  **reg_analyse_descent**
    *default: 0 (false)*

     Linear registration: write comma separated gradient descent parameters and gradients to stdout and verbose gradient descent output to stderr

*  **reg_bbgd**
    *default: 1 (true)*

     Linear registration: use Barzilai Borwein gradient descent

*  **reg_coherence_len**
    *default: 3.0*

     Linear registration: estimated spatial coherence length in voxels

*  **reg_gd_convergence_data_smooth**
    *default: 0.8*

     Linear registration: control point trajectory smoothing value used in convergence check parameter range: [0...1]

*  **reg_gd_convergence_min_iter**
    *default: 10*

     Linear registration: minumum number of iterations until convergence check is activated

*  **reg_gd_convergence_slope_smooth**
    *default: 0.1*

     Linear registration: control point trajectory slope smoothing value used in convergence check parameter range: [0...1]

*  **reg_gd_convergence_thresh**
    *default: 5e-3*

     Linear registration: threshold for convergence check using the smoothed control point trajectories measured in fraction of a voxel

*  **reg_gdweight_matrix**
    *default: 0.0003*

     Linear registration: weight for optimisation of linear (3x3) matrix parameters

*  **reg_gdweight_translation**
    *default: 1*

     Linear registration: weight for optimisation of translation parameters

*  **reg_stop_len**
    *default: 0.0001*

     Linear registration: smallest gradient descent step measured in fraction of a voxel at which to stop registration

