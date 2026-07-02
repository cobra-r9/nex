

#include <stdlib.h>
#include "tree.h"
#include "layout.h"

/* ============================== SHARED ============================== */
/* Used only by LAYOUT_TALL and LAYOUT_WIDE, both of which render leaves
 * as an ordered master + stack rather than by tree topology. */

static int cmp_leaf_seq(const void *a, const void *b) {
	node_t *na = *(node_t * const *) a;
	node_t *nb = *(node_t * const *) b;

	if (na->insertion_seq < nb->insertion_seq) {
		return -1;
	}
	if (na->insertion_seq > nb->insertion_seq) {
		return 1;
	}
	return 0;
}

static node_t **collect_ordered_leaves(desktop_t *d, int *out_count) {
	int capacity = 16;
	int count = 0;
	node_t **leaves = malloc(capacity * sizeof(node_t *));

	if (leaves == NULL) {
		*out_count = 0;
		return NULL;
	}

	for (node_t *f = first_extrema(d->root); f != NULL; f = next_leaf(f, d->root)) {
		if (f->hidden || f->client == NULL) {
			continue;
		}
		if (count == capacity) {
			capacity *= 2;
			node_t **rleaves = realloc(leaves, capacity * sizeof(node_t *));
			if (rleaves == NULL) {
				free(leaves);
				*out_count = 0;
				return NULL;
			}
			leaves = rleaves;
		}
		leaves[count++] = f;
	}

	qsort(leaves, count, sizeof(node_t *), cmp_leaf_seq);
	*out_count = count;
	return leaves;
}

/* ============================== BINARY ============================== */
/* Classic tree-split layout. Geometry comes straight from the tree:
 * each internal node's split_type/split_ratio divides its rect between
 * its two children, recursively, down to the leaves. */

static void layout_binary_apply(monitor_t *m, desktop_t *d, node_t *n, xcb_rectangle_t rect) {
	if (n == NULL) {
		return;
	}

	if (is_leaf(n)) {
		render_node(m, d, n, rect);
		return;
	}

	xcb_rectangle_t first_rect;
	xcb_rectangle_t second_rect;

	if (n->first_child->vacant || n->second_child->vacant) {
		first_rect = rect;
		second_rect = rect;
	} else if (n->split_type == TYPE_VERTICAL) {
		unsigned int fence = rect.width * n->split_ratio;

		if ((n->first_child->constraints.min_width + n->second_child->constraints.min_width) <= rect.width) {
			if (fence < n->first_child->constraints.min_width) {
				fence = n->first_child->constraints.min_width;
				n->split_ratio = (double) fence / (double) rect.width;
			} else if (fence > (uint16_t) (rect.width - n->second_child->constraints.min_width)) {
				fence = (rect.width - n->second_child->constraints.min_width);
				n->split_ratio = (double) fence / (double) rect.width;
			}
		}

		first_rect = (xcb_rectangle_t) {rect.x, rect.y, fence, rect.height};
		second_rect = (xcb_rectangle_t) {rect.x + fence, rect.y, rect.width - fence, rect.height};
	} else {
		unsigned int fence = rect.height * n->split_ratio;

		if ((n->first_child->constraints.min_height + n->second_child->constraints.min_height) <= rect.height) {
			if (fence < n->first_child->constraints.min_height) {
				fence = n->first_child->constraints.min_height;
				n->split_ratio = (double) fence / (double) rect.height;
			} else if (fence > (uint16_t) (rect.height - n->second_child->constraints.min_height)) {
				fence = (rect.height - n->second_child->constraints.min_height);
				n->split_ratio = (double) fence / (double) rect.height;
			}
		}

		first_rect = (xcb_rectangle_t) {rect.x, rect.y, rect.width, fence};
		second_rect = (xcb_rectangle_t) {rect.x, rect.y + fence, rect.width, rect.height - fence};
	}

	layout_binary_apply(m, d, n->first_child, first_rect);
	layout_binary_apply(m, d, n->second_child, second_rect);
}

void layout_binary_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect) {
	layout_binary_apply(m, d, d->root, rect);
}

/* ============================== MONOCLE ============================== */
/* Every leaf gets the full rect. No ordering needed - which one is on
 * top is already handled by stack.c's focus-driven stacking order. This
 * intentionally does not filter hidden/receptacle leaves, matching the
 * behavior monocle already had when it piggybacked on the binary
 * recursion. */

void layout_monocle_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect) {
	for (node_t *f = first_extrema(d->root); f != NULL; f = next_leaf(f, d->root)) {
		render_node(m, d, f, rect);
	}
}

/* ============================== TALL ============================== */
/* Master column + stacked column. VARIANT_NORMAL puts master on the
 * left, VARIANT_REVERSED on the right. */

void layout_tall_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect) {
	int n_leaves;
	node_t **leaves = collect_ordered_leaves(d, &n_leaves);

    int tiled_n = 0;
    for (int i = 0; i < n_leaves; i++) {
        if (leaves[i]->vacant) {
            render_node(m, d, leaves[i], rect);
        } else {
            leaves[tiled_n++] = leaves[i];
        }
    }
    n_leaves = tiled_n;

	if (n_leaves == 0) {
		free(leaves);
		return;
	}

	unsigned int master_width = (n_leaves == 1) ? rect.width : (unsigned int) (rect.width * d->master_ratio);
	unsigned int stack_width = rect.width - master_width;

	xcb_rectangle_t master_rect;
	xcb_rectangle_t stack_rect;

	if (d->layout_variant == VARIANT_NORMAL) {
		master_rect = (xcb_rectangle_t) {rect.x, rect.y, master_width, rect.height};
		stack_rect = (xcb_rectangle_t) {(int16_t) (rect.x + master_width), rect.y, stack_width, rect.height};
	} else {
		stack_rect = (xcb_rectangle_t) {rect.x, rect.y, stack_width, rect.height};
		master_rect = (xcb_rectangle_t) {(int16_t) (rect.x + stack_width), rect.y, master_width, rect.height};
	}

	render_node(m, d, leaves[0], master_rect);

	int stack_n = n_leaves - 1;
	for (int i = 1; i < n_leaves; i++) {
		int si = i - 1;
		unsigned int h = stack_rect.height / stack_n;
		xcb_rectangle_t r = {
			stack_rect.x,
			(int16_t) (stack_rect.y + si * h),
			stack_rect.width,
			(si == stack_n - 1) ? (stack_rect.height - si * h) : h
		};
		render_node(m, d, leaves[i], r);
	}

	free(leaves);
}

/* ============================== WIDE ============================== */
/* Master row + stacked row. VARIANT_NORMAL puts master on top,
 * VARIANT_REVERSED on the bottom. Same shape as TALL, axes swapped. */

void layout_wide_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect) {
	int n_leaves;
	node_t **leaves = collect_ordered_leaves(d, &n_leaves);

    int tiled_n = 0;
    for (int i = 0; i < n_leaves; i++) {
        if (leaves[i]->vacant) {
            render_node(m, d, leaves[i], rect);
        } else {
            leaves[tiled_n++] = leaves[i];
        }
    }
    n_leaves = tiled_n;

	if (n_leaves == 0) {
		free(leaves);
		return;
	}

	unsigned int master_height = (n_leaves == 1) ? rect.height : (unsigned int) (rect.height * d->master_ratio);
	unsigned int stack_height = rect.height - master_height;

	xcb_rectangle_t master_rect;
	xcb_rectangle_t stack_rect;

	if (d->layout_variant == VARIANT_NORMAL) {
		master_rect = (xcb_rectangle_t) {rect.x, rect.y, rect.width, master_height};
		stack_rect = (xcb_rectangle_t) {rect.x, (int16_t) (rect.y + master_height), rect.width, stack_height};
	} else {
		stack_rect = (xcb_rectangle_t) {rect.x, rect.y, rect.width, stack_height};
		master_rect = (xcb_rectangle_t) {rect.x, (int16_t) (rect.y + stack_height), rect.width, master_height};
	}

	render_node(m, d, leaves[0], master_rect);

	int stack_n = n_leaves - 1;
	for (int i = 1; i < n_leaves; i++) {
		int si = i - 1;
		unsigned int w = stack_rect.width / stack_n;
		xcb_rectangle_t r = {
			(int16_t) (stack_rect.x + si * w),
			stack_rect.y,
			(si == stack_n - 1) ? (stack_rect.width - si * w) : w,
			stack_rect.height
		};
		render_node(m, d, leaves[i], r);
	}

	free(leaves);
}


/* ============================== STRINGS ============================== */
/* Canonical name is "binary", not the old "tiled" - "tiled" is still
 * accepted as an input alias in parse_layout, but output always uses the
 * real enum name now that there's more than one tiled-style layout. */

const char *layout_str(layout_t l) {
	switch (l) {
		case LAYOUT_BINARY:
			return "binary";
		case LAYOUT_MONOCLE:
			return "monocle";
		case LAYOUT_TALL:
			return "tall";
		case LAYOUT_WIDE:
			return "wide";
	}
	return "binary";
}

char layout_chr(layout_t l) {
	switch (l) {
		case LAYOUT_BINARY:
			return 'B';
		case LAYOUT_MONOCLE:
			return 'M';
		case LAYOUT_TALL:
			return 'T';
		case LAYOUT_WIDE:
			return 'W';
	}
	return 'B';
}

const char *layout_variant_str(layout_variant_t v) {
	switch (v) {
		case VARIANT_NORMAL:
			return "normal";
		case VARIANT_REVERSED:
			return "reversed";
	}
	return "normal";
}
