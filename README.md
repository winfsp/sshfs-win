# SSHFS-Win - SSHFS for Windows

SSHFS-Win is a minimal port of [SSHFS](https://github.com/libfuse/sshfs) to Windows. Under the hood it uses [Cygwin](https://cygwin.com) for the POSIX environment and [WinFsp](http://www.secfs.net/winfsp/) for the FUSE functionality.

SSHFS-Win requires the latest version of WinFsp to be installed; you can find it here: http://www.secfs.net/winfsp/download/. It does not require Cygwin to be installed, all the necessary files are included in the SSHFS-Win installer.

## How to use

Once you have installed WinFsp and SSHFS-Win you can start an SSHFS session to a remote computer using the following syntax:

    \\sshfs\[locuser=]user@host[!port]

For example, you can map a network drive to billz@linux-host by using the syntax:

    \\sshfs\billz@linux-host

As a more complicated example, you can map a network drive to billz@linux-host at port 9999, but give access rights to the local user billziss by using the syntax:

    \\sshfs\billziss=billz@linux-host!9999

You can use the Windows Explorer "Map Network Drive" functionality or you can use the `net use` command from the command line.

## Project Organization

This is a very simple project:

- `sshfs` is a submodule pointing to the original SSHFS project.
- `sshfs-win.c` is a simple wrapper around the sshfs program that is used to implement the "Map Network Drive" functionality.
- `sshfs-win.wxs` is a the Wix file that describes the SSHFS-Win installer.
- `patches` is a directory with a couple of simple patches over SSHFS.
- `Makefile` drives the overall process of building SSHFS-Win and packaging it into an MSI.

## License

SSHFS-Win uses the same license as SSHFS, which is GPLv2.

SSHFS-Win packages the following components:

- Cygwin: LGPLv3
- GLib2: LGPLv2
- SSH: "all components are under a BSD licence, or a licence more free than that"
