/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2013 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  nseel-eval.c

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <string.h>
#include <ctype.h>
#include "ns-eel-int.h"
#include "../wdlcstring.h"

#ifdef NSEEL_SUPER_MINIMAL_LEXER

  int nseellex(opcodeRec **output, YYLTYPE * yylloc_param, compileContext *scctx)
  {
    int rv,toklen=0;
    *output = 0;
    while ((rv=scctx->rdbuf[0]) && (rv== ' ' || rv=='\t' || rv == '\r' || rv == '\n')) scctx->rdbuf++;

    if (rv)
    {
      char buf[NSEEL_MAX_VARIABLE_NAMELEN*2];
      int l;
      char *ss = scctx->rdbuf++;
      if (isalpha(rv) || rv == '_')
      {
        while ((rv=scctx->rdbuf[0]) && (isalnum(rv) || rv == '_' || rv == '.')) scctx->rdbuf++;
        l = scctx->rdbuf - ss + 1;
        if (l > sizeof(buf)) l=sizeof(buf);
        lstrcpyn_safe(buf,ss,l);      

        rv=0;
        *output = nseel_lookup(scctx,&rv,buf);
      }
      else if (rv == '$')
      {
        int ok=1;
        switch (scctx->rdbuf[0])
        {
          case 'x':
          case 'X':
            scctx->rdbuf++;
            while ((rv=scctx->rdbuf[0]) && ((rv>='0' && rv<='9') || (rv>='a' && rv<='f') || (rv>='A' && rv<='F'))) scctx->rdbuf++;
          break;
          case '~':
            scctx->rdbuf++;
            while ((rv=scctx->rdbuf[0]) && (rv>='0' && rv<='9')) scctx->rdbuf++;
          break;
          case '\'':
            if (scctx->rdbuf[1] && scctx->rdbuf[2] == '\'') scctx->rdbuf += 3;
          break;
          default:
            if (toupper(scctx->rdbuf[0]) == 'P')
            {
              if (toupper(scctx->rdbuf[1]) == 'I') scctx->rdbuf+=2;
              else if (toupper(scctx->rdbuf[1]) == 'H' && toupper(scctx->rdbuf[2]) == 'I') scctx->rdbuf+=3;
              else ok=0;
            }
            else if (toupper(scctx->rdbuf[0]) == 'E')
            {
              scctx->rdbuf++;
            }
            else 
            {
              ok=0;
            }
          break;
        }
        if (ok)
        {
          l = scctx->rdbuf - ss + 1;
          if (l > sizeof(buf)) l=sizeof(buf);
          lstrcpyn_safe(buf,ss,l);
          *output = nseel_translate(scctx,buf);
          rv=VALUE;
        }
      }
      else if ((rv >= '0' && rv <= '9') || (rv == '.' && (scctx->rdbuf[0] >= '0' && scctx->rdbuf[0] <= '9')))
      {
        if (rv == '0' && (scctx->rdbuf[0] == 'x' || scctx->rdbuf[0] == 'X'))
        {
          scctx->rdbuf++;
          while ((rv=scctx->rdbuf[0]) && ((rv>='0' && rv<='9') || (rv>='a' && rv<='f') || (rv>='A' && rv<='F'))) scctx->rdbuf++;
        }
        else
        {
          int pcnt=rv == '.';
          while ((rv=scctx->rdbuf[0]) && ((rv>='0' && rv<='9') || (rv == '.' && !pcnt++))) scctx->rdbuf++;       
        }
        l = scctx->rdbuf - ss + 1;
        if (l > sizeof(buf)) l=sizeof(buf);
        lstrcpyn_safe(buf,ss,l);
        *output = nseel_translate(scctx,buf);
        rv=VALUE;
      }
      else
      {
        if (rv == '<')
        {
          const char nc=*scctx->rdbuf;
          if (nc == '<')
          {
            scctx->rdbuf++;
            rv=TOKEN_SHL;
          }
          else if (nc == '=')
          {
            scctx->rdbuf++;
            rv=TOKEN_LTE;
          }
        }
        else if (rv == '>')
        {
          const char nc=*scctx->rdbuf;
          if (nc == '>')
          {
            scctx->rdbuf++;
            rv=TOKEN_SHR;
          }
          else if (nc == '=')
          {
            scctx->rdbuf++;
            rv=TOKEN_GTE;
          }
        }
        else if (rv == '=')
        {
          if (*scctx->rdbuf == '=')
          {
            scctx->rdbuf++;
            if (*scctx->rdbuf == '=')
            {
              scctx->rdbuf++;
              rv=TOKEN_EQ_EXACT;
            }
            else
              rv=TOKEN_EQ;
          }
        }
        else if (rv == '!')
        {
          if (*scctx->rdbuf == '=')
          {
            scctx->rdbuf++;
            if (*scctx->rdbuf == '=')
            {
              scctx->rdbuf++;
              rv=TOKEN_NE_EXACT;
            }
            else
              rv=TOKEN_NE;
          }
        }
        else if (rv == '&' && *scctx->rdbuf == '&')
        {
          scctx->rdbuf++;
          rv = TOKEN_LOGICAL_AND;
        }      
        else if (rv == '|' && *scctx->rdbuf == '|')
        {
          scctx->rdbuf++;
          rv = TOKEN_LOGICAL_OR;
        }
        else if (rv != 0 && *scctx->rdbuf == '=')
        {         
          switch (rv)
          {
            case '+': rv=TOKEN_ADD_OP; scctx->rdbuf++; break;
            case '-': rv=TOKEN_SUB_OP; scctx->rdbuf++; break;
            case '%': rv=TOKEN_MOD_OP; scctx->rdbuf++; break;
            case '|': rv=TOKEN_OR_OP;  scctx->rdbuf++; break;
            case '&': rv=TOKEN_AND_OP; scctx->rdbuf++; break;
            case '~': rv=TOKEN_XOR_OP; scctx->rdbuf++; break;
            case '/': rv=TOKEN_DIV_OP; scctx->rdbuf++; break;
            case '*': rv=TOKEN_MUL_OP; scctx->rdbuf++; break;
            case '^': rv=TOKEN_POW_OP; scctx->rdbuf++; break;
          }
        }
      }
      toklen = scctx->rdbuf - ss;
    }
  
    yylloc_param->first_column = scctx->rdbuf - scctx->rdbuf_start - toklen;
    return rv;
  }
  void nseelerror(YYLTYPE *pos,compileContext *ctx, const char *str)
  {
    ctx->errVar=pos->first_column>0?pos->first_column:1;
  }
#else

  int nseel_gets(compileContext *ctx, char *buf, size_t sz)
  {
    int n=0;
    if (ctx->inputbufferptr) while (n < sz)
    {
      char c=ctx->inputbufferptr[0];
      if (!c) break;
      if (c == '/' && ctx->inputbufferptr[1] == '*')
      {
        ctx->inputbufferptr+=2; // skip /*

        while (ctx->inputbufferptr[0] && (ctx->inputbufferptr[0]  != '*' || ctx->inputbufferptr[1] != '/'))  ctx->inputbufferptr++;
        if (ctx->inputbufferptr[0]) ctx->inputbufferptr+=2; // skip */
        continue;
      }

      ctx->inputbufferptr++;
      buf[n++] = c;
    }
    return n;

  }


  //#define EEL_TRACE_LEX

  #ifdef EEL_TRACE_LEX
  #define nseellex nseellex2

  #endif
  #include "lex.nseel.c"

  #ifdef EEL_TRACE_LEX

  #undef nseellex

  int nseellex(YYSTYPE * yylval_param, YYLTYPE * yylloc_param , yyscan_t yyscanner)
  {
    int a=nseellex2(yylval_param,yylloc_param,yyscanner);

    char buf[512];
    sprintf(buf,"tok: %c (%d)\n",a,a);
    OutputDebugString(buf);
    return a;
  }
  #endif//EEL_TRACE_LEX


  void nseelerror(YYLTYPE *pos,compileContext *ctx, const char *str)
  {
    ctx->errVar=pos->first_column>0?pos->first_column:1;
    ctx->errVar_l = pos->first_line;
  }
#endif // !NSEEL_SUPER_MINIMAL_LEXER
