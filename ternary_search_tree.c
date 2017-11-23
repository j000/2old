#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include <unicode/ustdio.h>
#include <unicode/ustring.h>

#include "ternary_search_tree.h"

#define U2(X) u ## X
#define U(X) U2(X)

#define MAX_WORD_LENGTH 50
#define INDENT 6
#define COLOR u"\033[34m"
#define COLOR2 u"\033[33m"
#define RESET u"\033[0m"
/* from main.c */
void *my_calloc(size_t n, size_t size);
void *my_malloc(size_t size);

static uint_fast32_t nodes = 0;
static uint_fast32_t words = 0;

struct tst_node {
	UChar data;				/*!< character stored in node */
	bool eos;				/*!< end of string */
	struct tst_node *less,	/*!< pointer to left subtree */
		*same,				/*!< pointer to middle subtree */
		*more;				/*!< pointer to right subtree */
	uint_fast16_t t;		/*!< number of accesses (for rotations) */
};

/**
 * Create new TST node.
 */
struct tst_node *new_tst_node(UChar data) {
	/* TODO: Support for suffixes instead of chars? */
#if 1
	struct tst_node *tmp = my_calloc(1, sizeof(*tmp));
#else /* if 1 */
	struct tst_node *tmp = my_malloc(sizeof(*tmp));

	tmp->less = tmp->same = tmp->more = NULL;
#endif /* if 1 */

	++nodes;

	tmp->data = data;
	return tmp;
}

/**
 * Insert new word into TST.
 */
void tst_insert(
	struct tst_node **root,	/* removed arg */
	UChar *word
) {
	/* Tree is empty */
	if (*root == NULL)
		*root = new_tst_node(*word);

	if (*word > (*root)->data) {
		tst_insert(&((*root)->more), word);
		return;
	}

	if (*word < (*root)->data) {
		tst_insert(&((*root)->less), word);
		return;
	}

	if (word[1] == '\0') {
		if (!(*root)->eos)
			++words;
		(*root)->eos = 1;
		return;
	}

	tst_insert(&((*root)->same), &word[1]);
}

/**
 * A recursive function to print TST.
 */
void tst_print_util(struct tst_node *root, int depth, UChar *prefix) {
	if (root == NULL) {
		u_printf_u(RESET u"\n");
		return;
	}

	int_fast32_t tmp = u_strlen(prefix);

	/* Go to left subtree */
	u_printf_u(RESET u"%C%3" U(PRIuFAST16), root->data, root->t);

	if (root->eos)
		u_printf_u(COLOR2 u"·" COLOR);
	else
		u_printf_u(COLOR u"─");
	u_printf_u(u"────");
	u_strncat(prefix, u"│     ", MAX_WORD_LENGTH * INDENT);
	tst_print_util(root->less, depth + 1, prefix);

	/* Go to middle subtree */
	u_printf_u(COLOR u"%.*S├" COLOR2 u"─────" RESET, tmp, prefix);
	tst_print_util(root->same, depth + 1, prefix);

	/* Go to right subtree */
	prefix[tmp] = u' ';
	u_printf_u(COLOR u"%.*S└─────" RESET, tmp, prefix);
	tst_print_util(root->more, depth + 1, prefix);
	prefix[tmp] = u'\0';
}

/**
 * Print TST with nice format.
 */
void tst_print(struct tst_node *root) {
	UChar prefix[MAX_WORD_LENGTH * INDENT + 1] = u"";

	tst_print_util(root, 0, prefix);
}

/**
 * A recursive helper function to traverse TST.
 */
void tst_traverse_util(
	struct tst_node *root,
	UChar *buffer,
	int depth
) {
	if (root == NULL)
		return;

	/* Go to left subtree */
	tst_traverse_util(root->less, buffer, depth);

	/* store character in this node */
	buffer[depth] = root->data;

	if (root->eos) {
		buffer[depth + 1] = '\0';
		u_printf("%S\n", buffer);
	}

	/* Go to middle subtree */
	tst_traverse_util(root->same, buffer, depth + 1);

	/* Go to right subtree */
	tst_traverse_util(root->more, buffer, depth);
}

/**
 * Print TST contents.
 */
void tst_traverse(struct tst_node *root) {
	UChar buffer[50 + 1] = { '\0' };

	tst_traverse_util(root, buffer, 0);
}

/**
 * A recursive helper function to search TST.
 */
bool tst_search_recursive(struct tst_node *root, UChar *word) {
	if (root == NULL)
		return false;
	if (*word < root->data)
		return tst_search_recursive(root->less, word);
	if (*word > root->data)
		return tst_search_recursive(root->more, word);
	if (word[1] == u'\0')
		return root->eos;
	return tst_search_recursive(root->same, &word[1]);
}

/**
 * A helper function with conditional rotation.
 * Based on http://www.bcs.org/upload/pdf/oommen.pdf
 */
bool tst_search_conditional(
	struct tst_node **root,
	UChar *word,
	struct tst_node **parent
) {
	if (*root == NULL)
		return false;

	(*root)->t += 1;/* update counter */
	if (*word < (*root)->data)
		return tst_search_conditional(&(*root)->less, word, root);
	if (*word > (*root)->data)
		return tst_search_conditional(&(*root)->more, word, root);

	bool out = false;
	int_fast32_t tmp = 0;

	if (parent != NULL) {
		if ((*parent)->less == (*root))
			tmp = 2 * (*root)->t -
				((*root)->more == NULL ? 0 : (*root)->more->t) - (*parent)->t;
		else if ((*parent)->more == (*root))
			tmp = 2 * (*root)->t -
				((*root)->less == NULL ? 0 : (*root)->less->t) - (*parent)->t;
	}
	if (word[1] != u'\0')
		out = tst_search_conditional(&(*root)->same, &word[1], root);
	else
		out = (*root)->eos;

	if (tmp > 0) {
		uint_fast16_t alpha = (*root)->t -
			((*root)->less ==
			NULL ? 0 : (*root)->less->t) -
			((*root)->more == NULL ? 0 : (*root)->more->t);
		uint_fast16_t p_alpha = (*parent)->t -
			((*parent)->less ==
			NULL ? 0 : (*parent)->less->t) -
			((*parent)->more == NULL ? 0 : (*parent)->more->t);

		struct tst_node *t = NULL;

		/* rotate node upwards */
		if ((*parent)->less == (*root)) {
			/* we are left node */
			/* rotate right */
			t = (*root)->more;
			(*root)->more = (*parent);
			(*parent) = (*root);
			(*root) = t;

			/* for later use */
			t = (*parent)->more;
		} else {
			/* we are right node */
			/* rotate left */
			t = (*root)->less;
			(*root)->less = (*parent);
			(*parent) = (*root);
			(*root) = t;

			/* for later use */
			t = (*parent)->less;
		}

		t->t = p_alpha +
			(t->less ==
			NULL ? 0 : t->less->t) + (t->more == NULL ? 0 : t->more->t);
		(*parent)->t = alpha +
			((*parent)->less ==
			NULL ? 0 : (*parent)->less->t) +
			((*parent)->more == NULL ? 0 : (*parent)->more->t);
	}
	return out;
}

/**
 * Search TST for a given word.
 */
bool tst_search(struct tst_node **root, UChar *word) {
	/* return tst_search_recursive(*root, word); */
	return tst_search_conditional(root, word, NULL);
}

/**
 * Free all memory.
 */
void tst_free(struct tst_node **root) {
	if (*root == NULL)
		return;

	words = 0;
	nodes = 0;
	tst_free(&(*root)->less);
	tst_free(&(*root)->same);
	tst_free(&(*root)->more);
	free(*root);
}

/**
 * Return struct size
 */
size_t get_size(void) {
	return sizeof(struct tst_node);
}

/**
 * Return number of words
 */
uint_fast32_t get_words(void) {
	return words;
}

/**
 * Return number of nodes
 */
uint_fast32_t get_nodes(void) {
	return nodes;
}
