#include "stdafx.h"

#include "topologyStatisticsCmd.h"

#include "Utils/STL_Macros.h"
#include "Utils/Maya_Macros.h"
#include "Utils/Maya_Utils.h"
#include "Utils/MatlabInterface.h"
#include "Utils/GMM_Macros.h"
#include "Utils/MatlabGMMDataExchange.h"


topologyStatisticsCmd::topologyStatisticsCmd()
{

}

void* topologyStatisticsCmd::creator()
{
	return new topologyStatisticsCmd;
}

MString topologyStatisticsCmd::commandName()
{
	return "topologyStatisticsCmd";
}

bool topologyStatisticsCmd::isUndoable() const
{
	return false;
}

MStatus	topologyStatisticsCmd::doIt(const MArgList& argList)
{
	MStatus stat = MS::kSuccess;

	MSyntax commandSyntax = syntax();
	MArgDatabase argData(commandSyntax, argList, &stat);
	MCHECKERROR(stat, "Wrong syntax for command " + commandName());

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

	MString msg;
	int v, f, e, c, b, g, prevIndex;
	// ***** Mesh name
	MGlobal::displayInfo("Mesh name: " + meshFn.name());
	// ***** Number of vertices
	v = meshFn.numVertices();
	msg.set(v);
	MGlobal::displayInfo("Number of vertices: " + msg);
	// ***** Number of faces
	f = meshFn.numPolygons();
	msg.set(f);
	MGlobal::displayInfo("Number of faces: " + msg);
	// ***** Number of edges
	e = meshFn.numEdges();
	msg.set(e);
	MGlobal::displayInfo("Number of edges: " + msg);

	// ***** Is triangle mesh
	msg.set("Yes");
	MItMeshPolygon face_it(meshFn.object());
	while (!face_it.isDone())
	{
		if (3 != face_it.polygonVertexCount())
		{
			msg.set("No");
			break;
		}
		face_it.next();
	}
	MGlobal::displayInfo("Is triangles mesh: " + msg);
	
	// ***** Number of connected components
	c = 0;
	std::vector<bool> isVisited(meshFn.numVertices(), false);
	MItMeshVertex vertex_it(meshFn.object());
	for (int i = 0; i < meshFn.numVertices(); i++)
	{
		if (!isVisited.at(i))
		{
			c++;
			vertex_it.setIndex(i, prevIndex);
			vertexDfs(isVisited, vertex_it);
		}
	}
	msg.set(c);

	// ***** Number of boundaries
	b = 0;
	std::vector<bool> isBoundary(meshFn.numEdges(), false);
	MItMeshEdge edge_it(meshFn.object());
	while (!edge_it.isDone())
	{
		if (edge_it.onBoundary())
			isBoundary.at(edge_it.index()) = true;
		edge_it.next();
	}

	edge_it.reset();
	isVisited.assign(meshFn.numEdges(), false);
	for (int i = 0; i < meshFn.numEdges(); i++)
	{
		if (isBoundary.at(i) && !isVisited.at(i))
		{
			b++;
			edge_it.setIndex(i, prevIndex);
			edgeDfs(isVisited, isBoundary, edge_it);
		}
	}
	msg.set(b);
	
	// ***** Genus
	g = (v + f - e - 2 * c + b) / -2;
	msg.set(g);

	MGlobal::displayInfo("Genus: " + msg);
	MGlobal::displayInfo("Number of connected components: " + msg);
	MGlobal::displayInfo("Number of boundaries: " + msg);

	return MS::kSuccess;
}

void topologyStatisticsCmd::vertexDfs(std::vector<bool>& isVisited, MItMeshVertex& vertex_it)
{
	isVisited.at(vertex_it.index()) = true;
	MIntArray neighbors;
	vertex_it.getConnectedVertices(neighbors);

	int prevIndex;
	for (int i = 0; i < neighbors.length(); i++)
	{
		if (!isVisited.at(neighbors[i]))
		{
			vertex_it.setIndex(neighbors[i], prevIndex);
			vertexDfs(isVisited, vertex_it);
		}
	}
}

void topologyStatisticsCmd::edgeDfs(std::vector<bool>& isVisited, const std::vector<bool>& isBoundary, MItMeshEdge& edge_it)
{
	isVisited.at(edge_it.index()) = true;
	MIntArray neighbors;
	edge_it.getConnectedEdges(neighbors);

	int prevIndex;
	for (int i = 0; i < neighbors.length(); i++)
	{
		if (isBoundary.at(neighbors[i]) && !isVisited.at(neighbors[i]))
		{
			edge_it.setIndex(neighbors[i], prevIndex);
			edgeDfs(isVisited, isBoundary, edge_it);
		}
	}
}

MSyntax topologyStatisticsCmd::syntax()
{
	MStatus stat;
	MSyntax commandSyntax;

	stat = commandSyntax.setObjectType(MSyntax::kSelectionList, 1, 1); //expect exactly one object
	MCHECKERRORNORET(stat, "Can't create Syntax object for this command");

	commandSyntax.useSelectionAsDefault(true);
	return commandSyntax;
}