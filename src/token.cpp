#include "mysh.h"

MYS_TOKEN *pTokenInitialize()
{
	MYS_TOKEN *pToken = (MYS_TOKEN *)MemoryAlloc(sizeof(MYS_TOKEN), "pTokenInitialize", __LINE__);
	pToken->uLength = 0;
	pToken->uBufferLength = 512;
	pToken->pBuffer = (char *)MemoryAlloc(pToken->uBufferLength, "pTokenInitialize", __LINE__);
	pToken->iNestString = 0;
	return pToken;
}

void TokenTerminate(MYS_TOKEN *pToken)
{
	MemoryFree(pToken->pBuffer);
	MemoryFree(pToken);
}

void TokenReset(MYS_TOKEN *pToken)
{
	pToken->uLength = 0;
	pToken->iNestString = 0;
}

void TokenPutchar(MYS_TOKEN *pToken, char Code)
{
	if (pToken->uLength > pToken->uBufferLength)
	{
		pToken->uBufferLength += 512;
		pToken->pBuffer = (char *)MemoryRealloc(pToken->pBuffer, pToken->uBufferLength);
	}
	pToken->pBuffer[pToken->uLength++] = Code;
	pToken->pBuffer[pToken->uLength] = 0x00;
}

bool bTokenCompleted(MYS_TOKEN *pToken)
{
	if (pToken->iNestString == 0)
		return true;
	else
		return false;
}

unsigned int uTokenLength(MYS_TOKEN *pToken)
{
	return pToken->uLength;
}

char *pTokenBuffer(MYS_TOKEN *pToken)
{
	return pToken->pBuffer;
}

static char gc_Separators[] = " {}[]()<>\n\t/%;\\";

char *pTokenFromString(MYS mys, char *cp_InStr)
{
	MYS_TOKEN *pToken = ((MYSD *)mys)->pToken;
	char *cp_in = cp_InStr;
	bool bFound = false;

	do
	{
		char *cp_pos = strchr(gc_Separators, *cp_in);
		if (cp_pos == NULL)
		{
			TokenPutchar(pToken, *cp_in++);
			bFound = true;
		}
		else
		{
			switch (*cp_pos)
			{
			case '/':
				if (pToken->iNestString)
				{
					TokenPutchar(pToken, *cp_in++);
				}
				else if (bFound)
				{
					TokenPutchar(pToken, 0x00);
					return cp_in;
				}
				else
				{
					bFound = true;
					TokenPutchar(pToken, *cp_in++);
				}
				break;

			case '\n':
			case '\t':
			case ' ':
				if (pToken->iNestString)
				{
					TokenPutchar(pToken, *cp_in++);
				}
				else if (bFound)
				{
					TokenPutchar(pToken, 0x00);
					return ++cp_in;
				}
				else
				{
					cp_in++;
				}
				break;

			case '\\':
				if (pToken->iNestString)
				{
					switch (*++cp_in)
					{
					case 'n':
						TokenPutchar(pToken, '\n');
						cp_in++;
						break;
					case 't':
						TokenPutchar(pToken, '\t');
						cp_in++;
						break;
					case '0':
						TokenPutchar(pToken, 0x00);
						cp_in++;
						break;
					default:
						TokenPutchar(pToken, '\\');
						TokenPutchar(pToken, *cp_in);
						cp_in++;
						break;
					}
				}
				break;

			case '(':
				if (pToken->iNestString)
				{
					pToken->iNestString++;
					TokenPutchar(pToken, *cp_in++);
				}
				else if (bFound)
				{
					TokenPutchar(pToken, 0x00);
					return cp_in; // leave '('
				}
				else
				{
					pToken->iNestString++;
					TokenPutchar(pToken, *cp_in++);
				}
				break;

			case ')':
				if (pToken->iNestString > 1)
				{
					pToken->iNestString--;
					TokenPutchar(pToken, *cp_in++);
				}
				else if (pToken->iNestString > 0)
				{
					pToken->iNestString--;
					TokenPutchar(pToken, *cp_in);
					TokenPutchar(pToken, 0x00);
					return ++cp_in;
				}
				else
				{
					iError(mys, INVALIDSEQUENCE, __func__, __LINE__, "Cannot use ')' without '('");
					return NULL;
				}
				break;

			case '{':
			case '}':
			case '[':
			case ']':
				if (pToken->iNestString)
				{
					TokenPutchar(pToken, *cp_in++);
				}
				else if (bFound)
				{
					TokenPutchar(pToken, 0x00);
					return cp_in; // leave '['
				}
				else
				{
					TokenPutchar(pToken, *cp_in);
					TokenPutchar(pToken, 0x00);
					return ++cp_in; // not leave '['
				}
				break;

			case '%':
			case ';':
				if (pToken->iNestString)
				{
					TokenPutchar(pToken, *cp_in++);
				}
				else if (bFound)
				{
					TokenPutchar(pToken, 0x00);
					cp_in += strlen(cp_in);
					return cp_in;
				}
				else
				{
					cp_in += strlen(cp_in);
					return cp_in;
				}
				break;

			default:
				bFound = true;
				TokenPutchar(pToken, *cp_in++);
				break;
			}
		}
	} while (*cp_in != 0x00);

	return cp_in;
}

void PushToken(MYS mys, char *c_Token)
{
	char c_Buffer[MYS_MAX_STRING];

	if (((MYSD *)mys)->bDebugMessage)
		MYS_fprintf(MYS_stdout, "\t\t\t\t\t\t\tDBG TOKEN[%d] %s\n", STACK_LEVEL(mys), c_Token);

	switch (*c_Token)
	{
	case '(':
		memcpy(c_Buffer, c_Token + 1, strlen(c_Token + 1));
		c_Buffer[strlen(c_Token + 1) - 1] = 0x0;
		PushString(mys, c_Buffer);
		break;
	case '[':
		PushMark(mys, false);
		break;
	case '{':
		((MYSD *)mys)->iNestExecArray++;
		PushMark(mys, true);
		break;
	case '}':
		if (((MYSD *)mys)->iNestExecArray > 0)
		{
			((MYSD *)mys)->iNestExecArray--;
		}
		else
		{
			MYS_fprintf(MYS_stdout, "** MYS ERROR: '}' is used without '{'\n");
			return;
		}
		PushName(mys, true, c_Token);
		break;
	case '/':
		PushName(mys, false, c_Token + 1);
		break;

	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (strchr(c_Token, '.'))
		{
			PushReal(mys, atof(c_Token));
		}
		else
		{
			PushInteger(mys, atoi(c_Token));
		}
		break;

	case '.':
		PushReal(mys, atof(c_Token));
		break;

	default: // name executable
		PushName(mys, true, c_Token);
		break;
	}
}
