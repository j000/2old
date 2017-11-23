#ifndef TERNARY_SEARCH_TREE_H
#define TERNARY_SEARCH_TREE_H

#include "bool.h"

/* our struct */
struct tst_node;
/* main API */
void tst_insert(struct tst_node **root, UChar *word);
void tst_traverse(struct tst_node *root);
void tst_print(struct tst_node *root);
void tst_free(struct tst_node **root);
bool tst_search(struct tst_node **root, UChar *word);

/* different helpers */
uint_fast32_t get_words(void);
uint_fast32_t get_nodes(void);
size_t get_size(void);

#endif /* TERNARY_SEARCH_TREE_H */
