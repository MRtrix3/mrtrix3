Note that here we warp FOD images into template space *without* FOD reorientation. Reorientation will be performed in a separate subsequent step::

    foreach * : mrtransform IN/fod.mif -warp IN/subject2template_warp.mif -noreorientation IN/fod_in_template_space.mif
