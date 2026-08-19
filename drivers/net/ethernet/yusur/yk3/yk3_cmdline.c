// SPDX-License-Identifier: GPL-2.0

#ifndef YK3_APP_CMDLINE
#define YK3_APP_CMDLINE 0
#endif

#if YK3_APP_CMDLINE
#include <string.h>
#define kstrdup(x, f)	strdup(x)
#define cmdline_debug	eprint
#else
#include "yk3_base.h"
#define cmdline_debug(fmt, args...)	yk3_do_nothing()
#endif

#include "yk3_cmdline.h"
/******************************************************************************/
#if YK3_CMDLINE
static char *yk3_cmdline;
#if !YK3_APP_CMDLINE
module_param_named(cmdline, yk3_cmdline, charp, 0444);
MODULE_PARM_DESC(yk3_cmdline, "yk3 driver cmdline");
#endif
/******************************************************************************/
// per function
//	1. config by dev
//	2. or inheritance from card or numa or global
enum yk3_cmdline_domain {
	YK3_CMDLINE_DOMAIN_GLOBAL	= 0,
	YK3_CMDLINE_DOMAIN_NUMA		= 1,
	YK3_CMDLINE_DOMAIN_CARD		= 2,
	YK3_CMDLINE_DOMAIN_DEV		= 3,

	YK3_CMDLINE_DOMAIN_END
};

enum yk3_cmdline_charset {
	YK3_CMDLINE_CHARSET_UNKNOWN	= -1,

	YK3_CMDLINE_CHARSET_SPACE	= 0,	// [ \t\r\n]
	YK3_CMDLINE_CHARSET_GLOBAL	= 1,	// *
	YK3_CMDLINE_CHARSET_CONTEXT	= 2,	// [0-9a-zA-Z_:.]
	YK3_CMDLINE_CHARSET_LB		= 3,	// {
	YK3_CMDLINE_CHARSET_RB		= 4,	// }
	YK3_CMDLINE_CHARSET_EQ		= 5,	// =
	YK3_CMDLINE_CHARSET_COMMA	= 6,	// ,
	YK3_CMDLINE_CHARSET_EOL		= 7,	// ;
	YK3_CMDLINE_CHARSET_EOF		= 8,	// '\0'

	YK3_CMDLINE_CHARSET_END
};

static const char *cmdline_charset_names[YK3_CMDLINE_CHARSET_END] = {
	[YK3_CMDLINE_CHARSET_SPACE]	= "space",
	[YK3_CMDLINE_CHARSET_GLOBAL]	= "global",
	[YK3_CMDLINE_CHARSET_CONTEXT]	= "context",
	[YK3_CMDLINE_CHARSET_LB]	= "lb",
	[YK3_CMDLINE_CHARSET_RB]	= "rb",
	[YK3_CMDLINE_CHARSET_EQ]	= "eq",
	[YK3_CMDLINE_CHARSET_COMMA]	= "comma",
	[YK3_CMDLINE_CHARSET_EOL]	= "eol",
	[YK3_CMDLINE_CHARSET_EOF]	= "eof",
};

static inline bool
is_cmdline_charset_eof(int charset) {
	return charset == YK3_CMDLINE_CHARSET_EOF;
}

static inline bool
is_good_cmdline_charset(int charset) {
	return is_good_enum(charset, YK3_CMDLINE_CHARSET_END);
}

static inline const char *
cmdline_charset_name(int charset) {
	return is_good_cmdline_charset(charset) ? cmdline_charset_names[charset] : YK3_UNKNOWN;
}

static enum yk3_cmdline_charset
cmdline_charset(int ch)
{
	switch (ch) {
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return YK3_CMDLINE_CHARSET_SPACE;
	case '*':
		return YK3_CMDLINE_CHARSET_GLOBAL;
	case '{':
		return YK3_CMDLINE_CHARSET_LB;
	case '}':
		return YK3_CMDLINE_CHARSET_RB;
	case '=':
		return YK3_CMDLINE_CHARSET_EQ;
	case ',':
		return YK3_CMDLINE_CHARSET_COMMA;
	case ';':
		return YK3_CMDLINE_CHARSET_EOL;
	case '\0':
		return YK3_CMDLINE_CHARSET_EOF;
	case '0' ... '9':
	case 'a' ... 'z':
	case 'A' ... 'Z':
	case '_':
	case ':': // bdf/card
	case '.': // bdf
		return YK3_CMDLINE_CHARSET_CONTEXT;
	default:
		yk3_err("invalid cmdline char[%d/%c]", ch, ch);
		return YK3_CMDLINE_CHARSET_UNKNOWN;
	}
}

enum yk3_cmdline_fsm {
	YK3_CMDLINE_FSM_INIT	= 0,
	YK3_CMDLINE_FSM_DOMAIN	= 1,
	YK3_CMDLINE_FSM_LB	= 2,
	YK3_CMDLINE_FSM_RB	= 3,
	YK3_CMDLINE_FSM_KEY	= 4,
	YK3_CMDLINE_FSM_VALUE	= 5,
	YK3_CMDLINE_FSM_EQ	= 6,
	YK3_CMDLINE_FSM_COMMA	= 7,
	YK3_CMDLINE_FSM_EOL	= 8,

	YK3_CMDLINE_FSM_END,
	YK3_CMDLINE_FSM_STOP	= YK3_CMDLINE_FSM_END,
};

static const char *cmdline_fsm_names[YK3_CMDLINE_FSM_END] = {
	[YK3_CMDLINE_FSM_INIT]	= "init",
	[YK3_CMDLINE_FSM_DOMAIN] = "domain",
	[YK3_CMDLINE_FSM_LB]	= "lb",
	[YK3_CMDLINE_FSM_RB]	= "rb",
	[YK3_CMDLINE_FSM_KEY]	= "key",
	[YK3_CMDLINE_FSM_VALUE]	= "value",
	[YK3_CMDLINE_FSM_EQ]	= "eq",
	[YK3_CMDLINE_FSM_COMMA]	= "comma",
	[YK3_CMDLINE_FSM_EOL]	= "eol",
};

static inline bool
is_good_cmdline_fsm(int fsm) {
	return is_good_enum(fsm, YK3_CMDLINE_FSM_END);
}

static inline const char *
cmdline_fsm_name(int fsm) {
	return is_good_cmdline_fsm(fsm) ? cmdline_fsm_names[fsm] : YK3_UNKNOWN;
}

struct yk3_cmdline_ident {
	enum yk3_cmdline_domain domain;

	union {
		int numa;
		struct yk3_card_id card;
		struct yk3_bdf bdf;
	};
};

struct yk3_cmdline_inst {
	struct yk3_cmdline_ident ident;
	struct list_head node;
	struct yk3_cmdline_cfg cfg;
};

struct yk3_cmdline {
	int count;

	struct {
		int count;
		struct list_head list;
	} inst[YK3_CMDLINE_DOMAIN_END];
};

struct yk3_cmdline_parser {
	char *buf;
	char *p_start;
	char *p_end;
	char *key;
	u64 value[YK3_CMDLINE_VALUE_COUNT];

	enum yk3_cmdline_fsm	fsm;
	enum yk3_cmdline_domain domain;
	int backup;
	int numa;
	int value_count;

	enum yk3_cmdline_charset c_start;
	enum yk3_cmdline_charset c_end;

	struct yk3_bdf bdf;
	struct yk3_card_id card;
};

static struct yk3_cmdline cmdline;
#include "yk3_cmdline.c.def"
/******************************************************************************/
static void
cmdline_init(void)
{
	int i;

	for (i = 0; i < YK3_CMDLINE_DOMAIN_END; i++)
		INIT_LIST_HEAD(&cmdline.inst[i].list);
}

static inline bool
is_good_cmdline_field(enum yk3_cmdline_field field) {
	return field < YK3_CMDLINE_FIELD_END;
}

static inline bool
is_cmdline_raw_var(struct yk3_cmdline_var *var) {
	return !(var->flag & YK3_CMDLINE_VAR_REF);
}

static inline bool
is_cmdline_raw_array(struct yk3_cmdline_var *var) {
	return (var->type == YK3_CMDLINE_VAR_ARRAY) && is_cmdline_raw_var(var);
}

static inline void
cmdline_array_set(struct yk3_cmdline_var *var, int idx, u64 value) {
	var->array[idx] = value;
}

static inline void
cmdline_value_set(struct yk3_cmdline_var *var, u64 value) {
	var->value = value;
}

static int
cmdline_array_create(struct yk3_cmdline_var *var, u32 count)
{
	var->type	= YK3_CMDLINE_VAR_ARRAY;
	var->subtype	= 0;
	var->flag	= YK3_CMDLINE_VAR_ENABLE;
	var->count	= count;

	var->array = kcalloc(count, sizeof(u64), GFP_KERNEL);
	if (!var->array)
		return -ENOMEM;

	return 0;
}

static void
cmdline_array_destroy(struct yk3_cmdline_var *var)
{
	if (is_cmdline_raw_array(var))
		kfree(var->array);
}

static void
cmdline_var_release(struct yk3_cmdline_var *var)
{
	switch (var->type) {
	case YK3_CMDLINE_VAR_ARRAY:
		cmdline_array_destroy(var);
		break;
	default:
		yk3_do_nothing();
		break;
	}
}

static struct yk3_cmdline_var
cmdline_var_clone(struct yk3_cmdline_var *var)
{
	struct yk3_cmdline_var tmp = *var;

	tmp.flag |= YK3_CMDLINE_VAR_REF;

	return tmp;
}

static struct yk3_cmdline_var
cmdline_var_getby(struct yk3_cmdline_ident *ident,
		  enum yk3_cmdline_domain domain,
		  enum yk3_cmdline_field field)
{
	struct yk3_cmdline_inst *inst;
	struct yk3_cmdline_var var = {0};

	list_for_each_entry(inst, &cmdline.inst[domain].list, node) {
		if (yk3_objeq(ident, &inst->ident)) {
			var = cmdline_var_clone(&inst->cfg.field[field]);

			return var;
		}
	}

	return var;
}

static struct yk3_cmdline_var
cmdline_var_getbyglobal(enum yk3_cmdline_field field)
{
	struct yk3_cmdline_ident ident = { .domain = YK3_CMDLINE_DOMAIN_GLOBAL };

	return cmdline_var_getby(&ident, YK3_CMDLINE_DOMAIN_GLOBAL, field);
}

static struct yk3_cmdline_var
cmdline_var_getbynuma(int numa, enum yk3_cmdline_field field)
{
	struct yk3_cmdline_ident ident = {
		.domain = YK3_CMDLINE_DOMAIN_NUMA,
		.numa = numa,
	};
	struct yk3_cmdline_var var;

	var = cmdline_var_getby(&ident, YK3_CMDLINE_DOMAIN_NUMA, field);
	if (is_yk3_cmdline_var_enable(&var))
		return var;

	return cmdline_var_getbyglobal(field);
}

static struct yk3_cmdline_var
cmdline_var_getbycard(struct yk3_card_id card, int numa, enum yk3_cmdline_field field)
{
	struct yk3_cmdline_ident ident = {
		.domain = YK3_CMDLINE_DOMAIN_CARD,
		.card = card,
	};
	struct yk3_cmdline_var var;

	var = cmdline_var_getby(&ident, YK3_CMDLINE_DOMAIN_CARD, field);
	if (is_yk3_cmdline_var_enable(&var))
		return var;

	return cmdline_var_getbynuma(numa, field);
}

static struct yk3_cmdline_var
cmdline_var_getbydev(struct yk3_bdf bdf, int numa, enum yk3_cmdline_field field)
{
	struct yk3_cmdline_ident ident = {
		.domain = YK3_CMDLINE_DOMAIN_DEV,
		.bdf = bdf,
	};
	struct yk3_cmdline_var var;
	struct yk3_card_id card = {
		.domain	= bdf.domain,
		.bus	= bdf.bus,
		.devid	= bdf.devid,
	};

	var = cmdline_var_getby(&ident, YK3_CMDLINE_DOMAIN_DEV, field);
	if (is_yk3_cmdline_var_enable(&var))
		return var;

	return cmdline_var_getbycard(card, numa, field);
}

static enum yk3_cmdline_field
cmdline_field_getbyname(const char *name)
{
	int i;

	for (i = 0; i < YK3_CMDLINE_FIELD_END; i++) {
		if (yk3_streq(name, cmdline_field_name[i]))
			return i;
	}

	return YK3_CMDLINE_FIELD_END;
}

static void
cmdline_cfg_release(struct yk3_cmdline_cfg *cfg)
{
	int i;

	for (i = 0; i < YK3_CMDLINE_FIELD_END; i++)
		cmdline_var_release(&cfg->field[i]);
}

static inline void
cmdline_inst_insert(struct yk3_cmdline_inst *inst)
{
	list_add(&inst->node, &cmdline.inst[inst->ident.domain].list);
	cmdline.inst[inst->ident.domain].count++;
	cmdline.count++;
}

static inline void
cmdline_inst_remove(struct yk3_cmdline_inst *inst)
{
	list_del(&inst->node);
	cmdline.inst[inst->ident.domain].count--;
	cmdline.count--;
}

static struct yk3_cmdline_inst *
cmdline_inst_create(struct yk3_cmdline_ident ident)
{
	struct yk3_cmdline_inst *inst;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return NULL;

	inst->ident = ident;
	cmdline_inst_insert(inst);

	return inst;
}

static void
cmdline_inst_destroy(struct yk3_cmdline_inst *inst)
{
	cmdline_inst_remove(inst);
	cmdline_cfg_release(&inst->cfg);
	kfree(inst);
}

/******************************************************************************/
static int
cmdline_parser_char(int start, int ch)
{
	int input = cmdline_charset(ch);

	cmdline_debug("charset[%s] input[%s] [%c]",
		      cmdline_charset_name(start), cmdline_charset_name(input), ch);

	switch (start) {
	case YK3_CMDLINE_CHARSET_GLOBAL:
		// global + lb: domain ==> lb
		switch (input) {
		case YK3_CMDLINE_CHARSET_LB:
			return input;
		default:
			return -EINVAL;
		}
	case YK3_CMDLINE_CHARSET_CONTEXT:
		// context + context	: context continue
		// context + lb		: domain==> lb
		// context + eq		: key	==> eq
		// context + comma	: value ==> comma
		// context + eol	: value ==> eol
		switch (input) {
		case YK3_CMDLINE_CHARSET_CONTEXT:
		case YK3_CMDLINE_CHARSET_LB:
		case YK3_CMDLINE_CHARSET_EQ:
		case YK3_CMDLINE_CHARSET_COMMA:
		case YK3_CMDLINE_CHARSET_EOL:
			return input;
		default:
			return -EINVAL;
		}
	case YK3_CMDLINE_CHARSET_LB:
		// lb + context: lb ==> key
		switch (input) {
		case YK3_CMDLINE_CHARSET_CONTEXT:
			return input;
		default:
			return -EINVAL;
		}
	case YK3_CMDLINE_CHARSET_RB:
		// rb + context : rb ==> domain
		// rb + eof     : rb ==> eof
		switch (input) {
		case YK3_CMDLINE_CHARSET_CONTEXT:
		case YK3_CMDLINE_CHARSET_EOF:
			return input;
		default:
			return -EINVAL;
		}
	case YK3_CMDLINE_CHARSET_EQ:
	case YK3_CMDLINE_CHARSET_COMMA:
		// eq/comma + context: eq/comma ==> value
		switch (input) {
		case YK3_CMDLINE_CHARSET_CONTEXT:
			return input;
		default:
			return -EINVAL;
		}
	case YK3_CMDLINE_CHARSET_EOL:
		// eol + context : eol ==> key
		// eol + rb      : eol ==> rb
		switch (input) {
		case YK3_CMDLINE_CHARSET_CONTEXT:
		case YK3_CMDLINE_CHARSET_RB:
			return input;
		default:
			return -EINVAL;
		}
	case YK3_CMDLINE_CHARSET_EOF:
		// eof + any : eof ==> eof
		return YK3_CMDLINE_CHARSET_EOF;
	case YK3_CMDLINE_CHARSET_SPACE:
	default:
		return -EINVAL;
	}
}

static bool
is_good_cmdline_word(const char *word)
{
	return !strpbrk(word, YK3_CMDLINE_KEY_INVALID);
}

static int
cmdline_parser_word_start(struct yk3_cmdline_parser *parser)
{
	int c_start;

	c_start = cmdline_charset(parser->backup);
	if (c_start < 0)
		return c_start;

	// restore end char
	*parser->p_end = parser->backup;
	// start point to end
	parser->p_start = parser->p_end;
	parser->c_start = c_start;

	// cmdline_debug("restore char[%c]", parser->backup);

	return c_start;
}

static void
cmdline_parser_word_stop(struct yk3_cmdline_parser *parser, char *p_end)
{
	parser->p_end = p_end;
	parser->backup = *p_end;
	*p_end = 0;
	parser->c_end = cmdline_charset(parser->backup);

	// cmdline_debug("save char[%c]", parser->backup);
}

static int
cmdline_fsm_to(struct yk3_cmdline_parser *parser, int fsm)
{
	cmdline_debug("fsm %s ==> %s", cmdline_fsm_name(parser->fsm), cmdline_fsm_name(fsm));

	parser->fsm = fsm;

	return 0;
}

static int
cmdline_fsm_to_domain(struct yk3_cmdline_parser *parser)
{
	const char *word = parser->p_start;
	char *end = NULL;
	unsigned int domain, bus, devid, function, numa;

	numa = strtoul(word, &end, 0);
	if (end == parser->p_end) {
		// word is number
		parser->numa = numa;
		parser->domain = YK3_CMDLINE_DOMAIN_NUMA;
		cmdline_debug("domain: numa[%d]", numa);
	} else if (word[0] == '*') {
		// word is "*"
		parser->domain = YK3_CMDLINE_DOMAIN_GLOBAL;
		cmdline_debug("domain: global");
	} else if (sscanf(word, "%x:%x:%x.%x", &domain, &bus, &devid, &function) == 4) {
		// word is bdf
		if (!is_good_bdf(domain, bus, devid, function)) {
			yk3_err("invalid bdf: %s", word);
			return -EPROTO;
		}

		parser->domain = YK3_CMDLINE_DOMAIN_DEV;

		parser->bdf.domain	= domain;
		parser->bdf.bus		= bus;
		parser->bdf.devid	= devid;
		parser->bdf.function	= function;
		cmdline_debug("domain: bdf[%s]", word);
	} else if (sscanf(word, "%x:%x:%x", &domain, &bus, &devid) == 3) {
		// word is card
		if (!is_good_card_id(domain, bus, devid)) {
			yk3_err("invalid card: %s", word);
			return -EPROTO;
		}

		parser->domain = YK3_CMDLINE_DOMAIN_CARD;

		parser->card.domain	= domain;
		parser->card.bus	= bus;
		parser->card.devid	= devid;
		cmdline_debug("domain: card[%s]", word);
	} else {
		// word is unknown
		yk3_err("domain: unknown[%s]", word);
		return -EPROTO;
	}

	return cmdline_fsm_to(parser, YK3_CMDLINE_FSM_DOMAIN);
}

static int
cmdline_fsm_to_key(struct yk3_cmdline_parser *parser)
{
	const char *word = parser->p_start;

	if (!is_good_cmdline_word(word)) {
		yk3_err("invalid cmdline key[%s]", word);
		return -EPROTO;
	}

	kfree(parser->key);
	parser->key = kstrdup(word, GFP_KERNEL);

	cmdline_debug("key[%s]", word);
	return cmdline_fsm_to(parser, YK3_CMDLINE_FSM_KEY);
}

static int
cmdline_fsm_to_value(struct yk3_cmdline_parser *parser)
{
	const char *word = parser->p_start;
	char *end = NULL;

	parser->value[parser->value_count++] = strtoul(word, &end, 0);
	if (end != parser->p_end) {
		yk3_err("invalid cmdline value[%s]", word);
		return -EPROTO;
	}
	cmdline_debug("value[%s]", word);

	return cmdline_fsm_to(parser, YK3_CMDLINE_FSM_VALUE);
}

static int
cmdline_fsm_init(struct yk3_cmdline_parser *parser)
{
	// input: */numa/bdf/card
	// fsm: init ==> domain
	return cmdline_fsm_to_domain(parser);
}

static int
cmdline_fsm_domain(struct yk3_cmdline_parser *parser)
{
	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_LB:
		// input: lb
		// fsm: domain ==> lb
		return cmdline_fsm_to(parser, YK3_CMDLINE_FSM_LB);
	default:
		return -EPROTO;
	}
}

static int
cmdline_fsm_lb(struct yk3_cmdline_parser *parser)
{
	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_CONTEXT:
		// input: key
		// fsm: lb ==> key
		return cmdline_fsm_to_key(parser);
	default:
		return -EPROTO;
	}
}

static int
cmdline_fsm_key(struct yk3_cmdline_parser *parser)
{
	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_EQ:
		// input: eq
		// fsm: key ==> eq
		return cmdline_fsm_to(parser, YK3_CMDLINE_FSM_EQ);
	default:
		return -EPROTO;
	}
}

static int
cmdline_fsm_eq(struct yk3_cmdline_parser *parser)
{
	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_CONTEXT:
		// input: value
		// fsm: eq ==> value
		return cmdline_fsm_to_value(parser);
	default:
		return -EPROTO;
	}
}

static int
cmdline_fsm_value(struct yk3_cmdline_parser *parser)
{
	int ret;

	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_COMMA:
		// input: comma
		// fsm: value ==> comma
		return cmdline_fsm_to(parser, YK3_CMDLINE_FSM_COMMA);
	case YK3_CMDLINE_CHARSET_EOL:
		// input: eol
		// fsm: value ==> eol
		ret = cmdline_fsm_to(parser, YK3_CMDLINE_FSM_EOL);

		// TODO: k/v complete, update var
		return ret;
	default:
		return -EPROTO;
	}
}

static int
cmdline_fsm_comma(struct yk3_cmdline_parser *parser)
{
	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_CONTEXT:
		// input: value
		// fsm: comma ==> value
		return cmdline_fsm_to_value(parser);
	default:
		return -EPROTO;
	}
}

static int
cmdline_fsm_eol(struct yk3_cmdline_parser *parser)
{
	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_CONTEXT:
		// input: key
		// fsm: eol ==> key
		return cmdline_fsm_to_key(parser);
	case YK3_CMDLINE_CHARSET_RB:
		// input: rb
		// fsm: eol ==> rb
		return cmdline_fsm_to(parser, YK3_CMDLINE_FSM_RB);
	default:
		return -EPROTO;
	}
}

static int
cmdline_fsm_rb(struct yk3_cmdline_parser *parser)
{
	switch (parser->c_start) {
	case YK3_CMDLINE_CHARSET_CONTEXT:
		// input: domain
		// fsm: rb ==> domain
		return cmdline_fsm_to_domain(parser);
	default:
		return -EPROTO;
	}
}

static int (*cmdline_fsm_handler[YK3_CMDLINE_FSM_END])(struct yk3_cmdline_parser *parser) = {
	[YK3_CMDLINE_FSM_INIT]	= cmdline_fsm_init,
	[YK3_CMDLINE_FSM_DOMAIN] = cmdline_fsm_domain,
	[YK3_CMDLINE_FSM_LB]	= cmdline_fsm_lb,
	[YK3_CMDLINE_FSM_RB]	= cmdline_fsm_rb,
	[YK3_CMDLINE_FSM_KEY]	= cmdline_fsm_key,
	[YK3_CMDLINE_FSM_VALUE] = cmdline_fsm_value,
	[YK3_CMDLINE_FSM_EQ]	= cmdline_fsm_eq,
	[YK3_CMDLINE_FSM_COMMA]	= cmdline_fsm_comma,
	[YK3_CMDLINE_FSM_EOL]	= cmdline_fsm_eol,
};

static int
cmdline_parser_word_handle(struct yk3_cmdline_parser *parser)
{
	int fsm = parser->fsm;

	cmdline_debug("fsm[%s] input word[%s] start[%s]...",
		      cmdline_fsm_name(fsm),
		      parser->p_start,
		      cmdline_charset_name(parser->c_start));
	if (is_good_cmdline_fsm(fsm))
		return (*cmdline_fsm_handler[fsm])(parser);

	return -EPROTO;
}

static int
cmdline_parser_word(struct yk3_cmdline_parser *parser)
{
	int c_start, c_end;
	char *p;

	c_start = cmdline_parser_word_start(parser);
	if (c_start < 0) {
		return c_start;
	} else if (is_cmdline_charset_eof(c_start)) {
		cmdline_debug("eof, parser stop");
		return YK3_CMDLINE_FSM_STOP;
	}

	for (p = parser->p_start + 1; ; p++) {
		c_end = cmdline_parser_char(c_start, *p);
		if (c_end < 0)
			return c_end;
		else if (c_start != c_end)
			break;
	}

	cmdline_parser_word_stop(parser, p);
	return cmdline_parser_word_handle(parser);
}

static int
cmdline_parser_parse(struct yk3_cmdline_parser *parser)
{
	int ret;

	while (1) {
		// >0: stop
		// <0: error, return error
		// =0: continue
		ret = cmdline_parser_word(parser);
		if (ret)
			break;
	}

	kfree(parser->key);
	return (ret < 0) ? ret : 0;
}

static int
cmdline_parser_copy(struct yk3_cmdline_parser *parser)
{
	const char *s = yk3_cmdline;
	char *d = parser->buf;
	int charset;

	while (*s) {
		charset = cmdline_charset(*s);
		if (charset < 0) {
			// invalid char
			return -EINVAL;
		} else if (charset == YK3_CMDLINE_CHARSET_SPACE) {
			// skip space
			s++;
		} else {
			// valid char, copy it
			*d++ = *s++;
		}
	}

	return 0;
}

static int
cmdline_parser_init(struct yk3_cmdline_parser *parser)
{
	int len = strlen(yk3_cmdline);
	int ret;

	parser->buf = kzalloc(1 + len, GFP_KERNEL);
	if (!parser->buf)
		return -ENOMEM;

	ret = cmdline_parser_copy(parser);
	if (ret < 0)
		return ret;
	cmdline_debug("cmdline zip: %s", parser->buf);
	cmdline_parser_word_stop(parser, parser->buf);

	return 0;
}

static int
cmdline_parse(void)
{
	struct yk3_cmdline_parser parser = { 0 };
	int ret;

	ret = cmdline_parser_init(&parser);
	if (ret < 0)
		return ret;

	ret = cmdline_parser_parse(&parser);
	kfree(parser.buf);
	return ret;
}

/******************************************************************************/
void yk3_cmdline_cfg_getbyglobal(struct yk3_cmdline_cfg *cfg)
{
	int i;

	for (i = 0; i < YK3_CMDLINE_FIELD_END; i++)
		cfg->field[i] = cmdline_var_getbyglobal(i);
}

void yk3_cmdline_cfg_getbynuma(int numa, struct yk3_cmdline_cfg *cfg)
{
	int i;

	for (i = 0; i < YK3_CMDLINE_FIELD_END; i++)
		cfg->field[i] = cmdline_var_getbynuma(numa, i);
}

void yk3_cmdline_cfg_getbycard(struct yk3_card_id card, int numa, struct yk3_cmdline_cfg *cfg)
{
	int i;

	for (i = 0; i < YK3_CMDLINE_FIELD_END; i++)
		cfg->field[i] = cmdline_var_getbycard(card, numa, i);
}

void yk3_cmdline_cfg_getbydev(struct yk3_bdf bdf, int numa, struct yk3_cmdline_cfg *cfg)
{
	int i;

	for (i = 0; i < YK3_CMDLINE_FIELD_END; i++)
		cfg->field[i] = cmdline_var_getbydev(bdf, numa, i);
}

int yk3_cmdline_init(void)
{
	cmdline_init();

	return cmdline_parse();
}

void yk3_cmdline_fini(void)
{
	struct yk3_cmdline_inst *inst, *tmp;
	int i;

	for (i = 0; i < YK3_CMDLINE_DOMAIN_END; i++) {
		list_for_each_entry_safe(inst, tmp, &cmdline.inst[i].list, node) {
			cmdline_inst_destroy(inst);
		}
	}
}

/******************************************************************************/
#endif
