/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _YK3_DUMP_H
#define _YK3_DUMP_H
/******************************************************************************/
#include "yk3_base.h"

/* checkpatch: ignore REPEATED_WORD */
/*
 *           raw format(like UltraEdit)
 *
 *       :                                     ;
 *  Line :       Hexadecimal Content           ; Raw Content
 *       : 0 1 2 3  4 5 6 7  8 9 A B  C D E F  ;
 *       :                                     ;
 * xxxxH : 00112233 44556677 8899aabb ccddeeff ; cccccccccccccccc
 * xxxxH : 00112233 44556677 8899aabb ccddeeff ; cccccccccccccccc
 * xxxxH : 00112233 44556677 8899aabb ccddeeff ; cccccccccccccccc
 * xxxxH : 00112233 44556677 8899aabb ccddeeff ; cccccccccccccccc
 * xxxxH : 00112233 44556677 8899aabb ccddeeff ; cccccccccccccccc
 * xxxxH : 00112233 44556677 8899aa            ; ccccccccccc
 */

#define YK3_DUMP_LINE_BLOCK         4
#define YK3_DUMP_LINE_BLOCK_BYTES   4
#define YK3_DUMP_LINE_LIMIT         80
#define YK3_DUMP_LINE_BYTES         (YK3_DUMP_LINE_BLOCK_BYTES * YK3_DUMP_LINE_BLOCK)
#define YK3_DUMP_LINE_MAX (0 \
	+ 8 /* "xxxxH : " */ \
	+ (2 * YK3_DUMP_LINE_BLOCK_BYTES + 1) * YK3_DUMP_LINE_BLOCK /* 4*"xxxxxxxx " */ \
	+ 2 /* "; " */ \
	+ YK3_DUMP_LINE_BYTES /* "cccccccccccccccc" */ \
	+ 1 /* "\n" */ \
)

#if YK3_DUMP_LINE_MAX > YK3_DUMP_LINE_LIMIT
#error "must YK3_DUMP_LINE_MAX < YK3_DUMP_LINE_LIMIT"
#endif

#define YK3_DUMP_LINE_SEPARATOR \
	"=============================================================="
#define YK3_DUMP_LINE_SEPARATOR_SUB \
	"--------------------------------------------------------------"

#define YK3_DUMP_LINE_HEADER0 "      :                                     ;\n"
#define YK3_DUMP_LINE_HEADER1 " Line :       Hexadecimal Content           ; Raw Content\n"
#define YK3_DUMP_LINE_HEADER2 "      : 0 1 2 3  4 5 6 7  8 9 A B  C D E F  ;\n"
#define YK3_DUMP_LINE_HEADER3 "      :                                     ;\n"

typedef void yk3_dump_line_f(void *ctx, const char *line);
void yk3_seq_dump_line(struct seq_file *seq, const char *line);

void yk3_dump_line_helper(int line, const void *raw, int len,
			  yk3_dump_line_f *dump_line, void *ctx);
void yk3_dump_buffer_helper(const void *buffer, int len,
			    yk3_dump_line_f *dump_line, void *ctx);

#define yk3_dump_line(_line, _raw, _len) yk3_dump_line_helper(_line, _raw, _len, NULL, NULL)

#define yk3_dump_buffer(_buf, _len) yk3_dump_buffer_helper(_buf, _len, NULL, NULL)

#define yk3_dump_buffer_by(_is_dump, _buf, _len) \
	do {                                         \
		if (_is_dump) {                          \
			yk3_dump_buffer(_buf, _len);         \
		}                                        \
	} while (0)
/******************************************************************************/
#endif /* _YK3_DUMP_H */
