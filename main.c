/**
 * @file
 * Main file. It contains main() function and some other program-level stuff.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>				/* isatty() */

/* unicode */
/* #define UCHAR_TYPE int_fast16_t */
#include <unicode/ustdio.h>
#include <unicode/ustring.h>
#include <unicode/uclean.h>

#include "ternary_search_tree.h"

/* BNF */
/*
 * <produkcja> ::= <symbol> = <wyrażenie>.
 * <wyrażenie> ::= <składnik> { , <składnik> }
 * <składnik>  ::= <czynnik> { <czynnik> }
 * <czynnik>   ::= <symbol> | [ <składnik> ]
 */

/* global variables */
UFILE *u_stdin = NULL;
UFILE *u_stdout = NULL;

uint_fast32_t memory = 0;
/* memory managment */
/**
 * Malloc wrapper: shows message and exits on error.
 * @param size bytes to allocate
 */
void *my_malloc(size_t size) {
	void *tmp = malloc(size);

	memory += size;

	if (tmp != NULL)
		return tmp;
	fprintf(
		stderr,
		"Malloc failed (%ld bytes): %s. Aborting.\n",
		size,
		strerror(errno)
	);
	exit(EXIT_FAILURE);
}

/**
 * Calloc wrapper: shows message and exits on error.
 * @param n number of elements to allocate
 * @param size element size in bytes
 */
void *my_calloc(size_t n, size_t size) {
	void *tmp = calloc(n, size);

	memory += n * size;

	if (tmp != NULL)
		return tmp;
	fprintf(
		stderr,
		"Calloc failed (%ld x %ld bytes): %s. Aborting.\n",
		n,
		size,
		strerror(errno)
	);
	exit(EXIT_FAILURE);
}

void print_pretty_bytes(uint_fast64_t bytes) {
	const char *suffixes[] = {
		"B",
		"KiB",
		"MiB",
		"GiB",
		"TiB",
		"PiB"
	};
	const uint8_t suffix_count = sizeof(suffixes) / sizeof(suffixes[0]);
	double count = bytes;
	uint_fast64_t s = 0;	/* which suffix to use */

	while (count >= 1024 && s + 1 < suffix_count) {
		s++;
		count /= 1024;
	}
	if (count - (int)count == 0.0)
		printf("%d %s", (int)count, suffixes[s]);
	else
		printf("%.1f %s", count, suffixes[s]);
}

/* **** */

enum char_type {
	TYPE_EOF = EOF,
	TYPE_DIGIT = 0,
	TYPE_ALPHA,
	TYPE_OPERATOR,
	TYPE_WHITESPACE,
	TYPE_PUNCT,
	TYPE_OTHER
};

enum char_type classifier(UChar c) {
	if (u_isdigit(c))
		return TYPE_DIGIT;
	/* if (u_isalpha(c)) */
	if (u_isUAlphabetic(c) || c == u'_')
		return TYPE_ALPHA;
	if (u_isUWhiteSpace(c))
		return TYPE_WHITESPACE;
	if (u_ispunct(c))
		return TYPE_PUNCT;

	return TYPE_OTHER;
}

/* **** */
int main(int argc, char **argv) {
	UChar c = 0;
	UChar *buf = NULL;
	const size_t max_word_length = 50;
	bool is_tty = true;
	unsigned int i = 0;

	/* prepare unicode stdin/stdout */
	u_stdin = u_finit(stdin, NULL, NULL);
	if (u_stdin == NULL) {
		fprintf(stderr, "u_finit failed for stdin\n");
		exit(EXIT_FAILURE);
	}

	u_stdout = u_finit(stdout, NULL, NULL);
	if (u_stdout == NULL) {
		fprintf(stderr, "u_finit failed for stdout\n");
		exit(EXIT_FAILURE);
	}

	if (!isatty(STDIN_FILENO))
		is_tty = false;

	/* **** */
	struct tst_node *tree = NULL;

	buf = my_calloc(max_word_length + 1, sizeof(*buf));

	/* **** */
	i = 0;
	while ((c = u_fgetc(u_stdin)) != U_EOF) {
		if (classifier(c) != TYPE_ALPHA && classifier(c) != TYPE_DIGIT) {
			/* for weirdly long words */
			/* TODO: handle this gracefully */
			buf[max_word_length] = '\0';
			if (i > 0) {
				if (tst_search(&tree, buf)) {
					if (is_tty)
						u_printf_u(u" Found: %S\n", buf);
				} else {
					if (is_tty)
						u_printf_u(u"Adding: %S\n", buf);
					tst_insert(&tree, buf);
				}
				/* tst_traverse(tree); */
				/* tst_print(tree); */
			}
			i = 0;
			buf[0] = u'\0';
			buf[1] = u'\0';
		} else {
			buf[i++] = c;
			buf[i] = u'\0';
		}
	}

	/* tst_print(tree); */
	/* tst_traverse(tree); */
	printf("Allocated ");
	print_pretty_bytes(memory);
	printf(
		" for %" PRIuFAST32 " words (%" PRIuFAST32 " nodes).\n",
		get_words(),
		get_nodes()
	);

	/* cleanup */
	free(buf);
	tst_free(&tree);
	/* unicode cleanup */
	u_fclose(u_stdin);
	u_fclose(u_stdout);
	u_cleanup();
	return EXIT_SUCCESS;
}
