#include "simoscript.h"

/***************************************************
					 OPERATORS
****************************************************/
int __opr_pstack(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	for (int i = ((SMSD *)sms)->iOPStackUsing - 1; i > -1; i--)
	{
		SMS_fprintf(SMS_stdout, "%3d| ", i);
		ShowOPStack(&((SMSD *)sms)->OPStack[i], i);
	}

	return SMS_OK;
}

int __opr_quit(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	return SMS_QUIT;
}

int __opr_eqeq(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "==");

	int i = ((SMSD *)sms)->iOPStackUsing - 1;
	ShowOPStack(&((SMSD *)sms)->OPStack[i], i);
	((SMSD *)sms)->iOPStackUsing--;

	return SMS_OK;
}

int __opr_eq(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "eq");

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

	Pushboolean(sms, bObjCompare(&Obj1, &Obj2));

	return SMS_OK;
}

int __opr_ne(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "ne");

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

	Pushboolean(sms, !bObjCompare(&Obj1, &Obj2));

	return SMS_OK;
}

int __opr_closearray(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	int i, iLengthArray = 0;
	for (i = STACK_LEVEL(sms) - 1; i >= 0; i--)
	{
		if (STACK_OBJ_FIRST(sms, i)->ObjType == OTE_MARK && STACK_OBJ_FIRST(sms, i)->bExecutable == false)
			break;
		iLengthArray++;
	}

	if (i < 0)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "]");

	int iStackLevelForArray = i;

	SMS_OBJ *pObjArray = STACK_OBJ_FIRST(sms, iStackLevelForArray);
	pObjArray->bExecutable = false;
	pObjArray->ObjType = OTE_ARRAY;
	pObjArray->un.pVar = pVarCreate(sms, VTE_ARRAY, &iLengthArray);

	int iArrayIndex = 0;
	SMS_OBJ *pObjInArray = pObjArray->un.pVar->un.pObjInArray;
	for (int j = i + 1; j < STACK_LEVEL(sms);)
	{
		pObjInArray[iArrayIndex++] = *STACK_OBJ_FIRST(sms, j++);
	}
	pObjArray->un.pVar->iLength = iArrayIndex;

	((SMSD *)sms)->iOPStackUsing = iStackLevelForArray + 1;

	return SMS_OK;
}

int iCloseArray(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing++;
	return __opr_closearray(sms);
}

int __opr_closeexecarray(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	int i, iLengthArray = 0;
	for (i = STACK_LEVEL(sms) - 1; i > -1; i--)
	{
		if (STACK_OBJ_FIRST(sms, i)->ObjType == OTE_MARK && STACK_OBJ_FIRST(sms, i)->bExecutable == true)
			break;
		iLengthArray++;
	}

	int iStackLevelForExecArray = i;

	SMS_OBJ *pObjExecArray = STACK_OBJ_FIRST(sms, iStackLevelForExecArray);
	pObjExecArray->bExecutable = true;
	pObjExecArray->ObjType = OTE_ARRAY;
	pObjExecArray->un.pVar = pVarCreate(sms, VTE_ARRAY, &iLengthArray);

	int iArrayIndex = 0;
	SMS_OBJ *pObjInArray = pObjExecArray->un.pVar->un.pObjInArray;
	for (int j = i + 1; j < STACK_LEVEL(sms);)
	{
		pObjInArray[iArrayIndex++] = *STACK_OBJ_FIRST(sms, j++);
	}
	pObjExecArray->un.pVar->iLength = iArrayIndex;

	((SMSD *)sms)->iOPStackUsing = iStackLevelForExecArray + 1;

	return SMS_OK;
}

int __opr_abs(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "abs");
	}

	SMS_OBJ *pObj = STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 1;

	if (pObj->ObjType == OTE_INTEGER)
	{
		PushInteger(sms, abs(GET_INTEGER(pObj)));
	}
	else if (pObj->ObjType == OTE_REAL)
	{
		PushReal(sms, (GET_REAL(pObj) < 0 ? -1. * GET_REAL(pObj) : GET_REAL(pObj)));
	}
	else
	{
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "abs");
	}

	return SMS_OK;
}

int iCloseExecArray(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing++;
	return __opr_closeexecarray(sms);
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

static int _calculate(SMS sms, CALCULATE cal)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "calculation's");
	}

	SMS_OBJ *pObj1 = STACK_OBJ_LAST(sms, -1);
	SMS_OBJ *pObj2 = STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

	double v1, v2;

	if (pObj1->ObjType == OTE_INTEGER)
	{
		if (pObj2->ObjType == OTE_INTEGER)
		{
			switch (cal)
			{
			case CAL_ADD:
				PushInteger(sms, pObj1->un.iInteger + pObj2->un.iInteger);
				break;
			case CAL_SUB:
				PushInteger(sms, pObj1->un.iInteger - pObj2->un.iInteger);
				break;
			case CAL_MUL:
				PushInteger(sms, pObj1->un.iInteger * pObj2->un.iInteger);
				break;
			case CAL_DIV:
				PushInteger(sms, pObj1->un.iInteger / pObj2->un.iInteger);
				break;
			case CAL_MOD:
				PushInteger(sms, pObj1->un.iInteger % pObj2->un.iInteger);
				break;
			case CAL_OR:
				PushInteger(sms, pObj1->un.iInteger | pObj2->un.iInteger);
				break;
			case CAL_AND:
				PushInteger(sms, pObj1->un.iInteger & pObj2->un.iInteger);
				break;
			case CAL_XOR:
				PushInteger(sms, pObj1->un.iInteger ^ pObj2->un.iInteger);
				break;
			}

			return SMS_OK;
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
				Pushboolean(sms, pObj1->un.iInteger | pObj2->un.iInteger);
				break;
			case CAL_AND:
				Pushboolean(sms, pObj1->un.iInteger & pObj2->un.iInteger);
				break;
			case CAL_XOR:
				Pushboolean(sms, pObj1->un.iInteger ^ pObj2->un.iInteger);
				break;
			default:
				OPSTACK_RESTORE(sms);
				return iError(sms, TYPECHECK, __func__, __LINE__, "calculation's");
				break;
			}
			return SMS_OK;
		}
		else
		{
			OPSTACK_RESTORE(sms);
			return iError(sms, TYPECHECK, __func__, __LINE__, "calculation's");
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
		PushReal(sms, v1 + v2);
		break;
	case CAL_SUB:
		PushReal(sms, v1 - v2);
		break;
	case CAL_MUL:
		PushReal(sms, v1 * v2);
		break;
	case CAL_DIV:
		PushReal(sms, v1 / v2);
		break;
	case CAL_MOD:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "mod");
	}

	return SMS_OK;
}

int __opr_add(SMS sms)
{
	return _calculate(sms, CAL_ADD);
}

int __opr_sub(SMS sms)
{
	return _calculate(sms, CAL_SUB);
}

int __opr_mul(SMS sms)
{
	return _calculate(sms, CAL_MUL);
}

int __opr_div(SMS sms)
{
	return _calculate(sms, CAL_DIV);
}

int __opr_mod(SMS sms)
{
	return _calculate(sms, CAL_MOD);
}

int __opr_or(SMS sms)
{
	return _calculate(sms, CAL_OR);
}

int __opr_xor(SMS sms)
{
	return _calculate(sms, CAL_XOR);
}

int __opr_and(SMS sms)
{
	return _calculate(sms, CAL_AND);
}

int __opr_sqrt(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 1)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "sqrt");

	SMS_OBJ *pObj = STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 1;

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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "sqrt");
	}

	PushReal(sms, value);

	return SMS_OK;
}

int __opr_debug(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "debug");

	SMS_OBJ Objboolean = *STACK_OBJ_LAST(sms, 0);
	if (Objboolean.ObjType != OTE_boolEAN)
		return iError(sms, TYPECHECK, __func__, __LINE__, "debug");

	((SMSD *)sms)->iOPStackUsing--;

	if (Objboolean.un.iInteger)
		((SMSD *)sms)->bDebugMessage = true;
	else
		((SMSD *)sms)->bDebugMessage = false;

	return SMS_OK;
}

int __opr_exec(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	return iExecute(sms);
}

int __opr_execfile(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	if (Obj.ObjType != OTE_STRING)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "execfile");
	}

	FILE *fp = fopen(GET_STRING((&Obj)), "r");
	if (fp == NULL)
	{
		return iError(sms, INVALIDDATA, __func__, __LINE__, "execfile", GET_STRING((&Obj)));
	}

	((SMSD *)sms)->iOPStackUsing--;

	FILE *fpIn = ((SMSD *)sms)->fpIn;
	bool bPrompt = ((SMSD *)sms)->bPrompt;

	((SMSD *)sms)->bPrompt = false;
	((SMSD *)sms)->fpIn = fp;
	int retv = SmsMainLoop(sms);
	((SMSD *)sms)->fpIn = fpIn;
	((SMSD *)sms)->bPrompt = bPrompt;

	fclose(fp);

	return (retv == SMS_EOF ? SMS_OK : retv);
}

int __opr_if(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Objboolean = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjProc = *STACK_OBJ_LAST(sms, 0);

	if (Objboolean.ObjType != OTE_boolEAN)
		return iError(sms, TYPECHECK, __func__, __LINE__, "if");

	if (ObjProc.ObjType != OTE_ARRAY || ObjProc.bExecutable == false)
		return iError(sms, TYPECHECK, __func__, __LINE__, "if");

	((SMSD *)sms)->iOPStackUsing -= 2;

	if (GET_INTEGER((&Objboolean)) == 1)
	{
		PushObj(sms, &ObjProc);
		return iExecute(sms);
	}

	return SMS_OK;
}

int __opr_ifelse(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Objboolean = *STACK_OBJ_LAST(sms, -2);
	SMS_OBJ ObjProctrue = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjProcfalse = *STACK_OBJ_LAST(sms, 0);

	if (Objboolean.ObjType != OTE_boolEAN)
		return iError(sms, TYPECHECK, __func__, __LINE__, "ifelse");

	if (ObjProctrue.ObjType != OTE_ARRAY || ObjProctrue.bExecutable == false)
		return iError(sms, TYPECHECK, __func__, __LINE__, "ifelse");

	if (ObjProcfalse.ObjType != OTE_ARRAY || ObjProcfalse.bExecutable == false)
		return iError(sms, TYPECHECK, __func__, __LINE__, "ifelse");

	((SMSD *)sms)->iOPStackUsing -= 3;

	if (GET_INTEGER((&Objboolean)) == 1)
	{
		PushObj(sms, &ObjProctrue);
	}
	else
	{
		PushObj(sms, &ObjProcfalse);
	}
	return iExecute(sms);
}

int __opr_exit(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	return SMS_EXIT;
}

int __opr_loop(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "loop");

	SMS_OBJ ObjProc = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 1;

	int retv = 1;

	do
	{
		PushObj(sms, &ObjProc);
		retv = iExecute(sms);
		if (retv < 1)
			break;
	} while (true);

	if (retv < 0)
		return retv;
	else
		return SMS_OK;
}

int __opr_repeat(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "repeat");

	SMS_OBJ ObjToNum = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjProc = *STACK_OBJ_LAST(sms, 0);

	if (ObjToNum.ObjType != OTE_INTEGER)
		return iError(sms, TYPECHECK, __func__, __LINE__, "repeat");

	if (ObjProc.ObjType != OTE_ARRAY)
		return iError(sms, TYPECHECK, __func__, __LINE__, "repeat");

	((SMSD *)sms)->iOPStackUsing -= 2;

	int retv = 1;

	for (int i = 0; i < ObjToNum.un.iInteger; i++)
	{
		PushObj(sms, &ObjProc);
		retv = iExecute(sms);
		if (retv < 1)
			break;
	}

	if (retv < 0)
		return retv;
	else
		return SMS_OK;
}

int __opr_for(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 4)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "for");

	SMS_OBJ ObjInitialValue = *STACK_OBJ_LAST(sms, -3);
	SMS_OBJ ObjStepValue = *STACK_OBJ_LAST(sms, -2);
	SMS_OBJ ObjLimitValue = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjProc = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 4;

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
				PushInteger(sms, i);
				PushObj(sms, &ObjProc);
				retv = iExecute(sms);
				if (retv < 1)
					break;
			}
		}
		else
		{
			for (int i = initialVal; i >= limitVal; i += stepVal)
			{
				PushInteger(sms, i);
				PushObj(sms, &ObjProc);
				retv = iExecute(sms);
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
				PushReal(sms, i);
				PushObj(sms, &ObjProc);
				retv = iExecute(sms);
				if (retv < 1)
					break;
			}
		}
		else
		{
			for (double i = initialVal; i >= limitVal; i += stepVal)
			{
				PushReal(sms, i);
				PushObj(sms, &ObjProc);
				retv = iExecute(sms);
				if (retv < 1)
					break;
			}
		}
	}
	else
	{
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "for");
	}

	if (retv < 0)
		return retv;
	else
		return SMS_OK;
}

int __opr_forall(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	SMS_OBJ ObjToVar = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjProc = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

	int retv = 1;

	switch (ObjToVar.ObjType)
	{
	case OTE_ARRAY:
	{
		SMS_OBJ *pObjInArray = ObjToVar.un.pVar->un.pObjInArray;
		for (int i = 0; i < ObjToVar.un.pVar->iLength; i++)
		{
			PushObj(sms, &pObjInArray[i]);
			PushObj(sms, &ObjProc);
			retv = iExecute(sms);
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
			PushInteger(sms, (long)pString[i]);
			PushObj(sms, &ObjProc);
			retv = iExecute(sms);
			if (retv < 1)
				break;
		}
		break;
	}
	case OTE_DICTIONARY:
	{
		SMS_DICT_ENTRY *pObjInDict = ObjToVar.un.pVar->un.pDictEntry;
		for (int i = 0; i < ObjToVar.un.pVar->iLength; i++)
		{
			PushObj(sms, &pObjInDict[i].ObjKey);
			PushObj(sms, &pObjInDict[i].ObjValue);
			PushObj(sms, &ObjProc);
			retv = iExecute(sms);
			if (retv < 1)
				break;
		}
		break;
	}
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "forall");
	}

	if (retv < 0)
		return retv;
	else
		return SMS_OK;
}

int __opr_pop(SMS sms)
{
	if (((SMSD *)sms)->iOPStackUsing < 2)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "pop");

	((SMSD *)sms)->iOPStackUsing -= 2;
	return SMS_OK;
}

int __opr_clear(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing = 0;
	return SMS_OK;
}

int __opr_cleartomark(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	do
	{
	} while (STACK_OBJ_FIRST(sms, ((SMSD *)sms)->iOPStackUsing-- - 1)->ObjType != OTE_MARK);

	return SMS_OK;
}

int __opr_exch(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "exch");

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);
	*STACK_OBJ_LAST(sms, 0) = *STACK_OBJ_LAST(sms, -1);
	*STACK_OBJ_LAST(sms, -1) = Obj;
	return SMS_OK;
}

int __opr_dup(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);
	PushObj(sms, &Obj);
	return SMS_OK;
}

int __opr_index(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	if (Obj.ObjType != OTE_INTEGER)
		return iError(sms, TYPECHECK, __func__, __LINE__, "index");

	if (((SMSD *)sms)->iOPStackUsing < Obj.un.iInteger)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "index");

	((SMSD *)sms)->iOPStackUsing -= 1;

	PushObj(sms, STACK_OBJ_LAST(sms, -1 * Obj.un.iInteger));

	return SMS_OK;
}

int __opr_roll(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ ObjStep = *STACK_OBJ_LAST(sms, 0);
	SMS_OBJ ObjNum = *STACK_OBJ_LAST(sms, -1);

	int iNum, iStep;
	if (bGetInteger(sms, &ObjNum, &iNum, "roll") == false)
		return false;
	if (bGetInteger(sms, &ObjStep, &iStep, "roll") == false)
		return false;
	if (((SMSD *)sms)->iOPStackUsing < 2 + iNum)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "roll");

	if (iNum > 32)
		return iError(sms, RANGECHECK, __func__, __LINE__, "roll");

	((SMSD *)sms)->iOPStackUsing -= 2;

	SMS_OBJ WorkStack[32];
	SMS_OBJ *pStackTargetBottom = STACK_OBJ_LAST(sms, -iNum + 1);
	memcpy(WorkStack, pStackTargetBottom, sizeof(SMS_OBJ) * iNum);

	iStep = iStep % iNum;

	if (iStep > 0)
	{
		memcpy(&pStackTargetBottom[iStep], &WorkStack[0], sizeof(SMS_OBJ) * (iNum - abs(iStep)));
		memcpy(pStackTargetBottom, &WorkStack[iNum - abs(iStep)], sizeof(SMS_OBJ) * abs(iStep));
	}
	else
	{
		memcpy(pStackTargetBottom, &WorkStack[-iStep], sizeof(SMS_OBJ) * (iNum + iStep));
		memcpy(&pStackTargetBottom[iNum + iStep], &WorkStack[0], sizeof(SMS_OBJ) * -iStep);
	}

	return SMS_OK;
}

int __opr_def(SMS sms)
{

	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "def");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	if (Obj1.ObjType != OTE_NAME)
		return iError(sms, TYPECHECK, __func__, __LINE__, "def");

	((SMSD *)sms)->iOPStackUsing -= 2;

	int retv = (bRegistObjToDict(sms, CURRENTDICT(sms), &Obj1, &Obj2) == true ? 1 : 0);

	return retv;
}

int __opr_dict(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "dict");
	}

	SMS_OBJ ObjInt = *STACK_OBJ_LAST(sms, 0);
	if (ObjInt.ObjType != OTE_INTEGER)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "dict");
	}

	SMS_OBJ *pObjDict = STACK_OBJ_LAST(sms, 0); /* Put an obj on the mark obj. */
	pObjDict->bExecutable = true;
	pObjDict->ObjType = OTE_DICTIONARY;
	int iValue = (int)GET_INTEGER((&ObjInt));
	pObjDict->un.pVar = pVarCreate(sms, VTE_DICTIONARY, &iValue);

	return SMS_OK;
}

int __opr_currentdict(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ *pObjDict = STACK_OBJ_NEXT(sms);
	*pObjDict = ((SMSD *)sms)->DictStack[((SMSD *)sms)->iDictStackUsing - 1];

	((SMSD *)sms)->iOPStackUsing++;

	return SMS_OK;
}

int __opr_begin(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "begin");
	}
	SMS_OBJ ObjDict = *STACK_OBJ_LAST(sms, 0);
	if (ObjDict.ObjType != OTE_DICTIONARY)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "begin");
	}
	((SMSD *)sms)->iOPStackUsing--;
	PushDict(sms, &ObjDict);

	return SMS_OK;
}

int __opr_end(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	if (((SMSD *)sms)->iDictStackUsing < 2)
	{
		return iError(sms, DICTSTACKUNDERFLOW, __func__, __LINE__, "end");
	}
	PopDict(sms);
	return SMS_OK;
}

int __opr_prompt(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	((SMSD *)sms)->bPrompt = true;
	return SMS_OK;
}

int __opr_true(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	Pushboolean(sms, 1);

	return SMS_OK;
}

int __opr_null(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	PushNull(sms);

	return SMS_OK;
}

int __opr_false(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	Pushboolean(sms, 0);

	return SMS_OK;
}

int __opr_length(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);
	switch (Obj.ObjType)
	{
	case OTE_NAME:
	case OTE_STRING:
	case OTE_ARRAY:
	case OTE_DICTIONARY:
		break;
	default:
		return iError(sms, TYPECHECK, __func__, __LINE__, "length");
	}

	((SMSD *)sms)->iOPStackUsing--;

	PushInteger(sms, Obj.un.pVar->iLength);

	return SMS_OK;
}

int __opr_aload(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ ObjArray = *STACK_OBJ_LAST(sms, 0);
	switch (ObjArray.ObjType)
	{
	case OTE_ARRAY:
		break;
	default:
		return iError(sms, TYPECHECK, __func__, __LINE__, "aload");
	}

	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ *pObjInArray = GET_OBJINARRAY(&ObjArray);

	for (int i = 0; i < ObjArray.un.pVar->iLength; i++)
	{
		PushObj(sms, &pObjInArray[i]);
	}

	PushObj(sms, &ObjArray);

	return SMS_OK;
}

int __opr_load(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);
	switch (Obj.ObjType)
	{
	case OTE_NAME:
	case OTE_STRING:
		break;
	default:
		return iError(sms, TYPECHECK, __func__, __LINE__, "load");
	}

	((SMSD *)sms)->iOPStackUsing--;

	PushObj(sms, pObjSearchInDicts(sms, &Obj));

	return SMS_OK;
}

int __opr_maxlength(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);
	switch (Obj.ObjType)
	{
	case OTE_NAME:
	case OTE_STRING:
	case OTE_ARRAY:
	case OTE_DICTIONARY:
		break;
	default:
		return iError(sms, TYPECHECK, __func__, __LINE__, "maxlength");
	}

	((SMSD *)sms)->iOPStackUsing--;

	PushInteger(sms, Obj.un.pVar->iBufferLength);

	return SMS_OK;
}

int __opr_checkmem(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	CheckMemory((char *)"_opr_checkmem");

	return true;
}

int __opr_enummem(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	EnumMemory(sms);

	return SMS_OK;
}

int __opr_mark(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	PushMark(sms, false);
	return SMS_OK;
}

int __opr_getinterval(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 3)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "getinterval");
	}

	SMS_OBJ ObjAny = *STACK_OBJ_LAST(sms, -2);
	SMS_OBJ ObjIndex = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjCount = *STACK_OBJ_LAST(sms, 0);

	if (ObjIndex.ObjType != OTE_INTEGER)
		return iError(sms, TYPECHECK, __func__, __LINE__, "getinterval");

	if (ObjCount.ObjType != OTE_INTEGER)
		return iError(sms, TYPECHECK, __func__, __LINE__, "getinterval");

	if (ObjAny.un.pVar->iLength < ObjIndex.un.iInteger + ObjCount.un.iInteger)
		return iError(sms, RANGECHECK, __func__, __LINE__, "getinterval");

	((SMSD *)sms)->iOPStackUsing -= 3;

	if (ObjAny.ObjType == OTE_STRING)
	{
		char buf[1024];
		memcpy(buf, GET_STRING(&ObjAny) + GET_INTEGER(&ObjIndex), GET_INTEGER(&ObjCount));
		buf[GET_INTEGER(&ObjCount)] = 0x00;
		PushString(sms, buf);
	}
	else if (ObjAny.ObjType == OTE_ARRAY)
	{
		PushMark(sms, false);

		SMS_OBJ *pObjInArray = GET_OBJINARRAY(&ObjAny);
		for (int i = GET_INTEGER(&ObjIndex); i < GET_INTEGER(&ObjIndex) + GET_INTEGER(&ObjCount); i++)
			PushObj(sms, &pObjInArray[i]);

		iCloseArray(sms);
	}
	else
	{
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "getinterval");
	}

	return SMS_OK;
}

int __opr_putinterval(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 3)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "putinterval");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -2);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj3 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 3;

	switch (Obj1.ObjType)
	{
	case OTE_ARRAY:
	{
		if (Obj2.ObjType != OTE_INTEGER || Obj3.ObjType != OTE_ARRAY)
			return iError(sms, TYPECHECK, __func__, __LINE__, "putinterval");
		SMS_OBJ *pObjTar = GET_OBJINARRAY(&Obj1);
		SMS_OBJ *pObjSrc = GET_OBJINARRAY(&Obj3);
		for (int t = GET_INTEGER(&Obj2), s = 0; s < Obj3.un.pVar->iLength; s++, t++)
			pObjTar[t] = pObjSrc[s];
		break;
	}
	case OTE_STRING:
	{
		if (Obj2.ObjType != OTE_INTEGER || Obj3.ObjType != OTE_STRING)
			return iError(sms, TYPECHECK, __func__, __LINE__, "putinterval");
		char *pStrTar = GET_STRING(&Obj1);
		char *pStrSrc = GET_STRING(&Obj3);
		for (int t = GET_INTEGER(&Obj2), s = 0; s < Obj3.un.pVar->iLength; s++, t++)
			pStrTar[t] = pStrSrc[s];
		break;
	}
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "putinterval");
	}

	return SMS_OK;
}

int __opr_get(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "get");
	}

	SMS_OBJ ObjAny = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjKey = *STACK_OBJ_LAST(sms, 0);

	switch (ObjAny.ObjType)
	{
	case OTE_STRING:
	{
		char *ptr = GET_STRING(&ObjAny);
		if (ObjKey.ObjType != OTE_INTEGER)
			return iError(sms, TYPECHECK, __func__, __LINE__, "get");
		if (ObjAny.un.pVar->iLength - 1 < GET_INTEGER(&ObjKey))
			return iError(sms, RANGECHECK, __func__, __LINE__, "get");
		((SMSD *)sms)->iOPStackUsing -= 2;
		PushInteger(sms, ptr[GET_INTEGER(&ObjKey)]);
		break;
	}
	case OTE_ARRAY:
	{
		SMS_OBJ *pObjInArray = GET_OBJINARRAY(&ObjAny);
		if (ObjKey.ObjType != OTE_INTEGER)
			return iError(sms, TYPECHECK, __func__, __LINE__, "get");
		if (ObjAny.un.pVar->iLength - 1 < GET_INTEGER(&ObjKey))
			return iError(sms, RANGECHECK, __func__, __LINE__, "get");
		((SMSD *)sms)->iOPStackUsing -= 2;
		PushObj(sms, &pObjInArray[GET_INTEGER(&ObjKey)]);
		break;
	}
	case OTE_DICTIONARY:
	{
		if (ObjKey.ObjType != OTE_NAME)
			return iError(sms, TYPECHECK, __func__, __LINE__, "get");
		SMS_OBJ *pObj = pObjSearchInTheDict(sms, &ObjAny, &ObjKey);
		if (pObj == NULL)
			return iError(sms, UNDEFINED, __func__, __LINE__, "ObjKey.un.pVar->un.pString");
		((SMSD *)sms)->iOPStackUsing -= 2;
		PushObj(sms, pObj);
		break;
	}
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "get");
	}

	return SMS_OK;
}

int __opr_time(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

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

	PushString(sms, buf);

	return SMS_OK;
}

int __opr_print(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "print");
	}

	SMS_fprintf(SMS_stdout, "%s", GET_STRING(STACK_OBJ_LAST(sms, 0)));

	((SMSD *)sms)->iOPStackUsing--;

	return SMS_OK;
}

int __opr_cvs(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "cvs");
	}
	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);
	((SMSD *)sms)->iOPStackUsing--;

	switch (Obj.ObjType)
	{
	case OTE_INTEGER:
	{
		char buf[128];
		sprintf(buf, "%ld", Obj.un.iInteger);
		PushString(sms, buf);
		break;
	}
	case OTE_REAL:
	{
		char buf[128];
		sprintf(buf, "%lf", Obj.un.dReal);
		PushString(sms, buf);
		break;
	}
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "cvs");
	}

	return SMS_OK;
}

int __opr_cvx(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "cvx");
	}

	SMS_OBJ *pObj = STACK_OBJ_LAST(sms, 0);
	pObj->bExecutable = true;

	return SMS_OK;
}

int __opr_cvn(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "cvn");
	}

	SMS_OBJ *pObj = STACK_OBJ_LAST(sms, 0);
	pObj->bExecutable = false;
	pObj->ObjType = OTE_NAME;

	return SMS_OK;
}

int __opr_concat(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "concat");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	if (Obj1.ObjType != Obj2.ObjType || Obj1.ObjType != OTE_STRING)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "concat");
	}

	((SMSD *)sms)->iOPStackUsing -= 2;

	char tmp[PATH_MAX * 2];
	sprintf(tmp, "%s%s", Obj1.un.pVar->un.pString, Obj2.un.pVar->un.pString);
	PushString(sms, tmp);

	return SMS_OK;
}

int __opr_cvi(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "cvi");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	switch (Obj.ObjType)
	{
	case OTE_REAL:
		((SMSD *)sms)->iOPStackUsing -= 1;
		PushInteger(sms, (long)Obj.un.dReal);
		break;
	case OTE_INTEGER:
		break;
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "cvi");
	}
	return SMS_OK;
}

int __opr_cvr(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "cvr");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	switch (Obj.ObjType)
	{
	case OTE_REAL:
		break;
	case OTE_INTEGER:
		((SMSD *)sms)->iOPStackUsing -= 1;
		PushReal(sms, (double)Obj.un.iInteger);
		break;
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "cvr");
	}

	return SMS_OK;
}

int __opr_round(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "round");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	if (Obj.ObjType != OTE_REAL)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "round");
	}

	((SMSD *)sms)->iOPStackUsing -= 1;

	PushReal(sms, round(Obj.un.dReal));

	return SMS_OK;
}

int __opr_truncate(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "truncate");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	if (Obj.ObjType != OTE_REAL)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "truncate");
	}

	((SMSD *)sms)->iOPStackUsing -= 1;

	PushReal(sms, trunc(Obj.un.dReal));

	return SMS_OK;
}

int __opr_search(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "search");
	}

	SMS_OBJ ObjString = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjSeek = *STACK_OBJ_LAST(sms, 0);

	if (ObjString.ObjType != ObjSeek.ObjType || ObjString.ObjType != OTE_STRING)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "search");
	}

	char tmp[1024];
	memcpy(tmp, ObjString.un.pVar->un.pString, strlen(ObjString.un.pVar->un.pString) + 1);

	char *ptr = (char *)strstr(tmp, ObjSeek.un.pVar->un.pString);
	if (ptr)
	{
		((SMSD *)sms)->iOPStackUsing -= 2;
		PushString(sms, ptr + ObjSeek.un.pVar->iLength);
		PushString(sms, ObjSeek.un.pVar->un.pString);
		*ptr = 0x00;
		PushString(sms, tmp);
		Pushboolean(sms, 1);
	}
	else
	{
		((SMSD *)sms)->iOPStackUsing--;
		Pushboolean(sms, 0);
	}

	return SMS_OK;
}

FILE *SMS_stdout;

int __opr_stdout(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "stdout");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 1;

	switch (Obj.ObjType)
	{
	case OTE_STRING:
		SMS_stdout = fopen(Obj.un.pVar->un.pString, "w");
		break;
	case OTE_NULL:
		if (SMS_stdout != stdout)
		{
			fclose(SMS_stdout);
		}
		SMS_stdout = stdout;
		break;
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "stdout");
		break;
	}

	return SMS_OK;
}

int __opr_put(SMS sms)
{
	// array index any put -
	// string index int put -
	// dict name any put -
	// appdata name any put -
	// devmode name any put -

	((SMSD *)sms)->iOPStackUsing--;

	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 3)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "put");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -2);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj3 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 3;

	switch (Obj1.ObjType)
	{
	case OTE_ARRAY:
	{
		if (Obj2.ObjType != OTE_INTEGER)
		{
			OPSTACK_RESTORE(sms);
			return iError(sms, TYPECHECK, __func__, __LINE__, "put");
		}
		SMS_OBJ *pObjInArray = GET_OBJINARRAY(&Obj1);
		pObjInArray[Obj2.un.iInteger] = Obj3;
		break;
	}
	case OTE_STRING:
	{
		if (Obj2.ObjType != OTE_INTEGER)
		{
			OPSTACK_RESTORE(sms);
			return iError(sms, TYPECHECK, __func__, __LINE__, "put");
		}
		char *ptr = GET_STRING(&Obj1);
		if (Obj3.ObjType != OTE_INTEGER)
		{
			OPSTACK_RESTORE(sms);
			return iError(sms, TYPECHECK, __func__, __LINE__, "put");
		}
		ptr[Obj2.un.iInteger] = (char)Obj3.un.iInteger;
		break;
	}
	case OTE_DICTIONARY:
	{
		if (Obj2.ObjType != OTE_NAME)
		{
			OPSTACK_RESTORE(sms);
			return iError(sms, TYPECHECK, __func__, __LINE__, "put");
		}
		bRegistObjToDict(sms, &Obj1, &Obj2, &Obj3);
		break;
	}
	}

	return SMS_OK;
}

int __opr_gt(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "gt");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "gt");
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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "gt");
		break;
	}

	Pushboolean(sms, n1 > n2 ? true : false);

	return SMS_OK;
}

int __opr_ge(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "ge");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "ge");
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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "ge");
		break;
	}

	Pushboolean(sms, n1 >= n2 ? true : false);

	return SMS_OK;
}

int __opr_lt(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "lt");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "lt");
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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "lt");
		break;
	}

	Pushboolean(sms, n1 < n2 ? true : false);

	return SMS_OK;
}

int __opr_le(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "le");
	}

	SMS_OBJ Obj1 = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ Obj2 = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 2;

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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "le");
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
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "le");
		break;
	}

	Pushboolean(sms, n1 <= n2 ? true : false);

	return SMS_OK;
}

int __opr_ceil(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;
	OPSTACK_SAVE(sms);

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "ceil");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	((SMSD *)sms)->iOPStackUsing -= 1;

	switch (Obj.ObjType)
	{
	case OTE_INTEGER:
		PushInteger(sms, GET_INTEGER(&Obj));
		break;
	case OTE_REAL:
		PushInteger(sms, ceil(GET_REAL(&Obj)));
		break;
	default:
		OPSTACK_RESTORE(sms);
		return iError(sms, TYPECHECK, __func__, __LINE__, "ceil");
		break;
	}

	return SMS_OK;
}

int __opr_array(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "array");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	if (Obj.ObjType != OTE_INTEGER)
		return iError(sms, TYPECHECK, __func__, __LINE__, "array");

	int iLengthArray = GET_INTEGER(&Obj);

	SMS_OBJ *pObjArray = STACK_OBJ_LAST(sms, 0);
	pObjArray->bExecutable = false;
	pObjArray->ObjType = OTE_ARRAY;
	pObjArray->un.pVar = pVarCreate(sms, VTE_ARRAY, &iLengthArray);

	SMS_OBJ *pObjInArray = pObjArray->un.pVar->un.pObjInArray;
	for (int i = 0; i < iLengthArray; i++)
	{
		pObjInArray[i].ObjType = OTE_NULL;
	}
	pObjArray->un.pVar->iLength = iLengthArray;

	return SMS_OK;
}

int __opr_copy(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 2)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "copy");
	}

	SMS_OBJ SrcObj = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ DstObj = *STACK_OBJ_LAST(sms, 0);

	if (SrcObj.ObjType != OTE_ARRAY || DstObj.ObjType != OTE_ARRAY)
		return iError(sms, TYPECHECK, __func__, __LINE__, "copy");

	if (SrcObj.un.pVar->iLength > DstObj.un.pVar->iLength)
		return iError(sms, RANGECHECK, __func__, __LINE__, "copy");

	((SMSD *)sms)->iOPStackUsing -= 2;

	SMS_OBJ *pSrcObjInArray = GET_OBJINARRAY(&SrcObj);
	SMS_OBJ *pDstObjInArray = GET_OBJINARRAY(&DstObj);

	for (int i = 0; i < SrcObj.un.pVar->iLength; i++)
	{
		pDstObjInArray[i] = pSrcObjInArray[i];
	}

	PushObj(sms, &DstObj);

	return SMS_OK;
}

void ShowDictOperators(SMS sms);

int __opr_help(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		ShowDictOperators(sms);
	}
	else
	{
		SMS_OBJ ObjKey = *STACK_OBJ_LAST(sms, 0);
		if (ObjKey.ObjType != OTE_NAME || ObjKey.bExecutable == true)
			return iError(sms, TYPECHECK, __func__, __LINE__, "help");

		SMS_OBJ *pObjValue = pObjSearchInDicts(sms, &ObjKey);
		if (pObjValue == NULL)
			return iError(sms, UNDEFINED, __func__, __LINE__, ObjKey.un.pVar->un.pString);

		if (pObjValue->ObjType != OTE_OPERATOR)
			return iError(sms, TYPECHECK, __func__, __LINE__, "help");

		((SMSD *)sms)->iOPStackUsing--;

		SmsHelp(sms, pObjValue->un.pOperator->pKeyName, pObjValue->un.pOperator->pOperatorParams, pObjValue->un.pOperator->pOperatorSpec);
	}
	return SMS_OK;
}

int __opr_pwd(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	char buf[PATH_MAX];
	getcwd(buf, PATH_MAX);
	PushString(sms, buf);
	return SMS_OK;
}

int __opr_cd(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	if (((SMSD *)sms)->iOPStackUsing < 1)
	{
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "cd");
	}

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	if (Obj.ObjType != OTE_STRING)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "cd");
	}

	((SMSD *)sms)->iOPStackUsing -= 1;

	chdir(GET_STRING(&Obj));
	char pathname[PATH_MAX];
	getcwd(pathname, PATH_MAX);
	PushString(sms, pathname);

	return SMS_OK;
}

int __opr_enumfiles(SMS sms)
{
	int retv = SMS_OK;

	SMSD *smsd = (SMSD *)sms;

	smsd->iOPStackUsing--; /* pop OPERATOR */

	if (smsd->iOPStackUsing < 2)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "enumfiles");

	SMS_OBJ Pattern = *STACK_OBJ_LAST(sms, -1);
	if (Pattern.ObjType != OTE_STRING)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "enumfiles");
	}

	SMS_OBJ ObjProc = *STACK_OBJ_LAST(sms, 0);
	if (ObjProc.ObjType != OTE_ARRAY || ObjProc.bExecutable != true)
	{
		return iError(sms, TYPECHECK, __func__, __LINE__, "enumfiles");
	}

	smsd->iOPStackUsing -= 2;

	char folderPath[PATH_MAX] = {};

	const char *pattern = GET_STRING(&Pattern);
	glob_t glob_result;

	if (glob(pattern, GLOB_TILDE, nullptr, &glob_result) == 0)
	{
		// 結果を列挙
		for (size_t i = 0; i < glob_result.gl_pathc; ++i)
		{
			PushString(sms, glob_result.gl_pathv[i]);
			PushObj(sms, &ObjProc);
			retv = iExecute(sms);
			if (retv < 1)
				break;
		}
	}
	// メモリを解放
	globfree(&glob_result);

	return retv;
}

int __opr_getenv(SMS sms)
{
	((SMSD *)sms)->iOPStackUsing--;

	SMS_OBJ ObjString = *STACK_OBJ_LAST(sms, 0);

	if (ObjString.ObjType != OTE_STRING)
		return iError(sms, TYPECHECK, __func__, __LINE__, "getenv");

	((SMSD *)sms)->iOPStackUsing -= 1;

	char *env = getenv(GET_STRING(&ObjString));
	if (env)
	{
		PushString(sms, env);
		Pushboolean(sms, 1);
	}
	else
	{
		PushString(sms, (char *)"");
		Pushboolean(sms, 0);
	}

	return SMS_OK;
}

int __opr_error(SMS sms)
{
	SMSD *smsd = (SMSD *)sms;

	smsd->iOPStackUsing--; /* pop OPERATOR */

	SMS_OBJ Obj = *STACK_OBJ_LAST(sms, 0);

	smsd->iOPStackUsing -= 1;

	return iError(sms, (ERRORTYPE)Obj.un.iInteger, __func__, __LINE__, "sample implementation");
}

int __opr_messagebox(SMS sms)
{
	SMSD *smsd = (SMSD *)sms;

	smsd->iOPStackUsing--; /* pop OPERATOR */

	if (smsd->iOPStackUsing < 2)
		return iError(sms, STACKUNDERFLOW, __func__, __LINE__, "messagebox");

	SMS_OBJ ObjTitle = *STACK_OBJ_LAST(sms, -1);
	SMS_OBJ ObjMessage = *STACK_OBJ_LAST(sms, 0);

	if (ObjTitle.ObjType != OTE_STRING || ObjMessage.ObjType != OTE_STRING)
		return iError(sms, TYPECHECK, __func__, __LINE__, "messagebox");

	char *title = GET_STRING(&ObjTitle);
	char *msg = GET_STRING(&ObjMessage);

	char command[PATH_MAX];
	sprintf(command, "LIBGL_ALWAYS_SOFTWARE=1 zenity --question --title=\"%s\" --text=\"%s\"", title, msg);

	int retv = system(command);

	smsd->iOPStackUsing -= 2;

	if (retv == 0)
		Pushboolean(sms, 1);
	else
		Pushboolean(sms, 0);

	return SMS_OK;
}

/***************************************************
				  OPERATORS TABLE
****************************************************/
static const SMS_OPERATOR _operators[] =
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
		{"debug", __opr_debug, "<bool> debug -", "Make sms debugmode"},
		{"enummem", __opr_enummem, "- enummem -", "Enumrate memorystatus"},
		{"checkmem", __opr_checkmem, "- checkmem -", "Check memory status"},

		{"cd", __opr_cd, "<string> cd -", "Change the current directory sms refers"},
		{"pwd", __opr_pwd, "- pwd <string>", "Return current directory sms refers"},
		{"enumfiles", __opr_enumfiles, "<string/pathAndWildcard> <proc> enumfiles <string/path> <string/filename1> <string/path> <string/filename2> ... ", "Return <string/path> <string/filename> searched with <pathAndWildcard> and execute <proc>"},
		{"getenv", __opr_getenv, "<string> getenv <string> <bool>", "Return an env value"},
		{"prompt", __opr_prompt, "- prompt -", "Show 'SMS>'"},
		{"quit", __opr_quit, "- quit -", "Quit from sms"},
		{"messagebox", __opr_messagebox, "<string#1> <string#2> messagebox --true/false--", "shows a messagebox having the tile string#1, the message string#2 and Yes No bottoms, and returs true for yes, false for no."},

		{NULL, NULL, "", NULL}};

/***************************************************
			  REGIST OPERATORS TO DICT
****************************************************/
void InitDictOperators(SMS sms)
{
	SMS_VAR *pVar = ((SMSD *)sms)->DictStack[0].un.pVar;

	for (int i = 0; _operators[i].pKeyName != NULL; i++)
	{
		AddOperatorToDict(sms, (char *)_operators[i].pKeyName, &_operators[i]);
	}
}

void ShowDictOperators(SMS sms)
{
	for (int i = 0; _operators[i].pKeyName != NULL; i++)
	{
		SmsHelp(sms, (char *)_operators[i].pKeyName, (char *)_operators[i].pOperatorParams, (char *)_operators[i].pOperatorSpec);
	}
}