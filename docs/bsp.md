# Binary Space Layout

The **nex** window manager comes with 4 different layouts (which will be extended in the future), with each layout having its own algorithm for rendering, creating, moving and freeing the windows. The stock *[bspwm](https://github.com/baskerville/bspwm)* had only two layouts which are (bsp layout + monocle layout).

## Insertion modes

When *nex* receives a new window, it inserts it into a window tree at the specified insertion point (a leaf) using the insertion mode specified for that insertion point.

The insertion mode tells *nex* how it should alter the tree in order to insert new windows on a given insertion point.

By default the insertion point is the focused window and its insertion mode is *automatic*.

### Manual mode

The user can specify a region in the insertion point where the next new window should appear by sending a *node -p|--presel-dir DIR* message to *nex*.

The *DIR* argument allows to specify how the insertion point should be split (horizontally or vertically) and if the new window should be the first or the second child of the new internal node (the insertion point will become its *brother*).

After doing so the insertion point goes into *manual* mode.

Let's consider the following scenario:

```
            a                          a                          a
           / \                        / \                        / \
          1   b         --->         c   b         --->         c   b
          ^  / \                    / \ / \                    / \ / \
            2   3                  4  1 2  3                  d  1 2  3
                                   ^                         / \
                                                            5   4
                                                            ^

+-----------------------+  +-----------------------+  +-----------------------+
|           |           |  |           |           |  |     |     |           |
|           |     2     |  |     4     |     2     |  |  5  |  4  |     2     |
|           |           |  |     ^     |           |  |  ^  |     |           |
|     1     |-----------|  |-----------|-----------|  |-----------|-----------|
|     ^     |           |  |           |           |  |           |           |
|           |     3     |  |     1     |     3     |  |     1     |     3     |
|           |           |  |           |           |  |           |           |
+-----------------------+  +-----------------------+  +-----------------------+

            X                          Y                          Z 
```

In state *X*, the insertion point is *1*.

We send the following message to *nex*: *node -p north*.

Then add a new window: *4*, this leads to state *Y*: the new internal node, *c* becomes *a*'s first child.

Finally we send another message: *node -p west* and add window *5*.

The ratio of the preselection (that ends up being the ratio of the split of the new internal node) can be changed with the *node -o|--presel-ratio* message.

### Automatic mode

The *automatic* mode, as opposed to the *manual* mode, doesn't require any user choice. The way the new window is inserted is determined by the value of the automatic scheme and the initial polarity settings.

#### Longest side scheme

When the value of the automatic scheme is `longest_side`, the window will be attached as if the insertion point was in manual mode and the split direction was chosen based on the dimensions of the tiling rectangle and the initial polarity.

Let's consider the following scenario, where the initial polarity is set to `second_child`:

```
             1                          a                          a
             ^                         / \                        / \
                         --->         1   2         --->         1   b
                                          ^                         / \
                                                                   2   3
                                                                       ^

 +-----------------------+  +-----------------------+  +-----------------------+
 |                       |  |           |           |  |           |           |
 |                       |  |           |           |  |           |     2     |
 |                       |  |           |           |  |           |           |
 |           1           |  |     1     |     2     |  |     1     |-----------|
 |           ^           |  |           |     ^     |  |           |           |
 |                       |  |           |           |  |           |     3     |
 |                       |  |           |           |  |           |     ^     |
 +-----------------------+  +-----------------------+  +-----------------------+

             X                          Y                          Z
```

In state *X*, a new window is added.

Since *1* is wide, it gets split vertically and *2* is added as *a*'s second child given the initial polarity.

This leads to *Y* where we insert window *3*. *2* is tall and is therefore split horizontally. *3* is once again added as *b*'s second child.

#### Alternate scheme

When the value of the automatic scheme is `alternate`, the window will be attached as if the insertion point was in manual mode and the split direction was chosen based on the split type of the insertion point's parent and the initial polarity. If the parent is split horizontally, the insertion point will be split vertically and vice versa.

#### Spiral scheme

When the value of the automatic scheme is `spiral`, the window will *take the space* of the insertion point.

Let's dive into the details with the following scenario:

```
             a                          a                          a
            / \                        / \                        / \
           1   b         --->         1   c         --->         1   d
              / \                        / \                        / \
             2   3                      4   b                      5   c
             ^                          ^  / \                     ^  / \
                                          3   2                      b   4
                                                                    / \
                                                                   3   2

 +-----------------------+  +-----------------------+  +-----------------------+
 |           |           |  |           |           |  |           |           |
 |           |     2     |  |           |     4     |  |           |     5     |
 |           |     ^     |  |           |     ^     |  |           |     ^     |
 |     1     |-----------|  |     1     |-----------|  |     1     |-----------|
 |           |           |  |           |     |     |  |           |  3  |     |
 |           |     3     |  |           |  3  |  2  |  |           |-----|  4  |
 |           |           |  |           |     |     |  |           |  2  |     |
 +-----------------------+  +-----------------------+  +-----------------------+

             X                          Y                          Z
```

In state *X*, the insertion point, *2* is in automatic mode.

When we add a new window, *4*, the whole tree rooted at *b* is reattached, as the second child of a new internal node, *c*.

The splitting parameters of *b* (type: *horizontal*, ratio: *½*) are copied to *c* and *b* is rotated by 90° clockwise.

The tiling rectangle of *4* in state *Y* is equal to the tiling rectangle of *2* in state *X*.

Then the insertion of *5*, with *4* as insertion point, leads to *Z*.

The *spiral* automatic scheme generates window spirals that rotate clockwise (resp. anti-clockwise) if the insertion point is the first (resp. second) child of its parent.



