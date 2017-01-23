To account for changes to both within-voxel fibre density and macroscopic atrophy, fibre density and fibre cross-section must be combined (a measure we call fibre density & cross-section, FDC). This enables a more complete picture of group differences in white matter. Note that as discussed in `this paper <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_, group differences in FD or FC alone must be interpreted with care in crossing-fibre regions. However group differences in FDC are more directly interpretable. To generate the combined measure we 'modulate' the FD by FC::

    mkdir ../template/fdc
    cp ../template/fc/index.mif ../template/fdc
    cp ../template/fc/directions.mif ../template/fdc
    foreach * : mrcalc ../template/fd/IN.mif ../template/fc/IN.mif -mult ../template/fdc/IN.mif
