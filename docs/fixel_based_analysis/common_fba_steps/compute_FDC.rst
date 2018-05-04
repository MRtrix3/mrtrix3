The total capacity of a fibre bundle to carry information, is modulated both by the local fibre density at the voxel (fixel) level, as well as its cross-sectional size. Here we compute a combined metric, which factors in the effects of both FD and FC, resulting in a fibre density and cross-section (FDC) metric::

    mkdir ../template/fdc
    cp ../template/fc/index.mif ../template/fdc
    cp ../template/fc/directions.mif ../template/fdc
    foreach * : mrcalc ../template/fd/IN.mif ../template/fc/IN.mif -mult ../template/fdc/IN.mif

This is also a nice example of how calculations across multiple fixel data files can be performed. However, note that this is only valid if these both share the same set of original fixels (in this case, the template fixel mask). Because the fixels also have to be stored in *exactly* the same order for this to work correctly, great care has to be taken the :code:`index.mif` files (in this case of the :code:`../template/fd` and :code:`../template/fc` folders) related to all input fixel data files that are used in the :code:`mrcalc` command are *exact copies* of each other.

