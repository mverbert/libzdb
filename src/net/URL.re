/*
 * Copyright (C) 2008 Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "Config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "URL.h"


/**
 * Implementation of the URL interface. URL_create() does not 
 * handle wide character code but URL_new() does. The scanner 
 * handle ISO Latin 1 or UTF-8 encoded url's transparently.
 *
 * @version \$Id: URL.re,v 1.32 2008/03/20 11:28:54 hauk Exp $
 * @file
 */


/* ----------------------------------------------------------- Definitions */

typedef struct param_t {
        char *name;
        char *value;
        struct param_t *next;
} *param_t;

#define T URL_T
struct T {
	int port;
       	char *ref;
	char *path;
	char *host;
	char *user;
        char *qptr;
	char *query;
	char *portStr;
	char *protocol;
	char *password;
	char *toString;
        param_t params;
        char **paramNames;
	uchar_t *data;
	uchar_t *buffer;
	uchar_t *marker, *ctx, *limit, *token;
        /* Keep the above align with zild URL_T */
};

/* Unsafe URL characters: [00-1F, 7F-FF] <>\"#%{}|\\^[] ` */
static const uchar_t urlunsafe[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static const uchar_t b2x[][256] = {
        "00", "01", "02", "03", "04", "05", "06", "07", 
        "08", "09", "0A", "0B", "0C", "0D", "0E", "0F", 
        "10", "11", "12", "13", "14", "15", "16", "17", 
        "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", 
        "20", "21", "22", "23", "24", "25", "26", "27", 
        "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", 
        "30", "31", "32", "33", "34", "35", "36", "37", 
        "38", "39", "3A", "3B", "3C", "3D", "3E", "3F", 
        "40", "41", "42", "43", "44", "45", "46", "47", 
        "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", 
        "50", "51", "52", "53", "54", "55", "56", "57", 
        "58", "59", "5A", "5B", "5C", "5D", "5E", "5F", 
        "60", "61", "62", "63", "64", "65", "66", "67", 
        "68", "69", "6A", "6B", "6C", "6D", "6E", "6F", 
        "70", "71", "72", "73", "74", "75", "76", "77", 
        "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
        "80", "81", "82", "83", "84", "85", "86", "87", 
        "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
        "90", "91", "92", "93", "94", "95", "96", "97", 
        "98", "99", "9A", "9B", "9C", "9D", "9E", "9F", 
        "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", 
        "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", 
        "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", 
        "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF", 
        "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", 
        "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
        "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", 
        "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", 
        "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", 
        "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF", 
        "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", 
        "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"
};


#define BUFFER_SIZE 8192
#define UNKNOWN_PORT -1
#define YYCTYPE       uchar_t
#define YYCURSOR      U->buffer  
#define YYLIMIT       U->limit  
#define YYMARKER      U->marker
#define YYMARKER      U->marker
#define YYCTXMARKER   U->ctx
#define YYFILL(n)     ((void)0)
#define YYTOKEN       U->token
#define SET_PROTOCOL(PORT) *(YYCURSOR-3)=0; \
	U->protocol=U->token;U->port=PORT;goto parse
#define STRDUP(s) (s?Str_dup(s):NULL)
#define STRNDUP(s, n) (s?Str_ndup(s, n):NULL)
	

/* ------------------------------------------------------------ Prototypes */


static int x2b(char *x);
static int parseURL(T U);
static void setParams(T U);
static void freeParams(param_t p);


/* ----------------------------------------------------- Protected methods */

#ifdef ZILD_PACKAGE_PROTECTED
#pragma GCC visibility push(hidden)
#endif

T URL_new(const char *url) {
	T U;
	if (! (url && *url))
		return NULL;
#ifndef ZILD_PACKAGE_PROTECTED
        Exception_init();
#endif
	NEW(U);
	U->data = (uchar_t*)Str_dup(url);
	YYCURSOR = U->data;
	U->port = UNKNOWN_PORT;
	YYLIMIT = U->data + strlen(U->data);
	if (parseURL(U)) {
                if (U->query) 
                        setParams(U);
		return U;
        } 
	URL_free(&U);
	return NULL;
}


T URL_create(const char *url, ...) {
	int n;
	va_list ap;
	uchar_t buf[BUFFER_SIZE];
	if (! (url && *url))
		return NULL;
	va_start(ap, url);
	n = vsnprintf(buf, BUFFER_SIZE, url, ap);
	va_end(ap);
	if (n < 0 || n >= BUFFER_SIZE) 
		return NULL;
	return URL_new(buf);
}


void URL_free(T *U) {
	assert(U && *U);
        if ((*U)->params) freeParams((*U)->params);
        FREE((*U)->paramNames);
	FREE((*U)->toString);
	FREE((*U)->portStr);
	FREE((*U)->query);
	FREE((*U)->data);
	FREE((*U)->host);
	FREE(*U);
}


/* ------------------------------------------------------------ Properties */


const char *URL_getProtocol(T U) {
	assert(U);
	return U->protocol;
}


const char *URL_getUser(T U) {
	assert(U);
	return U->user;
}


const char *URL_getPassword(T U) {
	assert(U);
	return U->password;
}


const char *URL_getHost(T U) {
	assert(U);
	return U->host;
}


int URL_getPort(T U) {
	assert(U);
	return U->port;
}


const char *URL_getPath(T U) {
	assert(U);
	return U->path;
}


const char *URL_getQueryString(T U) {
	assert(U);
	return U->query;
}


const char **URL_getParameterNames(T U) {
        assert(U);
        if (U->params && (U->paramNames == NULL)) {
                param_t p;
                int i = 0, len = 0;
                for (p = U->params; p; p = p->next) len++;
                U->paramNames = ALLOC((len + 1) * sizeof(*U->paramNames));
                for (p = U->params; p; p = p->next)
                        U->paramNames[i++] = p->name;
                U->paramNames[i] = NULL;
        }
	return (const char **)U->paramNames;
}


const char *URL_getParameter(T U, const char *name) {
	assert(U);
        if (U->params && name) {
                param_t p;
                for (p = U->params; p; p = p->next) {
                        if (Str_isByteEqual(p->name, name))
                                return p->value;
                }
        }
        return NULL;
}


const char *URL_toString(T U) {
	assert(U);
	if (U->toString == NULL) {
		U->toString = Str_cat("%s://%s%s%s%s%s%s%s%s%s%s", 
                                      U->protocol,
                                      U->user?U->user:"",
                                      U->password?":":"",
                                      U->password?U->password:"",
                                      U->user?"@":"",
                                      U->host?U->host:"",
                                      U->portStr?":":"",
                                      U->portStr?U->portStr:"",
                                      U->path?U->path:"",
                                      U->query?"?":"",
                                      U->query?U->query:""); 
	}
	return U->toString;
}


char *URL_unescape(char *url) {
	if (url && *url) {
                register int x, y;
                for (x=0, y=0; url[y]; ++x, ++y) {
                        if ((url[x] = url[y]) == '+')
                                url[x] = ' ';
                        else if (url[x] == '%') {
                                url[x] = x2b(&url[y+1]);
                                y += 2;
                        }
                }
                url[x] = '\0';
        }
	return url;
}


char *URL_escape(const char *url) {
        char *escaped = NULL;
        if (url) {
                char *p = escaped = ALLOC(3 * strlen(url) + 1);
                for (; *url; url++) {
                        if (urlunsafe[(unsigned char)(*url)]) {
                                *p++= '%';
                                *p++= b2x[(unsigned char)(*url)][0];
                                *p++= b2x[(unsigned char)(*url)][1];
                        } else {
                                *(p++) = *url;
                        }
                }
                *p = 0;
        }
        return escaped;
}


char *URL_normalize(char *path) {
        char c;
	int i,j;
	if (! path)
		return NULL;
        for (i=j=0; (c=path[i]); ++i) {
                if (c=='/') {
                        while (path[i+1]=='/') ++i;	
                } else if (c=='.' && j && path[j-1]=='/') {
                        if (path[i+1]=='.' && (path[i+2]=='/' || path[i+2]==0)) {
                                if (j>1)
                                        for (j-=2; path[j]!='/' && j>0; --j);
                                i+=2;
                        } else if (path[i+1]=='/' || path[i+1]==0) {
                                ++i;
                                continue;
                        } else
                                c=':';
                }
                if (! (path[j]=path[i])) break; ++j;
	}
	if (! j) { path[0]='/'; j=1; }
	path[j]=0;
	if (path[0]=='/' && path[1]=='/') {
		for (i=j=0;(c=path[i]); ++i) {
			if (c=='/') {
				while (path[i+1]=='/') ++i;	
			}
			if (! (path[j]=path[i])) break; 
			++j;
		}
		path[j]=0;
	}
	return path;
}

#ifdef ZILD_PACKAGE_PROTECTED
#pragma GCC visibility pop
#endif

/* ------------------------------------------------------- Private methods */


static int parseURL(T U) {
	/*!re2c
	ws		= [ \t\r\n];
	any		= [\000-\377];
	protocol        = [a-zA-Z0-9]+"://";
	auth            = ([\040-\377]\[@])+[@];
	host            = ([a-zA-Z0-9\-]+)([.]([a-zA-Z0-9\-]+))*;
	port            = [:][0-9]+;
	path            = [/]([\041-\377]\[?#;])*;
	query           = ([\041-\377]\[#])*;
	parameterkey    = ([\041-\377]\[=])+;
	parametervalue  = ([\041-\377]\[&])*;
	*/
proto:
	if (YYCURSOR >= YYLIMIT)
		return false;
	YYTOKEN=  YYCURSOR;
	/*!re2c

        ws         {
                        goto proto;
		   }

        "mysql://" {
                      	SET_PROTOCOL(MYSQL_DEFAULT_PORT);
                   }
                   
        "postgresql://" {
                      	SET_PROTOCOL(POSTGRESQL_DEFAULT_PORT);
                   }

        protocol   {
                      	SET_PROTOCOL(UNKNOWN_PORT);
                   }
    
        any        {
                      	goto proto;
                   }
	*/
parse:
	if (YYCURSOR >= YYLIMIT)
		return true;
	YYTOKEN=  YYCURSOR;
	/*!re2c
    
        ws         { 
                        goto parse; 
                   }

        auth       {
                        char *p;
                        *(YYCURSOR-1) = 0;
                        U->user = YYTOKEN;
                        p = strchr(U->user, ':');
                        if (p) {
                                *(p++) = 0;
                                U->password = p;
                        }
                        goto parse; 
                   }

        host       {
                        U->host = STRNDUP(YYTOKEN, (YYCURSOR - YYTOKEN));
                        goto parse; 
                   }

        port       {
                        U->portStr = STRNDUP(YYTOKEN+1, (YYCURSOR-YYTOKEN-1));
                        U->port = Str_parseInt(U->portStr);
                        goto parse; 
                   }

        path       {
                        *YYCURSOR = 0;
                        U->path = YYTOKEN;
                        return true;
                   }
                   
        path[?]    {
                        *(YYCURSOR-1) = 0;
                        U->path = YYTOKEN;
                        goto query; 
                   }
                   
       any         {
                      	return true;
                   }
                   
	*/
query:
	if (YYCURSOR >= YYLIMIT)
		return true;
	YYTOKEN =  YYCURSOR;
	/*!re2c

        query      {
                        *YYCURSOR = 0;
                        U->qptr = YYTOKEN;
                        U->query = STRNDUP(YYTOKEN, (YYCURSOR-YYTOKEN));
                        return true;
                   }

        any        { 
                      return true;     
                   }
		   
	*/
        return false;
}


/*
 * Scan the query string for params. 
 * RFC 2396/3.3 URL path parameters are ignored in this version
 */
static void setParams(T U) {
	uchar_t *l, *s, *t;
        param_t param = NULL;
        s = U->qptr;
	l = s + strlen(s);
#undef YYCURSOR
#undef YYLIMIT
#undef YYMARKER
#undef YYFILL
#undef YYTOKEN
#define YYCURSOR        s
#define YYLIMIT         l
#define YYMARKER        
#define YYFILL(n)
#define YYTOKEN         t
start:
        if (YYCURSOR >= YYLIMIT)
                return;
	YYTOKEN = YYCURSOR;
	/*!re2c

        parameterkey {
                        /* No parameters, but a querystring */
                        return;
                   }
                      
	parameterkey/[=] {
                        NEW(param);
                        param->name = YYTOKEN;
                        param->next = U->params;
                        U->params = param;
                        goto start;
                   }
                   
	[=]parametervalue[&]? {
                        *YYTOKEN++= 0;
                        if (*(YYCURSOR-1)=='&')
                                *(YYCURSOR-1)= 0;
                        if (param==NULL) /* format error */
                                return; 
                         param->value = YYTOKEN;
			 goto start;
                   }
                   
        any        { 
	                 return;
		   }
	*/
}


static int x2b(char *x) {
	register int b;
	b = ((x[0] >= 'A') ? ((x[0] & 0xdf) - 'A')+10 : (x[0] - '0'));
	b *= 16;
	b += (x[1] >= 'A' ? ((x[1] & 0xdf) - 'A')+10 : (x[1] - '0'));
	return b;
}


static void freeParams(param_t p) {
        param_t q;
        for (;p; p = q) {
                q = p->next;
                FREE(p);
        }
}
