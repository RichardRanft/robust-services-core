# Preface to Articles

This is a preface to the RSC articles on CodeProject.

## The Repository

RSC is an open-source framework for developing robust C++ applications. Its
repository contains over 230K lines of code organized into static libraries.
Each static library is implemented in its own namespace so that the layer to
which code belongs will be clear.

## Downloading the Code

The _.zip_ file attached to an article contains most of the repository but
excludes the [_output_](/output) directory, which only contains files that
are generated when running tests.

An article's _.zip_ file seldom contains RSC's latest release. It is usually
updated only when the code that it discusses has evolved.

The [Releases](https://github.com/GregUtas/robust-services-core/releases) page
lists stable releases. Unless a release only changed documentation, it includes
executables. However, RSC must be properly installed to run them. See the
[installation guide](/docs/Installing.md).

## Finding Code

Unless otherwise noted, the code in an article resides in the namespace
`NodeBase`, which is implemented in the [_nb_](/src/nb) directory. `NodeBase`
is RSC's lowest layer. It contains about 55K lines of code and provides
base classes for things such as

- [initialization/reinitialization](https://www.codeproject.com/Articles/5254138/Robust-Cplusplus-Initialization-and-Restarts)
- [configuration](https://www.codeproject.com/Articles/5274153/Robust-Cplusplus-Operational-Aspects)
- [multi-threading](https://www.codeproject.com/Articles/5246597/Robust-Cplusplus-P-and-V-Considered-Harmful)
- [object pooling](https://www.codeproject.com/Articles/5166096/Robust-Cplusplus-Object-Pools)
- [CLI commands](https://www.codeproject.com/Articles/5269493/A-Command-Line-Interface-CLI-Framework)
- [logging](https://www.codeproject.com/Articles/5274153/Robust-Cplusplus-Operational-Aspects)
- [debugging](https://www.codeproject.com/Articles/5255828/Debugging-Live-Systems)

RSC usually defines a class in a _.h_ of the same name and implements it
in a _.cpp_ of the same name. It should therefore be easy to find the code
that an article discusses.

## Target Platforms

RSC can run on either Windows or Linux, which are supported by an abstraction
layer that should allow RSC to be ported to other platforms with modest effort.
The Windows and Linux targets (in _*.win.cpp_ and _*.linux.cpp_ files) total
less than 1600 lines of code each, excluding comments and blanks.

## Using RSC

When building on RSC, you'll always use `NodeBase`. If your application
uses UDP- or TCP-based protocols, investigate the namespace `NetworkBase` 
in the [_nw_](/src/nw) directory, and the namespace `SessionBase` in the
[_sb_](/src/sb) directory. There are currently no articles about those layers,
but some [documents](/docs/README.md) discuss them.

## Modifying RSC

If you don't want to build on RSC, you can copy and modify its code to meet
your needs, subject to the terms of its LGPL-3.0 license. But because RSC is
an integrated _framework_, you'll have some work to do. Unlike a library,
RSC's classes collaborate freely rather than being designed for standalone
use.

## Deleted Code

The code in an article omits details that are irrelevant to the article's
purpose. They are nonetheless important and need to be considered if copying
and modifying RSC software. These details include

- configuration parameters, logs, alarms, and statistics, as described in
[Robust C++: Operational Aspects](https://www.codeproject.com/Articles/5274153/Robust-Cplusplus-Operational-Aspects);
- memory types, as described in
[Robust C++ : Initialization and Restarts](https://www.codeproject.com/Articles/5254138/Robust-Cplusplus-Initialization-and-Restarts); and
- trace tools, as described in
[Debugging Live Systems](https://www.codeproject.com/Articles/5255828/Debugging-Live-Systems).

## What to Read Next

You probably found your first RSC article because its topic interested you.
For an overview of RSC, read
[Software Techniques for Lemmings](https://www.codeproject.com/Articles/5258540/Software-Techniques-for-Lemmings),
which discusses the rationale behind various aspects of its design.

And if you find RSC useful or interesting, please give this repository a
star by clicking on the button at the upper right of this page.
