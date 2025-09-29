.. _third-party-software:

*MRtrix3* and third-party neuroimaging software
===============================================

A minority of the *MRtrix3* commands are written in the Python language,
due to their image processing steps being achievable
using a clever arrangement of existing software commands
and/or making use of commands that are provided
as part of neuroimaging software packages other than *MRtrix3*.
While the latter has been done with the exclusive goal of providing
useful tools to the research community,
it has unfortunately led to confusion around attribution of software tools
to the software packages that provide them,
as well as a loss of attribution of efforts
to the researchers responsible for their development.
The purpose of this page is to provide a reference document
that details which third-party software packages are used by which *MRtrix3* commands,
and how these should be properly attributed.

Third-party software attribution
--------------------------------

While several *MRtrix3* commands make use of one or more third-party neuroimaging software tools,
there is no official arrangement betwen the *MRtrix3* project
and the maintainers resposible for such tools.
Where a researcher makes use of these specific *MRtrix3* commands,
it is their resposibility to ensure that those other softwares,
as well as any published research methods implemented within the commands utilised,
are attributed with the same degree of detail as would be warranted
were the researcher to be interfacing with that third party software directly.
It is the desire of the *MRtrix3* development team
that the execution of such software tools by *MRtrix3* commands
*increase* rather than *decrease* both the utilisation and citation of such tools.

Any *MRtrix3* command that may make use of a third-party software tool
should issue the following notification at commencement of execution,
along with a link to this very documentation page::

> Note that this script may make use of commands / algorithms
> from neuroimaging software other than MRtrix3.
> PLEASE ENSURE that any non-MRtrix3 software,
> as well as any research methods they provide,
> are recognised and cited appropriately
> (consult the help page (-help option) for specific references).

Third-party software and containers
-----------------------------------

For software version ``3.0.3`` -- ``3.0.x``,
the *MRtrix3* containers came bundled with "minified" versions
of multiple third-party neuroimaging software packages,
wherein only the components of those packages
that were utilised by at least one *MRtrix3* command were included.
From version ``3.1.0`` this is *no longer the case*;
these containers contain *only* *MRtrix3*.
As such, if one attempts to execute using one of these containers
an *MRtrix3* command that invokes a third-party command,
the following error will result::

> You are attempting to execute from within the MRtrix3 container
> an MRtrix3 Python script that itself invokes a command
> from another neuroimaging software package.
> This external neuroimaging software package is however not bundled
> within the official MRtrix3 container.
> If you require this specific functionality,
> you may consider one of the following options:
> - Configuring for running MRtrix3 a native environment
>   that incorporates not only MRtrix3 but also other neuroimaging packages;
> - Utilising an alternative container
>   within which is installed both MRtrix3 and other neuroimaging packages
>   (for which there will hopefully be a supported solution in the future).

Third-party software tool listing
---------------------------------

The following provides a comprehensive listing of all neuromaging software tools
that may be invoked by a command provided within *MRtrix3*.
The requisite citations listed here echo those that are specified
within the help page documentation of the respective *MRtrix3* commands.

Analysis of Functional NeuroImages (AFNI)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*Website*: https://afni.nimh.nih.gov/
*Citation*: R.W. Cox. AFNI: software for analysis and visualization of functional magnetic resonance neuroimages. Comput. Biomed. Res., 29 (1996), pp. 162-173.

-   `3dAutomask`
    *Invoked by MRtrix3*: `dwi2mask 3dautomask`

Advanced Normalisation Tools (ANTs)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*Website*: https://stnava.github.io/ANTs/
*Citation*: B.B. Avants, N.J. Tustison, G. Song, P.A. Cook, A. Klein, J.C. Gee. A reproducible evaluation of ANTs similarity metric performance in brain image registration. Neuroimage 2011;54(3):2033-44.

-   `N4BiasFieldCorrection`
    *Invoked by MRtrix3*: `dwibiascorrect ants`
    *Citation*: Tustison, N.; Avants, B.; Cook, P.; Zheng, Y.; Egan, A.; Yushkevich, P. & Gee, J. N4ITK: Improved N3 Bias Correction. IEEE Transactions on Medical Imaging, 2010, 29, 1310-1320

-   `antsRegistration`, `antsRegistrationSyNQuick.sh`, `antsApplyTransforms`
    *Invoked by MRtrix3*: `dwi2mask b02template`

-   `antsBrainExtraction.sh`
    *Invoked by MRtrix3*: `dwi2mask ants`

Automatic Registration Toolbox (ART)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*Website*: https://www.nitrc.org/projects/art

-   `acpcdetect`
    *Invoked by MRtrix3*: `5ttgen hsvs`
    *Citation*: Ardekani, B.; Bachman, A.H. Model-based automatic detection of the anterior and posterior commissures on MRI scans. NeuroImage, 2009, 46(3), 677-682

FreeSurfer
^^^^^^^^^^

*Website*: https://surfer.nmr.mgh.harvard.edu/
*Citation*: Fischl, B. Freesurfer. NeuroImage, 2012, 62(2), 774-781

-   `mri_synthstrip`
    *Invoked by MRtrix3*: `dwi2mask synthstrip`
    *Citation*: A. Hoopes, J. S. Mora, A. V. Dalca, B. Fischl, M. Hoffmann. SynthStrip: Skull-Stripping for Any Brain Image. NeuroImage, 2022, 260, 119474

FMRIB Software Library (FSL)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

*Website*: https://www.fmrib.ox.ac.uk/fsl
*Citation*: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

-   `bet`
    *Invoked by MRtrix3*: `dwi2mask bet`, `5ttgen fsl`
    *Citation*: Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155

-   `eddy`
    *Invoked by MRtrix3*: `dwifslpreproc`
    *Citations*:
    -   Andersson, J. L. & Sotiropoulos, S. N. An integrated approach to correction for off-resonance effects and subject movement in diffusion MR imaging. NeuroImage, 2015, 125, 1063-1078
    -   Andersson, J. L. R.; Graham, M. S.; Zsoldos, E. & Sotiropoulos, S. N. Incorporating outlier detection and replacement into a non-parametric framework for movement and distortion correction of diffusion MR images. NeuroImage, 2016, 141, 556-572
    -   Andersson, J. L. R.; Graham, M. S.; Drobnjak, I.; Zhang, H.; Filippini, N. & Bastiani, M. Towards a comprehensive framework for movement and distortion correction of diffusion MR images: Within volume movement. NeuroImage, 2017, 152, 450-466

-   `eddy_quad`
    *Invoked by MRtrix3*: `dwifslpreproc`
    *Citation*: Bastiani, M.; Cottaar, M.; Fitzgibbon, S.P.; Suri, S.; Alfaro-Almagro, F.; Sotiropoulos, S.N.; Jbabdi, S.; Andersson, J.L.R. Automated quality control for within and between studies diffusion MRI data using a non-parametric framework for movement and distortion correction. NeuroImage, 2019, 184, 801-812

-   `fast`
    *Invoked by MRtrix3*: `5ttgen fsl`, `5ttgen hsvs`, `dwibiascorrect fsl`
    *Citation*: Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57

-   `flirt`, `fnirt`
    *Invoked by MRtrix3*: `dwi2mask b02template`

-   `run_first_all`
    *Invoked by MRtrix3*: `5ttgen fsl`, `5ttgen hsvs`, labelsgmfirst`
    *Citation*: Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922

-   `topup`, `applytopup`
    *Invoked by MRtrix3*: `dwifslpreproc`
    *Citation*: Andersson, J. L.; Skare, S. & Ashburner, J. How to correct susceptibility distortions in spin-echo echo-planar images: application to diffusion tensor imaging. NeuroImage, 2003, 20, 870-888
