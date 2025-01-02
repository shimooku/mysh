#include "simoscript.h"

#define MAX_PRINTF_STRING 2048

typedef struct SMS_MEMORY_HEADER
{
	struct SMS_MEMORY_HEADER *pNext;
	struct SMS_MEMORY_HEADER *pBefore;
	size_t iSize;
	char info[256];
	size_t dummy;
} SMS_MEMORY_HEADER;

static size_t _sz_MemorySize = 0;
static SMS_MEMORY_HEADER *p_MemoryLinkFirst = NULL;
static SMS_MEMORY_HEADER *p_MemoryLinkLast = NULL;

#define CHECKFLAG (size_t)0x12345678

void CheckMemory(char *title)
{
	SMS_MEMORY_HEADER *ptr = p_MemoryLinkFirst;
	for (; ptr != NULL; ptr = ptr->pNext)
	{
		if (ptr->dummy != CHECKFLAG)
		{
			if (title)
				fprintf(SMS_stdout, "(%s) %s\n", title, ptr->info);
			fprintf(SMS_stdout, "*********** Memory Check Error (HEADER PART) ***********\n");
		}
		char *checkarea = (char *)ptr + ptr->iSize + sizeof(SMS_MEMORY_HEADER);
		if (*(size_t *)checkarea != CHECKFLAG)
		{
			if (title)
				fprintf(SMS_stdout, "(%s) %s\n", title, ptr->info);
			fprintf(SMS_stdout, "*********** Memory Check Error (FOOTER PART) ***********\n");
		}
	}
}

void EnumMemory(SMS sms)
{
	SMS_MEMORY_HEADER *ptr = p_MemoryLinkFirst;
	for (; ptr != NULL; ptr = ptr->pNext)
	{
		SmsMessage(sms, "SMS EnumMemory: %s Size: %zd byte", ptr->info, ptr->iSize);
	}
}

void *MemoryAlloc(size_t size, const char *name, int line)
{
	CheckMemory((char *)"MemoryAlloc");

	_sz_MemorySize += size;

	SMS_MEMORY_HEADER *ptr = (SMS_MEMORY_HEADER *)malloc(size + sizeof(SMS_MEMORY_HEADER) + sizeof(size_t));
	if (ptr == NULL)
		goto EXIT;

	memset(ptr, 0, size + sizeof(SMS_MEMORY_HEADER) + sizeof(size_t));

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

	*(size_t *)((char *)ptr + sizeof(SMS_MEMORY_HEADER) + size) = CHECKFLAG;

EXIT:
	if (ptr)
		return (void *)((char *)ptr + sizeof(SMS_MEMORY_HEADER));
	else
		return NULL;
}

void *MemoryRealloc(void *pMem, size_t size)
{
	SMS_MEMORY_HEADER *top = (SMS_MEMORY_HEADER *)((char *)pMem - sizeof(SMS_MEMORY_HEADER));

	size_t original_size = top->iSize;

	SMS_MEMORY_HEADER *new_top = (SMS_MEMORY_HEADER *)realloc(top, size + sizeof(SMS_MEMORY_HEADER) + sizeof(size_t));

	new_top->iSize = size;

	_sz_MemorySize -= original_size;
	_sz_MemorySize += size;

	*(size_t *)((char *)new_top + sizeof(SMS_MEMORY_HEADER) + size) = CHECKFLAG;

	return (void *)((char *)new_top + sizeof(SMS_MEMORY_HEADER));
}

void MemoryFree(void *pMem)
{
	CheckMemory((char *)"MemoryFree");

	SMS_MEMORY_HEADER *ptr = (SMS_MEMORY_HEADER *)((char *)pMem - sizeof(SMS_MEMORY_HEADER));

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

int iSmsParseString(SMS sms, char *pBuffer)
{
	char *cp_ptr = pBuffer;
	char *utf8_ptr = nullptr;
	int retv = 1;

	do
	{
		cp_ptr = pTokenFromString(sms, cp_ptr);

		if (bTokenCompleted(TOKEN(sms)) == false)
		{
			fgets(pBuffer, 512, ((SMSD *)sms)->fpIn);
			cp_ptr = pBuffer;
		}
		else if (uTokenLength(TOKEN(sms)))
		{
			PushToken(sms, pTokenBuffer(TOKEN(sms)));
			TokenReset(TOKEN(sms));
			retv = iExecute(sms);
			if (retv < 1)
				break;
		}
	} while (*cp_ptr);

	return retv;
}

int SmsMainLoop(SMS sms)
{
	SMSD *smsd = (SMSD *)sms;
	char c_Buffer[SMS_MAX_STRING];
	int retv = 1;

	smsd->iNestMainLoop++;

	do
	{
		if (((SMSD *)sms)->bPrompt)
			printf("SMS> ");
		memset(c_Buffer, 0x00, SMS_MAX_STRING);
		if (fgets(c_Buffer, 512, smsd->fpIn) != NULL /*&& setjmp(gc_Env) == false*/)
		{
			if (((SMSD *)sms)->bDebugMessage)
				SMS_fprintf(SMS_stdout, "\t\t\t\t\t\t\tDBG [%d] %s", STACK_LEVEL(sms), c_Buffer);

			retv = iSmsParseString(sms, c_Buffer);

			if (((SMSD *)sms)->bPrompt)
			{
				if (retv != SMS_QUIT)
					retv = SMS_OK;
			}
			else if (retv == 0)
			{
				retv = SMS_OK;
			}
		}
		else
		{
			retv = SMS_EOF;
		}
	} while (retv > 0);

	smsd->iNestMainLoop--;

	return retv;
}

SMS pSmsInitialize()
{
	SMSD *con = (SMSD *)MemoryAlloc(sizeof(SMSD), "pSmsInitialize", __LINE__);
	memset(con, 0, sizeof(SMSD));

	con->iDictStackUsing = 1;
	con->DictStack[0].ObjType = OTE_DICTIONARY;
	con->DictStack[0].bExecutable = false;
	int iLength = 1024;
	con->DictStack[0].un.pVar = pVarCreate(con, VTE_DICTIONARY, (void *)&iLength);

	con->pToken = pTokenInitialize();
	con->iNestExecArray = 0;
	con->bPrompt = false;

	con->fpIn = stdin;

	SMS_stdout = stdout;

	con->iNestMainLoop = 0;

	InitDictOperators(con);

	return (SMS)con;
}

void SmsTerminate(SMS sms)
{
	SMSD *smsd = (SMSD *)sms;
	TokenTerminate(smsd->pToken);
	VarClean(sms);
	MemoryFree(sms);
	EnumMemory(sms);
}

/*
	For VTE_ARRAY and VTE_DICTIONARY, the function doesn't put any value.
	You have to use PutVarEntry function for putting values.
*/
SMS_VAR *pVarCreate(SMS sms, VAR_TYPE_ENUM VarType, void *vp_Value)
{
	((SMSD *)sms)->iVarSeqNo++;

	SMS_VAR *pVar = (SMS_VAR *)MemoryAlloc(sizeof(SMS_VAR), "pVarCreate", __LINE__);
	memset(pVar, 0, sizeof(SMS_VAR));

	pVar->iSeqNo = ((SMSD *)sms)->iVarSeqNo;

	pVar->VarType = VarType;

	if (((SMSD *)sms)->pVarLast == NULL)
	{
		((SMSD *)sms)->pVarLast = pVar;
		((SMSD *)sms)->pVarFirst = pVar;
		pVar->pVarBefore = NULL;
		pVar->pVarNext = NULL;
	}
	else
	{
		((SMSD *)sms)->pVarLast->pVarNext = pVar;
		pVar->pVarBefore = ((SMSD *)sms)->pVarLast;
		pVar->pVarNext = NULL;
		((SMSD *)sms)->pVarLast = pVar;
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
		pVar->un.pObjInArray = (SMS_OBJ *)MemoryAlloc(pVar->iBufferLength * sizeof(SMS_OBJ), "pVarCreate", __LINE__);
		memset(pVar->un.pObjInArray, 0, pVar->iBufferLength * sizeof(SMS_OBJ));
		break;
	case VTE_DICTIONARY:
		pVar->iLength = 0;
		pVar->iBufferLength = 64;
		if (vp_Value)
			pVar->iBufferLength = *(int *)vp_Value;
		pVar->un.pDictEntry = (SMS_DICT_ENTRY *)MemoryAlloc(pVar->iBufferLength * sizeof(SMS_DICT_ENTRY), "pVarCreate", __LINE__);
		memset(pVar->un.pDictEntry, 0, pVar->iBufferLength * sizeof(SMS_DICT_ENTRY));
		break;
	}
	return pVar;
}

void VarDelete(SMS sms, SMS_VAR *pVar)
{
	if (pVar->pVarNext == NULL)
	{
		((SMSD *)sms)->pVarLast = pVar->pVarBefore;
		if (pVar->pVarBefore)
			pVar->pVarBefore->pVarNext = NULL;
		else
			((SMSD *)sms)->pVarFirst = NULL;
	}
	else if (pVar->pVarBefore == NULL)
	{
		((SMSD *)sms)->pVarFirst = pVar->pVarNext;
		if (pVar->pVarNext)
			pVar->pVarNext->pVarBefore = NULL;
		else
			((SMSD *)sms)->pVarLast = NULL;
	}
	else
	{
		SMS_VAR *pVarTmp = pVar->pVarBefore->pVarNext;
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

void VarClean(SMS sms)
{
	for (SMS_VAR *pVar = ((SMSD *)sms)->pVarFirst; pVar != NULL; pVar = ((SMSD *)sms)->pVarFirst)
	{
		VarDelete(sms, pVar);
	}
}

void PushNull(SMS sms)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_NULL;
	con->iOPStackUsing++;
}

void PushString(SMS sms, char *c_String)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_STRING;
	cur->un.pVar = pVarCreate(sms, VTE_STRING, c_String);
	con->iOPStackUsing++;
}

void PushName(SMS sms, bool b_Exec, char *c_String)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = b_Exec;
	cur->ObjType = OTE_NAME;
	cur->un.pVar = pVarCreate(sms, VTE_NAME, c_String);
	con->iOPStackUsing++;
}

void PushReal(SMS sms, double dReal)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_REAL;
	cur->un.dReal = dReal;
	con->iOPStackUsing++;
}

void PushInteger(SMS sms, long iInteger)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_INTEGER;
	cur->un.iInteger = iInteger;
	con->iOPStackUsing++;
}

bool bGetInteger(SMS sms, SMS_OBJ *pObj, int *pInteger, const char *pTitle)
{
	if (pObj->ObjType != OTE_INTEGER)
		return iError(sms, TYPECHECK, __func__, __LINE__, pTitle);

	*pInteger = pObj->un.iInteger;
	return true;
}

bool bGetReal(SMS sms, SMS_OBJ *pObj, double *pReal, const char *pTitle)
{
	if (pObj->ObjType != OTE_REAL)
		return iError(sms, TYPECHECK, __func__, __LINE__, pTitle);

	*pReal = pObj->un.dReal;
	return true;
}

bool bGetString(SMS sms, SMS_OBJ *pObj, char **ppString, const char *pTitle)
{
	if (pObj->ObjType != OTE_STRING)
		return iError(sms, TYPECHECK, __func__, __LINE__, pTitle);

	*ppString = pObj->un.pVar->un.pString;
	return true;
}

void Pushboolean(SMS sms, long iInteger)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = false;
	cur->ObjType = OTE_boolEAN;
	cur->un.iInteger = iInteger;
	con->iOPStackUsing++;
}

void PushMark(SMS sms, bool bExec)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	cur->bExecutable = bExec;
	cur->ObjType = OTE_MARK;
	con->iOPStackUsing++;
}

void PushObj(SMS sms, SMS_OBJ *pObj)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ *cur = con->OPStack + con->iOPStackUsing;

	*cur = *pObj;
	con->iOPStackUsing++;
}

int iExecute(SMS sms)
{
	SMSD *con = (SMSD *)sms;
	SMS_OBJ Obj = con->OPStack[con->iOPStackUsing - 1];
	int retv = 1;

	if (Obj.bExecutable == true)
	{
		// The executable object can be OTE_NAME, OTE_ARRAY or OTE_OPERATOR.
		switch (Obj.ObjType)
		{
		case OTE_NAME:
			if (B_OPENEXECARRAY(sms) == false || Obj.un.pVar->un.pString[0] == '}')
			{
				SMS_OBJ *pObjKey = &Obj;
				SMS_OBJ *pObjValue = pObjSearchInDicts(sms, pObjKey);
				if (pObjValue == NULL)
				{
					//((SMSD *)sms)->iOPStackUsing--;
					return iError(sms, UNDEFINED, __func__, __LINE__, GET_STRING(pObjKey));
				}
				else if (pObjValue->ObjType == OTE_OPERATOR)
				{
					if (((SMSD *)sms)->bDebugMessage)
						SMS_fprintf(SMS_stdout, "\t\t\t\t\t\t\tDBG OPERATOR[%d] %s\n", STACK_LEVEL(sms), pObjValue->un.pOperator->pKeyName);
					retv = (*pObjValue->un.pOperator->pIntOperator)(sms);
					if (retv < 1)
						break;
				}
				else
				{
					if (((SMSD *)sms)->bDebugMessage)
						SMS_fprintf(SMS_stdout, "\t\t\t\t\t\t\tDBG DICTVALUE[%d] %s\n", STACK_LEVEL(sms), GET_STRING(pObjKey));
					((SMSD *)sms)->iOPStackUsing--;
					PushObj(sms, pObjValue);
					retv = iExecute(sms);
					if (retv < 1)
						break;
				}
			}
			break;

		case OTE_ARRAY:
			((SMSD *)sms)->iOPStackUsing--;
			for (int i = 0; i < Obj.un.pVar->iLength; i++)
			{
				SMS_OBJ *pObjInArray = Obj.un.pVar->un.pObjInArray + i;

				PushObj(sms, pObjInArray);
				if (!(pObjInArray->ObjType == OTE_ARRAY && pObjInArray->bExecutable == true))
				{
					retv = iExecute(sms);
					if (retv < 1)
						break;
				}
			}
			break;

		case OTE_STRING:
			((SMSD *)sms)->iOPStackUsing--;
			if (((SMSD *)sms)->bDebugMessage)
				SMS_fprintf(SMS_stdout, "\t\t\t\t\t\t\tDBG STRING[%d] %s\n", STACK_LEVEL(sms), Obj.un.pVar->un.pString);
			retv = iSmsParseString(sms, Obj.un.pVar->un.pString);
			break;

		case OTE_OPERATOR:
			if (((SMSD *)sms)->bDebugMessage)
				SMS_fprintf(SMS_stdout, "\t\t\t\t\t\t\tDBG OPERATOR[%d] %s\n", STACK_LEVEL(sms), Obj.un.pOperator->pKeyName);
			retv = (Obj.un.pOperator->pIntOperator)(sms);
			break;
		default:
			retv = 1;
			break;
		}
	}
	return retv;
}

bool bRegistObjToDict(SMS sms, SMS_OBJ *pObjDict, SMS_OBJ *pObjKey, SMS_OBJ *pObjValue)
{
	SMS_OBJ *pObjExistValue = NULL;

	SMS_OBJ *pDict = &((SMSD *)sms)->DictStack[((SMSD *)sms)->iDictStackUsing - 1];
	for (int j = 0; j < pDict->un.pVar->iLength; j++)
	{
		SMS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
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
		SMS_VAR *pVarDict = pObjDict->un.pVar;
		pVarDict->iLength++;
		if (pVarDict->iLength > pVarDict->iBufferLength)
		{
			pVarDict->iBufferLength += 64;
			pVarDict->un.pDictEntry = (SMS_DICT_ENTRY *)MemoryRealloc(pVarDict->un.pDictEntry, sizeof(SMS_DICT_ENTRY) * pVarDict->iBufferLength);
		}
		pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey = *pObjKey;
		pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue = *pObjValue;
	}
	return true;
}

void AddOperatorToDict(SMS sms, char *pName, const SMS_OPERATOR *pOperator)
{
	SMS_OBJ *pDictCurrent = CURRENTDICT(sms);
	SMS_VAR *pVarDict = pDictCurrent->un.pVar;
	pVarDict->iLength++;
	if (pVarDict->iLength > pVarDict->iBufferLength)
	{
		pVarDict->iBufferLength += 64;
		pVarDict->un.pDictEntry = (SMS_DICT_ENTRY *)MemoryRealloc(pVarDict->un.pDictEntry, pVarDict->iBufferLength);
	}
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey.ObjType = OTE_NAME;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey.bExecutable = false;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjKey.un.pVar = pVarCreate(sms, VTE_NAME, pName);

	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue.ObjType = OTE_OPERATOR;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue.bExecutable = true;
	pVarDict->un.pDictEntry[pVarDict->iLength - 1].ObjValue.un.pOperator = pOperator;
}

void PushDict(SMS sms, SMS_OBJ *pObjDict)
{
	((SMSD *)sms)->iDictStackUsing++;
	((SMSD *)sms)->DictStack[((SMSD *)sms)->iDictStackUsing - 1] = *pObjDict;
}

void PopDict(SMS sms)
{
	((SMSD *)sms)->iDictStackUsing--;
}

bool bObjCompare(SMS_OBJ *pObj1, SMS_OBJ *pObj2)
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

SMS_OBJ *pObjSearchInDicts(SMS sms, SMS_OBJ *pObjKey)
{
	for (int i = ((SMSD *)sms)->iDictStackUsing - 1; i > -1; i--)
	{
		SMS_OBJ *pDict = &((SMSD *)sms)->DictStack[i];
		for (int j = 0; j < pDict->un.pVar->iLength; j++)
		{
			SMS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
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

SMS_OBJ *pObjSearchInTheDict(SMS sms, SMS_OBJ *pDict, SMS_OBJ *pObjKey)
{
	for (int j = 0; j < pDict->un.pVar->iLength; j++)
	{
		SMS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
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

SMS_OBJ *pObjSearchInTheDictWithString(SMS sms, SMS_OBJ *pDict, const char *pKeyString)
{
	for (int j = 0; j < pDict->un.pVar->iLength; j++)
	{
		SMS_DICT_ENTRY *pEntry = DICTENTRY(pDict, j);
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

void ShowArray(SMS_OBJ *pObj)
{
	if (pObj->bExecutable == true)
		SMS_fprintf(SMS_stdout, "{");
	else
		SMS_fprintf(SMS_stdout, "[");

	for (int i = 0; i < pObj->un.pVar->iLength; i++)
	{
		SMS_fprintf(SMS_stdout, " ");
		switch (pObj->un.pVar->un.pObjInArray[i].ObjType)
		{
		case OTE_NULL:
			SMS_fprintf(SMS_stdout, "--null--");
			break;
		case OTE_NAME:
			if (pObj->un.pVar->un.pObjInArray[i].bExecutable == true)
				SMS_fprintf(SMS_stdout, "%s", pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pName);
			else
				SMS_fprintf(SMS_stdout, "/%s", pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pName);
			break;
		case OTE_STRING:
#if 0 // def _DEBUG
			SMS_printf("[SMS %d] %s\n", i, pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pString);
#endif
			SMS_fprintf(SMS_stdout, "(%s)", pObj->un.pVar->un.pObjInArray[i].un.pVar->un.pString);
			break;
		case OTE_boolEAN:
			SMS_fprintf(SMS_stdout, "%s", (pObj->un.pVar->un.pObjInArray[i].un.iInteger == 1 ? "--true--" : "--false--"));
			break;
		case OTE_INTEGER:
			SMS_fprintf(SMS_stdout, "%ld", pObj->un.pVar->un.pObjInArray[i].un.iInteger);
			break;
		case OTE_REAL:
			SMS_fprintf(SMS_stdout, "%lf", pObj->un.pVar->un.pObjInArray[i].un.dReal);
			break;
		case OTE_MARK:
			if (pObj->un.pVar->un.pObjInArray[i].bExecutable == true)
				SMS_fprintf(SMS_stdout, "{");
			else
				SMS_fprintf(SMS_stdout, "[");
			break;
		case OTE_ARRAY:
			ShowArray(&pObj->un.pVar->un.pObjInArray[i]);
			break;
		case OTE_OPERATOR:
			SMS_fprintf(SMS_stdout, "--%s--", pObj->un.pOperator->pKeyName);
			break;
		case OTE_DICTIONARY:
			SMS_fprintf(SMS_stdout, "--dictionary--");
			break;
		}
	}
	if (pObj->bExecutable == true)
		SMS_fprintf(SMS_stdout, " }");
	else
		SMS_fprintf(SMS_stdout, " ]");
}

void ShowOPStack(SMS_OBJ *stack, int index)
{
	switch (stack->ObjType)
	{
	case OTE_NULL:
		SMS_fprintf(SMS_stdout, "--null--\n");
		break;
	case OTE_NAME:
		SMS_fprintf(SMS_stdout, "/%s\n", GET_NAME(stack));
		break;
	case OTE_boolEAN:
		SMS_fprintf(SMS_stdout, "%s\n", (GET_INTEGER(stack) == 1 ? "--true--" : "--false--"));
		break;
	case OTE_STRING:
#if 0 // def _DEBUG
		printf("[SMS] %s\n", GET_NAME(stack));
#endif
		if (stack->bExecutable)
			SMS_fprintf(SMS_stdout, "{%s}\n", GET_NAME(stack));
		else
			SMS_fprintf(SMS_stdout, "(%s)\n", GET_NAME(stack));
		break;
	case OTE_INTEGER:
		SMS_fprintf(SMS_stdout, "%ld\n", GET_INTEGER(stack));
		break;
	case OTE_REAL:
		SMS_fprintf(SMS_stdout, "%#g\n", GET_REAL(stack));
		break;
	case OTE_MARK:
		if (MARK_EXECUTABLE((*stack)) == true)
			SMS_fprintf(SMS_stdout, "{\n");
		else
			SMS_fprintf(SMS_stdout, "[\n");
		break;
	case OTE_ARRAY:
		ShowArray(stack);
		SMS_fprintf(SMS_stdout, "\n");
		break;
	case OTE_OPERATOR:
		SMS_fprintf(SMS_stdout, "--%s--\n", stack->un.pOperator->pKeyName);
		break;
	case OTE_DICTIONARY:
		SMS_fprintf(SMS_stdout, "--dictionary--\n");
		break;
	}
}

int iError(SMS sms, ERRORTYPE type, const char *filename, const int lineno, const char *opt1, const char *opt2)
{
	char *fname = basename(strdup(filename));

	if (opt1)
	{
		if (opt2)
			SMS_fprintf(SMS_stdout, " ##[%s - %s %s @ %s(L%d)]##\n", _errtxt[type], opt1, opt2, fname, lineno);
		else
			SMS_fprintf(SMS_stdout, " ##[%s - %s @ %s(L%d)]##\n", _errtxt[type], opt1, fname, lineno);
	}
	else
		SMS_fprintf(SMS_stdout, " ##[%s @ %s(L%d)]##\n", _errtxt[type], fname, lineno);

	for (int i = 0; i < ((SMSD *)sms)->iOPStackUsing; i++)
	{
		SMS_fprintf(SMS_stdout, "%3d| ", i);
		ShowOPStack(&((SMSD *)sms)->OPStack[i], i);
	}

	return SMS_ERR;
}

void SmsMessage(
	SMS sms,
	const char *format,
	...)
{
	char message[MAX_PRINTF_STRING];

	va_list args;
	va_start(args, format);
	int rv = vsprintf(message, format, args);
	va_end(args);

	SMS_fprintf(SMS_stdout, " ##[SMS: %s]##\n", message);
}

void SmsHelp(SMS sms, const char *s1, const char *s2, const char *s3)
{
	if (s2 && s3)
		SMS_fprintf(SMS_stdout, "[%s] %s\n spec: %s\n", s1, s2, s3);
	else
		SMS_fprintf(SMS_stdout, "%s\n", s1);
	return;
}

int main(int argc, char *argv[])
{
	SMS sms = pSmsInitialize();
	SmsMainLoop(sms);
	SmsTerminate(sms);
	return 0;
}