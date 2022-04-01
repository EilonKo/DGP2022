#include "stdafx.h"

#include "colorMeshVerticesCmd.h"

#include "Utils/STL_Macros.h"
#include "Utils/Maya_Macros.h"
#include "Utils/Maya_Utils.h"
#include "Utils/MatlabInterface.h"
#include "Utils/GMM_Macros.h"
#include "Utils/MatlabGMMDataExchange.h"
#include "Utils/Utilities.h" // mapColor function


colorMeshVerticesCmd::colorMeshVerticesCmd()
{

}

void* colorMeshVerticesCmd::creator()
{
	return new colorMeshVerticesCmd;
}

MString colorMeshVerticesCmd::commandName()
{
	return "colorMeshVerticesCmd";
}

bool colorMeshVerticesCmd::isUndoable() const
{
	return false;
}

MStatus	colorMeshVerticesCmd::doIt(const MArgList& argList)
{
	MStatus stat = MS::kSuccess;

	MSyntax commandSyntax = syntax();
	MArgDatabase argData(commandSyntax, argList, &stat);
	MCHECKERROR(stat, "Wrong syntax for command " + commandName());

	int argS;
	stat = argData.getCommandArgument(0, argS);
	MCHECKERRORNORET(stat, "Invalid argument. Argument should be <1> or <2>.");
	if (1 != argS && 2 != argS)
		MCHECKERRORNORET(MS::kFailure, "Invalid argument. Argument should be <1> or <2>.");

	MSelectionList objectsList;
	stat = argData.getObjects(objectsList);
	MCHECKERROR(stat, "Can't access object list");

	MObject object;
	stat = objectsList.getDependNode(0, object);
	MCHECKERROR(stat, "Can't access object");

	MObject meshObject;
	stat = Maya_Utils::getMe_a_Mesh(object, meshObject);
	MCHECKERROR(stat, "Object is not a mesh");

	MFnMesh meshFn(meshObject, &stat);
	MCHECKERROR(stat, "Can't access mesh");


	const int numPolygons = meshFn.numPolygons(&stat);

	MItMeshPolygon poly(meshObject);
	if(!poly.isPlanar(&stat) || poly.isLamina(&stat) || poly.isHoled(&stat))
	{
		MCHECKERROR(MS::kFailure, "The given polygon shape is either self intersecting, holed or non-planar which are not supported");
	}

	unsigned int temp; 
	for (int i=0; i<numPolygons; i++)
	{
		temp=poly.polygonVertexCount();
		if ( 3 != temp )
			MCHECKERROR(MS::kFailure, "This is not a triangle mesh!");
		poly.next();
	}
	
	meshFn.deleteColorSet("Valence");
	MString s1 = meshFn.createColorSetWithName("Valence");
	meshFn.deleteColorSet("Curvature");
	MString s2 = meshFn.createColorSetWithName("Curvature");

	// ********** Valence color set **********
	MItMeshVertex vertex_it(meshFn.object());
	MIntArray vertexList;
	MColorArray colors;
	int curIndex, valence;

	while ( !vertex_it.isDone() )
	{
		curIndex = vertex_it.index();
		vertexList.append(curIndex);
		vertex_it.numConnectedEdges(valence);
		switch ( valence )
		{
		case 0:
		case 1:
		case 2:
		case 3: // <=3
			colors.append( 0.5f , 0.0f , 1.0f ); // purple
			break;
		case 4:
			colors.append( 0.0f , 1.0f , 1.0f ); // cyan
			break;
		case 5: 
			colors.append( 1.0f , 0.0f , 1.0f  ); // pink
			break;
		case 6:
			colors.append(0.0f, 1.0f, 0.0f); // green
			break;
		case 7:
			colors.append(1.0f, 1.0f, 0.5f); // yellow
			break;
		case 8:
			colors.append(0.0f, 0.0f, 1.0f); // blue
			break;
		default: // >=9
			colors.append(1.0f, 0.0f, 0.0f); // red
			break;
		}
		vertex_it.next();
	}
	
	meshFn.setCurrentColorSetName(s1);
	meshFn.setVertexColors(colors, vertexList);

	
	// ********** Curvature color set **********
	colors.clear();
	vertexList.clear();
	MItMeshPolygon face_it(meshFn.object());
	MDoubleArray curvatures = MDoubleArray(meshFn.numVertices());
	MIntArray faceVertices;
	MPointArray facePoints;
	MVectorArray vectors = MVectorArray(2);
	double angle;

	// set base value: (2*pi / pi) for (non / boundary)
	vertex_it.reset();
	while (!vertex_it.isDone())
	{
		curIndex = vertex_it.index();
		vertexList.append(curIndex); // vertexList[i] = i
		if (vertex_it.onBoundary())
			curvatures[curIndex] = M_PI;
		else
			curvatures[curIndex] = 2 * M_PI;
		vertex_it.next();
	}

	// subtruct angles from each vertex
	while (!face_it.isDone())
	{
		face_it.getVertices(faceVertices);
		face_it.getPoints(facePoints);
		for (int i = 0; i < facePoints.length(); i++)
		{
			vectors[0] = MVector(facePoints[i]) - MVector(facePoints[(i + 1) % 3]);
			vectors[1] = MVector(facePoints[i]) - MVector(facePoints[(i + 2) % 3]);
			angle = vectors[0].angle(vectors[1]);
			curvatures[faceVertices[i]] -= angle;
		}
		face_it.next();
	}

	// min/max values
	double minC = curvatures[0], maxC = curvatures[0];
	for (int i = 1; i < curvatures.length(); i++)
	{
		if (curvatures[i] < minC)
			minC = curvatures[i];
		else if (curvatures[i] > maxC)
			maxC = curvatures[i];
	}
	if (argData.isFlagSet("-min"))
	{
		minC = argData.flagArgumentDouble("-min", 0, &stat);
		MCHECKERRORNORET(stat, "Can't access '-min' flag value");
	}
	if (argData.isFlagSet("-max"))
	{
		maxC = argData.flagArgumentDouble("-max", 0, &stat);
		MCHECKERRORNORET(stat, "Can't access '-max' flag value");
	}
	if (minC > maxC)
		MCHECKERROR(MS::kFailure, "Min value > Max value");

	MString msg = MString("Min value: ") + minC + MString(", Max value: ") + maxC;
	MGlobal::displayInfo(msg);

	// get colors
	float R, G, B;
	for (int i = 0; i < curvatures.length(); i++)
	{
		mapColor(curvatures[i], R, G, B, minC, maxC);
		colors.append(MColor(R, G, B));
	}

	meshFn.setCurrentColorSetName(s2);
	meshFn.setVertexColors(colors, vertexList);
	

	// ********** Apply **********
	MString s = argS == 1 ? s1 : s2;
	meshFn.setCurrentColorSetName(s);
	meshFn.setDisplayColors(true);

	return MS::kSuccess;
}

MSyntax colorMeshVerticesCmd::syntax()
{
	MStatus stat = MS::kSuccess;
	MSyntax commandSyntax;

	stat = commandSyntax.addArg(MSyntax::kUnsigned);
	MCHECKERRORNORET(stat, "Can't create Syntax object for this command");

	stat = commandSyntax.addFlag("-min", "-minVal", MSyntax::kDouble);
	MCHECKERRORNORET(stat, "Can't create Syntax object for this command");

	stat = commandSyntax.addFlag("-max", "-maxVal", MSyntax::kDouble);
	MCHECKERRORNORET(stat, "Can't create Syntax object for this command");

	stat = commandSyntax.setObjectType(MSyntax::kSelectionList, 1, 1); //expect exactly one object
	MCHECKERRORNORET(stat, "Can't create Syntax object for this command");

	commandSyntax.useSelectionAsDefault(true);
	return commandSyntax;
}
