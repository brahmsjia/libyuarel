/**
 * Copyright (C) 2016,2017 Jack Engqvist Johansson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "yuarel.h"

/**
 * Parse a non null terminated string into an integer.
 *
 * str: the string containing the number.
 * len: Number of characters to parse.
 */
static inline int
natoi(const char *str, size_t len)
{
	int i, r = 0;
	for (i = 0; i < len; i++) {
		r *= 10;
		r += str[i] - '0';
	}

	return r;
}

/**
 * Check if a URL is relative (no scheme and hostname).
 *
 * url: the string containing the URL to check.
 *
 * Returns 1 if relative, otherwise 0.
 */
static inline int
is_relative(const char *url)
{
	return (*url == '/') ? 1 : 0;
}

/**
 * Parse the scheme of a URL by inserting a null terminator after the scheme.
 *
 * str: the string containing the URL to parse. Will be modified.
 *
 * Returns a pointer to the hostname on success, otherwise NULL.
 */
static inline char *
parse_scheme(char *str)
{
	char *s;

	/* If not found or first in string, return error */
	s = strchr(str, ':');
	if (s == NULL || s == str) {
		return NULL;
	}

	/* If not followed by two slashes, return error */
	if (s[1] == '\0' || s[1] != '/' || s[2] == '\0' || s[2] != '/') {
		return NULL;
	}

	*s = '\0'; // Replace ':' with NULL

	return s + 3;
}

/**
 * Find a character in a string, replace it with '\0' and return the next
 * character in the string.
 *
 * str: the string to search in.
 * find: the character to search for.
 *
 * Returns a pointer to the character after the one to search for. If not
 * found, NULL is returned.
 */
static inline char *
find_and_terminate(char *str, char find)
{
	str = strchr(str, find);
	if (NULL == str) {
		return NULL;
	}

	*str = '\0';
	return str + 1;
}

/* Yes, the following functions could be implemented as preprocessor macros
     instead of inline functions, but I think that this approach will be more
     clean in this case. */
static inline char *
find_fragment(char *str)
{
	return find_and_terminate(str, '#');
}

static inline char *
find_query(char *str)
{
	return find_and_terminate(str, '?');
}

static inline char *
find_path(char *str)
{
	return find_and_terminate(str, '/');
}

/**
 * Parse a URL string to a struct.
 *
 * url: pointer to the struct where to store the parsed URL parts.
 * u:   the string containing the URL to be parsed.
 *
 * Returns 0 on success, otherwise -1.
 */
int
yuarel_parse(struct yuarel *url, char *u)
{
	if (NULL == url || NULL == u) {
		return -1;
	}

	memset(url, 0, sizeof (struct yuarel));

	/* (Fragment) */
	url->fragment = find_fragment(u);

	/* (Query) */
	url->query = find_query(u);

	/* Relative URL? Parse scheme and hostname */
	if (!is_relative(u)) {
		/* Scheme */
		url->scheme = u;
		u = parse_scheme(u);
		if (u == NULL) {
			return -1;
		}

		/* Host */
		if ('\0' == *u) {
			return -1;
		}
		url->host = u;

		/* (Path) */
		url->path = find_path(u);

		/* (Credentials) */
		u = strchr(url->host, '@');
		if (NULL != u) {
			/* Missing credentials? */
			if (u == url->host) {
				return -1;
			}

			url->username = url->host;
			url->host = u + 1;
			*u = '\0';

			u = strchr(url->username, ':');
			if (NULL == u) {
				return -1;
			}

			url->password = u + 1;
			*u = '\0';
		}

		/* Missing hostname? */
		if ('\0' == *url->host) {
			return -1;
		}

		/* (Port) */
		u = strchr(url->host, ':');
		if (NULL != u && (NULL == url->path || u < url->path)) {
			*(u++) = '\0';
			if ('\0' == *u) {
				return -1;
			}

			if (url->path) {
				url->port = natoi(u, url->path - u - 1);
			} else {
				url->port = atoi(u);
			}
		}

		/* Missing hostname? */
		if ('\0' == *url->host) {
			return -1;
		}
	} else {
		/* (Path) */
		url->path = find_path(u);
	}

	return 0;
}

/**
 * Split a path into several strings.
 *
 * No data is copied, the slashed are used as null terminators and then
 * pointers to each path part will be stored in **parts. Double slashes will be
 * treated as one.
 *
 * path: the path to split.
 * parts: a pointer to an array of (char *) where to store the result.
 * max_parts: max number of parts to parse.
 */
int
yuarel_split_path(char *path, char **parts, int max_parts)
{
	int i = 0;

	if (NULL == path || '\0' == *path) {
		return -1;
	}

	do {
		/* Forward to after slashes */
		while (*path == '/') path++;

		if ('\0' == *path) {
			break;
		}

		parts[i++] = path;

		path = strchr(path, '/');
		if (NULL == path) {
			break;
		}

		*(path++) = '\0';
	} while (i < max_parts);

	return i;
}

int
yuarel_parse_query(char *query, char delimiter, struct yuarel_param *params, int max_params)
{
	int i = 0;

	if (NULL == query || '\0' == *query) {
		return -1;
	}

	params[i++].key = query;
	while (i < max_params && NULL != (query = strchr(query, delimiter))) {
		*query = '\0';
		params[i].key = ++query;
		params[i].val = NULL;

		/* Go back and split previous param */
		if (i > 0) {
			if ((params[i - 1].val = strchr(params[i - 1].key, '=')) != NULL) {
				*(params[i - 1].val)++ = '\0';
			}
		}
		i++;
	}

	/* Go back and split last param */
	if ((params[i - 1].val = strchr(params[i - 1].key, '=')) != NULL) {
		*(params[i - 1].val)++ = '\0';
	}

    int n = i;
    while (i-- > 0) {
        printf("-------%s\n", params[i].val==NULL?"":params[i].val);
        php_url_decode(params[i].val);
        printf("+++++++%s\n", params[i].val==NULL?"":params[i].val);
    }

	return n;
}

char * build_json(TS_YP * yp, char * q )
{
    int i=0;
    char *p = q;
    *p++ = '{';
    char sep[2] = {'\0'};
    while (NULL != yp[i].key) {
        printf("%s: %s\n", yp[i].key, yp[i].val == NULL ? "": yp[i].val);
        sep[0] = i == 0 ? '\0' : ',';
        sprintf(p, "%s\"%s\":\"%s\"", sep, yp[i].key, yp[i].val == NULL ? "": yp[i].val);
        p += (int)strlen(p);
        i++;
    }
    *p = '}';
    return q;
} 

char * build_query(TS_YP * yp, char * q )
{
    int i=0;
    char *p = q;
    char sep[2] = {'\0'};
    char encode[256] = {'\0'};
    while (NULL != yp[i].key) {
        sep[0] = i == 0 ? '\0' : '&';
        sprintf(p, "%s%s=%s", sep, yp[i].key, yp[i].val == NULL ? "": php_url_encode(yp[i].val,encode));
        p += (int)strlen(p);
        i++;
    }
    return q;
} 

static unsigned char HEXCHARS[] = "0123456789ABCDEF";
/**
 *  *  * 16进制数转换成10进制数
 *   *   * 如：0xE4=14*16+4=228
 *    *    */
static int php_htoi(char *s)
{
    int value;
    int c;

    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}

char * php_url_encode(char const *s, char *d)
{
    register unsigned char c;
    char *to, *start;
    char const *from, *end;
    
    from = s;
    end  = s + strlen(s);
    start = to = d;

    while (from < end) 
    {
        c = *from++;

        if (c == ' ') 
        {
            *to++ = '+';
        } 
        else if ((c < '0' && c != '-' && c != '.') ||
                 (c < 'A' && c > '9') ||
                 (c > 'Z' && c < 'a' && c != '_') ||
                 (c > 'z')) 
        {
            to[0] = '%';
            to[1] = HEXCHARS[c >> 4];//将2进制转换成16进制表示
            to[2] = HEXCHARS[c & 15];//将2进制转换成16进制表示
            to += 3;
        }
        else 
        {
            *to++ = c;
        }
    }
    *to = 0;
    return start;
}

/**
 *  * 更改源串内容
 *   * 返回解码后的串
 *    */
char * php_url_decode(char *s)
{
    char *dest = s;
    char *data = s;

    if (s==NULL) return NULL;

    int len = strlen(s);
    while (len--) 
    {
        if (*data == '+') 
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) 
        {
            *dest = (char) php_htoi(data + 1);
            data += 2;
            len -= 2;
        } 
        else 
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return s;
}

