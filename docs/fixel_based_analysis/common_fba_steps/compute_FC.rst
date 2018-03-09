The fibre density metric, mapped directly without any modulation to the fixel template space as above, is only sensitive to the original density of intra-axonal space in each voxel. In other words, it ignores the cross-sectional size of the bundle, which is another property that would factor into the bundle's total intra-axonal space across its full cross-sectional extent, and hence influence its total capacity to carry information. In certain cases, for example, atrophy may impact this cross-sectional size, but not per se the local fibre density metric.

In this step, we compute a fixel-based metric related to morphological differences in fibre cross-section (FC), where information is derived entirely from the warps generated during registration (see `this paper <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ for more information)::

    foreach * : warp2metric IN/subject2template_warp.mif -fc ../template/fixel_mask ../template/fc IN.mif

However, for group statistical analysis of FC we recommend calculating the log(FC) to ensure data are centred about zero and normally distributed. Here, we create a separate fixel directory to store the log(FC) data and copy the fixel index and directions file across::

    mkdir ../template/log_fc
    cp ../template/fc/index.mif ../template/fc/directions.mif ../template/log_fc
    foreach * : mrcalc ../template/fc/IN.mif -log ../template/log_fc/IN.mif

