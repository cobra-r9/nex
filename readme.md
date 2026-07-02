## Introduction to Nex

Yet another dynamic window manager (dynamic tiling window manager). Sole purpose - provide a feature rich, shell scriptable, extended fork building on top of binary space partitioning and several other popular tiling window layouts.

Other tiling WMs exists, but they do not offer **configuration via bash/shell scripts**, though it's parent `bspwm` has this functionality - it does not have extended layouts.

### Description

**Nex** is a dynamic window manager which is an **extended fork of [bspwm](https://github.com/baskerville/bspwm)**, that supports additional features like tall left, tall right, wide top, wide bottom (the directions are with reference to the master window), master stack layouts and other layouts(soon).

>[!NOTE]
> This fork of bspwm aims at making bspwm a dynamic window manager with additional layouts and a more comprehensible command system via `nexl`. Also, this is a part of **learn c by building** phase I use to learn C. 

It only responds to X events, and the messages it receives on a dedicated socket.

**Nex Window Manager** comes with two parts : `nex`, the window manager itself, and `nexl`, the IPC for `nex` to provide it instructions - like swap the windows, focus the node, swap the windows, etc. 

`nexl` - the IPC works by writing to the socket which is created by `nex` and use it to provide various window management functions - focus windows, desktops, monitors, swap, switch, and several desktop specific layouts.

>[!NOTE]
> *nex* doesn't handle any keyboard or pointer inputs: a third party program (e.g. *sxhkd*) is needed in order to translate keyboard and pointer events to *nexl* invocations.



The outlined architecture is the following:

```
        PROCESS          SOCKET
sxhkd  -------->  nexl  <------>  nex
```

>[!IMPORTANT]
> **In future**, I have a plan to integrate the keybind funtionality into nex itself, as well as additional to `shell script` configuration file, I may also add a `lua` configuration functionality. Currently, this is just a project for **learning C and X11 Windowing**. I personally argue that, though the X11 protocol may be outdated compared to Wayland, the underlying C implications are not outdated. They are essential for anyone to understand how C actually works. 

### Configuration

The default configuration file is `$XDG_CONFIG_HOME/nex/nexrc`: this is simply a shell script that calls *nexl*. In most of the system, it may be 

```bash
~/.config/nex/nexrc 
```

An argument is passed to that script to indicate whether is was executed after a restart (`$1 -gt 0`) or not (`$1 -eq 0`).

Currently the keyboard and pointer bindings are defined with [sxhkd](https://github.com/baskerville/sxhkd).

### Monitors, desktops and windows

*nex* holds a list of monitors.

A monitor is just a rectangle that contains desktops.

A desktop is just a pointer to a tree.

Monitors only show the tree of one desktop at a time (their focused desktop).

The tree is a partition of a monitor's rectangle into smaller rectangular regions.

Each node in a tree either has zero or two children (in dwindle/binary layout).
Each internal node is responsible for splitting a rectangle in half (as the default split ratio in binary layout is 0.5).

A split is defined by two parameters: the type (horizontal or vertical) and the ratio (a real number *r* such that *0 < r < 1*) in the binary space layout (layout = binary). In other layouts, there is no such split which follows the binary tree, instead it follows a fixed algorithm to insert the new windows, defined by master split ratio and mode (relative location of the master window with respect to the slaves, where split ratio and horizontal or vertical have no effect.)

## Supported protocols and standards

- The RandR and Xinerama protocols.
- A subset of the EWMH and ICCCM standards.


## Get it in your system

To install it, install it. 

### Installation 
There is no way you can install it via package manager. But you can clone this repo and install it via `makepkg`, which makes this `nex` package manageable by `pacman` by following the steps. 

```bash 
# clone the repository :
git clone https://github.com/cobra-r9/nex && cd nex

# install the package via makepkg
makepkg -si 

# you will be prompted for the sudo password, enter it, and install it. 
```

If you are paranoid, then you can review the PKGBULD by : 

```bash
cat PKGBUILD
```

Once, verification is success, then you can proceed with the installation.

### Uninstallation 

As you have installed it with the help of `pacman`, just 

```bash
sudo pacman -Rns nex
```

The package will be gone.
