#ifndef _SMS_H_
#define _SMS_H_

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <iostream>
#include <limits.h>
#include <glob.h>
#include <math.h>

#define OPSTACK_SAVE(sms) int level = ((SMSD *)sms)->iOPStackUsing
#define OPSTACK_RESTORE(sms) ((SMSD *)sms)->iOPStackUsing = level
#define OPSTACK_POP(sms, n) ((SMSD *)sms)->iOPStackUsing -= n

typedef void *SMS;
extern FILE *SMS_stdout;

#define SMS_MAX 512
#define SMS_MAX_STRING 4096
#define SMS_MAX_STACKSIZE (1024 * 16)

typedef enum
{
	OTE_NAME = 0,
	OTE_STRING,
	OTE_WIDESTRING,
	OTE_boolEAN,
	OTE_INTEGER,
	OTE_REAL,
	OTE_MARK,
	OTE_ARRAY,
	OTE_OPERATOR,
	OTE_DICTIONARY,
	OTE_NULL
} OBJ_TYPE_ENUM;

typedef enum
{
	VTE_STRING = 0,
	VTE_WIDESTRING = 0,
	VTE_NAME,
	VTE_ARRAY,
	VTE_DICTIONARY
} VAR_TYPE_ENUM;

typedef int (*SMS_OPERATOR_FUNC)(SMS sms);

typedef struct SMS_OPERATOR
{
	const char *pKeyName;
	SMS_OPERATOR_FUNC pIntOperator;
	const char *pOperatorParams;
	const char *pOperatorSpec;
} SMS_OPERATOR;

typedef struct SMS_VAR
{
	unsigned int iSeqNo;
	struct SMS_VAR *pVarNext;
	struct SMS_VAR *pVarBefore;
	VAR_TYPE_ENUM VarType;
	int iLength;
	int iBufferLength;
	union
	{
		char *pName;
		char *pString;
		struct SMS_OBJ *pObjInArray;
		struct SMS_DICT_ENTRY *pDictEntry;
	} un;
} SMS_VAR;

typedef struct SMS_OBJ
{
	OBJ_TYPE_ENUM ObjType;
	bool bExecutable;
	union
	{
		long iInteger;
		double dReal;
		const SMS_OPERATOR *pOperator;
		SMS_VAR *pVar;
		void *pData;
	} un;
} SMS_OBJ;

typedef struct SMS_DICT_ENTRY
{
	struct SMS_OBJ ObjKey;
	struct SMS_OBJ ObjValue;
} SMS_DICT_ENTRY;

typedef struct SMS_TOKEN
{
	unsigned int uBufferLength;
	char *pBuffer;
	///
	unsigned int uLength;
	int iNestString;
} SMS_TOKEN;

typedef struct
{
	FILE *fpIn;
	SMS_VAR *pVarFirst;
	SMS_VAR *pVarLast;
	SMS_TOKEN *pToken;
	int iOPStackUsing;
	SMS_OBJ OPStack[SMS_MAX_STACKSIZE];
	int iDictStackUsing;
	SMS_OBJ DictStack[32];
	unsigned int iVarSeqNo;
	bool bPrompt;
	int iNestMainLoop;
	int iNestExecArray;
	bool bDebugMessage;
	char FontDir[256];
	char ResDir[256];
} SMSD;

#define CURRENTDICT(sms) &((SMSD *)sms)->DictStack[((SMSD *)sms)->iDictStackUsing - 1]
#define DICTENTRY(pObjDict, index) &(pObjDict->un.pVar->un.pDictEntry[index])

#define GET_DICTENTRY(obj) ((obj)->un.pVar->un.pDictEntry)
#define GET_OBJINARRAY(obj) ((obj)->un.pVar->un.pObjInArray)
#define GET_NAME(obj) ((obj)->un.pVar->un.pName)
#define GET_STRING(obj) ((obj)->un.pVar->un.pString)
#define GET_INTEGER(obj) ((obj)->un.iInteger)
#define GET_REAL(obj) ((obj)->un.dReal)

#define STACK_LEVEL(sms) ((SMSD *)sms)->iOPStackUsing
#define MARK_EXECUTABLE(obj) (obj.bExecutable)
#define STACK_OBJ_FIRST(sms, index) (&((SMSD *)sms)->OPStack[index])
#define STACK_OBJ_LAST(sms, index) (&((SMSD *)sms)->OPStack[((SMSD *)sms)->iOPStackUsing + index - 1])
#define STACK_OBJ_NEXT(sms) (&((SMSD *)sms)->OPStack[((SMSD *)sms)->iOPStackUsing])

#define TOKEN(sms) ((SMSD *)sms)->pToken
#define B_OPENEXECARRAY(sms) (((SMSD *)sms)->iNestExecArray > 0 ? true : false)

void AddOperatorToDict(SMS sms, char *pName, const SMS_OPERATOR *pOperator);

typedef enum
{
	TYPECHECK,
	UNDEFINED,
	DICTFULL,
	INVALIDSEQUENCE,
	STACKUNDERFLOW,
	INVALIDDATA,
	RANGECHECK,
	DICTSTACKUNDERFLOW,
	NOTSUPPORTED,
	PRINTPARAMMISMATCH,
} ERRORTYPE;

static const char *_errtxt[] =
	{
		"typecheck",
		"undefined",
		"dictful",
		"invalidsequence",
		"stackunderflow",
		"invaliddata",
		"rangecheck",
		"dictstack-underflow",
		"not-supported",
		"print-param-mismatch",
};

int iError(SMS sms, ERRORTYPE type, const char *file, int lineno, const char *opt1 = nullptr, const char *opt2 = nullptr);
int iExecute(SMS sms);
bool bObjCompare(SMS_OBJ *pObj1, SMS_OBJ *pObj2);

int iSmsParseString(SMS sms, char *str);
int SmsMainLoop(SMS sms);
void SmsTerminate(SMS sms);

bool bRegistObjToDict(SMS sms, SMS_OBJ *pObjDict, SMS_OBJ *pObjName, SMS_OBJ *pObjValue);
bool bTokenCompleted(SMS_TOKEN *pToken);

void CheckMemory(char *title);

void EnumMemory(SMS sms);

bool bGetInteger(SMS sms, SMS_OBJ *pObj, int *pInteger, const char *pTitle);
bool bGetString(SMS sms, SMS_OBJ *pObj, char **ppString, const char *pTitle);
bool bGetReal(SMS sms, SMS_OBJ *pObj, double *pReal, const char *pTitle);

int iCloseExecArray(SMS sms);
int iCloseArray(SMS sms);
void InitDictOperators(SMS sms);

void *MemoryAlloc(size_t size, const char *name, int line);
void MemoryFree(void *ptr);
void *MemoryRealloc(void *ptr, size_t size);

SMS_OBJ *pObjSearchInDicts(SMS sms, SMS_OBJ *ObjKey);
SMS_OBJ *pObjSearchInDicts(SMS sms, SMS_OBJ *ObjKey);
SMS_OBJ *pObjSearchInTheDict(SMS sms, SMS_OBJ *ObjDict, SMS_OBJ *ObjKey);
SMS_OBJ *pObjSearchInTheDictWithString(SMS sms, SMS_OBJ *pDict, const char *pKeyString);

SMS_TOKEN *pTokenInitialize();
char *pTokenBuffer(SMS_TOKEN *pToken);
char *pTokenFromString(SMS sms, char *cp_InStr);
SMS_VAR *pVarCreate(SMS sms, VAR_TYPE_ENUM VarType, void *vp_Value);
void PushToken(SMS sms, char *c_Token);

void PushNull(SMS sms);
void PushString(SMS sms, char *c_String);
#define CP_UTF16 1200
void PushName(SMS sms, bool b_Exec, char *c_String);
void PushReal(SMS sms, double dReal);
void PushInteger(SMS sms, long iInteger);
void Pushboolean(SMS sms, long iInteger);
void PushMark(SMS sms, bool bExec);
void PushObj(SMS sms, SMS_OBJ *pObj);
void PushDict(SMS sms, SMS_OBJ *pObjDict);
void PopDict(SMS sms);

void ShowArray(SMS_OBJ *pObj);
void ShowOPStack(SMS_OBJ *stack, int index);

void SmsHelp(SMS sms, const char *s1, const char *s2, const char *s3);
void SmsMessage(
	SMS sms,
	const char *format,
	...);
void TokenTerminate(SMS_TOKEN *token);
void TokenPutchar(SMS_TOKEN *pToken, char Code);
void TokenReset(SMS_TOKEN *pToken);

unsigned int uTokenLength(SMS_TOKEN *pToken);

void VarDelete(SMS sms, SMS_VAR *var);
void VarClean(SMS sms);

// SMS内部の文字コードはUTF8に統一（CPRapiとのインターフェースはUCS）
// Windowsコンソールに対するUTF8のprintfが正常に動作しないので，UCSに変換して表示する事にした

#define SMS_printf printf
#define SMS_fprintf fprintf

// #define SMS_ERROR_MSG "[ ** SMS ERROR:"

int __opr_def(SMS sms);
int __opr_exch(SMS sms);
int __opr_pop(SMS sms);

// 戻り値に関する仕様
//  1: 正常
//  0: exit
// -1: エラー
// -2: EOF

#define SMS_OK 1
#define SMS_EXIT 0
#define SMS_EOF -1
#define SMS_ERR -2
#define SMS_QUIT -3

#define MEGA (double)(1024 * 1024)

#endif //_SMS_H_