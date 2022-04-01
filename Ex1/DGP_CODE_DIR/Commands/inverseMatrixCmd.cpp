#include "stdafx.h"

#include "inverseMatrixCmd.h"

#include "Utils/STL_Macros.h"
#include "Utils/Maya_Macros.h"
#include "Utils/Maya_Utils.h"
#include "Utils/MatlabInterface.h"
#include "Utils/GMM_Macros.h"
#include "Utils/MatlabGMMDataExchange.h"


inverseMatrixCmd::inverseMatrixCmd()
{

}

void* inverseMatrixCmd::creator()
{
	return new inverseMatrixCmd;
}

MString inverseMatrixCmd::commandName()
{
	return "inverseMatrixCmd";
}

bool inverseMatrixCmd::isUndoable() const
{
	return false;
}

MStatus	inverseMatrixCmd::doIt(const MArgList& argList)
{
	MStatus stat = MS::kSuccess;

	MSyntax commandSyntax = syntax();
	MArgDatabase argData(commandSyntax, argList, &stat);
	MCHECKERROR(stat, "Wrong syntax for command " + commandName());

	// get matrix
	GMMDenseColMatrix M(3, 3), det, invM;
	double arg, d;
	int index;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			index = i * 3 + j;
			stat = argData.getCommandArgument(index, arg);
			MCHECKERRORNORET(stat, "Invalid argument number " + index);
			M(i, j) = arg;
		}
	}

	// register to matlab and output
	int result = MatlabGMMDataExchange::SetEngineDenseMatrix("M", M);
	cout << "Matrix: " << M << endl;

	// Calculate on Matlab and get result
	MatlabInterface::GetEngine().EvalToCout("d = det(M)");
	result = MatlabGMMDataExchange::GetEngineDenseMatrix("d", det);
	if (det(0, 0) == 0)
		cout << "This is a singular matrix, doesn't have an inverse" << endl;
	else
	{
		MatlabInterface::GetEngine().EvalToCout("invM = inv(M)");
		result = MatlabGMMDataExchange::GetEngineDenseMatrix("invM", invM);
		cout << "Inverse Matrix: " << invM << endl;
	}

	return MS::kSuccess;
}

MSyntax inverseMatrixCmd::syntax()
{
	MStatus stat;
	MSyntax commandSyntax;

	// 9 argumnets
	for (int i = 0; i < 9; i++)
	{
		stat = commandSyntax.addArg(MSyntax::kDouble);
		MCHECKERRORNORET(stat, "Can't create Syntax object for this command");
	}

	stat = commandSyntax.setObjectType(MSyntax::kSelectionList, 1, 1); //expect exactly one object
	MCHECKERRORNORET(stat, "Can't create Syntax object for this command");

	commandSyntax.useSelectionAsDefault(true);
	return commandSyntax;
}
