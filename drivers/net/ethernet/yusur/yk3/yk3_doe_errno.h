/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_DOE_ERRNO_H_
#define __YK3_DOE_ERRNO_H_
/******************************************************************************/
// error code:
//	0~255   : linux error code
//	256~299 : bug error code
//	300~399 : driver error code
//      400~    : hardware error code
enum yk3_doe_error_code {
	E_DOE_BUG	= 256,
	E_DOE_SWERR	= 300,
	E_DOE_HWERR	= 400,

	E_DOE_HWCLOSED	= E_DOE_SWERR + 0, // hardware closed
	E_DOE_TIMEOUT	= E_DOE_SWERR + 1, // command timeout
	E_DOE_NOTSUPP	= E_DOE_SWERR + 2, // not support(like @ VM)
	E_DOE_INVALID	= E_DOE_SWERR + 3, // invalid api param
	E_DOE_EXIST	= E_DOE_SWERR + 4, // table/entry exist
	E_DOE_NOTEXIST	= E_DOE_SWERR + 5, // table/entry not exist
	E_DOE_EXCEED	= E_DOE_SWERR + 6, // too more table/entry
	E_DOE_NOMEM	= E_DOE_SWERR + 7,
	E_DOE_FULL	= E_DOE_SWERR + 8,
	E_DOE_EMPTY	= E_DOE_SWERR + 9,
	E_DOE_NOCMD	= E_DOE_SWERR + 10,
	E_DOE_USING	= E_DOE_SWERR + 11,

	E_DOE_BUG_EMPTY_CB		= E_DOE_BUG + 1, // bug: send empty command buffer
	E_DOE_BUG_RD_NOT_SYNC		= E_DOE_BUG + 2,
	E_DOE_BUG_FAST_POOL_EMPTY	= E_DOE_BUG + 3,
};

static inline int
doe_hw_err(int hw_err) {
	return -(E_DOE_HWERR + hw_err);
}

/******************************************************************************/
#endif /* __YK3_DOE_ERRNO_H_ */
