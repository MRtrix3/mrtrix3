.. _config_file_options:

##########################################
List of MRtrix3 configuration file options
##########################################

.. option:: AmbientIntensity

    *default: 0.5*

     The default intensity for the ambient light in OpenGL renders.

.. option:: AnalyseLeftToRight

    *default: 0 (false)*

     A boolean value to indicate whether images in Analyse format should be assumed to be in LAS orientation (default) or RAS (when this is option is turned on).

.. option:: BValueScaling

    *default: 1 (true)*

     Specifies whether the b-values should be scaled by the squared norm of the gradient vectors when loading a DW gradient scheme. This is commonly required to correctly interpret images acquired on scanners that nominally only allow a single b-value, as the common workaround is to scale the gradient vectors to modulate the actual b-value.

.. option:: BZeroThreshold

    *default: 10.0*

     Specifies the b-value threshold for determining those image volumes that correspond to b=0.

.. option:: BackgroundColor

    *default: 1.0,1.0,1.0*

     The default colour to use for the background in OpenGL panels, notably the SH viewer.

.. option:: ConnectomeEdgeAssociatedAlphaMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the transparency of edges connected to one selected node.

.. option:: ConnectomeEdgeAssociatedColour

    *default: 0.0,0.0,0.0*

     The colour mixed in to edges connected to one currently selected node.

.. option:: ConnectomeEdgeAssociatedColourFade

    *default: 0.5*

     The fraction of the colour of an edge connected to one selected node determined by the fixed colour.

.. option:: ConnectomeEdgeAssociatedSizeMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the size of edges connected to one selected node.

.. option:: ConnectomeEdgeOtherAlphaMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the transparency of edges not connected to any selected node.

.. option:: ConnectomeEdgeOtherColour

    *default: 0.0,0.0,0.0*

     The colour mixed in to edges not connected to any currently selected node.

.. option:: ConnectomeEdgeOtherColourFade

    *default: 0.75*

     The fraction of the colour of an edge not connected to any selected node determined by the fixed colour.

.. option:: ConnectomeEdgeOtherSizeMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the size of edges not connected to any selected node.

.. option:: ConnectomeEdgeOtherVisibilityOverride

    *default: true*

     Whether or not to force invisibility of edges not connected to any selected node.

.. option:: ConnectomeEdgeSelectedAlphaMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the transparency of edges connected to two selected nodes.

.. option:: ConnectomeEdgeSelectedColour

    *default: 0.9,0.9,1.0*

     The colour used to highlight the edges connected to two currently selected nodes.

.. option:: ConnectomeEdgeSelectedColourFade

    *default: 0.5*

     The fraction of the colour of an edge connected to two selected nodes determined by the fixed selection highlight colour.

.. option:: ConnectomeEdgeSelectedSizeMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the size of edges connected to two selected nodes.

.. option:: ConnectomeEdgeSelectedVisibilityOverride

    *default: false*

     Whether or not to force visibility of edges connected to two selected nodes.

.. option:: ConnectomeNodeAssociatedAlphaMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the transparency of nodes associated with a selected node.

.. option:: ConnectomeNodeAssociatedColour

    *default: 0.0,0.0,0.0*

     The colour mixed in to those nodes associated with any selected node.

.. option:: ConnectomeNodeAssociatedColourFade

    *default: 0.5*

     The fraction of the colour of an associated node determined by the fixed associated highlight colour.

.. option:: ConnectomeNodeAssociatedSizeMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the size of nodes associated with a selected node.

.. option:: ConnectomeNodeOtherAlphaMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the transparency of nodes not currently selected nor associated with a selected node.

.. option:: ConnectomeNodeOtherColour

    *default: 0.0,0.0,0.0*

     The colour mixed in to those nodes currently not selected nor associated with any selected node.

.. option:: ConnectomeNodeOtherColourFade

    *default: 0.75*

     The fraction of the colour of an unselected, non-associated node determined by the fixed not-selected highlight colour.

.. option:: ConnectomeNodeOtherSizeMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the size of nodes not currently selected nor associated with a selected node.

.. option:: ConnectomeNodeOtherVisibilityOverride

    *default: false*

     Whether or not nodes are forced to be invisible when not selected or associated with any selected node.

.. option:: ConnectomeNodeSelectedAlphaMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the transparency of selected nodes.

.. option:: ConnectomeNodeSelectedColour

    *default: 1.0,1.0,1.0*

     The colour used to highlight those nodes currently selected.

.. option:: ConnectomeNodeSelectedColourFade

    *default: 0.75*

     The fraction of the colour of a selected node determined by the fixed selection highlight colour.

.. option:: ConnectomeNodeSelectedSizeMultiplier

    *default: 1.0*

     The multiplicative factor to apply to the size of selected nodes.

.. option:: ConnectomeNodeSelectedVisibilityOverride

    *default: true*

     Whether or not nodes are forced to be visible when selected.

.. option:: DiffuseIntensity

    *default: 0.5*

     The default intensity for the diffuse light in OpenGL renders.

.. option:: FailOnWarn

    *default: 0 (false)*

     A boolean value specifying whether MRtrix applications should abort as soon as any (otherwise non-fatal) warning is issued.

.. option:: FontSize

    *default: 10*

     The size (in points) of the font to be used in OpenGL viewports (mrview and shview).

.. option:: HelpCommand

    *default: less*

     The command to use to display each command's help page (leave empty to send directly to the terminal).

.. option:: IconSize

    *default: 30*

     The size of the icons in the main MRView toolbar.

.. option:: ImageInterpolation

    *default: true*

     Interpolation switched on in the main image.

.. option:: InitialToolBarPosition

    *default: top*

     The starting position of the MRView toolbar. Valid values are: top, bottom, left, right.

.. option:: LightPosition

    *default: 1.0,1.0,3.0*

     The default position vector to use for the light in OpenGL renders.

.. option:: MRViewColourBarHeight

    *default: 100*

     The height of the colourbar in MRView, in pixels.

.. option:: MRViewColourBarInset

    *default: 20*

     How far away from the edge of the main window to place the colourbar in MRView, in pixels.

.. option:: MRViewColourBarPosition

    *default: bottomright*

     The position of the colourbar within the main window in MRView. Valid values are: bottomleft, bottomright, topleft, topright.

.. option:: MRViewColourBarTextOffset

    *default: 10*

     How far away from the colourbar to place the associated text, in pixels.

.. option:: MRViewColourBarWidth

    *default: 20*

     The width of the colourbar in MRView, in pixels.

.. option:: MRViewColourHorizontalPadding

    *default: 100*

     The width in pixels between horizontally adjacent colour bars.

.. option:: MRViewDefaultTractGeomType

    *default: Pseudotubes*

     The default geometry type used to render tractograms. Options are Pseudotubes, Lines or Points

.. option:: MRViewDockFloating

    *default: 0 (false)*

     Whether MRView tools should start docked in the main window, or floating (detached from the main window).

.. option:: MRViewFocusModifierKey

    *default: meta (cmd on MacOSX)*

     Modifier key to select focus mode in MRView. Valid choices include shift, alt, ctrl, meta (on MacOSX: shift, alt, ctrl, cmd).

.. option:: MRViewImageBackgroundColour

    *default: 0,0,0 (black)*

     The default image background colour in the main MRView window.

.. option:: MRViewInitWindowSize

    *default: 512,512*

     Initial window size of MRView in pixels.

.. option:: MRViewMaxNumColourmapRows

    *default: 3*

     The maximal number of rows used to layout a collection of rendered colourbars Note, that all tool-specific colourbars will form a single collection.

.. option:: MRViewMoveModifierKey

    *default: shift*

     Modifier key to select move mode in MRView. Valid choices include shift, alt, ctrl, meta (on MacOSX: shift, alt, ctrl, cmd).

.. option:: MRViewOdfScale

    *default: 1.0*

     The factor by which the ODF overlay is scaled.

.. option:: MRViewRotateModifierKey

    *default: ctrl*

     Modifier key to select rotate mode in MRView. Valid choices include shift, alt, ctrl, meta (on MacOSX: shift, alt, ctrl, cmd).

.. option:: MRViewShowColourbar

    *default: true*

     Colourbar shown in main image overlay.

.. option:: MRViewShowComments

    *default: true*

     Comments shown in main image overlay.

.. option:: MRViewShowFocus

    *default: true*

     Focus cross hair shown in main image.

.. option:: MRViewShowOrientationLabel

    *default: true*

     Anatomical orientation information shown in main image overlay.

.. option:: MRViewShowVoxelInformation

    *default: true*

     Voxel information shown in main image overlay.

.. option:: MRViewToolFontSize

    *default: 2 points less than the standard system font*

     The point size for the font to use in MRView tools.

.. option:: MRViewToolsColourBarPosition

    *default: topright*

     The position of all visible tool colourbars within the main window in MRView. Valid values are: bottomleft, bottomright, topleft, topright.

.. option:: MSAA

    *default: 0 (false)*

     How many samples to use for multi-sample anti-aliasing (to improve display quality).

.. option:: NIfTIAllowBitwise

    *default: 0 (false)*

     A boolean value to indicate whether bitwise storage of binary data is permitted (most 3rd party software packages don't support bitwise data). If false (the default), data will be stored using more widely supported unsigned 8-bit integers.

.. option:: NIfTIAlwaysUseVer2

    *default: 0 (false)*

     A boolean value to indicate whether NIfTI images should always be written in the new NIfTI-2 format. If false, images will be written in the older NIfTI-1 format by default, with the exception being files where the number of voxels along any axis exceeds the maximum permissible in that format (32767), in which case the output file will automatically switch to the NIfTI-2 format.

.. option:: NIfTIAutoLoadJSON

    *default: 0 (false)*

     A boolean value to indicate whether, when opening NIfTI images, any corresponding JSON file should be automatically loaded.

.. option:: NIfTIAutoSaveJSON

    *default: 0 (false)*

     A boolean value to indicate whether, when writing NIfTI images, a corresponding JSON file should be automatically created in order to save any header entries that cannot be stored in the NIfTI header.

.. option:: NIfTIUseSform

    *default: 0 (false)*

     A boolean value to control whether, in cases where both the sform and qform transformations are defined in an input NIfTI image, but those transformations differ, the sform transformation should be used in preference to the qform matrix (the default behaviour).

.. option:: NeedOpenGLCoreProfile

    *default: 1 (true)*

     Whether the creation of an OpenGL 3.3 context requires it to be a core profile (needed on newer versions of the ATI drivers on Linux, for instance).

.. option:: NumberOfThreads

    *default: number of threads provided by hardware*

     Set the default number of CPU threads to use for multi-threading.

.. option:: NumberOfUndos

    *default: 16*

     The number of undo operations permitted in the MRView ROI editor tool.

.. option:: ObjectColor

    *default: 1,1,0 (yellow)*

     The default colour to use for objects (i.e. SH glyphs) when not colouring by direction.

.. option:: RegAnalyseDescent

    *default: 0 (false)*

     Linear registration: write comma separated gradient descent parameters and gradients to stdout and verbose gradient descent output to stderr.

.. option:: RegCoherenceLen

    *default: 3.0*

     Linear registration: estimated spatial coherence length in voxels.

.. option:: RegGdConvergenceDataSmooth

    *default: 0.8*

     Linear registration: control point trajectory smoothing value used in convergence check parameter range: [0...1].

.. option:: RegGdConvergenceMinIter

    *default: 10*

     Linear registration: minimum number of iterations until convergence check is activated.

.. option:: RegGdConvergenceSlopeSmooth

    *default: 0.1*

     Linear registration: control point trajectory slope smoothing value used in convergence check parameter range: [0...1].

.. option:: RegGdConvergenceThresh

    *default: 5e-3*

     Linear registration: threshold for convergence check using the smoothed control point trajectories measured in fraction of a voxel.

.. option:: RegGdWeightMatrix

    *default: 0.0003*

     Linear registration: weight for optimisation of linear (3x3) matrix parameters.

.. option:: RegGdWeightTranslation

    *default: 1*

     Linear registration: weight for optimisation of translation parameters.

.. option:: RegStopLen

    *default: 0.0001*

     Linear registration: smallest gradient descent step measured in fraction of a voxel at which to stop registration.

.. option:: ScriptTmpDir

    *default: `.`*

     The location in which to generate the temporary directories to be used by MRtrix Python scripts. By default they will be generated in the working directory. Note that this setting does not influence the location in which piped images and other temporary files are created by MRtrix3; that is determined based on config file option :option:`TmpFileDir`.

.. option:: ScriptTmpPrefix

    *default: `<script>-tmp-`*

     The prefix to use when generating a unique name for a Python script temporary directory. By default the name of the invoked script itself will be used, followed by `-tmp-` (six random characters are then appended to produce a unique name in cases where a script may be run multiple times in parallel).

.. option:: SparseDataInitialSize

    *default: 16777216*

     Initial buffer size for data in MRtrix sparse image format file (in bytes).

.. option:: SpecularExponent

    *default: 5.0*

     The default exponent for the specular light in OpenGL renders.

.. option:: SpecularIntensity

    *default: 0.5*

     The default intensity for the specular light in OpenGL renders.

.. option:: TckgenEarlyExit

    *default: 0 (false)*

     Specifies whether tckgen should be terminated prematurely in cases where it appears as though the target number of accepted streamlines is not going to be met.

.. option:: TerminalColor

    *default: 1 (true)*

     A boolean value to indicate whether colours should be used in the terminal.

.. option:: TmpFileDir

    *default: `/tmp` (on Unix), `.` (on Windows)*

     The prefix for temporary files (as used in pipelines). By default, these files get written to the current folder on Windows machines, which may cause performance issues, particularly when operating over distributed file systems. On Unix machines, the default is /tmp/, which is typically a RAM file system and should therefore be fast; but may cause issues on machines with little RAM capacity or where write-access to this location is not permitted. Note that this location can also be manipulated using the `MRTRIX_TMPFILE_DIR` environment variable, without editing the config file. Note also that this setting does not influence the location in which Python scripts construct their temporary directories; that is determined based on config file option ScriptTmpDir.

.. option:: TmpFilePrefix

    *default: `mrtrix-tmp-`*

     The prefix to use for the basename of temporary files. This will be used to generate a unique filename for the temporary file, by adding random characters to this prefix, followed by a suitable suffix (depending on file type). Note that this prefix can also be manipulated using the `MRTRIX_TMPFILE_PREFIX` environment variable, without editing the config file.

.. option:: ToolbarStyle

    *default: 2*

     The style of the main toolbar buttons in MRView. See Qt's documentation for Qt::ToolButtonStyle.

.. option:: TrackWriterBufferSize

    *default: 16777216*

     The size of the write-back buffer (in bytes) to use when writing track files. MRtrix will store the output tracks in a relatively large buffer to limit the number of write() calls, avoid associated issues such as file fragmentation.

.. option:: VSync

    *default: 0 (false)*

     Whether the screen update should synchronise with the monitor's vertical refresh (to avoid tearing artefacts).

