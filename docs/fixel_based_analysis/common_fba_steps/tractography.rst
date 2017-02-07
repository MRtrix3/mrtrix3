Statistical analysis using `connectivity-based fixel enhancement <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ exploits connectivity information derived from probabilistic fibre tractography. To generate a whole-brain tractogram from the FOD template. Note the remaining steps from here on are executed from the template directory::

    cd ../template
    tckgen -angle 22.5 -maxlen 250 -minlen 10 -power 1.0 fod_template.mif -seed_image voxel_mask.mif -mask voxel_mask.mif -number 20000000 tracks_20_million.tck
