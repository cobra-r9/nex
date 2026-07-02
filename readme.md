## Description

**Nex** is a tiling window manager which is an **extended fork of [bspwm](https://github.com/baskerville/bspwm)**, that supports additional features like tall left, tall right, wide top, wide bottom (the directions are with reference to the master window), master stack layouts and other layouts(soon).

It only responds to X events, and the messages it receives on a dedicated socket.

*nexl* is a program that writes messages on *nex*'s socket.

*nex* doesn't handle any keyboard or pointer inputs: a third party program (e.g. *sxhkd*) is needed in order to translate keyboard and pointer events to *nexl* invocations.

The outlined architecture is the following:

```
        PROCESS          SOCKET
sxhkd  -------->  nexl  <------>  nex
```

## Configuration

The default configuration file is `$XDG_CONFIG_HOME/nex/nexrc`: this is simply a shell script that calls *nexl*.

An argument is passed to that script to indicate whether is was executed after a restart (`$1 -gt 0`) or not (`$1 -eq 0`).

Keyboard and pointer bindings are defined with [sxhkd](https://github.com/baskerville/sxhkd).

Example configuration files can be found in the [examples](examples) directory.

## Monitors, desktops and windows

*nex* holds a list of monitors.

A monitor is just a rectangle that contains desktops.

A desktop is just a pointer to a tree.

Monitors only show the tree of one desktop at a time (their focused desktop).

The tree is a partition of a monitor's rectangle into smaller rectangular regions.

Each node in a tree either has zero or two children.

Each internal node is responsible for splitting a rectangle in half.

A split is defined by two parameters: the type (horizontal or vertical) and the ratio (a real number *r* such that *0 < r < 1*) in the binary space layout (layout = binary). In other layouts, there is no such split which follows the binary tree, instead it follows a fixed algorithm to insert the new windows, defined by master split ratio and mode (relative location of the master window with respect to the slaves, where split ratio and horizontal or vertical have no effect.)

Each leaf node holds exactly one window.

## Supported protocols and standards

- The RandR and Xinerama protocols.
- A subset of the EWMH and ICCCM standards.


