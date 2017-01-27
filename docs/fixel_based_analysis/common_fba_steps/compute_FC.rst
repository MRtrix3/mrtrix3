Apparent fibre density, and other related measures that are influenced by the quantity of restricted water, only permit the investigation of group differences in the number of axons that manifest as a change to *within-voxel* density. However, depending on the disease type and stage, changes to the number of axons may also manifest as macroscopic differences in brain morphology. This step computes a fixel-based metric related to morphological differences in fibre cross-section, where information is derived entirely from the warps generated during registration (see `this paper <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ for more information)::

    foreach * : warp2metric IN/subject2template_warp.mif -fc ../template/fixel_mask ../template/fc IN.mif

Note that the FC files will be used in the next step. However, for group statistical analysis of FC we recommend taking the log(FC) to ensure data are centred about zero and normally distributed. We could place all the log(FC) fixel data files in the same fixel directory as the FC files (as long as they are named differently. However to keep things tidy, create a separate fixel directory to store the log(FC) data and copy the fixel index and directions file across::

    mkdir ../template/log_fc
    cp ../template/fc/index.mif ../template/log_fc
    cp ../template/fc/directions.mif ../template/log_fc
    foreach * : mrcalc ../template/fc/IN.mif -log ../template/log_fc/IN.mif
