
.. _list-of-mrtrix3-scripts:

#######################
List of MRtrix3 scripts
#######################




.. toctree::
    :hidden:

    scripts/5ttgen
    scripts/dwi2response
    scripts/dwibiascorrect
    scripts/dwicat
    scripts/dwigradcheck
    scripts/dwinormalise
    scripts/dwipreproc
    scripts/dwishellmath
    scripts/foreach
    scripts/labelsgmfix
    scripts/population_template
    scripts/responsemean


.. csv-table::
    :header: "Command", "Synopsis"

    :ref:`5ttgen`, "Generate a 5TT image suitable for ACT"
    :ref:`dwi2response`, "Estimate response function(s) for spherical deconvolution"
    :ref:`dwibiascorrect`, "Perform B1 field inhomogeneity correction for a DWI volume series"
    :ref:`dwicat`, "Concatenating multiple DWI series accounting for differential intensity scaling"
    :ref:`dwigradcheck`, "Check the orientation of the diffusion gradient table"
    :ref:`dwinormalise`, "Perform various forms of intensity normalisation of DWIs"
    :ref:`dwipreproc`, "Perform diffusion image pre-processing using FSL's eddy tool; including inhomogeneity distortion correction using FSL's topup tool if possible"
    :ref:`dwishellmath`, "Apply an mrmath operation to each b-value shell in a DWI series"
    :ref:`foreach`, "Perform some arbitrary processing step for each of a set of inputs"
    :ref:`labelsgmfix`, "In a FreeSurfer parcellation image, replace the sub-cortical grey matter structure delineations using FSL FIRST"
    :ref:`population_template`, "Generates an unbiased group-average template from a series of images"
    :ref:`responsemean`, "Calculate the mean response function from a set of text files"
