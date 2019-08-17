function write_mrtrix_tsf (tsf, filename)

% function: write_mrtrix_tsf (tracks, filename)
%
% writes the tsf data stored as a cell array in the 'data' field of the
% tsf variable to the MRtrix format tsf 'filename'. All other fields
% of the tsf variable will be written as text entries in the header, and are
% expected to supplied as character arrays.

assert(isfield(tsf, 'data'), ...
  'input tracks variable does not contain required ''data'' field');

assert(iscell(tsf.data), ...
  'input tracks.data variable should be a cell array');

f = fopen (filename, 'w', 'ieee-le');
assert(f ~= -1, 'error opening %s', filename);

fprintf (f, 'mrtrix track scalars\ndatatype: Float32LE\ncount: %d\n', numel(tsf.data));
%fprintf (f, 'mrtrix track scalars\n');
names = fieldnames(tsf);
for i=1:size(names)
  if strcmpi (names{i}, 'data'), continue; end
  if strcmpi (names{i}, 'count'), continue; end
  if strcmpi (names{i}, 'datatype'), continue; end
  fprintf (f, '%s: %s\n', names{i}, getfield(tsf, names{i}));
end
data_offset = ftell (f) + 20;
data_offset=data_offset+mod((4-mod(data_offset,4)),4);
fprintf (f, 'file: . %d\nEND\n', data_offset);

fwrite (f, zeros(data_offset-ftell(f),1), 'uint8');
for i = 1:numel(tsf.data)
  fwrite (f, tsf.data{i}', 'float32');
  fwrite (f, nan , 'float32');
end
fclose (f);