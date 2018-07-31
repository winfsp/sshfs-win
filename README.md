<h1 align="center">
    SSHFS-Win &middot; SSHFS for Windows
</h1>

<p align="center">
    <b>Install</b><br>
    <a href="https://github.com/billziss-gh/sshfs-win/releases/latest">
        <img src="https://img.shields.io/github/release/billziss-gh/sshfs-win.svg?label=stable&style=for-the-badge"/>
    </a>
    <a href="https://chocolatey.org/packages/sshfs">
        <img src="https://img.shields.io/badge/choco-install%20sshfs-black.svg?style=for-the-badge"/>
    </a>
    <a href="https://github.com/mhogomchungu/sirikali/releases/latest">
        <img src="https://img.shields.io/github/release/mhogomchungu/sirikali.svg?label=GUI%20Frontend&style=for-the-badge"/>
    </a>
</p>

SSHFS-Win is a minimal port of [SSHFS](https://github.com/libfuse/sshfs) to Windows. Under the hood it uses [Cygwin](https://cygwin.com) for the POSIX environment and [WinFsp](https://github.com/billziss-gh/winfsp) for the FUSE functionality.

## How to install

- Install the latest version of [WinFsp](https://github.com/billziss-gh/winfsp/releases/latest).
- Install the latest version of [SSHFS-Win](https://github.com/billziss-gh/sshfs-win/releases/latest). Choose the x64 or x86 installer according to your computer's architecture.

## How to use

Once you have installed WinFsp and SSHFS-Win you can start an SSHFS session to a remote computer using the following syntax:

    \\sshfs\[locuser=]user@host[!port][\path]

For example, you can map a network drive to billz@linux-host by using the syntax:

    \\sshfs\billz@linux-host

As a more complicated example, you can map a network drive to billz@linux-host at port 9999, but give access rights to the local user billziss by using the syntax:

    \\sshfs\billziss=billz@linux-host!9999

It is also possible to map the remote root directory by starting the `path` with a double backslash as in the following example:

    \\sshfs\billz@linux-host\\home\billz

You can also mount the remote's root `/` directory using the following format:

    \\sshfs\user@host\..\..

You can use the Windows Explorer "Map Network Drive" functionality or you can use the `net use` command from the command line.

## GUI front end

[SiriKali](https://mhogomchungu.github.io/sirikali/) is a GUI front end for SSHFS-Win (and other file systems). Instructions on setting up SiriKali for SSHFS-Win can be found at this [link](https://github.com/mhogomchungu/sirikali/wiki/Frequently-Asked-Questions#90-how-do-i-add-options-to-connect-to-an-ssh-server). Please report problems with SiriKali in its [issues](https://github.com/mhogomchungu/sirikali/issues) page.

## Project Organization

This is a very simple project:

- `sshfs` is a submodule pointing to the original SSHFS project.
- `sshfs-win.c` is a simple wrapper around the sshfs program that is used to implement the "Map Network Drive" functionality.
- `sshfs-win.wxs` is a the Wix file that describes the SSHFS-Win installer.
- `patches` is a directory with a couple of simple patches over SSHFS.
- `Makefile` drives the overall process of building SSHFS-Win and packaging it into an MSI.

## License

SSHFS-Win uses the same license as SSHFS, which is GPLv2+. It interfaces with WinFsp which is GPLv3 with a FLOSS exception.

It also packages the following components:

- Cygwin: LGPLv3
- GLib2: LGPLv2
- SSH: "all components are under a BSD licence, or a licence more free than that"
