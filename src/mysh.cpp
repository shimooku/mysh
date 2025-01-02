#include "mysh.h"

#define MAX_PRINTF_STRING 2048

typedef struct MYS_MEMORY_HEADER
{
	struct MYS_MEMORY_HEADER *pNext;
	struct MYS_MEMORY_HEADER *pBefore;
	size_t iSize;
	char info[256];
	size_t dummy;
} MYS_MEMORY_HEADER;

static size_t _sz_MemorySize = 0;
static MYS_MEMORY_HEADER *p_MemoryLinkFirst = NULL;
static MYS_MEMORY_HEADER *p_MemoryLinkLast = NULL;

#define CHECKFLAG (size_t)0x12345678

void CheckMemory(char *title)
{
	MYS_MEMORY_HEADER *ptr = p_MemoryLinkFirst;
	for (; ptr != NULL; ptr = ptr->pNext)
	{
		if (ptr->dummy != CHECKFLAG)
		{
			if (title)
				fprintf(MYS_stdout, "(%s) %s\n", title, ptr->info);
			fprintf(MYS_stdout, "*********** Memory Check Error (HEADER PART) ***********\n");
		}
		char *checkarea = (char *)ptr + ptr->iSize + sizeof(MYS_MEMORY_HEADER);
		if (*(size_t *)checkarea != CHECKFLAG)
		{
			if (title)
				fprintf(MYS_stdout, "(%s) %s\n", title, ptr->info);
			fprintf(MYS_stdout, "*********** Memory Check Error (FOOTER PART) ***********\n");
		}
	}
}

void EnumMemory(MYS mys)
{
	MYS_MEMORY_HEADER *ptr = p_MemoryLinkFirst;
	for (; ptr != NULL; ptr = ptr->pNext)
	{
		MysMessage(mys, "MYS EnumMemory: %s Size: %zd byte", ptr->info, ptr->iSize);
	}
}

void *MemoryAlloc(size_t size, const char *name, int line)
{
	CheckMemory((char *)"MemoryAlloc");

	_sz_MemorySize += size;

	MYS_MEMORY_HEADER *ptr = (MYS_MEMORY_HEADER *)malloc(size + sizeof(MYS_MEMORY_HEADER) + sizeof(size_t));
	if (ptr == NULL)
		goto EXIT;

	memset(ptr, 0, size + sizeof(MYS_MEMORY_HEADER) + sizeof(size_t));

	ptr->dummy = CHECKFLAG;

	if (p_MemoryLinkLast == NULL)
	{
		p_MemoryLinkLast = ptr;
		p_MemoryLinkFirst = ptr;
		ptr->pBefore = NULL;
		ptr->pNext = NULL;
	}
	else
	{
		p_MemoryLinkLast->pNext = ptr;
		ptr->pBefore = p_MemoryLinkLast;
		ptr->pNext = NULL;
		p_MemoryLinkLast = ptr;
	}

	sprintf(ptr->info, "%s(L%d)", name, line);

	ptr->iSize = size;

	*(size_t *)((char *)ptr + sizeof(MYS_MEMORY_HEADER) + size) = CHECKFLAG;

EXIT:
	if (ptr)
		return (void *)((char *)ptr + sizeof(MYS_MEMORY_HEADER));
	else
		return NULL;
}

void *MemoryRealloc(void *pMem, size_t size)
{
	MYS_MEMORY_HEADER *top = (MYS_MEMORY_HEADER *)((char *)pMem - sizeof(MYS_MEMORY_HEADER));

	size_t original_size = top->iSize;

	MYS_MEMORY_HEADER *new_top = (MYS_MEMORY_HEADER *)realloc(top, size + sizeof(MYS_MEMORY_HEADER) + sizeof(size_t));

	new_top->iSize = size;

	_sz_MemorySize -= original_size;
	_sz_MemorySize += size;

	*(size_t *)((char *)new_top + sizeof(MYS_MEMORY_HEADER) + size) = CHECKFLAG;

	return (void *)((char *)new_top + sizeof(MYS_MEMORY_HEADER));
}

void MemoryFree(void *pMem)
{
	CheckMemory((char *)"MemoryFree");

	MYS_MEMORY_HEADER *ptr = (MYS_MEMORY_HEADER *)((char *)pMem - sizeof(MYS_MEMORY_HEADER));

	if (ptr->pNext == NULL)
	{
		p_MemoryLinkLast = ptr->pBefore;
		if (ptr->pBefore)
			ptr->pBefore->pNext = NULL;
		else
			p_MemoryLinkFirst = NULL;
	}
	else if (ptr->pBefore == NULL)
	{
		p_MemoryLinkFirst = ptr->pNext;
		if (ptr->pNext)
			ptr->pNext->pBefore = NULL;
		else
			p_MemoryLinkLast = NULL;
	}
	else
	{
		ptr->pBefore->pNext = ptr->pNext;
		ptr->pNext->pBefore = ptr->pBefore;
	}

	_sz_MemorySize -= ptr->iSize;

	free(ptr);
}

int iMysParseString(MYS mys, char *pBuffer)
{
	char *cp_ptr = pBuffer;
	char *utf8_ptr = nullptr;
	int retv = 1;

	do
	{
		cp_ptr = pTokenFromString(mys, cp_ptr);

		if (bTokenCompleted(TOKEN(mys)) == false)
		{
			fgets(pBuffer, 512, ((MYSD *)mys)->fpIn);
			cp_ptr = pBuffer;
		}
		else if (uTokenLength(TOKEN(mys)))
		{
			PushToken(mys, pTokenBuffer(TOKEN(mys)));
			TokenReset(TOKEN(mys));
			retv = iExecute(mys);
			if (retv < 1)
				break;
		}
	} while (*cp_ptr);

	return retv;
}

int MysMainLoop(MYS mys)
{
	MYSD *mysd = (MYSD *)mys;
	char c_Buffer[MYS_MAX_STRING];
	int retv = 1;

	mysd->iNestMainLoop++;

	do
	{
		if (((MYSD *)mys)->bPrompt)
			printf("msh> ");
		memset(c_Buffer, 0x00, MYS_MAX_STRING);
		if (fgets(c_Buffer, 512, mysd->fpIn) != NULL /*&& setjmp(gc_Env) == false*/)
		{
			if (((MYSD *)mys)->bDebugMessage)
				MYS_fprintf(MYS_stdout, "\t\t\t\t\t\t\tDBG [%d] %s", STACK_LEVEL(mys), c_Buffer);

			retv = iMysParseString(mys, c_Buffer);

			if (((MYSD *)mys)->bPrompt)
			{
				if (retv != MYS_QUIT)
					retv = MYS_OK;
			}
			else if (retv == 0)
			{
				retv = MYS_OK;
			}
		}
		else
		{
			retv = MYS_EOF;
		}
	} while (retv > 0);

	mysd->iNestMainLoop--;

	return retv;
}

MYS pMysInitialize()
{
	MYSD *con = (MYSD *)MemoryAlloc(sizeof(MYSD), "pMysInitialize", __LINE__);
	memset(con, 0, sizeof(MYSD));

	con->iDictStackUsing = 1;
	con->DictStack[0].ObjType = OTE_DICTIONARY;
	con->DictStack[0].bExecutable = false;
	int iLength = 1024;
	con->DictStack[0].un.pVar = pVarCreate(con, VTE_DICTIONARY, (void *)&iLength);

	con->pToken = pTokenInitialize();
	con->iNestExecArray = 0;
	con->bPrompt = false;

	con->fpIn = stdin;

	MYS_stdout = stdout;

	con->iNestMainLoop = 0;

	InitDictOperators(con);

	return (MYS)con;
}

void MysTerminate(MYS mys)
{
	MYSD *mysd = (MYSD *)mys;
	TokenTerminate(mysd->pToken);
	VarClean(mys);
	MemoryFree(mys);
	EnumMemory(mys);
}

/*
	For VTE_ARRAY and VTE_DICTIONARY, the function doesn't put any value.
	You have to use PutVarEntry function for putting values.
*/
MYS_VAR *pVarCreate(MYS mys, VAR_TYPE_ENUM VarType, void *vp_Value)
{
	((MYSD *)mys)->iVarSeqNo++;

	MYS_VAR *pVar = (MYS_VAR *)MemoryAlloc(sizeof(MYS_VAR), "pVarCreate", __LINE__);
	memset(pVar, 0, sizeof(MYS_VAR));

	pVar->iSeqNo = ((MYSD *)mys)->iVarSeqNo;

	pVar->VarType = VarType;

	if (((MYSD *)mys)->pVarLast == NULL)
	{
		((MYSD *)mys)->pVarLast = pVar;
		((MYSD *)mys)->pVarFirst = pVar;
		pVar->pVarBefore = NULL;
		pVar->pVarNext = NULL;
	}
	else
	{
		((MYSD *)mys)->pVarLast->pVarNext = pVar;
		pVar->pVarBefore = ((MYSD *)mys)->pVarLast;
		pVar->pVarNext = NULL;
		((MYSD *)mys)->pVarLast = pVar;
	}

	switch (VarType)
	{
	case VTE_NAME:
		pVar->iLength = (int)strlen((char *)vp_Value);
		pVar->iBufferLength = pVar->iLength + 1;
		pVar->un.pName = (char *)MemoryAlloc(pVar->iBufferLength, "pVarCreate", __LINE__);
		memcpy(pVar->un.pName, vp_Value, pVar->iLength + 1);
		break;
	case VTE_STRING:
		pVar->iLength = (int)strlen((char *)vp_Value);
		pVar->iBufferLength = pVar->iLength + 1;
		pVar->un.pString = (char *)MemoryAlloc(pVar->iBufferLength, "pVarCreate", __LINE__);
		memcpy(pVar->un.pString, vp_Value, pVar->iLength + 1);
		break;
	case VTE_ARRAY:
		pVar->iLength = 0;
		pVar->iBufferLength = 10;
		if (vp_Value)
			pVar->iBufferLength = *(int *)vp_Value;
		pVar->un.pObjInArray = (MYS_OBJ *)MemoryAlloc(pVar->iBufferLength * sizeof(MYS_OBJ), "pVarCreate", __LINE__);
		memset(pVar->un.pObjInArray, 0, pVar->iBufferLength * sizeof(MYS_OBJ));
		break;
	case VTE_DICTIONARY:
		pVar->iLength = 0;
		pVar->iBufferLength = 64;
		if (vp_Value)
			pVar->iBufferLength = *(int *)vp_Value;
		pVar->un.pDictEntry = (MYS_DICT_ENTRY *)MemoryAlloc(pVar->iBufferLength * sizeof(MYS_DICT_ENTRY), "pVarCreate", __LINE__);
		memset(pVar->un.pDictEntry, 0, pVar->iBufferLength * sizeof(MYS_DICT_ENTRY));
		break;
	}
	return pVar;
}

void VarDelete(MYS mys, MYS_VAR *pVar)
{
	if (pVar->pVarNext == NULL)
	{
		((MYSD *)mys)->pVarLast = pVar->pVarBefore;
		if (pVar->pVarBefore)
			pVar->pVarBefore->pVarNext = NULL;
		else
			((MYSD *)mys)->pVarFirst = NULL;
	}
	else if (pVar->pVarBefore == NULL)
	{
		((MYSD *)mys)->pVarFirst = pVar->pVarNext;
		if (pVar->pVarNext)
			pVar->pVarNext->pVarBefore = NULL;
		else
			((MYSD *)mys)->pVarLast = NULL;
	}
	else
	{
		MYS_VAR *pVarTmp = pVar->pVarBefore->pVarNext;
		pVar->pVarBefore->pVarNext = pVar->pVarNext->pVarBefore;
		pVar->pVarNext->pVarBefore = pVarTmp;
	}

	switch (pVar->VarType)
	{
	case VTE_NAME:
		MemoryFree(pVar->un.pName);
		MemoryFree(pVar);
		break;
	case VTE_STRING:
		MemoryFree(pVar->un.pString);
		MemoryFree(pVar);
		break;
	case VTE_ARRAY:
		MemoryFree(pVar->un.pObjInArray);
		MemoryFree(pVar);
		break;
	case VTE_DICTIONARY:
		MemoryFree(pVar->un.pDictEntry);
		MemoryFree(pVar);
		break;
	default:
		// This is not a var.
		return;
	}
}

void VarClean(MYS mys)
{
	for (MYS_VAR *pVar = ((MYSD *)mys)->pVarFirst; pVar != NULL; pVar = ((MYSD *)mys)->pVarFirst)
	{
		VarDelete(mys, pVar);
	}
}

void PushNull(MYS mys)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_NULL;
	con->iOPStackUsing++;
}

void PushString(MYS mys, char *c_String)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_STRING;
	cur->un.pVar = pVarCreate(mys, VTE_STRING, c_String);
	con->iOPStackUsing++;
}

void PushName(MYS mys, bool b_Exec, char *c_String)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = b_Exec;
	cur->ObjType = OTE_NAME;
	cur->un.pVar = pVarCreate(mys, VTE_NAME, c_String);
	con->iOPStackUsing++;
}

void PushReal(MYS mys, double dReal)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_REAL;
	cur->un.dReal = dReal;
	con->iOPStackUsing++;
}

void PushInteger(MYS mys, long iInteger)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_INTEGER;
	cur->un.iInteger = iInteger;
	con->iOPStackUsing++;
}

bool bGetInteger(MYS mys, MYS_OBJ *pObj, int *pInteger, const char *pTitle)
{
	if (pObj->ObjType != OTE_INTEGER)
		return iError(mys, TYPECHECK, __func__, __LINE__, pTitle);

	*pInteger = pObj->un.iInteger;
	return true;
}

bool bGetReal(MYS mys, MYS_OBJ *pObj, double *pReal, const char *pTitle)
{
	if (pObj->ObjType != OTE_REAL)
		return iError(mys, TYPECHECK, __func__, __LINE__, pTitle);

	*pReal = pObj->un.dReal;
	return true;
}

bool bGetString(MYS mys, MYS_OBJ *pObj, char **ppString, const char *pTitle)
{
	if (pObj->ObjType != OTE_STRING)
		return iError(mys, TYPECHECK, __func__, __LINE__, pTitle);

	*ppString = pObj->un.pVar->un.pString;
	return true;
}

void Pushboolean(MYS mys, long iInteger)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_boolEAN;
	cur->un.iInteger = iInteger;
	con->iOPStackUsing++;
}

void PushMark(MYS mys, bool bExec)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = bExec;
	cur->ObjType = OTE_MARK;
	con->iOPStackUsing++;
}

void PushObj(MYS mys, MYS_OBJ *pObj)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	*cur = *pObj;
	con->iOPStackUsing++;
}

int iExecute(MYS mys)
{
	MYSD *con = (MYSD *)mys;
	MYS_OBJ Obj = con->OPStack[con->iOPStackUsing - 1];
	int retv = 1;

	if (Obj.bExecutable == true)
	{
		// The executable object can be OTE_NAME, OTE_ARRAY or OTE_OPERATOR.
		switch (Obj.ObjType)
		{
		case OTE_NAME:
			if (B_OPENEXECARRAY(mys) == false || Obj.un.pVar->un.pString[0] == '}')
			{
				MYS_OBJ *pObjKey = &Obj;
				MYS_OBJ *pObjValue = pObjSearchInDicts(mys, pObjKey);
				if (pObjValue == NULL)
				{
					//((MYSD *)mys)->iOPStackUsing--;
					return iError(mys, UNDEFINED, __func__, __LINE__, GET_STRING(pObjKey));
				}
				else if (pObjValue->ObjType == OTE_OPERATOR)
				{
					if (((MYSD *)mys)->bDebugMessage)
						MYS_fprintf(MYS_stdout, "\t\t\t\t\t\t\tDBG OPERATOR[%d] %s\n", STACK_LEVEL(mys), pObjValue->un.pOperator->pKeyName);
					retv = (*pObjValue->un.pOperator->pIntOperator)(mys);
					if (retv < 1)
						break;
				}
				else
				{
					if (((MYSD *)mys)->bDebugMessage)
						MYS_fprintf(MYS_stdout, "\t\t\t\t\t\t\tDBG DICTVALUE[%d] %s\n", STACK_LEVEL(mys), GET_STRING(pObjKey));
					((MYSD *)mys)->iOPStackUsing--;
					PushObj(mys, pObjValue);
					retv = iExecute(mys);
					if (retv < 1)
						break;
				}
			}
			break;

		case OTE_ARRAY:
			((MYSD *)mys)->iOPStackUsing--;
			for (int i = 0; i < Obj.un.pVar->iLength; i++)
			{
				MYS_OBJ *pObjInArray = Obj.un.pVar->un.pObjInArray + i;

				PushObj(mys, pObjInArray);
				if (!(pObjInArray->ObjType == OTE_ARRAY && pObjInArray->bExecutable == true))
				{
					retv = iExecute(mys);
					if (retv < 1)
						break;
				}
			}
			break;

		case OTE_STRING:
			((MYSD *)mys)->iOPStackUsing--;
			if (((MYSD *)mys)->bDebugMessage)
				MYS_fprintf(MYS_stdout, "\t\t\t\t\t\t\tDBG STRING[%d] %s\n", STACK_LEVEL(mys), Obj.un.pVar->un.pString);
			retv = iMysParseString(mys, Obj.un.pVar->un.pString);
			break;

		case OTE_OPERATOR:
			if (((MYSD *)mys)->bDebugMessage)
				MYS_fprintf(MYS_stdout, "\t\t\t\t\t\t\tDBG OPERATOR[%d] %s\n", STACK_LEVEL(mys), Obj.un.pOperator->pKeyName);
			retv = (Obj.un.pOperator->pIntOperator)(mys);
			break;
		default:
			retv = 1;
			break;
		}
	}
	return retv;
}

bool bRegistObjToDict(MYS mys, MYS_OBJ *pObjDict, MYS_OBJ *pObjKey, MYS_OBJ *pObjValue)
{
	MYS_OBJ *pObjExistValue = NULL;

	MYS_OBJ *pDict = &((MYSD *)mys)->DictStack[((MYSD *)mys)->iDictStackUsing - 1];
	for (int j = 0; j < pDict->un.pVar->iLength; j++)
	{
		MYS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
		if (bObjCompare(pObjKey, &pEntry->ObjKey))
		{
			pObjExistValue = &pEntry->ObjValue;
		}
	}

	if (pObjExistValue)
	{
		*pObjExistValue = *pObjValue;
	}
	else
	{
		MYS_VAR *pVarDict = pObjDict->un.pVar;
		pVarDict->iLength++;
		if (pVarDict->iLength > pVarDict->iBufferLength)
		{
			pVarDict->iBufferLength += 64;
			pVarDict->un.pDictEntry = (MYS_DICT_ENTRY *)MemoryRealloc(pVarDict->un.pDictEntry, sizeof(MYS_DICT_ENTRY) * pVarDict->iBufferLength);
		}
		pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey = *pObjKey;
		pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue = *pObjValue;
	}
	return true;
}

void AddOperatorToDict(MYS mys, char *pName, const MYS_OPERATOR *pOperator)
{
	MYS_OBJ *pDictCurrent = CURRENTDICT(mys);
	MYS_VAR *pVarDict = pDictCurrent->un.pVar;
	pVarDict->iLength++;
	if (pVarDict->iLength > pVarDict->iBufferLength)
	{
		pVarDict->iBufferLength += 64;
		pVarDict->un.pDictEntry = (MYS_DICT_ENTRY *)MemoryRealloc(pVarDict->un.pDictEntry, pVarDict->iBufferLength);
	}
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey.ObjType = OTE_NAME;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey.bExecutable = false;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey.un.pVar = pVarCreate(mys, VTE_NAME, pName);

	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue.ObjType = OTE_OPERATOR;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue.bExecutable = true;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue.un.pOperator = pOperator;
}

void PushDict(MYS mys, MYS_OBJ *pObjDict)
{
	((MYSD *)mys)->iDictStackUsing++;
	((MYSD *)mys)->DictStack[((MYSD *)mys)->iDictStackUsing - 1] = *pObjDict;
}

void PopDict(MYS mys)
{
	((MYSD *)mys)->iDictStackUsing--;
}

bool bObjCompare(MYS_OBJ *pObj1, MYS_OBJ *pObj2)
{

	if (pObj1->ObjType == pObj2->ObjType)
	{
		bool bRet = false;
		switch (pObj1->ObjType)
		{
		case OTE_NAME:
			if (strcmp(pObj1->un.pVar->un.pName, pObj2->un.pVar->un.pName) == 0)
				bRet = true;
			break;
		case OTE_STRING:
			if (strcmp(pObj1->un.pVar->un.pString, pObj2->un.pVar->un.pString) == 0)
				bRet = true;
			break;
		case OTE_boolEAN:
		case OTE_INTEGER:
			if (pObj1->un.iInteger == pObj2->un.iInteger)
				bRet = true;
			break;
		case OTE_REAL:
			if (pObj1->un.dReal == pObj2->un.dReal)
				bRet = true;
			break;
		case OTE_MARK:
		case OTE_ARRAY:
		case OTE_OPERATOR:
		case OTE_DICTIONARY:
			break;
		}
		return bRet;
	}
	else
	{
		return false;
	}
}

MYS_OBJ *pObjSearchInDicts(MYS mys, MYS_OBJ *pObjKey)
{
	for (int i = ((MYSD *)mys)->iDictStackUsing - 1; i > -1; i--)
	{
		MYS_OBJ *pDict = &((MYSD *)mys)->DictStack[i];
		for (int j = 0; j < pDict->un.pVar->iLength; j++)
		{
			MYS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
			if (pEntry->ObjKey.un.pVar->iLength == pObjKey->un.pVar->iLength)
			{
				if (strcmp(pEntry->ObjKey.un.pVar->un.pName, pObjKey->un.pVar->un.pName) == 0)
				{
					return &pEntry->ObjValue;
				}
			}
		}
	}
	return NULL;
}

MYS_OBJ *pObjSearchInTheDict(MYS mys, MYS_OBJ *pDict, MYS_OBJ *pObjKey)
{
	for (int j = 0; j < pDict->un.pVar->iLength; j++)
	{
		MYS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
		if (pEntry->ObjKey.un.pVar->iLength == pObjKey->un.pVar->iLength)
		{
			if (strcmp(pEntry->ObjKey.un.pVar->un.pName, pObjKey->un.pVar->un.pName) == 0)
			{
				return &pEntry->ObjValue;
			}
		}
	}
	return NULL;
}

MYS_OBJ *pObjSearchInTheDictWithString(MYS mys, MYS_OBJ *pDict, const char *pKeyString)
{
	for (int j = 0; j < pDict->un.pVar->iLength; j++)
	{
		MYS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
		if (pEntry->ObjKey.un.pVar->iLength == strlen(pKeyString))
		{
			if (strcmp(pEntry->ObjKey.un.pVar->un.pName, pKeyString) == 0)
			{
				return &pEntry->ObjValue;
			}
		}
	}
	return NULL;
}

void ShowArray(MYS_OBJ *pObj)
{
	if (pObj->bExecutable == true)
		MYS_fprintf(MYS_stdout, "{");
	else
		MYS_fprintf(MYS_stdout, "[");

	for (int i = 0; i < pObj->un.pVar->iLength; i++)
	{
		MYS_fprintf(MYS_stdout, " ");
		switch (pObj->un.pVar->un.pObjInArray[i].ObjType)
		{
		case OTE_NULL:
			MYS_fprintf(MYS_stdout, "--null--");
			break;
		case OTE_NAME:
			if (pObj->un.pVar->un.pObjInArray[i].bExecutable == true)
				MYS_fprintf(MYS_stdout, "%s", pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pName);
			else
				MYS_fprintf(MYS_stdout, "/%s", pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pName);
			break;
		case OTE_STRING:
#if 0 // def _DEBUG
			MYS_printf("[MYS %d] %s\n", i, pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pString);
#endif
			MYS_fprintf(MYS_stdout, "(%s)", pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pString);
			break;
		case OTE_boolEAN:
			MYS_fprintf(MYS_stdout, "%s", (pObj->un.pVar->un.pObjInArray[i].un.iInteger == 1 ? "--true--" : "--false--"));
			break;
		case OTE_INTEGER:
			MYS_fprintf(MYS_stdout, "%ld", pObj->un.pVar->un.pObjInArray[i].un.iInteger);
			break;
		case OTE_REAL:
			MYS_fprintf(MYS_stdout, "%lf", pObj->un.pVar->un.pObjInArray[i].un.dReal);
			break;
		case OTE_MARK:
			if (pObj->un.pVar->un.pObjInArray[i].bExecutable == true)
				MYS_fprintf(MYS_stdout, "{");
			else
				MYS_fprintf(MYS_stdout, "[");
			break;
		case OTE_ARRAY:
			ShowArray(&pObj->un.pVar->un.pObjInArray[i]);
			break;
		case OTE_OPERATOR:
			MYS_fprintf(MYS_stdout, "--%s--", pObj->un.pOperator->pKeyName);
			break;
		case OTE_DICTIONARY:
			MYS_fprintf(MYS_stdout, "--dictionary--");
			break;
		}
	}
	if (pObj->bExecutable == true)
		MYS_fprintf(MYS_stdout, " }");
	else
		MYS_fprintf(MYS_stdout, " ]");
}

void ShowOPStack(MYS_OBJ *stack, int index)
{
	switch (stack->ObjType)
	{
	case OTE_NULL:
		MYS_fprintf(MYS_stdout, "--null--\n");
		break;
	case OTE_NAME:
		MYS_fprintf(MYS_stdout, "/%s\n", GET_NAME(stack));
		break;
	case OTE_boolEAN:
		MYS_fprintf(MYS_stdout, "%s\n", (GET_INTEGER(stack) == 1 ? "--true--" : "--false--"));
		break;
	case OTE_STRING:
#if 0 // def _DEBUG
		printf("[MYS] %s\n", GET_NAME(stack));
#endif
		if (stack->bExecutable)
			MYS_fprintf(MYS_stdout, "{%s}\n", GET_NAME(stack));
		else
			MYS_fprintf(MYS_stdout, "(%s)\n", GET_NAME(stack));
		break;
	case OTE_INTEGER:
		MYS_fprintf(MYS_stdout, "%ld\n", GET_INTEGER(stack));
		break;
	case OTE_REAL:
		MYS_fprintf(MYS_stdout, "%#g\n", GET_REAL(stack));
		break;
	case OTE_MARK:
		if (MARK_EXECUTABLE((*stack)) == true)
			MYS_fprintf(MYS_stdout, "{\n");
		else
			MYS_fprintf(MYS_stdout, "[\n");
		break;
	case OTE_ARRAY:
		ShowArray(stack);
		MYS_fprintf(MYS_stdout, "\n");
		break;
	case OTE_OPERATOR:
		MYS_fprintf(MYS_stdout, "--%s--\n", stack->un.pOperator->pKeyName);
		break;
	case OTE_DICTIONARY:
		MYS_fprintf(MYS_stdout, "--dictionary--\n");
		break;
	}
}

int iError(MYS mys, ERRORTYPE type, const char *filename, const int lineno, const char *opt1, const char *opt2)
{
	char *fname = basename(strdup(filename));

	if (opt1)
	{
		if (opt2)
			MYS_fprintf(MYS_stdout, " ##[%s - %s %s @ %s(L%d)]##\n", _errtxt[type], opt1, opt2, fname, lineno);
		else
			MYS_fprintf(MYS_stdout, " ##[%s - %s @ %s(L%d)]##\n", _errtxt[type], opt1, fname, lineno);
	}
	else
		MYS_fprintf(MYS_stdout, " ##[%s @ %s(L%d)]##\n", _errtxt[type], fname, lineno);

	for (int i = 0; i < ((MYSD *)mys)->iOPStackUsing; i++)
	{
		MYS_fprintf(MYS_stdout, "%3d| ", i);
		ShowOPStack(&((MYSD *)mys)->OPStack[i], i);
	}

	return MYS_ERR;
}

void MysMessage(
	MYS mys,
	const char *format,
	...)
{
	char message[MAX_PRINTF_STRING];

	va_list args;
	va_start(args, format);
	int rv = vsprintf(message, format, args);
	va_end(args);

	MYS_fprintf(MYS_stdout, " ##[MYS: %s]##\n", message);
}

void MysHelp(MYS mys, const char *s1, const char *s2, const char *s3)
{
	if (s2 && s3)
		MYS_fprintf(MYS_stdout, "[%s] %s\n spec: %s\n", s1, s2, s3);
	else
		MYS_fprintf(MYS_stdout, "%s\n", s1);
	return;
}

int main(int argc, char *argv[])
{
	MYS mys = pMysInitialize();
	MysMainLoop(mys);
	MysTerminate(mys);
	return 0;
}