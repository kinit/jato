#ifndef __STATEMENT_H
#define __STATEMENT_H

#include <constant.h>
#include <stddef.h>
#include <jam.h>

enum statement_type {
	STMT_NOP,
	STMT_ASSIGN
};

struct statement {
	enum statement_type type;
	struct constant operand;
};

extern struct statement *stmt_from_bytecode(struct classblock *cb, unsigned char *code, size_t count);

#endif
