Plugin commands:

***** colorMeshVerticesCmd
Create 2 color sets (by valence and by curvature) and apply one of them on selected mesh.
Colors for valence: <=3:purple, 4:cyan, 5:pink, 6:green, 7:yellow, 8:blue, >=9:red.
Colors for curvature: dynamic range between blue(min) to red(max).
parameters:
	<double>: which color set to apply. 1 for valence, 2 for curvature.
optional flags: (for curvature)
	-min <double>: minimum value in color dynamic range.
	-max <double>: maximum value in color dynamic range.
usage example:
	colorMeshVerticesCmd 2 -min 0 -max 4

***** inverseMatrixCmd
Get a matrix and output its inverse if exist, by using Matlab.
parameters:
	<double> ... <double>: 9 argument list to form a 3x3 matrix, in row major format.
usage example:
	inverseMatrixCmd 1 0 0 0 1 0 0 0 1

***** topologyStatisticsCmd
Output topology statistics on selected mesh (name, is triangles, genus, number of: vertices, edges, faces, connected components, boundaries).
usage example:
	topologyStatisticsCmd