#ifndef _MYS_H_
#define _MYS_H_

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <iostream>
#include <limits.h>
#include <glob.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <future>
#include <termios.h>

#define OPSTACK_SAVE(mys) int level = ((MYSD *)mys)->iOPStackUsing
#define OPSTACK_RESTORE(mys) ((MYSD *)mys)->iOPStackUsing = level
#define OPSTACK_POP(mys, n) ((MYSD *)mys)->iOPStackUsing -= n

typedef void *MYS;
extern FILE *MYS_stdout;

#define MYS_MAX 512
#define MYS_MAX_STRING 4096
#define MYS_MAX_STACKSIZE (1024 * 16)

typedef enum
{
	OTE_NAME = 0,
	OTE_STRING,
	OTE_WIDESTRING,
	OTE_BOOL,
	OTE_INTEGER,
	OTE_REAL,
	OTE_MARK,
	OTE_ARRAY,
	OTE_OPERATOR,
	OTE_DICTIONARY,
	OTE_NULL,
	OTE_ASYNCTASK,
	OTE_FILE
} OBJ_TYPE_ENUM;

typedef enum
{
	VTE_STRING = 0,
	VTE_WIDESTRING = 0,
	VTE_NAME,
	VTE_ARRAY,
	VTE_DICTIONARY
} VAR_TYPE_ENUM;

typedef int (*MYS_OPERATOR_FUNC)(MYS mys);

typedef struct MYS_OPERATOR
{
	const char *pKeyName;
	MYS_OPERATOR_FUNC pIntOperator;
	const char *pOperatorParams;
	const char *pOperatorSpec;
} MYS_OPERATOR;

typedef struct MYS_VAR
{
	unsigned int iSeqNo;
	struct MYS_VAR *pVarNext;
	struct MYS_VAR *pVarBefore;
	VAR_TYPE_ENUM VarType;
	int iLength;
	int iBufferLength;
	union
	{
		char *pName;
		char *pString;
		struct MYS_OBJ *pObjInArray;
		struct MYS_DICT_ENTRY *pDictEntry;
	} un;
} MYS_VAR;

typedef struct MYS_OBJ
{
	OBJ_TYPE_ENUM ObjType;
	bool bExecutable;
	union
	{
		long iInteger;
		double dReal;
		const MYS_OPERATOR *pOperator;
		MYS_VAR *pVar;
		void *pData;
		FILE *fp;
	} un;
} MYS_OBJ;

typedef struct MYS_DICT_ENTRY
{
	struct MYS_OBJ ObjKey;
	struct MYS_OBJ ObjValue;
} MYS_DICT_ENTRY;

typedef struct MYS_TOKEN
{
	unsigned int uBufferLength;
	char *pBuffer;
	///
	unsigned int uLength;
	int iNestString;
} MYS_TOKEN;

typedef struct
{
	FILE *fpIn;
	MYS_VAR *pVarFirst;
	MYS_VAR *pVarLast;
	MYS_TOKEN *pToken;
	int iOPStackUsing;
	MYS_OBJ OPStack[MYS_MAX_STACKSIZE];
	int iDictStackUsing;
	MYS_OBJ DictStack[32];
	unsigned int iVarSeqNo;
	bool bPrompt;
	int iNestMainLoop;
	int iNestExecArray;
	bool bDebugMessage;
	char FontDir[256];
	char ResDir[256];
	std::vector<std::future<void>> ayncTasks;
} MYSD;

#define CURRENTDICT(mys) &((MYSD *)mys)->DictStack[((MYSD *)mys)->iDictStackUsing - 1]
#define DICTENTRY(pObjDict, index) &(pObjDict->un.pVar->un.pDictEntry[index])

#define GET_DICTENTRY(obj) ((obj)->un.pVar->un.pDictEntry)
#define GET_OBJINARRAY(obj) ((obj)->un.pVar->un.pObjInArray)
#define GET_NAME(obj) ((obj)->un.pVar->un.pName)
#define GET_STRING(obj) ((obj)->un.pVar->un.pString)
#define GET_INTEGER(obj) ((obj)->un.iInteger)
#define GET_REAL(obj) ((obj)->un.dReal)
#define GET_FILE(obj) ((obj)->un.fp)

#define STACK_LEVEL(mys) ((MYSD *)mys)->iOPStackUsing
#define MARK_EXECUTABLE(obj) (obj.bExecutable)
#define STACK_OBJ_FIRST(mys, index) (&((MYSD *)mys)->OPStack[index])
#define STACK_OBJ_LAST(mys, index) (&((MYSD *)mys)->OPStack[((MYSD *)mys)->iOPStackUsing + index - 1])
#define STACK_OBJ_NEXT(mys) (&((MYSD *)mys)->OPStack[((MYSD *)mys)->iOPStackUsing])

#define TOKEN(mys) ((MYSD *)mys)->pToken
#define B_OPENEXECARRAY(mys) (((MYSD *)mys)->iNestExecArray > 0 ? true : false)

void AddOperatorToDict(MYS mys, char *pName, const MYS_OPERATOR *pOperator);

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
	COMMAND_EXEC_ERROR
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
		"command execution error",
};

int iError(MYS mys, ERRORTYPE type, const char *file, int lineno, const char *opt1 = nullptr, const char *opt2 = nullptr);
int iExecute(MYS mys);
bool bObjCompare(MYS_OBJ *pObj1, MYS_OBJ *pObj2);

int iMysParseString(MYS mys, char *str);
int MysMainLoop(MYS mys);
void MysTerminate(MYS mys);

bool bRegistObjToDict(MYS mys, MYS_OBJ *pObjDict, MYS_OBJ *pObjName, MYS_OBJ *pObjValue);
bool bTokenCompleted(MYS_TOKEN *pToken);

void CheckMemory(char *title);

void EnumMemory(MYS mys);

bool bGetInteger(MYS mys, MYS_OBJ *pObj, int *pInteger, const char *pTitle);
bool bGetString(MYS mys, MYS_OBJ *pObj, char **ppString, const char *pTitle);
bool bGetReal(MYS mys, MYS_OBJ *pObj, double *pReal, const char *pTitle);

int iCloseExecArray(MYS mys);
int iCloseArray(MYS mys);
void InitDictOperators(MYS mys);

void *MemoryAlloc(size_t size, const char *name, int line);
void MemoryFree(void *ptr);
void *MemoryRealloc(void *ptr, size_t size);

MYS_OBJ *pObjSearchInDicts(MYS mys, MYS_OBJ *ObjKey);
MYS_OBJ *pObjSearchInDicts(MYS mys, MYS_OBJ *ObjKey);
MYS_OBJ *pObjSearchInTheDict(MYS mys, MYS_OBJ *ObjDict, MYS_OBJ *ObjKey);
MYS_OBJ *pObjSearchInTheDictWithString(MYS mys, MYS_OBJ *pDict, const char *pKeyString);

MYS_TOKEN *pTokenInitialize();
char *pTokenBuffer(MYS_TOKEN *pToken);
char *pTokenFromString(MYS mys, char *cp_InStr);
MYS_VAR *pVarCreate(MYS mys, VAR_TYPE_ENUM VarType, void *vp_Value);
void PushToken(MYS mys, char *c_Token);

void PushNull(MYS mys);
void PushString(MYS mys, char *c_String);
#define CP_UTF16 1200
void PushName(MYS mys, bool b_Exec, char *c_String);
void PushReal(MYS mys, double dReal);
void PushInteger(MYS mys, long iInteger);
void PushBool(MYS mys, long iInteger);
void PushMark(MYS mys, bool bExec);
void PushObj(MYS mys, MYS_OBJ *pObj);
void PushFile(MYS mys, FILE *fp);
void PushDict(MYS mys, MYS_OBJ *pObjDict);
void PopDict(MYS mys);

void ShowArray(MYS_OBJ *pObj);
void ShowOPStack(MYS_OBJ *stack, int index);

void MysHelp(MYS mys, const char *s1, const char *s2, const char *s3);
void MysMessage(
	MYS mys,
	const char *format,
	...);
void TokenTerminate(MYS_TOKEN *token);
void TokenPutchar(MYS_TOKEN *pToken, char Code);
void TokenReset(MYS_TOKEN *pToken);

unsigned int uTokenLength(MYS_TOKEN *pToken);

void VarDelete(MYS mys, MYS_VAR *var);
void VarClean(MYS mys);

// MYS内部の文字コードはUTF8に統一（CPRapiとのインターフェースはUCS）
// Windowsコンソールに対するUTF8のprintfが正常に動作しないので，UCSに変換して表示する事にした

#define MYS_printf printf
#define MYS_fprintf fprintf

// #define MYS_ERROR_MSG "[ ** MYS ERROR:"

int __opr_def(MYS mys);
int __opr_exch(MYS mys);
int __opr_pop(MYS mys);

// 戻り値に関する仕様
//  1: 正常
//  0: exit
// -1: エラー
// -2: EOF

#define MYS_OK 1
#define MYS_EXIT 0
#define MYS_EOF -1
#define MYS_ERR -2
#define MYS_QUIT -3

#define MEGA (double)(1024 * 1024)

#endif //_MYS_H_