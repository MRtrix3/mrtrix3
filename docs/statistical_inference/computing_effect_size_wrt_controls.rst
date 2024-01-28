Expressing the effect size relative to controls
===============================================

In Fixel-Based Analysis (FBA), the apparent Fibre Density (FD) and Fibre Density and Cross-section (FDC) are relative measures and have arbitrary units.
Therefore the units of :code:`abs_effect.mif` output from :ref:`fixelcfestats` are not directly interpretable.
In a patient-control group comparison t-test, one way to present results is to express the absolute effect size as a percentage relative to the control group mean.
This is potentially equalliy applicable to other fixel-wise or voxel-wise quantitative metrics.

To compute percentage decrease effect size use::

    mrcalc stats/abs_effect.mif control_mean.mif -div 100 -mult stats/percentage_effect.mif

The mean value in the control group can be obtained in one of two ways:

1.  Use the relevant beta coefficient image from the statistical inference command

    If there is a column in the design matrix that contains the value 1 for
    all subjects in the control group and 0 for all other subjects, *and*
    if any & all nuisance regressors were de-meaned prior to inserting
    them into the design matrix, then the relevant beta coefficient image
    provided by the statistical inference command (e.g. :code:`stats_fd/beta0.mif`)
    can be interpreted directly as the control group mean. If however
    any of these conditions are violated, then approach 2 below should be used.

2.  Explicitly calculate the control group mean

    Using smoothed data (eg. from :ref:`fixelfilter` in the case of FBA),
    the :ref:`mrmath` command with the :code:`mean` operation can be used to explicitly
    compute the mean across the set of images corresponding to the control group.

Because the Fibre Cross-section (FC) measure in FBA is a scale factor it is slightly more complicated to compute the percentage decrease.
The FC ratio between two subjects (or groups) tells us the direct scale factor between them.

For example, for a given fixel if the patient group mean FC is 0.7, and control mean is 1.4, then this implies encompassing fibre tract in the patients is half as big as the controls: 0.7/1.4 = 0.5.
I.e. this is a 50% reduction wrt to the controls: 1 - (FC_patients/FC_controls)

Because in the standard FBA pipeline it is in fact log(FC) that is tested, the "absolute effect" that is output from :ref:`fixelcfestats` is: abs_effect = log(FC_controls) - log(FC_patients) = log(FC_controls/FC_patients).
Therefore to get the percentage effect we need to perform  1 - 1/exp(abs_effect)::

   mrcalc 1 1 stats_log_fc/abs_effect.mif -exp -div -sub stats_log_fc/percentage_effect.mif
