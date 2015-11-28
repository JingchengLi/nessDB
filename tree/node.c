/*
 * Copyright (c) 2012-2014 The nessDB Project Developers. All rights reserved.
 * Code is licensed with GPL. See COPYING.GPL file.
 *
 */

#include "u.h"
#include "t.h"

void node_set_dirty(struct node *node)
{
	mutex_lock(&node->attr.mtx);
	if (!node->dirty)
		ngettime(&node->modified);
	node->dirty = 1;
	mutex_unlock(&node->attr.mtx);
}

void node_set_nondirty(struct node *node)
{
	mutex_lock(&node->attr.mtx);
	node->dirty = 0;
	mutex_unlock(&node->attr.mtx);
}

int node_is_dirty(struct node *node)
{
	int ret;

	mutex_lock(&node->attr.mtx);
	ret = (int)(node->dirty == 1);
	mutex_unlock(&node->attr.mtx);

	return ret;
}

/*
 * PROCESS:
 *	- find the 1st pivot index which pivot >= k
 *	- it's lowerbound binary search
 *
 *	   i0   i1   i2   i3
 *	+----+----+----+----+
 *	| 15 | 17 | 19 | +∞ |
 *	+----+----+----+----+
 *	so if key is 16, we will get 1(i1)
 */
int node_partition_idx(struct node *node, struct msg *k)
{
	int lo = 0;
	int hi = node->n_children - 2;
	int best = hi;

	nassert(node->n_children > 1);
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		int cmp = node->opts->bt_compare_func(node->pivots[mid].data, node->pivots[mid].size,
		                                      k->data, k->size);

		if (cmp >= 0) {
			best = mid;
			hi = mid - 1;
		} else {
			lo = mid + 1;
		}
	}

	return best;
}

enum node_state get_node_state(struct node *node)
{
	uint32_t children = node->n_children;

	if (nessunlikely(node->height == 0)) {
		if (node->i->size(node) >= node->opts->leaf_node_size)
			return FISSIBLE;
	} else {
		if (children >= node->opts->inner_node_fanout)
			return FISSIBLE;

		if (node->i->size(node) >= node->opts->inner_node_size)
			return FLUSHBLE;
	}

	return STABLE;
}
