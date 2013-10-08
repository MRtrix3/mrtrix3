function plot_paths (filename)

paths = load (filename);
N = size (paths,1);
ns = size (paths, 2)/6;

paths = reshape ([ repmat([0 0 0 0 0 1], N, 1) paths ], N, 6, ns+1);

X = squeeze (paths (:,1,:))';
Y = squeeze (paths (:,2,:))';
Z = squeeze (paths (:,3,:))';
plot3 (X, Y, Z, 'k+')
hold on

step = 0.05;
cX = reshape (paths (:,1,:),1, []);
cY = reshape (paths (:,2,:),1, []);
cZ = reshape (paths (:,3,:),1, []);
dX = step * reshape (paths (:,4,:),1, []);
dY = step * reshape (paths (:,5,:),1, []);
dZ = step * reshape (paths (:,6,:),1, []);

plot3 ([cX-dX; cX+dX], [cY-dY; cY+dY], [cZ-dZ; cZ+dZ], 'g-')
hold off

daspect ([1 1 1]);
set (gca, 'visible', 'off');
set (gcf, 'color', [1 1 1]);

