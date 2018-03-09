Statistical analysis using `connectivity-based fixel enhancement (CFE) <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ exploits local connectivity information derived from probabilistic fibre tractography, which acts as a neighbourhood definition for threshold-free enhancement of locally clustered statistic values. To generate a whole-brain tractogram from the FOD template (note the remaining steps from here on are executed from the template directory)::

    cd ../template
    tckgen -angle 22.5 -maxlen 250 -minlen 10 -power 1.0 wmfod_template.mif -seed_image template_mask.mif -mask template_mask.mif -select 20000000 tracks_20_million.tck

