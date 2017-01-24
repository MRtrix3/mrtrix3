Population template creation is the most time consuming step in a fixel-based analysis. If you have a large number of subjects in your study, we recommend building the template from a subset of 20-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (i.e. equal number of patients and controls). To build a template, place all FOD images in a single folder. We also recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly::

    mkdir -p ../template/fod_input
    mkdir -p ../template/mask_input

Symbolic link all FOD images (and masks) into a single input folder. If you have fewer than 40 subjects in your study, you can use the entire population to build the template::
