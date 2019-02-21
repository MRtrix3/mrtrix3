function tsf = read_mrtrix_tsf (filename)

% function: tracks = read_mrtrix_tsf (filename)
%
% returns a structure containing the header information and data for the MRtrix 
% format tsf 'filename' (i.e. files with the extension '.tsf').

f = fopen (filename, 'r');
if (f<1) 
  disp (['error opening ' filename ]);
  return
end
L = fgetl(f);
if ~strncmp(L, 'mrtrix track scalars', 20)
  fclose(f);
  disp ([filename ' is not in MRtrix format']);
  return
end

tsf = struct();

while 1
  L = fgetl(f);
  if ~ischar(L), break, end;
  L = strtrim(L);
  if strcmp(L, 'END'), break, end;
  d = strfind (L,':');
  if isempty(d)
    disp (['invalid line in header: ''' L ''' - ignored']);
  else
    key = lower(strtrim(L(1:d(1)-1)));
    value = strtrim(L(d(1)+1:end));
    if strcmp(key, 'file')
      file = value;
    elseif strcmp(key, 'datatype')
      tsf.datatype = value;
    else 
      tsf = setfield (tsf, key, value);
    end
  end
end
fclose(f);

if ~exist ('file') || ~isfield (tsf, 'datatype')
  disp ('critical entries missing in header - aborting')
  return
end

[ file, offset ] = strtok(file);
if ~strcmp(file,'.')
  disp ('unexpected file entry (should be set to current ''.'') - aborting')
  return;
end

if isempty(offset)
  disp ('no offset specified - aborting')
  return;
end
offset = str2num(char(offset));

datatype = lower(tsf.datatype);
byteorder = datatype(end-1:end);

if strcmp(byteorder, 'le')
  f = fopen (filename, 'r', 'l');
  datatype = datatype(1:end-2);
elseif strcmp(byteorder, 'be')
  f = fopen (filename, 'r', 'b');
  datatype = datatype(1:end-2);
else
  disp ('unexpected data type - aborting')
  return;
end

if (f<1) 
  disp (['error opening ' filename ]);
  return
end

fseek (f, offset, -1);
data = fread(f, inf, datatype);
fclose (f);

k = find (~isfinite(data(:,1)));

tsf.data = {};
pk = 1;
for n = 1:(prod(size(k)))
  tsf.data{end+1} = data(pk:(k(n)-1),:);
  pk = k(n)+1;
end
  

