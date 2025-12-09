# ned
*ned* is a **modernised** version of *ed(1)*.

![showcase](https://github.com/caseycronyn/assets/blob/4b74765b14f825963c3f22434856c01d11656174/ned-showcase.gif)

It adds contemporary code features to the [standard **UNIX** text editor](https://www.gnu.org/fun/jokes/ed-msg.en.html). These include:

- **Syntax Highlighting** — using *bat*
- **Code Completion** — powered by an *LSP* client using *clangd* for *C*
- **GNU Readline integration** — command-line editing of input lines

It is a fork of [oed](https://github.com/ibara/oed), written in *C*.

## Table of Contents
- [Installation](#Installation)
  - [Dependencies](#Dependencies)
- [Usage](#Usage)
- [Configuration](#Configuration)
- [About](#About)
- [Other ed's](#Other-ed's)

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

## Usage
- Syntax highlighting will happen automatically with any buffer printing commands.
- Code completion is initiated with TAB. It will show options if there are any.
- Readline integration is built into 'insert mode', initiated with `i` or `a`.

## Configuration
Both code completion and readline bindings (ie automatic completion, completion key, emacs or vi, parentheses highlighting) can be modified through *Readline's* [config file](https://www.gnu.org/software/bash/manual/html_node/Readline-Init-File.html) (`.inputrc` in the home directory). `ned` will respect these preferences.

## About
*ned* is an experiment in retrofitting technologies belonging to disparate eras. It adds a set of modern code editing features to ed in a way that hasn’t broken it. These features can easily be ignored and the tool used as it was designed, if you can bear the angry fruit salad. An *ed* developer could unwittingly become a *ned* developer if their sysadmin has set an alias for `ed=ned`.

## Other ed's
- [edbrowse](https://github.com/edbrowse/edbrowse) extends ed as a web browser and mail client, originally written for blind users.
- [hired](https://github.com/sidju/hired) is a Rust rewrite, adding extra commands and syntax highlighting and an ed-like interface.
- [Ed with Syntax Highlighting](https://github.com/Mathias-Fuchs/ed-with-syntax-highlighting) is written in C and C++, adding syntax highlighting.
