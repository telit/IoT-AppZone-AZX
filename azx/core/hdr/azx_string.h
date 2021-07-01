/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef AZX_STRING_H
#define AZX_STRING_H
/**
 * @file azx_string.h
 * @version 1.0.2
 * @dependencies core/azx_log
 * @author Sorin Basca
 * @date 01/06/2017
 *
 * @brief String manipulation library
 *
 * This library is a providing a fast and stack safe way to extract various data
 * types ((sub)strings, integers, floats) from long-winded strings whose format
 * is previously known.
 */
#include "m2mb_types.h"
#include "azx_log.h"

/**
 * @brief Returns TRUE if string is in list.
 *
 * @param[in] list The list of strings to search in.
 * @param[in] string The string to find.
 *
 * @return `TRUE` if the string is inside the list.
 */
BOOLEAN azx_containsString(const CHAR* const* list, const CHAR* string);

/**
 * @brief Goes over the provided string and returns the first number.
 *
 * @param[in] from_here the string to consider.
 * @param[out] ok whether the result is valid.
 *
 * @return The first number found in the provided string.
 */
INT32 azx_getFirstNumber(const CHAR* from_here, INT32* ok);

/**
 * @brief Parses and extracts values from a string.
 *
 * Extracts values from a string based on a format. The format allows one of the
 * following:
 *  - regular character - match the character
 *  - `%%` - match `%`
 *  - `%*[^<list-of-chars>]` - skip until the list of chars is found
 *  - `%*[<list-of-chars>]` - skip while characters from the list are found
 *  - `%<number>[^<list-of-chars>]` - store into a string characters until either
 *    `<number>` are stored (`NUL` will be appended, so allow space for it), or one of
 *    the chars in `<list of chars>` is found. If there is not enough space
 *    (`<number>+1`) to store the full match, then the parsing will stop straight
 *    away.
 *  - `%<number>[<list-of-chars>]` - as above, only keep inserting while the
 *    characters are the same as in the list
 *  - `%``x` - extract 32 bit integer
 *  - `%``d` - extract 32 bit integer
 *  - `%``u` - extract unsigned 32 bit integer
 *  - `%``f` - extract float
 *  - `%``ld` - extract 64 bit integer
 *  - `%``lu` - extract unsigned 64 bit integer
 *  - `%``_` - extract location within string (useful for parsing substrings)
 *
 * Where <list-of-chars> is expected, the list should **not** have more than 128
 * expanded characters. Some characters need to be escaped with '\'. The list of
 * them is the following: `"\^]"`. Also ranges are expanded, for example `[a-z]` is
 * expanded to `[abcdefghijklmnopqrstuv]`.
 *
 * This parser is implemented to support a subset of what is supported by
 * `sscanf`. Ideally `sscanf` should be used, but it could cause issues for big strings as it uses the internal HEAP. This implementation uses the application HEAP giving more control of memory usage on a modem
 *
 * If items are added into the parser, they should be done in a way that keeps
 * the format strings accepted as `sscanf`-compliant.
 *
 * Example:
 *
 *     UINT8 mode = 0xFF;
 *     CHAR operator[32] = { 0 };
 *     UINT8 act = 0xFF;
 *
 *     const CHAR* at_rsp = AZX_ATI_SEND_COMMAND(AZX_AT_DEFAULT_TIMEOUT, "AT+COPS?");
 *     // at_rsp now contains: '+COPS: 0,0,"Cyta-Voda",0'
 *     const CHAR* cops_fmt = "%*[^+]+COPS: %d,%*[^,],\"%32[^\"]\",%d";
 *
 *     if (!at_rsp || 1 > azx_parseStringf(at_rsp, rsp_fmt, &mode, operator, &act))
 *     {
 *         AZX_LOG_WARN("Unexpected +COPS response\r\n");
 *     }
 *     else
 *     {
 *         AZX_LOG_INFO("Operator: %s, act: %d, mode: %d\r\n", operator, act, mode);
 *     }
 *
 * @param[in] str The string from which to extract
 * @param[in] format The rules to follow when extracting
 * @param[in] ... Variadic parameters where the information from `str` that is
 *     needed it extracted to. The number of arguments depend on the format
 *     string specified. If the wrong number is passed, or the types of the
 *     parameters are not aligned to the format string, the outcome is
 *     unpredictable.
 *
 * @return Number of extracted items
 */
INT32 azx_parseStringf(const CHAR* str, const CHAR* format, ...);

/**
 * @brief Parse hex strings into data buffer.
 *
 * Parses a message of the format "#SOME_NAME: <size>\r\n<bytes:00A325...>". It
 * converts the hex bytes into `UINT8` values and stored them in the provided
 * buffer.
 *
 * @param[in] response The AT response
 * @param[in] tag The tag in the response (for example "#SRECV:")
 * @param[in] max_size The maximum number of bytes to store in the buffer
 * @param[out] buffer The buffer where to store the bytes
 *
 * @return The number of bytes stored in the buffer. If `max_size` is less than
 *     bytes available, some bytes will be lost and the return will be the
 *     number of bytes that should have been stored. If there are no bytes to
 *     parse, this returns 0. In case of error -1 is returned.
 */
INT32 azx_parseHexByteMsg(const CHAR* response, const CHAR* tag, INT32 max_size, UINT8* buffer);

#endif /*AZX_STRING_H*/
