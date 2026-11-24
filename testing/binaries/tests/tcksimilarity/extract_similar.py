import nibabel as nib
import numpy as np
import sys
import random

# Usage: python extract_similar.py index.nii.gz streamlines.nii.gz values.nii.gz [track_index]
# If track_index is not provided, a random one will be chosen.

index = np.squeeze(nib.load(sys.argv[1]).get_fdata())
streamlines = np.squeeze(nib.load(sys.argv[2]).get_fdata())
values = np.squeeze(nib.load(sys.argv[3]).get_fdata())

num_tracks = index.shape[0]

# Selection of track
if len(sys.argv) > 4:
    track_index = int(sys.argv[4])
    print(f"Using provided track index: {track_index}")
else:
    track_index = random.randint(0, num_tracks - 1)
    print(f"No track index provided, selected random track: {track_index}")

# Get similarity data for this specific track
curr_howmany = int(index[track_index, 0])
curr_offset = int(index[track_index, 1])

# Extract similar indices and similarity values
start = curr_offset
end = curr_offset + curr_howmany
similar_indices = streamlines[start:end].astype(int)
similar_scores = values[start:end]

print(f"Found {curr_howmany} similar tracks.")

ind_track = np.zeros(num_tracks, dtype=int)
val_track = np.zeros(num_tracks)

for i in range(len(similar_indices)):
    idx = similar_indices[i]
    if 0 <= idx < num_tracks:
        ind_track[idx] = 1
        val_track[idx] = similar_scores[i]

# Save output
np.savetxt('similarity_binary.txt', ind_track, fmt='%d')  # to be used as a binary track mask
np.savetxt('similarity_values.txt', val_track, fmt='%.6f')
