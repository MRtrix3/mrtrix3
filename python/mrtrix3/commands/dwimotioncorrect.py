#   Copyright (c) 2017-2019 Daan Christiaens
#
#   MRtrix and this add-on module are distributed in the hope
#   that it will be useful, but WITHOUT ANY WARRANTY; without
#   even the implied warranty of MERCHANTABILITY or FITNESS
#   FOR A PARTICULAR PURPOSE.
#
#   MOTION CORRECTION FOR DWI VOLUME SERIES
#
#   This script performs volume-to-series and slice-to-series registration
#   of diffusion-weighted images for motion correction in the brain.
#
#   Author:  Daan Christiaens
#            King's College London
#            daan.christiaens@kcl.ac.uk
#

from mrtrix3 import app, image, path, run, MRtrixError
import json


DEFAULT_CONFIG = """{
  "global": {
    "rec-reg": 0.005,
    "rec-zreg": 0.001,
    "svr": true,
    "rec-iter": 3,
    "reg-iter": 10,
    "reg-scale": 1.0,
    "lbreg": 0.001
  },
  "epochs": [
    {
      "svr": false,
      "reg-scale": 3.0
    },{
      "svr": false,
      "reg-scale": 2.4
    },{
      "reg-scale": 1.9
    },{
      "reg-scale": 1.5
    },{
      "reg-scale": 1.2
    },{
      "rec-iter": 10
    }
  ]
}"""



def usage(cmdline): #pylint: disable=unused-variable
    # base
    cmdline.set_author('Daan Christiaens (daan.christiaens@kcl.ac.uk)')
    cmdline.set_synopsis('Perform motion correction in a dMRI dataset')
    cmdline.add_description('Volume-level and/or slice-level motion correction for dMRI, '
                            'based on the SHARD representation for multi-shell data. ')
    # arguments
    cmdline.add_argument('input',  help='The input image series to be corrected')
    cmdline.add_argument('output', help='The output multi-shell SH coefficients')
    # options
    options = cmdline.add_argument_group('Options for the dwimotioncorrect script')
    options.add_argument('-mask', help='Manually provide a mask image for motion correction')
    options.add_argument('-lmax', help='SH basis order per shell (default = maximum with at least 30% oversampling)')
    options.add_argument('-rlmax', help='Reduced basis order per component for registration (default = 4,2,0 or lower if needed)')
    options.add_argument('-reg', help='Regularization for dwirecon (overrides config; default = 0.005)')
    options.add_argument('-zreg', help='Regularization for dwirecon (overrides config; default = 0.001)')
    options.add_argument('-setup', help='Configuration setup file (json structured)')
    options.add_argument('-mb', help='Multiband factor (default = 1)')
    options.add_argument('-sorder', help='Slice order (default = 2,1, for odd-even)')
    options.add_argument('-sspwidth', help='Slice thickness for Gaussian SSP (default = 1)')
    options.add_argument('-sspfile', help='Slice sensitivity profile as vector')
    options.add_argument('-fieldmap', help='B0 field map for distortion correction')
    options.add_argument('-fieldidx', help='Index of volume to which field map is aligned (default = 0)')
    options.add_argument('-pe_table', help='Phase encoding table in MRtrix format')
    options.add_argument('-pe_eddy', nargs=2, help='Phase encoding table in FSL acqp/index format')
    options.add_argument('-priorweights', help='Import prior slice weights')
    options.add_argument('-fixedweights', help='Import fixed slice weights')
    options.add_argument('-fixedmotion', help='Import fixed motion traces')
    options.add_argument('-driftfilter', action='store_true', help='Filter motion for drift')
    options.add_argument('-voxelweights', help='Import fixed voxel weights')
    options.add_argument('-export_motion', help='Export rigid motion parameters')
    options.add_argument('-export_weights', help='Export slice weights')
    app.add_dwgrad_import_options(cmdline)



def execute(): #pylint: disable=unused-variable
    try:
        import scipy
    except ImportError:
        raise MRtrixError('SciPy required: https://www.scipy.org/install.html')
    # import metadata
    grad_import_option = app.read_dwgrad_import_options()
    pe_import_option = ''
    if app.ARGS.pe_table:
        pe_import_option = ' -import_pe_table ' + path.from_user(app.ARGS.pe_table, True)
    elif app.ARGS.pe_eddy:
        pe_import_option = ' -import_pe_eddy ' + path.from_user(app.ARGS.pe_eddy[0], True) + ' ' + path.from_user(app.ARGS.pe_eddy[1], True)
    # check output path
    app.check_output_path(app.ARGS.output)
    if app.ARGS.export_motion:
        app.check_output_path(app.ARGS.export_motion)
    if app.ARGS.export_weights:
        app.check_output_path(app.ARGS.export_weights)
    # prepare working directory
    app.make_scratch_dir()
    run.command('mrconvert ' + path.from_user(app.ARGS.input, True) + ' ' + path.to_scratch('in.mif', True) + grad_import_option + pe_import_option)
    if app.ARGS.mask:
        run.command('mrconvert ' + path.from_user(app.ARGS.mask, True) + ' ' + path.to_scratch('mask.mif', True))
    if app.ARGS.fieldmap:
        run.command('mrconvert ' + path.from_user(app.ARGS.fieldmap, True) + ' ' + path.to_scratch('field.mif', True))
    app.goto_scratch_dir()

    # Make sure it's actually a DWI that's been passed
    header = image.Header('in.mif')
    dwi_sizes = header.size()
    if len(dwi_sizes) != 4:
        raise MRtrixError('Input image must be a 4D image')
    DW_scheme = image.mrinfo('in.mif', 'dwgrad').split('\n')
    if len(DW_scheme) != int(dwi_sizes[3]):
        raise MRtrixError('Input image does not contain valid DW gradient scheme')

    # Check PE table if field map is passed.
    if app.ARGS.fieldmap:
        PE_scheme = image.mrinfo('in.mif', 'petable').split('\n')
        if len(PE_scheme) != int(dwi_sizes[3]):
            raise MRtrixError('Input image does not contain valid phase encoding scheme')

    # Generate a brain mask if required, or check the mask if provided
    if app.ARGS.mask:
        if not image.match(header, 'mask.mif', up_to_dim=3):
            raise MRtrixError('Provided mask image does not match input DWI')
    else:
        run.command('dwi2mask in.mif mask.mif')

    # Image dimensions
    dims = list(map(int, header.size()))
    vox = list(map(float, header.spacing()[:3]))
    vu = round((vox[0]+vox[1])/2., 1)
    shells = [int(round(float(s))) for s in image.mrinfo('in.mif', 'shell_bvalues').split()]
    shell_sizes = [int(s) for s in image.mrinfo('in.mif', 'shell_sizes').split()]

    # Set lmax
    def get_max_sh_degree(N, oversampling_factor=1.3):
        for l in range(0,10,2):
            if (l+3)*(l+4)/2 * oversampling_factor > N:
                return l

    lmax = [(b>10) * get_max_sh_degree(n) for b, n in zip(shells, shell_sizes)]
    if app.ARGS.lmax:
        lmax = [int(l) for l in app.ARGS.lmax.split(',')]
    if len(lmax) != len(shells):
        raise MRtrixError('No. lmax must match no. shells.')

    # Set rlmax
    rlmax = [min(r,max(l-2,0)) for r, l in zip([4,2,0], sorted(lmax, reverse=True))]
    if app.ARGS.rlmax:
        rlmax = [int(l) for l in app.ARGS.rlmax.split(',')]
    if len(rlmax) > len(lmax) or max(rlmax) > max(lmax):
        raise MRtrixError('-rlmax invalid.')

    # Configuration
    config = json.loads(DEFAULT_CONFIG)
    if app.ARGS.setup:
        with open(path.from_user(app.ARGS.setup, True)) as f:
            customconfig = json.load(f)
            # use defaults for unspecified config options
            customconfig['global'] = {**config['global'], **customconfig['global']}
            # update config
            config.update(customconfig)

    # Override regularization options if provided
    if app.ARGS.reg:
        config['global']['rec-reg'] = float(app.ARGS.reg)
    if app.ARGS.zreg:
        config['global']['rec-zreg'] = float(app.ARGS.zreg)

    # SSP option
    ssp_option = ''
    if app.ARGS.sspfile:
        run.command('cp ' + path.from_user(app.ARGS.sspfile, True) + ' ssp.txt')
        ssp_option = ' -ssp ssp.txt'
    elif app.ARGS.sspwidth:
        ssp_option = ' -ssp ' + app.ARGS.sspwidth

    # Initialise radial basis with RF per shell.
    rfs = []
    for k, l in enumerate(lmax):
        fn = 'rf'+str(k+1)+'.txt'
        with open(fn, 'w') as f:
            for s in range(len(shells)):
                i = '1' if k==s else '0'
                f.write(' '.join([i,]*(l//2+1)) + '\n')
        rfs += [fn,]

    redrfs = ['redrf'+str(k+1)+'.txt' for k in range(len(rlmax))]

    # Force max no. threads
    nthr = ''
    if app.ARGS.nthreads:
        nthr = ' -nthreads ' + str(app.ARGS.nthreads)

    # Set multiband factor
    mb = 1
    if app.ARGS.mb:
        mb = int(app.ARGS.mb)

    # Slice order
    motfilt_option = ''
    if app.ARGS.sorder:
        p,s = app.ARGS.sorder.split(',')
        motfilt_option += ' -packs ' + p + ' -shift ' + s
    if app.ARGS.driftfilter:
        motfilt_option += ' -driftfilt'

    # Import fixed slice weights
    if app.ARGS.priorweights:
        run.command('cp ' + path.from_user(app.ARGS.priorweights, True) + ' priorweights.txt')
    if app.ARGS.fixedweights:
        run.command('cp ' + path.from_user(app.ARGS.fixedweights, True) + ' sliceweights.txt')
    # Import fixed motion traces
    if app.ARGS.fixedmotion:
        run.command('cp ' + path.from_user(app.ARGS.fixedmotion, True) + ' motion.txt')

    # Import voxel weights
    if app.ARGS.voxelweights:
        # mrcalc "hack" to check image dimensions & copy PE table
        run.command('mrcalc in.mif 0 -mult ' + path.from_user(app.ARGS.voxelweights, True) + ' -add voxelweights.mif')


    # Variable input file name
    global inputfn, voxweightsfn
    inputfn = 'in.mif'
    voxweightsfn = 'voxelweights.mif'



    #   __________ Function definitions __________

    def reconstep(k, conf):
        rcmd = 'dwirecon {} recon-{}.mif -spred spred.mif'.format(inputfn, k)
        rcmd += ' -maxiter {} -reg {} -zreg {}'.format(conf['rec-iter'], conf['rec-reg'], conf['rec-zreg'])
        rcmd += ssp_option + ' -rf ' + ' -rf '.join(rfs)
        if k>0:
            rcmd += ' -motion motion.txt -weights sliceweights.txt -init recon-{}.mif'.format(k-1)
        elif app.ARGS.priorweights:
            rcmd += ' -weights priorweights.txt'
        if app.ARGS.voxelweights:
            rcmd += ' -voxweights ' + voxweightsfn
        rcmd += ' -force' + nthr
        run.command(rcmd)


    def sliceweightstep(k):
        if app.ARGS.fixedweights:
            return;
        mask_opt = ' -mask mask.mif' + (' -motion motion.txt' if k>0 else '')
        run.command('dwisliceoutliergmm ' + inputfn + ' spred.mif -mb ' + str(mb) + mask_opt + ' sliceweights.txt -force')
        if app.ARGS.priorweights:
            import numpy as np
            W0 = np.loadtxt('priorweights.txt')
            W1 = np.loadtxt('sliceweights.txt')
            np.savetxt('sliceweights.txt', W1 * W0)


    def basisupdatestep(k, conf):
        lboption = ' -lbreg {}'.format(conf['lbreg']) if conf['lbreg']>0 else ''
        run.command('msshsvd recon-' + str(k) + '.mif -mask mask.mif -lmax ' + ','.join([str(l) for l in sorted(lmax)[::-1]]) + lboption + ' ' + ' '.join(rfs) + ' -force')


    def rankreduxstep(k, conf):
        lboption = ' -lbreg {}'.format(conf['lbreg']) if conf['lbreg']>0 else ''
        fwhmoption = ' -fwhm {}'.format(round(vu*conf['reg-scale'], 2))
        run.command('mrfilter recon-' + str(k) + '.mif smooth -' + fwhmoption + ' | ' +
                    'msshsvd - -mask mask.mif -lmax ' + ','.join([str(l) for l in rlmax]) +
                    lboption + ' ' + ' '.join(redrfs) + ' -proj rankredux.mif -force')


    def registrationstep(k, conf):
        if app.ARGS.fixedmotion:
            return;
        rcmd = 'dwislicealign {} rankredux.mif motion.txt -mask mask.mif -maxiter {}'.format(inputfn, conf['reg-iter'])
        rcmd += ' -mb ' + (str(mb) if conf['svr'] else '0') + (' -init motion.txt' if k>0 else '')
        rcmd += ssp_option + ' -force' + nthr
        run.command(rcmd)
        run.command('motionfilter motion.txt sliceweights.txt motion.txt -medfilt 5' + motfilt_option)


    def fieldalignstep(k):
        global inputfn, voxweightsfn
        if app.ARGS.fieldmap:
            cmdopt = ' -motion motion.txt -fidx ' + ('0' if not app.ARGS.fieldidx else app.ARGS.fieldidx) + ' -force' if k > 0 else ''
            run.command('mrfieldunwarp in.mif field.mif unwarped.mif' + cmdopt)
            if app.ARGS.voxelweights:
                run.command('mrfieldunwarp -nomodulation voxelweights.mif field.mif voxelweights-unwarped.mif' + cmdopt)
            inputfn = 'unwarped.mif'
            voxweightsfn = 'voxelweights-unwarped.mif'


    #   __________ Motion correction __________

    nepochs = len(config['epochs']) - 1
    for k, epoch in enumerate(config['epochs']):
        liveconfig = {**config['global']}
        liveconfig.update(epoch)
        # update template
        fieldalignstep(k)
        reconstep(k, liveconfig)
        # end on last recon step
        if k == nepochs:
            break
        # basis update and rank reduction
        basisupdatestep(k, liveconfig)
        rankreduxstep(k, liveconfig)
        # slice weights
        sliceweightstep(k)
        # register template to volumes
        registrationstep(k, liveconfig)


    #   __________ Copy outputs __________

    run.command('mrconvert recon-'+str(nepochs)+'.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval='recon-'+str(nepochs)+'.mif', force=app.FORCE_OVERWRITE)
    if app.ARGS.export_motion:
        run.command('cp motion.txt ' + path.from_user(app.ARGS.export_motion, True))
    if app.ARGS.export_weights:
        run.command('cp sliceweights.txt ' + path.from_user(app.ARGS.export_weights, True))



