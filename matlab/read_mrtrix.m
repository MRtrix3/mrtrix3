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

function image = read_mrtrix (filename)

% function: image = read_mrtrix (filename)
%
% returns a structure containing the header information and data for the MRtrix
% format image 'filename' (i.e. files with the extension '.mif' or '.mih').

f = fopen (filename, 'r');
assert(f ~= -1, 'error opening %s', filename);
L = fgetl(f);
if ~strncmp(L, 'mrtrix image', 12)
  fclose(f);
  error('%s is not in MRtrix format', filename);
end

transform = [];
dw_scheme = [];

while 1
  L = fgetl(f);
  if ~ischar(L), break, end;
  L = strtrim(L);
  if strcmp(L, 'END'), break, end;
  d = strfind (L,':');
  if isempty(d)
    disp (['invalid line in header: ''' L ''' - ignored']);
  else
    key = strtrim(L(1:d(1)-1));
    value = strtrim(L(d(1)+1:end));
    switch lower(key)
      case 'dim',        image.dim = str2num(char(split_strings (value, ',')))';
      case 'vox',        image.vox = str2num(char(split_strings (value, ',')))';
      case 'layout',     image.layout = value;
      case 'datatype',   image.datatype = value;
      case 'transform',  transform(end+1,:) = str2num(char(split_strings (value, ',')))';
      case 'file',       file = value;
      case 'dw_scheme',  dw_scheme(end+1,:) = str2num(char(split_strings (value, ',')))';
      otherwise,         image = add_field (image, key, value);
    end
  end
end
fclose(f);


if ~isempty(transform)
  image.transform = transform;
  image.transform(4,:) = [ 0 0 0 1 ];
end

if ~isempty(dw_scheme)
  image.dw_scheme = dw_scheme;
end

if ~isfield (image, 'dim') || ~exist ('file') || ...
  ~isfield (image, 'layout') || ~isfield (image, 'datatype')
  error('critical entries missing in header - not reading data');
end

layout = split_strings(image.layout, ',');
order = (abs(str2num (char(layout)))+1)';

[ file, offset ] = strtok(file);
if isempty(offset), offset = 0; else offset = str2num(char(offset)); end
[f,g] = fileparts(filename);
if strcmp(file,'.'), file = filename; else file = fullfile (f, file); end

datatype = lower(image.datatype);
byteorder = datatype(end-1:end);

if strcmp(byteorder, 'le')
  f = fopen (file, 'r', 'l');
  datatype = datatype(1:end-2);
elseif strcmp(byteorder, 'be')
  f = fopen (file, 'r', 'b');
  datatype = datatype(1:end-2);
else
  if strcmp(datatype, 'bit')
    datatype = 'bit1';
    f = fopen(file, 'r', 'b');
  else
    f = fopen(file, 'r');
  end
end

assert(f ~= -1, 'error opening %s', filename);

fseek (f, offset, -1);
image.data = fread (f, inf, datatype);
fclose (f);

order(order)= 1:size(order,2);
image.data = reshape (image.data, image.dim(order));
image.data = ipermute (image.data, order);
for i=1:size(order,2)
  if layout{i}(1) == '-'
    image.data = flip(image.data, i);
  end
end




function S = split_strings (V, delim)
  S = {};
  while size(V,2) > 0
    [R, V] = strtok(V,delim);
    S{end+1} = R;
  end



