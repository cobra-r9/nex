

#include <stdlib.h>
#include <math.h>
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

/* ============================== GRID ============================== */
/* Cells as close to a square arrangement as possible (cols =
 * ceil(sqrt(n)), rows = ceil(n / cols)), filled in insertion order.
 * VARIANT_NORMAL fills row-major (left to right, wrapping to the next
 * row); VARIANT_REVERSED fills column-major (top to bottom, wrapping to
 * the next column). Whichever line (row, in row-major; column, in
 * column-major) ends up short on cells because n doesn't divide evenly
 * has its cells stretched along the *other* axis so no space is left
 * empty - same "leftover stack member absorbs the remainder" approach
 * TALL/WIDE use for their stack, just applied per-line here. */

void layout_grid_arrange(monitor_t *m, desktop_t *d, xcb_rectangle_t rect) {
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

	bool col_major = (d->layout_variant == VARIANT_REVERSED);

	/* "per_line" = cells along the major axis before wrapping (a row's
	 * width in row-major, a column's height in column-major).
	 * "lines" = how many lines that wraps into along the minor axis. */
	int per_line = (int) ceil(sqrt((double) n_leaves));
	int lines = (int) ceil((double) n_leaves / (double) per_line);
	int last_line_count = n_leaves - (lines - 1) * per_line;

	unsigned int total_major = col_major ? rect.height : rect.width;
	unsigned int total_minor = col_major ? rect.width : rect.height;
	unsigned int line_minor = total_minor / lines;

	for (int i = 0; i < n_leaves; i++) {
		int line = i / per_line;
		int pos = i % per_line;
		bool is_last_line = (line == lines - 1);
		int line_count = is_last_line ? last_line_count : per_line;

		unsigned int cell_major = total_major / line_count;
		unsigned int major_off = pos * cell_major;
		unsigned int major_size = (pos == line_count - 1) ? (total_major - major_off) : cell_major;

		unsigned int minor_off = line * line_minor;
		unsigned int minor_size = is_last_line ? (total_minor - minor_off) : line_minor;

		xcb_rectangle_t r;
		if (col_major) {
			/* major axis runs down a column (height), minor axis
			 * steps across columns (width). */
			r = (xcb_rectangle_t) {
				(int16_t) (rect.x + minor_off),
				(int16_t) (rect.y + major_off),
				minor_size,
				major_size
			};
		} else {
			/* major axis runs along a row (width), minor axis
			 * steps down rows (height). */
			r = (xcb_rectangle_t) {
				(int16_t) (rect.x + major_off),
				(int16_t) (rect.y + minor_off),
				major_size,
				minor_size
			};
		}

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
		case LAYOUT_GRID:
			return "grid";
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
		case LAYOUT_GRID:
			return 'G';
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
