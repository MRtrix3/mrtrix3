import os.path, shutil
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run

def usage(base_parser, subparsers):
    parser = subparsers.add_parser('deep_atropos', parents=[base_parser])
    parser.set_author('Lucius S. Fekonja (lucius.fekonja[at]charite.de)')
    parser.set_synopsis('Generate the 5TT image based on a Deep Atropos segmentation image')
    parser.add_argument('input',
                       type=app.Parser.ImageIn(),
                       help='The input Deep Atropos segmentation image')
    parser.add_argument('output',
                       type=app.Parser.ImageOut(),
                       help='The output 5TT image')

def check_deep_atropos_input(image_path):
    dim = image.Header(image_path).size()
    if len(dim) != 3:
        raise MRtrixError(f'Image \'{str(image_path)}\' does not look like Deep Atropos segmentation (number of spatial dimensions is not 3)')

def execute():
    check_deep_atropos_input(app.ARGS.input)
    run.command(['mrconvert', app.ARGS.input, 'input.mif'])

    # Generate initial tissue-specific maps
    run.command('mrcalc input.mif 1 -eq CSF.mif')
    run.command('mrcalc input.mif 2 -eq cGM.mif')
    run.command('mrcalc input.mif 3 -eq WM1.mif')
    run.command('mrcalc input.mif 5 -eq WM2.mif')
    run.command('mrcalc input.mif 6 -eq WM3.mif')
    run.command('mrmath WM1.mif WM2.mif WM3.mif sum WM.mif')
    run.command('mrcalc input.mif 4 -eq sGM.mif')

    # Run connected components on WM for cleanup
    run.command('mrthreshold WM.mif - -abs 0.001 | '
                'maskfilter - connect - -connectivity | '
                'mrcalc 1 - 1 -gt -sub remove_unconnected_wm_mask.mif -datatype bit')

    # Preserve CSF and handle volume fractions
    run.command('mrcalc CSF.mif remove_unconnected_wm_mask.mif -mult csf_clean.mif')
    run.command('mrcalc 1.0 csf_clean.mif -sub sGM.mif -min sgm_clean.mif')
    
    # Calculate multiplier for volume fraction correction
    run.command('mrcalc 1.0 csf_clean.mif sgm_clean.mif -add -sub cGM.mif WM.mif -add -div multiplier.mif')
    run.command('mrcalc multiplier.mif -finite multiplier.mif 0.0 -if multiplier_noNAN.mif')
    
    # Apply corrections
    run.command('mrcalc cGM.mif multiplier_noNAN.mif -mult remove_unconnected_wm_mask.mif -mult cgm_clean.mif')
    run.command('mrcalc WM.mif multiplier_noNAN.mif -mult remove_unconnected_wm_mask.mif -mult wm_clean.mif')
    
    # Create empty pathological tissue map
    run.command('mrcalc wm_clean.mif 0 -mul path.mif')

    # Combine into 5TT format
    run.command('mrcat cgm_clean.mif sgm_clean.mif wm_clean.mif csf_clean.mif path.mif - -axis 3 | '
                'mrconvert - combined_precrop.mif -strides +2,+3,+4,+1')

    # Apply cropping unless disabled
    if app.ARGS.nocrop:
        run.function(os.rename, 'combined_precrop.mif', 'result.mif')
    else:
        run.command('mrmath combined_precrop.mif sum - -axis 3 | '
                   'mrthreshold - - -abs 0.5 | '
                   'mrgrid combined_precrop.mif crop result.mif -mask -')

    run.command(['mrconvert', 'result.mif', app.ARGS.output],
                mrconvert_keyval=app.ARGS.input,
                force=app.FORCE_OVERWRITE)