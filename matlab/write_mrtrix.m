% Copyright (c) 2008-2023 the MRtrix3 contributors.
%
% This Source Code Form is subject to the terms of the Mozilla Public
% License, v. 2.0. If a copy of the MPL was not distributed with this
% file, You can obtain one at http://mozilla.org/MPL/2.0/.
%
% Covered Software is provided under this License on an "as is"
% basis, without warranty of any kind, either expressed, implied, or
% statutory, including, without limitation, warranties that the
% Covered Software is free of defects, merchantable, fit for a
% particular purpose or non-infringing.
% See the Mozilla Public License v. 2.0 for more details.
%
% For more details, see http://www.mrtrix.org/.

function write_mrtrix (image, filename)

% function: write_mrtrix (image, filename)
%
% write the data contained in the structure 'image' in the MRtrix
% format image 'filename' (i.e. files with the extension '.mif' or '.mih').
%
% 'image' is either a N-dimensional array (N <= 16), or a structure containing
% the following fields:
%    image.data:            a N-dimensional array (N <= 16)
%    image.vox:             N-vector of voxel sizes (in mm) (default: { 2 }) [optional]
%    image.comments:        a cell array of strings [optional]
%    image.datatype:        the datatype specifier (default: float32) [optional]
%    image.mrtrix_version:  a character array [optional]
%    image.transform:       a 4x4 matrix [optional]
%    image.dw_scheme:       a NDWx4 matrix of gradient directions [optional]
 
% helper function: acts as ternary conditional operator
choose = @(varargin)varargin{end-varargin{1}};


fid = fopen (filename, 'w');
assert(fid ~= -1, 'error opening %s', filename);
fprintf (fid, 'mrtrix image\ndim: ');

if isstruct(image)
  dim = size(image.data);
else
  dim = size(image);
end

while prod(size(dim)) < 3
  dim(end+1) = 1;
end

fprintf (fid, '%d', dim(1));
fprintf (fid, ',%d', dim(2:end));

fprintf (fid, '\nvox: ');
if isstruct (image) && isfield (image, 'vox')
  fprintf (fid, '%.15f', image.vox(1));
  fprintf (fid, ',%.15f', image.vox(2:end));
else
  fprintf(fid, '2');
  fprintf(fid, ',%d', 2*ones(1,size(dim,2)-1));
end

fprintf (fid, '\nlayout: +0');
fprintf (fid, ',+%d', 1:(size(dim,2)-1));

[computerType, maxSize, endian] = computer;
if isstruct (image) && isfield (image, 'datatype')
  datatype = lower(image.datatype);
  switch datatype(end-1:end)
    case 'le', precision = datatype(1:end-2); byteorder = 'l';
    case 'be', precision = datatype(1:end-2); byteorder = 'b';
    otherwise
      switch datatype
        case 'bit',                precision = 'bit1'; byteorder = 'n';
        case { 'int8', 'uint8' },  precision = datatype; byteorder = 'n';
        otherwise, 
          precision = datatype;
          datatype = [ datatype choose(endian=='L','le','be') ];
          byteorder = choose (endian=='L', 'l', 'b');
      end
  end
else
  datatype = [ 'float32' choose(endian=='L','le','be') ];
  precision = 'float32';
  byteorder = choose (endian=='L', 'l', 'b');
end
fprintf (fid, [ '\ndatatype: ' datatype ]);

fprintf (fid, '\nmrtrix_version: %s', 'matlab');

if isstruct (image) && isfield (image, 'transform')
  fprintf (fid, '\ntransform: %.15f', image.transform(1,1));
  fprintf (fid, ',%.15f', image.transform(1,2:4));
  fprintf (fid, '\ntransform: %.15f', image.transform(2,1));
  fprintf (fid, ',%.15f', image.transform(2,2:4));
  fprintf (fid, '\ntransform: %.15f', image.transform(3,1));
  fprintf (fid, ',%.15f', image.transform(3,2:4));
end

if isstruct (image) && isfield (image, 'dw_scheme')
  for i=1:size(image.dw_scheme,1)
    fprintf (fid, '\ndw_scheme: %.15f', image.dw_scheme(i,1));
    fprintf (fid, ',%.15f', image.dw_scheme(i,2:4));
   end
end

% write out any other fields:
if isstruct (image)
  f = fieldnames(image);
  % don't worry about fields that have already been dealt with:
  f(strcmp(f,'dim')) = [];
  f(strcmp(f,'vox')) = [];
  f(strcmp(f,'layout')) = [];
  f(strcmp(f,'datatype')) = [];
  f(strcmp(f,'transform')) = [];
  f(strcmp(f,'dw_scheme')) = [];
  f(strcmp(f,'data')) = [];

  % write out contents of the remainging fields:
  for n = 1:numel(f)
    val = getfield (image, f{n});
    if iscell (val)
      for i=1:numel(val)
        fprintf (fid, '\n%s: %s', f{n}, val{i});
      end
    else
      fprintf (fid, '\n%s: %s', f{n}, val);
    end
  end
end

fprintf (fid, '\nfile: ');

if strcmp(filename(end-3:end), '.mif')
  dataoffset = ftell (fid) + 18;
  dataoffset = dataoffset + mod((4 - mod(dataoffset, 4)), 4);
  s = sprintf ('. %d\nEND\n              ', dataoffset);
  s = s(1:(dataoffset-ftell(fid)));
  fprintf (fid, s);
  fclose (fid);
  fid = fopen (filename, 'a+', byteorder);
elseif strcmp(filename(end-3:end), '.mih')
  datafile = [ filename(1:end-4) '.dat' ];
  fprintf (fid, '%s 0\nEND\n', datafile);
  fclose(fid);
  fid = fopen (datafile, 'w', byteorder);
else
  fclose(fid);
  error('unknown file suffix - aborting');
end

if isstruct(image)
  fwrite (fid, image.data, precision);
else
  fwrite (fid, image, precision);
end
fclose (fid);
