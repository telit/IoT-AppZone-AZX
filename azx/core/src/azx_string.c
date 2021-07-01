/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "m2mb_types.h"

#include "azx_log.h"

#include "azx_string.h"

/**
 * Extracts an integer from a string and returns to the first element after the
 * integer.
 *
 * If the string does not begin with an integer (after any whitespace which
 * would be ignored), then this will return NULL.
 *
 * @param[in] str The string for where to look for an integer (from the start).
 * @param[out] val Where to store the value.
 *
 * @return Location in the string after the integer, or NULL if no integer is
 *     found.
 */
static const CHAR* extract_int(const CHAR* str, INT32* val)
{
  CHAR* end = NULL;
  *val = strtol(str, &end, 10);
  if((const CHAR*)end == str)
  {
    return NULL;
  }
  return end;
}

static UINT32 extract_hex(const CHAR* str, CHAR** end, INT32 base)
{
  const CHAR* p = str;
  UINT32 result = 0;
  UINT8 digits = 0;
  (void)base;

  if(p[0] == '0' && p[1] == 'x')
  {
    p += 2;
  }

  while(*p != '\0' && digits < 8)
  {
    UINT32 single_digit = 0;
    if(*p >= '0' && *p <= '9')
    {
      single_digit = *p - '0';
    }
    else if(*p >= 'A' && *p <= 'F')
    {
      single_digit = *p - 'A' + 10;
    }
    else if(*p >= 'a' && *p <= 'f')
    {
      single_digit = *p - 'a' + 10;
    }
    else
    {
      break;
    }
    ++p;
    ++digits;
    result <<= 4;
    result |= single_digit;
  }

  *end = (CHAR*)p;

  return result;
}

#define extract_number(num_t, fn, str, fmt, base) do { \
  num_t* arg = va_arg(va, num_t*); \
  CHAR* end = NULL; \
  errno = 0; \
  num_t val = fn(str, &end, base); \
  if((const CHAR*)end == str || errno != 0) \
  { \
    AZX_LOG_TRACE("No " #num_t " to extract (err = %d)\r\n", errno); \
    break; \
  } \
  str = end; \
  *arg = val; \
  ++extracted; \
  AZX_LOG_TRACE("Extracted " fmt ". Now: %s\r\n", val, str); \
} while(0)

/**
 * Extracts a float from a string and returns to the first element after the
 * float.
 *
 * If the string does not begin with a float (after any whitespace which
 * would be ignored), then this will return NULL.
 *
 * @param[in] str The string for where to look for an float (from the start).
 * @param[out] val Where to store the value.
 *
 * @return Location in the string after the float, or NULL if no float is
 *     found.
 */
static const CHAR* extract_float(const CHAR* str, FLOAT32* val)
{
  CHAR* end = NULL;
  *val = strtof(str, &end);
  if((const CHAR*)end == str)
  {
    return NULL;
  }
  return end;
}

INT32 azx_getFirstNumber(const CHAR* string, INT32* ok)
{
  const CHAR* p=string;
  INT32 is_ok = 0;
  INT32 result = 0;
  INT32 negative = 0;

  if(!p)
  {
    goto end;
  }

  for(; *p != '\0' && (*p < '0' || *p > '9'); ++p)
  {
    negative = (*p == '-' ? 1 : 0);
  }

  for(; *p >= '0' && *p <= '9'; ++p)
  {
    is_ok = 1;
    result = (result * 10) + (*p - '0');
  }
  if(negative)
  {
    result = -result;
  }

end:
  if(ok)
  {
    *ok = is_ok;
  }
  return result;
}

#define MAX_ALLOWED_CHARS 128
struct AllowedChars
{
  BOOLEAN is_disallowed;
  CHAR chars[MAX_ALLOWED_CHARS+1];
};

/**
 * Extracts the list of characters are not allowed from a string under the form
 * "[^<list-of-chars>]", or characters that are allowed if the form is "[<list-of-chars>]".
 *
 * It allows escaping special characters with '\'.
 *
 * For example from [^abc], the string "abc" will be extracted. From [^\\\]\^a],
 * the string "\]^a" will be extracted.
 *
 * Also it allows range of characters. For example [a-z] will be expanded to
 * [abcdefghijklmnopqrstuvxyz] and [c-a] to [cba].
 *
 * @param[in] str The string from which to extract the data. Must start with "[^" and
 * contain some terminating "]".
 * @param[out] result The place where to store the result.
 *
 * @return Pointer in str where the parsing stops (first character after ']'),
 *     NULL in case of failure. If there is not enough space to get the result,
 *     a failure will be reported.
 */
static const CHAR* get_allowed_chars(const CHAR* str, struct AllowedChars* allowed_chars)
{
  CHAR* result = allowed_chars->chars;
  AZX_LOG_TRACE("Check: %s\r\n", str);

  /* Ensure the string starts as expected */
  if(!str || str[0] != '[')
  {
    return NULL;
  }
  ++str;

  if(str[0] == '^')
  {
    ++str;
    AZX_LOG_TRACE("Getting disallowed characters (%s)\r\n", str);
    allowed_chars->is_disallowed = TRUE;
  }
  else
  {
    AZX_LOG_TRACE("Getting allowed characters (%s)\r\n", str);
    allowed_chars->is_disallowed = FALSE;
  }

  /* Collect characters until we run out of string, of space, or find the end
   * marker. Allow for the NUL termination of result. */
  while(*str != '\0' && *str != ']' && (result - allowed_chars->chars < MAX_ALLOWED_CHARS))
  {
    AZX_LOG_TRACE("Getting allowed characters (%s)\r\n", str);
    /* If we find the escaping character, just store the following character. */
    if(*str == '\\')
    {
      ++str;
      if(*str == '\0')
      {
        /* Something should follow an escaping character, so return error */
        return NULL;
      }
    }
    if(*(str+1) == '-')
    {
      /* This is a range, so expand it */
      CHAR start = *(str++);
      CHAR end = *(++str);
      CHAR direction = 1;

      if(end == '\\')
      {
        /* Read past the escaping */
        end = *(++str);
      }
      ++str; /* Move behind the end character */

      if(start == '\0' || end == '\0')
      {
        /* If someone has the range without a start or end character, it's their own fault! */
        return NULL;
      }
      AZX_LOG_TRACE("Extract range %c-%c (%s)\r\n", start, end, str);
      if(start > end)
      {
        direction = -1;
      }
      while(((start-direction) != end) &&
          (result - allowed_chars->chars < MAX_ALLOWED_CHARS))
      {
        AZX_LOG_TRACE("Adding %c\r\n", start);
        *(result++) = start;
        start += direction;
      }
      AZX_LOG_TRACE("Done adding range, now %c-%c\r\n", start, end);
      if(start-direction != end)
      {
        /* The whole range cannot be recorded, so return an error */
        return NULL;
      }
      continue;
    }
    AZX_LOG_TRACE("Adding %c\r\n", *str);
    /* Simply store the next character and advance both strings. */
    *(result++) = *(str++);
  }

  *result = '\0';
  /* If we don't have the terminating character, then something must have gone
   * wrong, so return NULL, otherwise point to the following character. */
  return (*str == ']' ? str+1 : NULL);
}

/* Define some "utility function". It would be nice to define them as actual
 * functions, rather than macros, but it may look uglier when you call them,
 * as you'd need to pass address of pointers and also add a check which would
 * break in case of failure.
 */
#undef MATCH_AND_ADVANCE
#define MATCH_AND_ADVANCE(f, s) \
  if(*s != *f) \
  { \
    break; \
  } \
  ++s; \
  ++f

static BOOLEAN is_allowed(CHAR c, const struct AllowedChars* allowed_chars)
{
  return (allowed_chars->is_disallowed ?
    (NULL == strchr(allowed_chars->chars, c) ? TRUE : FALSE) :
    (NULL != strchr(allowed_chars->chars, c) ? TRUE : FALSE));
}

BOOLEAN azx_containsString(const CHAR* const* list, const CHAR* string)
{
  for(; *list != NULL; ++list)
  {
    if(strcmp(string, *list) == 0)
    {
      return TRUE;
    }
  }
  return FALSE;
}

#define EXTRACT_LOCATION() do { \
  const CHAR** arg = va_arg(va, const CHAR**); \
  *arg = str; \
  ++extracted; \
  AZX_LOG_TRACE("Extracted location: %s\r\n", str); \
} while(0)

INT32 azx_parseStringf(const CHAR* str, const CHAR* format, ...)
{
  static struct AllowedChars allowed_chars = {0};
  INT32 extracted = 0;

  va_list va;

  AZX_LOG_TRACE("Parsing %s\r\n", str);

  if(!str || !format)
  {
    /* Nothing to extract really */
    AZX_LOG_TRACE("Nothing to extract");
    return extracted;
  }

  va_start(va, format);

  /* Go through the format and match characters one by one, unless a % character
   * is found. In that case either escape, skip or extract information from the
   * string. */
  while(*format != '\0' && *str != '\0')
  {
    if(*format == '%')
    {
      ++format;

      if(*format == '%')
      { /* Match '%' */
        MATCH_AND_ADVANCE(format, str);
        AZX_LOG_TRACE("Matched %%. Now: %s\r\n", str);
      }

      else if(*format == '_')
      { /* Extract location */
        ++format;
        EXTRACT_LOCATION();
      }

      else if(*format == '*')
      { /* Skip according to allowed chars rules */
        ++format;
        format = get_allowed_chars(format, &allowed_chars);
        if(!format)
        {
          AZX_LOG_TRACE("No format for allowed chars\r\n");
          break;
        }
        while(*str != '\0' && is_allowed(*str, &allowed_chars))
        {
          ++str;
        }
        AZX_LOG_TRACE("Skipped. Now: %s\r\n", str);
      }

      else if(*format == 'x')
      { /* Read an integer */
        ++format;
        extract_number(UINT32, extract_hex, str, "0x%X", 16);
      }

      else if(*format == 'd')
      { /* Read an integer */
        ++format;
        extract_number(INT32, strtol, str, "%d", 10);
      }

      else if(*format == 'u')
      { /* Read an integer */
        ++format;
        extract_number(UINT32, strtoul, str, "%u", 10);
      }

      else if(format[0] == 'l' && format[1] == 'd')
      { /* Read an integer */
        format += 2;
        extract_number(INT64, strtoll, str, "%ld", 10);
      }

      else if(format[0] == 'l' && format[1] == 'u')
      { /* Read an integer */
        format += 2;
        extract_number(UINT64, strtoull, str, "%lu", 10);
      }


      else if(*format == 'f')
      { /* Read an integer */
        FLOAT32* arg = va_arg(va, FLOAT32*);
        FLOAT32 val = 0;
        ++format;
        str = extract_float(str, &val);
        if(!str)
        {
          AZX_LOG_TRACE("No FLOAT32 to extract\r\n");
          break;
        }
        *arg = val;
        ++extracted;
        AZX_LOG_TRACE("Extracted %f. Now: %s\r\n", val, str);
      }

      else
      { /* Read a string */
        INT32 count = 0;
        CHAR* arg = va_arg(va, CHAR*);
        CHAR* a = arg;
        /* First check the maximum size allowed for the string. */
        format = extract_int(format, &count);
        if(!format || count < 1)
        {
          AZX_LOG_TRACE("No size to extract\r\n");
          break;
        }
        /* Then get the list of characters which would signal the termination of
         * the string that is extracted. */
        format = get_allowed_chars(format, &allowed_chars);
        if(!format)
        {
          AZX_LOG_TRACE("No allowed chars to extract\r\n");
          break;
        }
        /* Go until either the destination space is finished, a terminating
         * character is found, or the source string finishes. */
        while(count > 0 && *str != '\0' && is_allowed(*str, &allowed_chars))
        {
          /* Extract the string and advance */
          *(arg++) = *(str++);
          --count;
        }
        *arg = '\0';
        /* If the destination space is finished without extracting everything,
         * return. */
        if(*str != '\0' && is_allowed(*str, &allowed_chars))
        {
          AZX_LOG_TRACE("Dest finished without extracting\r\n");
          break;
        }
        AZX_LOG_TRACE("Extracted %s. Now: %s\r\n", a, str);
        (void) a;
        ++extracted;
      }

    }
    else
    { /* Match the next character */
      AZX_LOG_TRACE("Advancing (%c,%c). Now: %s\r\n", *format, *str, str);
      MATCH_AND_ADVANCE(format, str);
    }
  }

  if(*str == '\0')
  {
    while(format[0] == '%' && format[1] == '_')
    {
      format += 2;
      EXTRACT_LOCATION();
    }
  }

  va_end(va);

  AZX_LOG_TRACE("Done. Extracted total %d. Now: %s\r\n", extracted, str);
  return extracted;
}

static const CHAR* go_to_size_field(const CHAR* p)
{
  p = strchr(p, ':');
  if(!p)
  {
    return NULL;
  }
  ++p;
  while(*p == ' ')
  {
    ++p;
  }

  if(*p == '\0')
  {
    return NULL;
  }
  return p;
}

static const CHAR* go_to_next_line(const CHAR* p)
{
  while(*p)
  {
    if(p[0] == '\r' && p[1] == '\n')
    {
      return &p[2];
    }
    else if(p[0] == '\r' || p[0] == '\n')
    {
      return &p[1];
    }
    ++p;
  }
  return NULL;
}

INT32 azx_parseHexByteMsg(const CHAR* response, const CHAR* tag, INT32 max_size, UINT8* buffer)
{
  const CHAR* p = NULL;
  INT32 i = 0;
  INT32 size = 0;

  /* format is "#SOME_NAME: <size>\r\n<bytes:00A325...> */
  if(NULL == (p = strstr(response, tag)) ||
      (NULL == (p = go_to_size_field(p))) ||
      (1 > azx_parseStringf(p, "%*[^0-9]%d", &size)) ||
      (NULL == (p = go_to_next_line(p))))
  {
    AZX_LOG_ERROR("Cannot find tag, size, or end of line\r\n");
    return -1;
  }

  AZX_LOG_DEBUG("Parsing %d bytes\r\n", size);
  if(size > max_size)
  {
    AZX_LOG_WARN("There are more bytes (%d) than space available (%d). "
        "Some bytes will be lost\r\n", size, max_size);
  }
  else
  {
    max_size = size;
  }

  for(i = 0; i < max_size; ++i)
  {
    CHAR str[3] = {0};
    INT32 val = 0;
    CHAR* end = NULL;

    str[0] = p[i*2];
    str[1] = p[i*2+1];
    val = strtol(str, &end, 16);

    if(end == NULL || *end != '\0')
    {
      AZX_LOG_ERROR("Unable to parse %s into a value, aborting\r\n", str);
      return -1;
    }

    buffer[i] = (UINT8)(val & 0xFF);
  }

  AZX_LOG_DEBUG("Finished parsing %d/%d bytes\r\n", max_size, size);
  return size;
}
