Run the template building script as follows::

    population_template ../template/fod_input -mask_dir ../template/mask_input ../template/wmfod_template.mif -voxel_size 1.3
    
The voxel size is typically set to match the voxel size of the input FOD images (which, in this pipeline, would typically be the resolution the preprocessed data was upsampled to, earlier on in the pipeline).

