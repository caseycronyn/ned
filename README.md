# ned
*ned* is a **modernised** version of *ed*.
![showcase](https://github.com/caseycronyn/assets/blob/4b74765b14f825963c3f22434856c01d11656174/ned-showcase.gif)

It adds contemporary code features to *ed(1)*, the *standard* **UNIX** text editor written in 1969. These include:

- **Syntax Highlighting** — semantically aware using *bat*
- **Code Completion** for *C* — powered by an *LSP* client using *clangd*
- **GNU Readline** integration — easier buffer editing

It is a fork of [oed](https://github.com/ibara/oed), a portable OpenBSD implementation, written in *C*.

## Table of Contents
- [Installation](#Installation)
  - [Dependencies](#Dependencies)
- [Usage](#Usage)
  - [Configuration](#Configuration)
- [About](#About)

***

## Installation

1. Install the [dependencies](#Dependencies).
2. Clone *ned*.
3. Follow the installation instructions for [oed](https://github.com/ibara/oed).
4. Link *Readline* into the program ([here](https://github.com/caseycronyn/assets/blob/7fc531da53f4c76e683045dc110957eae03c6100/Makefile)
 is how I did it).

### Dependencies

- [bat](https://github.com/sharkdp/bat)
- [clangd](https://github.com/clangd/clangd)
- [GNU Readline](https://ftp.gnu.org/gnu/readline/)

### Usage
- Syntax highlighting will happen automatically with any buffer printing commands.
- Code completion is initiated with TAB. It will show options if there are any.
- Readline integration is built into 'insert mode', initiated with `i` or `a`.

### Configuration
Both code completion and readline bindings (ie automatic completion, completion key, emacs or vi, parentheses highlighting) can be modified through *Readline's* [config file](https://www.gnu.org/software/bash/manual/html_node/Readline-Init-File.html) (`.inputrc` in the home directory). `ned` will respect these preferences.

## About
*ned* is an experiment in retrofitting technologies belonging to disparate eras. It adds a set of modern code editing features to ed in a way that hasn’t broken it. These features can easily be ignored and the tool used as it was designed, if you can bear the angry fruit salad. An *ed* developer could unwittingly become a *ned* developer if their sysadmin has set an alias for `ed=ned`.
