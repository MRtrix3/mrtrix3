The fibre density metric, mapped directly without any modulation to the fixel template space as above, is only sensitive to the original density of intra-axonal space in each voxel. In other words, it ignores the cross-sectional size of the bundle, which is another property that would factor into the bundle's total intra-axonal space across its full cross-sectional extent, and hence influence its total capacity to carry information. In certain cases, for example, atrophy may impact this cross-sectional size, but not per se the local fibre density metric.

In this step, we compute a fixel-based metric related to morphological differences in fibre cross-section (FC), where information is derived entirely from the warps generated during registration (see [Raffelt2017]_ for more information)::

    foreach * : warp2metric IN/subject2template_warp.mif -fc ../template/fixel_mask ../template/fc IN.mif

However, for group statistical analysis of FC we recommend calculating the log(FC) to ensure data are centred around zero and normally distributed. Here, we create a separate fixel directory to store the log(FC) data and copy the fixel index and directions file across::

    mkdir ../template/log_fc
    cp ../template/fc/index.mif ../template/fc/directions.mif ../template/log_fc
    foreach * : mrcalc ../template/fc/IN.mif -log ../template/log_fc/IN.mif

.. NOTE:: The FC (and hence also the log(FC)) as calculated here, is a *relative* metric, expressing the local fixel-wise cross-sectional size *relative* to this study's population template. While this makes it possible to interpret differences of FC *within* a single study (because only a single unique template is used in the study), the FC values should not be compared across different studies that each have their own population template. Reporting absolute quantities of FC, or absolute effect sizes of FC, also provides little information; as again, it is only meaningful with respect to the template.

