#pragma once


class topologyStatisticsCmd : public MPxCommand
{
private:
	
	void edgeDfs(std::vector<bool>& isVisited, const std::vector<bool>& isBoundary, MItMeshEdge& edge_it);
	void vertexDfs(std::vector<bool>& isVisited, MItMeshVertex& vertex_it);

public:

	topologyStatisticsCmd();
	virtual MStatus	doIt(const MArgList& argList);
	static void* creator();
	static MSyntax syntax();
	static MString commandName();
	virtual bool isUndoable() const;

};
