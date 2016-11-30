/*
 * STB params variable manipulation
 *
 * Copyright (C) 2016, Broadcom. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id:stbutils.h $
 */

#ifndef _stbutils_h_
#define _stbutils_h_

#ifndef _LANGUAGE_ASSEMBLY

#include <typedefs.h>
#include <bcmdefs.h>

struct param_tuple {
	char *name;
	char *value;
	struct param_tuple *next;
};

/*
 * Initialize STB params file access.
 */
extern int stbpriv_init(osl_t *osh);
extern void stbpriv_exit(osl_t *osh);

/*
 * Append a chunk of nvram variables to the list
 */
extern int stbparam_append(osl_t *osh, char *paramlst, uint paramsz);

/*
 * Get the value of STB param variable
 * @param	name	name of variable to get
 * @return	value of variable or NULL if undefined
 */
extern char * stbparam_get(const char *name);

/*
 * Match STB param variable.
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is string equal
 *		to match or FALSE otherwise
 */
static INLINE int
stbparam_match(const char *name, const char *match)
{
	const char *value = stbparam_get(name);
	return (value && !strcmp(value, match));
}

/*
 * Get all STB variables (format name=value\0 ... \0\0).
 * @param	buf	buffer to store variables
 * @param	count	size of buffer in bytes
 * @return	0 on success and errno on failure
 */
extern int stbparams_getall(char *buf, int count);


#endif /* _LANGUAGE_ASSEMBLY */

#endif /* _stbutils_h_ */
