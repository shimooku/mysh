#include "mysh.h"

/***************************************************
					 OPERATORS
****************************************************/
int __opr_pstack(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	for (int i = ((MYSD *)mys)->iOPStackUsing - 1; i > -1; i--)
	{
		MYS_fprintf(MYS_stdout, "%3d| ", i);
		ShowOPStack(&((MYSD *)mys)->OPStack[i], i);
	}

	return MYS_OK;
}

int __opr_quit(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	return MYS_QUIT;
}

int __opr_eqeq(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "==");

	int i = ((MYSD *)mys)->iOPStackUsing - 1;
	ShowOPStack(&((MYSD *)mys)->OPStack[i], i);
	((MYSD *)mys)->iOPStackUsing--;

	return MYS_OK;
}

int __opr_eq(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "eq");

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	Pushboolean(mys, bObjCompare(&Obj1, &Obj2));

	return MYS_OK;
}

int __opr_ne(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "ne");

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	Pushboolean(mys, !bObjCompare(&Obj1, &Obj2));

	return MYS_OK;
}

int __opr_closearray(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	int i, iLengthArray = 0;
	for (i = STACK_LEVEL(mys) - 1; i >= 0; i--)
	{
		if (STACK_OBJ_FIRST(mys, i)->ObjType == OTE_MARK && STACK_OBJ_FIRST(mys, i)->bExecutable == false)
			break;
		iLengthArray++;
	}

	if (i < 0)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "]");

	int iStackLevelForArray = i;

	MYS_OBJ *pObjArray = STACK_OBJ_FIRST(mys, iStackLevelForArray);
	pObjArray->bExecutable = false;
	pObjArray->ObjType = OTE_ARRAY;
	pObjArray->un.pVar = pVarCreate(mys, VTE_ARRAY, &iLengthArray);

	int iArrayIndex = 0;
	MYS_OBJ *pObjInArray = pObjArray->un.pVar->un.pObjInArray;
	for (int j = i + 1; j < STACK_LEVEL(mys);)
	{
		pObjInArray[iArrayIndex++] = *STACK_OBJ_FIRST(mys, j++);
	}
	pObjArray->un.pVar->iLength = iArrayIndex;

	((MYSD *)mys)->iOPStackUsing = iStackLevelForArray + 1;

	return MYS_OK;
}

int iCloseArray(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing++;
	return __opr_closearray(mys);
}

int __opr_closeexecarray(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	int i, iLengthArray = 0;
	for (i = STACK_LEVEL(mys) - 1; i > -1; i--)
	{
		if (STACK_OBJ_FIRST(mys, i)->ObjType == OTE_MARK && STACK_OBJ_FIRST(mys, i)->bExecutable == true)
			break;
		iLengthArray++;
	}

	int iStackLevelForExecArray = i;

	MYS_OBJ *pObjExecArray = STACK_OBJ_FIRST(mys, iStackLevelForExecArray);
	pObjExecArray->bExecutable = true;
	pObjExecArray->ObjType = OTE_ARRAY;
	pObjExecArray->un.pVar = pVarCreate(mys, VTE_ARRAY, &iLengthArray);

	int iArrayIndex = 0;
	MYS_OBJ *pObjInArray = pObjExecArray->un.pVar->un.pObjInArray;
	for (int j = i + 1; j < STACK_LEVEL(mys);)
	{
		pObjInArray[iArrayIndex++] = *STACK_OBJ_FIRST(mys, j++);
	}
	pObjExecArray->un.pVar->iLength = iArrayIndex;

	((MYSD *)mys)->iOPStackUsing = iStackLevelForExecArray + 1;

	return MYS_OK;
}

int __opr_abs(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "abs");
	}

	MYS_OBJ *pObj = STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 1;

	if (pObj->ObjType == OTE_INTEGER)
	{
		PushInteger(mys, abs(GET_INTEGER(pObj)));
	}
	else if (pObj->ObjType == OTE_REAL)
	{
		PushReal(mys, (GET_REAL(pObj) < 0 ? -1. * GET_REAL(pObj) : GET_REAL(pObj)));
	}
	else
	{
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "abs");
	}

	return MYS_OK;
}

int iCloseExecArray(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing++;
	return __opr_closeexecarray(mys);
}

typedef enum
{
	CAL_ADD = 0,
	CAL_SUB,
	CAL_MUL,
	CAL_DIV,
	CAL_MOD,
	CAL_OR,
	CAL_AND,
	CAL_XOR
} CALCULATE;

static int _calculate(MYS mys, CALCULATE cal)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "calculation's");
	}

	MYS_OBJ *pObj1 = STACK_OBJ_LAST(mys, -1);
	MYS_OBJ *pObj2 = STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	double v1, v2;

	if (pObj1->ObjType == OTE_INTEGER)
	{
		if (pObj2->ObjType == OTE_INTEGER)
		{
			switch (cal)
			{
			case CAL_ADD:
				PushInteger(mys, pObj1->un.iInteger + pObj2->un.iInteger);
				break;
			case CAL_SUB:
				PushInteger(mys, pObj1->un.iInteger - pObj2->un.iInteger);
				break;
			case CAL_MUL:
				PushInteger(mys, pObj1->un.iInteger * pObj2->un.iInteger);
				break;
			case CAL_DIV:
				PushInteger(mys, pObj1->un.iInteger / pObj2->un.iInteger);
				break;
			case CAL_MOD:
				PushInteger(mys, pObj1->un.iInteger % pObj2->un.iInteger);
				break;
			case CAL_OR:
				PushInteger(mys, pObj1->un.iInteger | pObj2->un.iInteger);
				break;
			case CAL_AND:
				PushInteger(mys, pObj1->un.iInteger & pObj2->un.iInteger);
				break;
			case CAL_XOR:
				PushInteger(mys, pObj1->un.iInteger ^ pObj2->un.iInteger);
				break;
			}

			return MYS_OK;
		}
		else
		{
			v1 = (double)pObj1->un.iInteger;
			v2 = pObj2->un.dReal;
		}
	}
	else if (pObj1->ObjType == OTE_boolEAN)
	{
		if (pObj2->ObjType == OTE_boolEAN)
		{
			switch (cal)
			{
			case CAL_OR:
				Pushboolean(mys, pObj1->un.iInteger | pObj2->un.iInteger);
				break;
			case CAL_AND:
				Pushboolean(mys, pObj1->un.iInteger & pObj2->un.iInteger);
				break;
			case CAL_XOR:
				Pushboolean(mys, pObj1->un.iInteger ^ pObj2->un.iInteger);
				break;
			default:
				OPSTACK_RESTORE(mys);
				return iError(mys, TYPECHECK, __func__, __LINE__, "calculation's");
				break;
			}
			return MYS_OK;
		}
		else
		{
			OPSTACK_RESTORE(mys);
			return iError(mys, TYPECHECK, __func__, __LINE__, "calculation's");
		}
	}
	else
	{
		v1 = pObj1->un.dReal;

		if (pObj2->ObjType == OTE_INTEGER)
		{
			v2 = (double)pObj2->un.iInteger;
		}
		else
		{
			v2 = pObj2->un.dReal;
		}
	}

	switch (cal)
	{
	case CAL_ADD:
		PushReal(mys, v1 + v2);
		break;
	case CAL_SUB:
		PushReal(mys, v1 - v2);
		break;
	case CAL_MUL:
		PushReal(mys, v1 * v2);
		break;
	case CAL_DIV:
		PushReal(mys, v1 / v2);
		break;
	case CAL_MOD:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "mod");
	}

	return MYS_OK;
}

int __opr_add(MYS mys)
{
	return _calculate(mys, CAL_ADD);
}

int __opr_sub(MYS mys)
{
	return _calculate(mys, CAL_SUB);
}

int __opr_mul(MYS mys)
{
	return _calculate(mys, CAL_MUL);
}

int __opr_div(MYS mys)
{
	return _calculate(mys, CAL_DIV);
}

int __opr_mod(MYS mys)
{
	return _calculate(mys, CAL_MOD);
}

int __opr_or(MYS mys)
{
	return _calculate(mys, CAL_OR);
}

int __opr_xor(MYS mys)
{
	return _calculate(mys, CAL_XOR);
}

int __opr_and(MYS mys)
{
	return _calculate(mys, CAL_AND);
}

int __opr_sqrt(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 1)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "sqrt");

	MYS_OBJ *pObj = STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 1;

	double value;

	switch (pObj->ObjType)
	{
	case OTE_INTEGER:
		value = sqrt((double)pObj->un.iInteger);
		break;
	case OTE_REAL:
		value = sqrt(pObj->un.dReal);
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "sqrt");
	}

	PushReal(mys, value);

	return MYS_OK;
}

int __opr_debug(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "debug");

	MYS_OBJ Objboolean = *STACK_OBJ_LAST(mys, 0);
	if (Objboolean.ObjType != OTE_boolEAN)
		return iError(mys, TYPECHECK, __func__, __LINE__, "debug");

	((MYSD *)mys)->iOPStackUsing--;

	if (Objboolean.un.iInteger)
		((MYSD *)mys)->bDebugMessage = true;
	else
		((MYSD *)mys)->bDebugMessage = false;

	return MYS_OK;
}

int __opr_exec(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	return iExecute(mys);
}

int __opr_execfile(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	if (Obj.ObjType != OTE_STRING)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "execfile");
	}

	FILE *fp = fopen(GET_STRING((&Obj)), "r");
	if (fp == NULL)
	{
		return iError(mys, INVALIDDATA, __func__, __LINE__, "execfile", GET_STRING((&Obj)));
	}

	((MYSD *)mys)->iOPStackUsing--;

	FILE *fpIn = ((MYSD *)mys)->fpIn;
	bool bPrompt = ((MYSD *)mys)->bPrompt;

	((MYSD *)mys)->bPrompt = false;
	((MYSD *)mys)->fpIn = fp;
	int retv = MysMainLoop(mys);
	((MYSD *)mys)->fpIn = fpIn;
	((MYSD *)mys)->bPrompt = bPrompt;

	fclose(fp);

	return (retv == MYS_EOF ? MYS_OK : retv);
}

int __opr_if(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Objboolean = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjProc = *STACK_OBJ_LAST(mys, 0);

	if (Objboolean.ObjType != OTE_boolEAN)
		return iError(mys, TYPECHECK, __func__, __LINE__, "if");

	if (ObjProc.ObjType != OTE_ARRAY || ObjProc.bExecutable == false)
		return iError(mys, TYPECHECK, __func__, __LINE__, "if");

	((MYSD *)mys)->iOPStackUsing -= 2;

	if (GET_INTEGER((&Objboolean)) == 1)
	{
		PushObj(mys, &ObjProc);
		return iExecute(mys);
	}

	return MYS_OK;
}

int __opr_ifelse(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Objboolean = *STACK_OBJ_LAST(mys, -2);
	MYS_OBJ ObjProctrue = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjProcfalse = *STACK_OBJ_LAST(mys, 0);

	if (Objboolean.ObjType != OTE_boolEAN)
		return iError(mys, TYPECHECK, __func__, __LINE__, "ifelse");

	if (ObjProctrue.ObjType != OTE_ARRAY || ObjProctrue.bExecutable == false)
		return iError(mys, TYPECHECK, __func__, __LINE__, "ifelse");

	if (ObjProcfalse.ObjType != OTE_ARRAY || ObjProcfalse.bExecutable == false)
		return iError(mys, TYPECHECK, __func__, __LINE__, "ifelse");

	((MYSD *)mys)->iOPStackUsing -= 3;

	if (GET_INTEGER((&Objboolean)) == 1)
	{
		PushObj(mys, &ObjProctrue);
	}
	else
	{
		PushObj(mys, &ObjProcfalse);
	}
	return iExecute(mys);
}

int __opr_exit(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	return MYS_EXIT;
}

int __opr_loop(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "loop");

	MYS_OBJ ObjProc = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 1;

	int retv = 1;

	do
	{
		PushObj(mys, &ObjProc);
		retv = iExecute(mys);
		if (retv < 1)
			break;
	} while (true);

	if (retv < 0)
		return retv;
	else
		return MYS_OK;
}

int __opr_repeat(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "repeat");

	MYS_OBJ ObjToNum = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjProc = *STACK_OBJ_LAST(mys, 0);

	if (ObjToNum.ObjType != OTE_INTEGER)
		return iError(mys, TYPECHECK, __func__, __LINE__, "repeat");

	if (ObjProc.ObjType != OTE_ARRAY)
		return iError(mys, TYPECHECK, __func__, __LINE__, "repeat");

	((MYSD *)mys)->iOPStackUsing -= 2;

	int retv = 1;

	for (int i = 0; i < ObjToNum.un.iInteger; i++)
	{
		PushObj(mys, &ObjProc);
		retv = iExecute(mys);
		if (retv < 1)
			break;
	}

	if (retv < 0)
		return retv;
	else
		return MYS_OK;
}

int __opr_for(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 4)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "for");

	MYS_OBJ ObjInitialValue = *STACK_OBJ_LAST(mys, -3);
	MYS_OBJ ObjStepValue = *STACK_OBJ_LAST(mys, -2);
	MYS_OBJ ObjLimitValue = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjProc = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 4;

	int retv = 1;

	if (ObjStepValue.ObjType == OTE_INTEGER)
	{
		int initialVal = GET_INTEGER(&ObjInitialValue);
		int stepVal = GET_INTEGER(&ObjStepValue);
		int limitVal = GET_INTEGER(&ObjLimitValue);
		if (stepVal > 0)
		{
			for (int i = initialVal; i <= limitVal; i += stepVal)
			{
				PushInteger(mys, i);
				PushObj(mys, &ObjProc);
				retv = iExecute(mys);
				if (retv < 1)
					break;
			}
		}
		else
		{
			for (int i = initialVal; i >= limitVal; i += stepVal)
			{
				PushInteger(mys, i);
				PushObj(mys, &ObjProc);
				retv = iExecute(mys);
				if (retv < 1)
					break;
			}
		}
	}
	else if (ObjStepValue.ObjType == OTE_REAL)
	{
		double initialVal, stepVal, limitVal;

		if (ObjInitialValue.ObjType == OTE_REAL)
			initialVal = GET_REAL(&ObjInitialValue);
		else
			initialVal = GET_INTEGER(&ObjInitialValue);

		if (ObjStepValue.ObjType == OTE_REAL)
			stepVal = GET_REAL(&ObjStepValue);
		else
			stepVal = GET_INTEGER(&ObjStepValue);

		if (ObjLimitValue.ObjType == OTE_REAL)
			limitVal = GET_REAL(&ObjLimitValue);
		else
			limitVal = GET_INTEGER(&ObjLimitValue);

		if (stepVal > .0)
		{
			for (double i = initialVal; i <= limitVal; i += stepVal)
			{
				PushReal(mys, i);
				PushObj(mys, &ObjProc);
				retv = iExecute(mys);
				if (retv < 1)
					break;
			}
		}
		else
		{
			for (double i = initialVal; i >= limitVal; i += stepVal)
			{
				PushReal(mys, i);
				PushObj(mys, &ObjProc);
				retv = iExecute(mys);
				if (retv < 1)
					break;
			}
		}
	}
	else
	{
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "for");
	}

	if (retv < 0)
		return retv;
	else
		return MYS_OK;
}

int __opr_forall(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	MYS_OBJ ObjToVar = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjProc = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	int retv = 1;

	switch (ObjToVar.ObjType)
	{
	case OTE_ARRAY:
	{
		MYS_OBJ *pObjInArray = ObjToVar.un.pVar->un.pObjInArray;
		for (int i = 0; i < ObjToVar.un.pVar->iLength; i++)
		{
			PushObj(mys, &pObjInArray[i]);
			PushObj(mys, &ObjProc);
			retv = iExecute(mys);
			if (retv < 1)
				break;
		}
		break;
	}
	case OTE_STRING:
	{
		char *pString = ObjToVar.un.pVar->un.pString;
		for (int i = 0; i < ObjToVar.un.pVar->iLength; i++)
		{
			PushInteger(mys, (long)pString[i]);
			PushObj(mys, &ObjProc);
			retv = iExecute(mys);
			if (retv < 1)
				break;
		}
		break;
	}
	case OTE_DICTIONARY:
	{
		MYS_DICT_ENTRY *pObjInDict = ObjToVar.un.pVar->un.pDictEntry;
		for (int i = 0; i < ObjToVar.un.pVar->iLength; i++)
		{
			PushObj(mys, &pObjInDict[i].ObjKey);
			PushObj(mys, &pObjInDict[i].ObjValue);
			PushObj(mys, &ObjProc);
			retv = iExecute(mys);
			if (retv < 1)
				break;
		}
		break;
	}
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "forall");
	}

	if (retv < 0)
		return retv;
	else
		return MYS_OK;
}

int __opr_pop(MYS mys)
{
	if (((MYSD *)mys)->iOPStackUsing < 2)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "pop");

	((MYSD *)mys)->iOPStackUsing -= 2;
	return MYS_OK;
}

int __opr_clear(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing = 0;
	return MYS_OK;
}

int __opr_cleartomark(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	do
	{
	} while (STACK_OBJ_FIRST(mys, ((MYSD *)mys)->iOPStackUsing-- - 1)->ObjType != OTE_MARK);

	return MYS_OK;
}

int __opr_exch(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "exch");

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);
	*STACK_OBJ_LAST(mys, 0) = *STACK_OBJ_LAST(mys, -1);
	*STACK_OBJ_LAST(mys, -1) = Obj;
	return MYS_OK;
}

int __opr_dup(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);
	PushObj(mys, &Obj);
	return MYS_OK;
}

int __opr_index(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	if (Obj.ObjType != OTE_INTEGER)
		return iError(mys, TYPECHECK, __func__, __LINE__, "index");

	if (((MYSD *)mys)->iOPStackUsing < Obj.un.iInteger)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "index");

	((MYSD *)mys)->iOPStackUsing -= 1;

	PushObj(mys, STACK_OBJ_LAST(mys, -1 * Obj.un.iInteger));

	return MYS_OK;
}

int __opr_roll(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ ObjStep = *STACK_OBJ_LAST(mys, 0);
	MYS_OBJ ObjNum = *STACK_OBJ_LAST(mys, -1);

	int iNum, iStep;
	if (bGetInteger(mys, &ObjNum, &iNum, "roll") == false)
		return false;
	if (bGetInteger(mys, &ObjStep, &iStep, "roll") == false)
		return false;
	if (((MYSD *)mys)->iOPStackUsing < 2 + iNum)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "roll");

	if (iNum > 32)
		return iError(mys, RANGECHECK, __func__, __LINE__, "roll");

	((MYSD *)mys)->iOPStackUsing -= 2;

	MYS_OBJ WorkStack[32];
	MYS_OBJ *pStackTargetBottom = STACK_OBJ_LAST(mys, -iNum + 1);
	memcpy(WorkStack, pStackTargetBottom, sizeof(MYS_OBJ) * iNum);

	iStep = iStep % iNum;

	if (iStep > 0)
	{
		memcpy(&pStackTargetBottom[iStep], &WorkStack[0], sizeof(MYS_OBJ) * (iNum - abs(iStep)));
		memcpy(pStackTargetBottom, &WorkStack[iNum - abs(iStep)], sizeof(MYS_OBJ) * abs(iStep));
	}
	else
	{
		memcpy(pStackTargetBottom, &WorkStack[-iStep], sizeof(MYS_OBJ) * (iNum + iStep));
		memcpy(&pStackTargetBottom[iNum + iStep], &WorkStack[0], sizeof(MYS_OBJ) * -iStep);
	}

	return MYS_OK;
}

int __opr_def(MYS mys)
{

	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "def");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	if (Obj1.ObjType != OTE_NAME)
		return iError(mys, TYPECHECK, __func__, __LINE__, "def");

	((MYSD *)mys)->iOPStackUsing -= 2;

	int retv = (bRegistObjToDict(mys, CURRENTDICT(mys), &Obj1, &Obj2) == true ? 1 : 0);

	return retv;
}

int __opr_dict(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "dict");
	}

	MYS_OBJ ObjInt = *STACK_OBJ_LAST(mys, 0);
	if (ObjInt.ObjType != OTE_INTEGER)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "dict");
	}

	MYS_OBJ *pObjDict = STACK_OBJ_LAST(mys, 0); /* Put an obj on the mark obj. */
	pObjDict->bExecutable = true;
	pObjDict->ObjType = OTE_DICTIONARY;
	int iValue = (int)GET_INTEGER((&ObjInt));
	pObjDict->un.pVar = pVarCreate(mys, VTE_DICTIONARY, &iValue);

	return MYS_OK;
}

int __opr_currentdict(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ *pObjDict = STACK_OBJ_NEXT(mys);
	*pObjDict = ((MYSD *)mys)->DictStack[((MYSD *)mys)->iDictStackUsing - 1];

	((MYSD *)mys)->iOPStackUsing++;

	return MYS_OK;
}

int __opr_begin(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "begin");
	}
	MYS_OBJ ObjDict = *STACK_OBJ_LAST(mys, 0);
	if (ObjDict.ObjType != OTE_DICTIONARY)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "begin");
	}
	((MYSD *)mys)->iOPStackUsing--;
	PushDict(mys, &ObjDict);

	return MYS_OK;
}

int __opr_end(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	if (((MYSD *)mys)->iDictStackUsing < 2)
	{
		return iError(mys, DICTSTACKUNDERFLOW, __func__, __LINE__, "end");
	}
	PopDict(mys);
	return MYS_OK;
}

int __opr_prompt(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	((MYSD *)mys)->bPrompt = true;
	return MYS_OK;
}

int __opr_true(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	Pushboolean(mys, 1);

	return MYS_OK;
}

int __opr_null(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	PushNull(mys);

	return MYS_OK;
}

int __opr_false(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	Pushboolean(mys, 0);

	return MYS_OK;
}

int __opr_length(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);
	switch (Obj.ObjType)
	{
	case OTE_NAME:
	case OTE_STRING:
	case OTE_ARRAY:
	case OTE_DICTIONARY:
		break;
	default:
		return iError(mys, TYPECHECK, __func__, __LINE__, "length");
	}

	((MYSD *)mys)->iOPStackUsing--;

	PushInteger(mys, Obj.un.pVar->iLength);

	return MYS_OK;
}

int __opr_aload(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ ObjArray = *STACK_OBJ_LAST(mys, 0);
	switch (ObjArray.ObjType)
	{
	case OTE_ARRAY:
		break;
	default:
		return iError(mys, TYPECHECK, __func__, __LINE__, "aload");
	}

	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ *pObjInArray = GET_OBJINARRAY(&ObjArray);

	for (int i = 0; i < ObjArray.un.pVar->iLength; i++)
	{
		PushObj(mys, &pObjInArray[i]);
	}

	PushObj(mys, &ObjArray);

	return MYS_OK;
}

int __opr_load(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);
	switch (Obj.ObjType)
	{
	case OTE_NAME:
	case OTE_STRING:
		break;
	default:
		return iError(mys, TYPECHECK, __func__, __LINE__, "load");
	}

	((MYSD *)mys)->iOPStackUsing--;

	PushObj(mys, pObjSearchInDicts(mys, &Obj));

	return MYS_OK;
}

int __opr_maxlength(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);
	switch (Obj.ObjType)
	{
	case OTE_NAME:
	case OTE_STRING:
	case OTE_ARRAY:
	case OTE_DICTIONARY:
		break;
	default:
		return iError(mys, TYPECHECK, __func__, __LINE__, "maxlength");
	}

	((MYSD *)mys)->iOPStackUsing--;

	PushInteger(mys, Obj.un.pVar->iBufferLength);

	return MYS_OK;
}

int __opr_checkmem(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	CheckMemory((char *)"_opr_checkmem");

	return true;
}

int __opr_enummem(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	EnumMemory(mys);

	return MYS_OK;
}

int __opr_mark(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	PushMark(mys, false);
	return MYS_OK;
}

int __opr_getinterval(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 3)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "getinterval");
	}

	MYS_OBJ ObjAny = *STACK_OBJ_LAST(mys, -2);
	MYS_OBJ ObjIndex = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjCount = *STACK_OBJ_LAST(mys, 0);

	if (ObjIndex.ObjType != OTE_INTEGER)
		return iError(mys, TYPECHECK, __func__, __LINE__, "getinterval");

	if (ObjCount.ObjType != OTE_INTEGER)
		return iError(mys, TYPECHECK, __func__, __LINE__, "getinterval");

	if (ObjAny.un.pVar->iLength < ObjIndex.un.iInteger + ObjCount.un.iInteger)
		return iError(mys, RANGECHECK, __func__, __LINE__, "getinterval");

	((MYSD *)mys)->iOPStackUsing -= 3;

	if (ObjAny.ObjType == OTE_STRING)
	{
		char buf[1024];
		memcpy(buf, GET_STRING(&ObjAny) + GET_INTEGER(&ObjIndex), GET_INTEGER(&ObjCount));
		buf[GET_INTEGER(&ObjCount)] = 0x00;
		PushString(mys, buf);
	}
	else if (ObjAny.ObjType == OTE_ARRAY)
	{
		PushMark(mys, false);

		MYS_OBJ *pObjInArray = GET_OBJINARRAY(&ObjAny);
		for (int i = GET_INTEGER(&ObjIndex); i < GET_INTEGER(&ObjIndex) + GET_INTEGER(&ObjCount); i++)
			PushObj(mys, &pObjInArray[i]);

		iCloseArray(mys);
	}
	else
	{
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "getinterval");
	}

	return MYS_OK;
}

int __opr_putinterval(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 3)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "putinterval");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -2);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj3 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 3;

	switch (Obj1.ObjType)
	{
	case OTE_ARRAY:
	{
		if (Obj2.ObjType != OTE_INTEGER || Obj3.ObjType != OTE_ARRAY)
			return iError(mys, TYPECHECK, __func__, __LINE__, "putinterval");
		MYS_OBJ *pObjTar = GET_OBJINARRAY(&Obj1);
		MYS_OBJ *pObjSrc = GET_OBJINARRAY(&Obj3);
		for (int t = GET_INTEGER(&Obj2), s = 0; s < Obj3.un.pVar->iLength; s++, t++)
			pObjTar[t] = pObjSrc[s];
		break;
	}
	case OTE_STRING:
	{
		if (Obj2.ObjType != OTE_INTEGER || Obj3.ObjType != OTE_STRING)
			return iError(mys, TYPECHECK, __func__, __LINE__, "putinterval");
		char *pStrTar = GET_STRING(&Obj1);
		char *pStrSrc = GET_STRING(&Obj3);
		for (int t = GET_INTEGER(&Obj2), s = 0; s < Obj3.un.pVar->iLength; s++, t++)
			pStrTar[t] = pStrSrc[s];
		break;
	}
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "putinterval");
	}

	return MYS_OK;
}

int __opr_get(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "get");
	}

	MYS_OBJ ObjAny = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjKey = *STACK_OBJ_LAST(mys, 0);

	switch (ObjAny.ObjType)
	{
	case OTE_STRING:
	{
		char *ptr = GET_STRING(&ObjAny);
		if (ObjKey.ObjType != OTE_INTEGER)
			return iError(mys, TYPECHECK, __func__, __LINE__, "get");
		if (ObjAny.un.pVar->iLength - 1 < GET_INTEGER(&ObjKey))
			return iError(mys, RANGECHECK, __func__, __LINE__, "get");
		((MYSD *)mys)->iOPStackUsing -= 2;
		PushInteger(mys, ptr[GET_INTEGER(&ObjKey)]);
		break;
	}
	case OTE_ARRAY:
	{
		MYS_OBJ *pObjInArray = GET_OBJINARRAY(&ObjAny);
		if (ObjKey.ObjType != OTE_INTEGER)
			return iError(mys, TYPECHECK, __func__, __LINE__, "get");
		if (ObjAny.un.pVar->iLength - 1 < GET_INTEGER(&ObjKey))
			return iError(mys, RANGECHECK, __func__, __LINE__, "get");
		((MYSD *)mys)->iOPStackUsing -= 2;
		PushObj(mys, &pObjInArray[GET_INTEGER(&ObjKey)]);
		break;
	}
	case OTE_DICTIONARY:
	{
		if (ObjKey.ObjType != OTE_NAME)
			return iError(mys, TYPECHECK, __func__, __LINE__, "get");
		MYS_OBJ *pObj = pObjSearchInTheDict(mys, &ObjAny, &ObjKey);
		if (pObj == NULL)
			return iError(mys, UNDEFINED, __func__, __LINE__, "ObjKey.un.pVar->un.pString");
		((MYSD *)mys)->iOPStackUsing -= 2;
		PushObj(mys, pObj);
		break;
	}
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "get");
	}

	return MYS_OK;
}

int __opr_time(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	time_t t = time(NULL);
	struct tm *local = localtime(&t);

	char buf[256];
	sprintf(buf, "%04d/%02d/%02d@%02d:%02d:%02d",
			local->tm_year + 1900,
			local->tm_mon + 1,
			local->tm_mday,
			local->tm_hour,
			local->tm_min,
			local->tm_sec);

	PushString(mys, buf);

	return MYS_OK;
}

int __opr_print(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "print");
	}

	MYS_fprintf(MYS_stdout, "%s", GET_STRING(STACK_OBJ_LAST(mys, 0)));

	((MYSD *)mys)->iOPStackUsing--;

	return MYS_OK;
}

int __opr_cvs(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "cvs");
	}
	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);
	((MYSD *)mys)->iOPStackUsing--;

	switch (Obj.ObjType)
	{
	case OTE_INTEGER:
	{
		char buf[128];
		sprintf(buf, "%ld", Obj.un.iInteger);
		PushString(mys, buf);
		break;
	}
	case OTE_REAL:
	{
		char buf[128];
		sprintf(buf, "%lf", Obj.un.dReal);
		PushString(mys, buf);
		break;
	}
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "cvs");
	}

	return MYS_OK;
}

int __opr_cvx(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "cvx");
	}

	MYS_OBJ *pObj = STACK_OBJ_LAST(mys, 0);
	pObj->bExecutable = true;

	return MYS_OK;
}

int __opr_cvn(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "cvn");
	}

	MYS_OBJ *pObj = STACK_OBJ_LAST(mys, 0);
	pObj->bExecutable = false;
	pObj->ObjType = OTE_NAME;

	return MYS_OK;
}

int __opr_concat(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "concat");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	if (Obj1.ObjType != Obj2.ObjType || Obj1.ObjType != OTE_STRING)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "concat");
	}

	((MYSD *)mys)->iOPStackUsing -= 2;

	char tmp[PATH_MAX * 2];
	sprintf(tmp, "%s%s", Obj1.un.pVar->un.pString, Obj2.un.pVar->un.pString);
	PushString(mys, tmp);

	return MYS_OK;
}

int __opr_cvi(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "cvi");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	switch (Obj.ObjType)
	{
	case OTE_REAL:
		((MYSD *)mys)->iOPStackUsing -= 1;
		PushInteger(mys, (long)Obj.un.dReal);
		break;
	case OTE_INTEGER:
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "cvi");
	}
	return MYS_OK;
}

int __opr_cvr(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "cvr");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	switch (Obj.ObjType)
	{
	case OTE_REAL:
		break;
	case OTE_INTEGER:
		((MYSD *)mys)->iOPStackUsing -= 1;
		PushReal(mys, (double)Obj.un.iInteger);
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "cvr");
	}

	return MYS_OK;
}

int __opr_round(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "round");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	if (Obj.ObjType != OTE_REAL)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "round");
	}

	((MYSD *)mys)->iOPStackUsing -= 1;

	PushReal(mys, round(Obj.un.dReal));

	return MYS_OK;
}

int __opr_truncate(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "truncate");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	if (Obj.ObjType != OTE_REAL)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "truncate");
	}

	((MYSD *)mys)->iOPStackUsing -= 1;

	PushReal(mys, trunc(Obj.un.dReal));

	return MYS_OK;
}

int __opr_search(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "search");
	}

	MYS_OBJ ObjString = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjSeek = *STACK_OBJ_LAST(mys, 0);

	if (ObjString.ObjType != ObjSeek.ObjType || ObjString.ObjType != OTE_STRING)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "search");
	}

	char tmp[1024];
	memcpy(tmp, ObjString.un.pVar->un.pString, strlen(ObjString.un.pVar->un.pString) + 1);

	char *ptr = (char *)strstr(tmp, ObjSeek.un.pVar->un.pString);
	if (ptr)
	{
		((MYSD *)mys)->iOPStackUsing -= 2;
		PushString(mys, ptr + ObjSeek.un.pVar->iLength);
		PushString(mys, ObjSeek.un.pVar->un.pString);
		*ptr = 0x00;
		PushString(mys, tmp);
		Pushboolean(mys, 1);
	}
	else
	{
		((MYSD *)mys)->iOPStackUsing--;
		Pushboolean(mys, 0);
	}

	return MYS_OK;
}

FILE *MYS_stdout;

int __opr_stdout(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "stdout");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 1;

	switch (Obj.ObjType)
	{
	case OTE_STRING:
		MYS_stdout = fopen(Obj.un.pVar->un.pString, "w");
		break;
	case OTE_NULL:
		if (MYS_stdout != stdout)
		{
			fclose(MYS_stdout);
		}
		MYS_stdout = stdout;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "stdout");
		break;
	}

	return MYS_OK;
}

int __opr_put(MYS mys)
{
	// array index any put -
	// string index int put -
	// dict name any put -
	// appdata name any put -
	// devmode name any put -

	((MYSD *)mys)->iOPStackUsing--;

	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 3)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "put");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -2);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj3 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 3;

	switch (Obj1.ObjType)
	{
	case OTE_ARRAY:
	{
		if (Obj2.ObjType != OTE_INTEGER)
		{
			OPSTACK_RESTORE(mys);
			return iError(mys, TYPECHECK, __func__, __LINE__, "put");
		}
		MYS_OBJ *pObjInArray = GET_OBJINARRAY(&Obj1);
		pObjInArray[Obj2.un.iInteger] = Obj3;
		break;
	}
	case OTE_STRING:
	{
		if (Obj2.ObjType != OTE_INTEGER)
		{
			OPSTACK_RESTORE(mys);
			return iError(mys, TYPECHECK, __func__, __LINE__, "put");
		}
		char *ptr = GET_STRING(&Obj1);
		if (Obj3.ObjType != OTE_INTEGER)
		{
			OPSTACK_RESTORE(mys);
			return iError(mys, TYPECHECK, __func__, __LINE__, "put");
		}
		ptr[Obj2.un.iInteger] = (char)Obj3.un.iInteger;
		break;
	}
	case OTE_DICTIONARY:
	{
		if (Obj2.ObjType != OTE_NAME)
		{
			OPSTACK_RESTORE(mys);
			return iError(mys, TYPECHECK, __func__, __LINE__, "put");
		}
		bRegistObjToDict(mys, &Obj1, &Obj2, &Obj3);
		break;
	}
	}

	return MYS_OK;
}

int __opr_gt(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "gt");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	bool bRv = true;

	double n1, n2;

	switch (Obj1.ObjType)
	{
	case OTE_INTEGER:
		n1 = Obj1.un.iInteger;
		break;
	case OTE_REAL:
		n1 = Obj1.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "gt");
		break;
	}

	switch (Obj2.ObjType)
	{
	case OTE_INTEGER:
		n2 = Obj2.un.iInteger;
		break;
	case OTE_REAL:
		n2 = Obj2.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "gt");
		break;
	}

	Pushboolean(mys, n1 > n2 ? true : false);

	return MYS_OK;
}

int __opr_ge(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "ge");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	bool bRv = true;

	double n1, n2;

	switch (Obj1.ObjType)
	{
	case OTE_INTEGER:
		n1 = Obj1.un.iInteger;
		break;
	case OTE_REAL:
		n1 = Obj1.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "ge");
		break;
	}

	switch (Obj2.ObjType)
	{
	case OTE_INTEGER:
		n2 = Obj2.un.iInteger;
		break;
	case OTE_REAL:
		n2 = Obj2.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "ge");
		break;
	}

	Pushboolean(mys, n1 >= n2 ? true : false);

	return MYS_OK;
}

int __opr_lt(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "lt");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	double n1, n2;

	switch (Obj1.ObjType)
	{
	case OTE_INTEGER:
		n1 = Obj1.un.iInteger;
		break;
	case OTE_REAL:
		n1 = Obj1.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "lt");
		break;
	}

	switch (Obj2.ObjType)
	{
	case OTE_INTEGER:
		n2 = Obj2.un.iInteger;
		break;
	case OTE_REAL:
		n2 = Obj2.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "lt");
		break;
	}

	Pushboolean(mys, n1 < n2 ? true : false);

	return MYS_OK;
}

int __opr_le(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "le");
	}

	MYS_OBJ Obj1 = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ Obj2 = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 2;

	bool bRv = true;

	double n1, n2;

	switch (Obj1.ObjType)
	{
	case OTE_INTEGER:
		n1 = Obj1.un.iInteger;
		break;
	case OTE_REAL:
		n1 = Obj1.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "le");
		break;
	}

	switch (Obj2.ObjType)
	{
	case OTE_INTEGER:
		n2 = Obj2.un.iInteger;
		break;
	case OTE_REAL:
		n2 = Obj2.un.dReal;
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "le");
		break;
	}

	Pushboolean(mys, n1 <= n2 ? true : false);

	return MYS_OK;
}

int __opr_ceil(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;
	OPSTACK_SAVE(mys);

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "ceil");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	((MYSD *)mys)->iOPStackUsing -= 1;

	switch (Obj.ObjType)
	{
	case OTE_INTEGER:
		PushInteger(mys, GET_INTEGER(&Obj));
		break;
	case OTE_REAL:
		PushInteger(mys, ceil(GET_REAL(&Obj)));
		break;
	default:
		OPSTACK_RESTORE(mys);
		return iError(mys, TYPECHECK, __func__, __LINE__, "ceil");
		break;
	}

	return MYS_OK;
}

int __opr_array(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "array");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	if (Obj.ObjType != OTE_INTEGER)
		return iError(mys, TYPECHECK, __func__, __LINE__, "array");

	int iLengthArray = GET_INTEGER(&Obj);

	MYS_OBJ *pObjArray = STACK_OBJ_LAST(mys, 0);
	pObjArray->bExecutable = false;
	pObjArray->ObjType = OTE_ARRAY;
	pObjArray->un.pVar = pVarCreate(mys, VTE_ARRAY, &iLengthArray);

	MYS_OBJ *pObjInArray = pObjArray->un.pVar->un.pObjInArray;
	for (int i = 0; i < iLengthArray; i++)
	{
		pObjInArray[i].ObjType = OTE_NULL;
	}
	pObjArray->un.pVar->iLength = iLengthArray;

	return MYS_OK;
}

int __opr_copy(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 2)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "copy");
	}

	MYS_OBJ SrcObj = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ DstObj = *STACK_OBJ_LAST(mys, 0);

	if (SrcObj.ObjType != OTE_ARRAY || DstObj.ObjType != OTE_ARRAY)
		return iError(mys, TYPECHECK, __func__, __LINE__, "copy");

	if (SrcObj.un.pVar->iLength > DstObj.un.pVar->iLength)
		return iError(mys, RANGECHECK, __func__, __LINE__, "copy");

	((MYSD *)mys)->iOPStackUsing -= 2;

	MYS_OBJ *pSrcObjInArray = GET_OBJINARRAY(&SrcObj);
	MYS_OBJ *pDstObjInArray = GET_OBJINARRAY(&DstObj);

	for (int i = 0; i < SrcObj.un.pVar->iLength; i++)
	{
		pDstObjInArray[i] = pSrcObjInArray[i];
	}

	PushObj(mys, &DstObj);

	return MYS_OK;
}

void ShowDictOperators(MYS mys);

int __opr_help(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		ShowDictOperators(mys);
	}
	else
	{
		MYS_OBJ ObjKey = *STACK_OBJ_LAST(mys, 0);
		if (ObjKey.ObjType != OTE_NAME || ObjKey.bExecutable == true)
			return iError(mys, TYPECHECK, __func__, __LINE__, "help");

		MYS_OBJ *pObjValue = pObjSearchInDicts(mys, &ObjKey);
		if (pObjValue == NULL)
			return iError(mys, UNDEFINED, __func__, __LINE__, ObjKey.un.pVar->un.pString);

		if (pObjValue->ObjType != OTE_OPERATOR)
			return iError(mys, TYPECHECK, __func__, __LINE__, "help");

		((MYSD *)mys)->iOPStackUsing--;

		MysHelp(mys, pObjValue->un.pOperator->pKeyName, pObjValue->un.pOperator->pOperatorParams, pObjValue->un.pOperator->pOperatorSpec);
	}
	return MYS_OK;
}

int __opr_pwd(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	char buf[PATH_MAX];
	getcwd(buf, PATH_MAX);
	PushString(mys, buf);
	return MYS_OK;
}

int __opr_cd(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	if (((MYSD *)mys)->iOPStackUsing < 1)
	{
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "cd");
	}

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	if (Obj.ObjType != OTE_STRING)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "cd");
	}

	((MYSD *)mys)->iOPStackUsing -= 1;

	chdir(GET_STRING(&Obj));
	char pathname[PATH_MAX];
	getcwd(pathname, PATH_MAX);
	PushString(mys, pathname);

	return MYS_OK;
}

int __opr_enumfiles(MYS mys)
{
	int retv = MYS_OK;

	MYSD *mysd = (MYSD *)mys;

	mysd->iOPStackUsing--; /* pop OPERATOR */

	if (mysd->iOPStackUsing < 2)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "enumfiles");

	MYS_OBJ Pattern = *STACK_OBJ_LAST(mys, -1);
	if (Pattern.ObjType != OTE_STRING)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "enumfiles");
	}

	MYS_OBJ ObjProc = *STACK_OBJ_LAST(mys, 0);
	if (ObjProc.ObjType != OTE_ARRAY || ObjProc.bExecutable != true)
	{
		return iError(mys, TYPECHECK, __func__, __LINE__, "enumfiles");
	}

	mysd->iOPStackUsing -= 2;

	char folderPath[PATH_MAX] = {};

	const char *pattern = GET_STRING(&Pattern);
	glob_t glob_result;

	if (glob(pattern, GLOB_TILDE, nullptr, &glob_result) == 0)
	{
		// 結果を列挙
		for (size_t i = 0; i < glob_result.gl_pathc; ++i)
		{
			PushString(mys, glob_result.gl_pathv[i]);
			PushObj(mys, &ObjProc);
			retv = iExecute(mys);
			if (retv < 1)
				break;
		}
	}
	// メモリを解放
	globfree(&glob_result);

	return retv;
}

int __opr_getenv(MYS mys)
{
	((MYSD *)mys)->iOPStackUsing--;

	MYS_OBJ ObjString = *STACK_OBJ_LAST(mys, 0);

	if (ObjString.ObjType != OTE_STRING)
		return iError(mys, TYPECHECK, __func__, __LINE__, "getenv");

	((MYSD *)mys)->iOPStackUsing -= 1;

	char *env = getenv(GET_STRING(&ObjString));
	if (env)
	{
		PushString(mys, env);
		Pushboolean(mys, 1);
	}
	else
	{
		PushString(mys, (char *)"");
		Pushboolean(mys, 0);
	}

	return MYS_OK;
}

int __opr_error(MYS mys)
{
	MYSD *mysd = (MYSD *)mys;

	mysd->iOPStackUsing--; /* pop OPERATOR */

	MYS_OBJ Obj = *STACK_OBJ_LAST(mys, 0);

	mysd->iOPStackUsing -= 1;

	return iError(mys, (ERRORTYPE)Obj.un.iInteger, __func__, __LINE__, "sample implementation");
}

int __opr_messagebox(MYS mys)
{
	MYSD *mysd = (MYSD *)mys;

	mysd->iOPStackUsing--; /* pop OPERATOR */

	if (mysd->iOPStackUsing < 2)
		return iError(mys, STACKUNDERFLOW, __func__, __LINE__, "messagebox");

	MYS_OBJ ObjTitle = *STACK_OBJ_LAST(mys, -1);
	MYS_OBJ ObjMessage = *STACK_OBJ_LAST(mys, 0);

	if (ObjTitle.ObjType != OTE_STRING || ObjMessage.ObjType != OTE_STRING)
		return iError(mys, TYPECHECK, __func__, __LINE__, "messagebox");

	char *title = GET_STRING(&ObjTitle);
	char *msg = GET_STRING(&ObjMessage);

	char command[PATH_MAX];
	sprintf(command, "LIBGL_ALWAYS_SOFTWARE=1 zenity --question --title=\"%s\" --text=\"%s\"", title, msg);

	int retv = system(command);

	mysd->iOPStackUsing -= 2;

	if (retv == 0)
		Pushboolean(mys, 1);
	else
		Pushboolean(mys, 0);

	return MYS_OK;
}

/***************************************************
				  OPERATORS TABLE
****************************************************/
static const MYS_OPERATOR _operators[] =
	{
		{"error", __opr_error, "<num> error <num>", "Enforce error with pushing <num>"},
		{"true", __opr_true, "- true --true--", "Push --true--"},
		{"false", __opr_false, "- false --false--", "Push --false--"},
		{"null", __opr_null, "- null --null--", "Push --null--"},

		{"abs", __opr_abs, "<num> abs <absolute of num>", ""},
		{"add", __opr_add, "<num#1> <num#2> add <num#1 + num#2>", "addition"},
		{"sub", __opr_sub, "<num#1> <num#2> sub <num#1 - num#2>", "substraction"},
		{"mul", __opr_mul, "<num#1> <num#2> mul <num#1 * num#2>", "multiplication"},
		{"div", __opr_div, "<num#1> <num#2> div <num#1 / num#2>", "division"},
		{"mod", __opr_mod, "<int#1> <int#2> mod <int#1 \% int#2>", "remainder"},
		{"sqrt", __opr_sqrt, "<num> sqrt <square root of num>", "square root"},
		{"ceil", __opr_ceil, "<num> ceiling <ceiling of num>", "ceiling"},
		{"round", __opr_round, "<num> round <rounding of num>", "rounding"},
		{"truncate", __opr_truncate, "<num> truncate <truncating of num>", "truncating"},
		{"and", __opr_and, "<bool#1|int#1> <bool#2|int#2> and <logical|bitwise and>", "logical|bitwise and"},
		{"or", __opr_or, "<bool#1|int#1> <bool#2|int#2> or <bool#3|int#3>", "logical|bitwise inclusive or"},
		{"xor", __opr_xor, "<bool#1|int#1> <bool#2|int#2> xor <bool#3|int#3>", "logical|bitwise exclusive or"},

		{"eq", __opr_eq, "<any#1> <any#2> eq <bool>", "Test equal"},
		{"ne", __opr_ne, "<any#1> <any#2> ne <bool>", "Test not equal"},
		{"ge", __opr_ge, "<num#1> <num#2> ge <bool>", "Test greater than or equal"},
		{"gt", __opr_gt, "<num#1> <num#2> gt <bool>", "Test greater than"},
		{"le", __opr_le, "<num#1> <num#2> le <bool>", "Test less than or equal"},
		{"lt", __opr_lt, "<num#1> <num#2> lt <bool>", "Test less than"},

		{"clear", __opr_clear, "<any#1> ... <any#n> clear -", "Discard all elements on the stack"},
		{"cleartomark", __opr_cleartomark, "<mark> <obj1> ... <objn> cleartomark -", "Discard elements down through <mark>"},
		{"dup", __opr_dup, "<any> dup <any> <any>", "Duplicate top element"},
		{"exch", __opr_exch, "<any#1> <any#2> exch <any#2> <any#1>", "Exchange top two elements"},
		{"index", __opr_index, "<any#n> ... <any#0> n index <any#n> ... <any#0> <any#n>", "copy n-th elements from top"},
		{"pop", __opr_pop, "<any> pop -", "Discard top element"},
		{"pstack", __opr_pstack, "<any#1> ... <any#n> pstack <any#1> ... <any#n>", "Print stack nondestructively using =="},
		{"print", __opr_print, "(string) print -", "Write string to standard output file"},
		{"roll", __opr_roll, "\n  (a) (b) (c) 3 -1 roll (b) (c) (a)\n  (a) (b) (c) 3 1 roll (c) (a) (b)", "Roll <n> elements up <j> times"},
		{"==", __opr_eqeq, "", "Print and discard top of stack"},

		{"cvi", __opr_cvi, "<num> cvi <int>", "Convert to integer"},
		{"cvr", __opr_cvr, "<num> cvr <real>", "Convert to real"},
		{"cvs", __opr_cvs, "<num> cvs <string>", "Convert to string"},
		{"cvx", __opr_cvx, "<any> cvs <any>", "Make object executable\t(1 2 add) cvx exec 3"},
		{"cvn", __opr_cvn, "<string> cvn <name>", "Make object nametype"},

		{"mark", __opr_mark, "mark --[--", "Push mark"},
		{"array", __opr_array, "<int> array <array>", "Create array of length <int>"},
		{"aload", __opr_aload, "<array> aload <any0> ... <anyn-1> <array>", "Push all elements of <array> on stack"},
		{"copy", __opr_copy, "<array1> <array2> copy <subarray2>", "Copy elements of <array1> to initial subarray of <array2>"},
		{"closearray", __opr_closearray, "<[> <any1> ... <anyn> closearray <array>", "End array construction"},
		{"]", __opr_closearray, "<[> <any1> ... <anyn> ] <array>", "End array construction"},
		{"}", __opr_closeexecarray, "<{> <any1> ... <anyn> } <execarray>", "End exection array construction"},

		{"begin", __opr_begin, "<dict> begin -", "Push <dict> on dictionary stack"},
		{"def", __opr_def, "<key> <value> def -", "Associate <key> and <value> in current dictionary"},
		{"end", __opr_end, "- end -", "Pop current dictionary off dictionary stack"},
		{"dict", __opr_dict, "<int> dict <dict>", "Create dictionary with capacity for <int> elements"},
		{"currentdict", __opr_currentdict, "- currentdict <dict>", "Return current dictionary"},
		{"load", __opr_load, "<key> load <value>", "Search dictionary stack for <key> and return associated value"},

		{"exec", __opr_exec, "<any> exec -", "Execute arbitrary object"},
		{"execfile", __opr_execfile, "<string> execfile -", "Execute a script file"},
		{"run", __opr_execfile, "<string> run -", "Execute a script file"},

		{"search", __opr_search, "<string> <seek> search <post> <match> <pre> true # <string> <seek> search <string> false", "Search for <seek> in <string>"},
		{"concat", __opr_concat, "<string1> <string2> concat <string3>", "Concatenate <string1> and <string2>"},
		{"put", __opr_put, "<array|dict|string> <index|key|index> <any|any|int> put -", "Put <any|any|int> into <array|dict|string> at <index|key|index>"},
		{"putinterval", __opr_putinterval, "<array1|string1> <index> <array2|string2> putinterval -", "Replace <subarray|substring> of <array1|string1> starting at index by <array2|string2>"},
		{"get", __opr_get, "<array|dict|string> <index> get <any>", "Return <any> element indexed by <index>"},
		{"getinterval", __opr_getinterval, "<array|string> <index> <count> getinterval <any>", "Return <subarray|substring> of <array|string> starting at index for count elements"},
		{"length", __opr_length, "<any> length <int>", "Return number of components in <any>"},
		{"maxlength", __opr_maxlength, "<dict|array|name|string> maxlength <int>", "Return current capacity of <dict|array|name|string>"},

		{"for", __opr_for, "<initial> <increment> <limit> proc for -", "Execute proc with values from <initial> by steps of <increment> to <limit>"},
		{"forall", __opr_forall, "<array|dict|string> <proc> forall -", "Execute proc for each element of <array|dict|string>"},
		{"loop", __opr_loop, "proc loop -", "Execute proc an indefinite number of times"},
		{"repeat", __opr_repeat, "<count> proc repeat -", "Execute proc int times"},
		{"exit", __opr_exit, "", "Exit innermost active loop"},
		{"if", __opr_if, "<bool> <proc> if -", "Execute <proc> if <bool> is true"},
		{"ifelse", __opr_ifelse, "<bool> <proc1> <proc2> ifelse -", "Execute <proc1> if <bool> is true, <proc2> if false"},

		{"time", __opr_time, "- time <currenttime>", "Return tickcount"},
		{"help", __opr_help, "- help -", "Show operator spec"},
		{"stdout", __opr_stdout, "<string|null> stdout -", "Set where to output. <string> indicates filename and <null> indicates stdout"},
		{"debug", __opr_debug, "<bool> debug -", "Make mys debugmode"},
		{"enummem", __opr_enummem, "- enummem -", "Enumrate memorystatus"},
		{"checkmem", __opr_checkmem, "- checkmem -", "Check memory status"},

		{"cd", __opr_cd, "<string> cd -", "Change the current directory mys refers"},
		{"pwd", __opr_pwd, "- pwd <string>", "Return current directory mys refers"},
		{"enumfiles", __opr_enumfiles, "<string/pathAndWildcard> <proc> enumfiles <string/path> <string/filename1> <string/path> <string/filename2> ... ", "Return <string/path> <string/filename> searched with <pathAndWildcard> and execute <proc>"},
		{"getenv", __opr_getenv, "<string> getenv <string> <bool>", "Return an env value"},
		{"prompt", __opr_prompt, "- prompt -", "Show 'MYS>'"},
		{"quit", __opr_quit, "- quit -", "Quit from mys"},
		{"messagebox", __opr_messagebox, "<string#1> <string#2> messagebox --true/false--", "shows a messagebox having the tile string#1, the message string#2 and Yes No bottoms, and returs true for yes, false for no."},

		{NULL, NULL, "", NULL}};

/***************************************************
			  REGIST OPERATORS TO DICT
****************************************************/
void InitDictOperators(MYS mys)
{
	MYS_VAR *pVar = ((MYSD *)mys)->DictStack[0].un.pVar;

	for (int i = 0; _operators[i].pKeyName != NULL; i++)
	{
		AddOperatorToDict(mys, (char *)_operators[i].pKeyName, &_operators[i]);
	}
}

void ShowDictOperators(MYS mys)
{
	for (int i = 0; _operators[i].pKeyName != NULL; i++)
	{
		MysHelp(mys, (char *)_operators[i].pKeyName, (char *)_operators[i].pOperatorParams, (char *)_operators[i].pOperatorSpec);
	}
}