Expressing the effect size relative to controls
===============================================

The apparent Fibre Density (FD) and Fibre Density and Cross-section (FDC) are relative measures and have arbitrary units. Therefore the units of :code:`abs_effect.mif` output from :ref:`fixelcfestats` are not directly interpretable. In a patient-control group comparison, one way to present results is to express the absolute effect size as a percentage relative to the control group mean.

To compute FD and FDC percentage decrease effect size use::

    mrcalc fd_stats/abs_effect.mif fd_stats/beta1.mif  -div 100 -mult fd_stats/percentage_effect.mif

where beta1.mif is the beta output that corresponds to your control population mean.

Because the Fibre Cross-section (FC) measure is a scale factor it is slightly more complicated to compute the percentage decrease. The FC ratio between two subjects (or groups) tells us the direct scale factor between them.

For example, for a given fixel if the patient group mean FC is 0.7, and control mean is 1.4, then this implies encompassing fibre tract in the patients is half as big as the controls: 0.7/1.4 = 0.5. I.e. this is a 50% reduction wrt to the controls: 1 - (FC_patients/FC_controls)

Because we peform FBA of log(FC), the abs_effect that is output from :ref:`fixelcfestats` is: abs_effect = log(FC_controls) - log(FC_patients) = log(FC_controls/FC_patients). Therefore to get the percentage effect we need to perform  1 - 1/exp(abs_effect)::

   mrcalc 1 1 fc_stats/abs_effect.mif -exp -div -sub fc_stats/fc_percentage_effect.mif
