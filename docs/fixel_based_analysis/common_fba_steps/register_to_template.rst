Register the FOD image from all subjects to the FOD template image::

    foreach * : mrregister IN/fod.mif -mask1 IN/dwi_mask_upsampled.mif ../template/fod_template.mif -nl_warp IN/subject2template_warp.mif IN/template2subject_warp.mif
