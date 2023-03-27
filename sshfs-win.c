/**
 * sshfs-win.c
 *
 * Copyright 2015-2021 Bill Zissimopoulos
 */
/*
 * This file is part of SSHFS-Win.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *sshfs = "/usr/bin/sshfs.exe";

#if 0
#define execve pr_execv
static void pr_execv(const char *path, char *argv[], ...)
{
    fprintf(stderr, "%s\n", path);
    for (; 0 != *argv; argv++)
        fprintf(stderr, "    %s\n", *argv);
}
#endif

static void usage(void)
{
    fprintf(stderr,
        "usage: sshfs-win cmd SSHFS_COMMAND_LINE\n"
        "    SSHFS_COMMAND_LINE  command line to pass to sshfs\n"
        "\n"
        "usage: sshfs-win svc PREFIX X: [LOCUSER] [SSHFS_OPTIONS]\n"
        "    PREFIX              Windows UNC prefix (note single backslash)\n"
        "                        \\sshfs[.SUFFIX]\\[LOCUSER=]REMUSER@HOST[!PORT][\\PATH]\n"
        "                        \\sshfs[.SUFFIX]\\alias[\\PATH]\n"
        "                        sshfs: remote user home dir\n"
        "                        sshfs.r: remote root dir\n"
        "                        sshfs.k: remote user home dir with key authentication\n"
        "                        sshfs.kr: remote root dir with key authentication\n"
        "    LOCUSER             local user (DOMAIN+USERNAME)\n"
        "    REMUSER             remote user\n"
        "    HOST                remote host\n"
        "    PORT                remote port\n"
        "    PATH                remote path (relative to remote home or root)\n"
        "    X:                  mount drive\n"
        "    SSHFS_OPTIONS       additional options to pass to SSHFS\n");
    exit(2);
}

static void concat_argv(char *dst[], char *src[])
{
    for (; 0 != *dst; dst++)
        ;
    for (; 0 != (*dst = *src); dst++, src++)
        ;
}

static void opt_escape(const char *opt, char *escaped, size_t size)
{
    const char *p = opt;
    char *q = escaped, *endq = escaped + size;
    for (; *p && endq > q + 1; p++, q++)
    {
        if (',' == *p || '\\' == *p)
            *q++ = '\\';
        *q = *p;
    }
    *q = '\0';
}

static uint32_t reg_get(const char *cls, const char *name)
{
    char regpath[256];
    int regfd;
    uint32_t value = -1;

    snprintf(regpath, sizeof regpath,
        "/proc/registry32/HKEY_LOCAL_MACHINE/Software/WinFsp/Services/%s/%s", cls, name);

    regfd = open(regpath, O_RDONLY);
    if (-1 != regfd)
    {
        if (sizeof value != read(regfd, &value, sizeof value))
            value = -1;
        close(regfd);
    }

    return value;
}

static int fixenv_and_execv(const char *path, char **argv)
{
    static char *default_environ[] = { "PATH=/bin", 0 };
    extern char **environ;
    char **oldenv, **oldp;
    char **newenv, **newp;
    size_t len, siz;
    int res;

    oldenv = environ;

    siz = 0;
    for (oldp = oldenv; *oldp; oldp++)
    {
        if (0 == strncmp(*oldp, "PATH=", 5))
        {
            len = strlen(*oldp + 5);
            siz += len + sizeof "PATH=/usr/bin:";
        }
        else
        {
            len = strlen(*oldp);
            siz += len + 1;
        }
    }
    oldp++;

    newenv = malloc((oldp - oldenv) * sizeof(char *) + siz);
    if (0 != newenv)
    {
        siz = (oldp - oldenv) * sizeof(char *);
        for (oldp = oldenv, newp = newenv; *oldp; oldp++, newp++)
        {
            *newp = (char *)newenv + siz;
            if (0 == strncmp(*oldp, "PATH=", 5))
            {
                len = strlen(*oldp + 5);
                siz += len + sizeof "PATH=/usr/bin:";
                memcpy(*newp, "PATH=/usr/bin:", sizeof "PATH=/usr/bin:" - 1);
                memcpy(*newp + sizeof "PATH=/usr/bin:" - 1, *oldp + 5, len + 1);
            }
            else
            {
                len = strlen(*oldp);
                siz += len + 1;
                memcpy(*newp, *oldp, len + 1);
            }
        }
        *newp = 0;
    }
    else
    {
        newenv = default_environ;
        newp = newenv + 1;
    }

#if 1
    res = execve(path, argv, newenv);
#else
    char **p;
    for (p = newenv; *p; p++)
        printf("%s\n", *p);
    assert(newp == p);
#endif

    if (newenv != default_environ)
        free(newenv);

    return res;
}

static int do_cmd(int argc, char *argv[])
{
    if (200 < argc)
        usage();

    char *sshfs_argv[256] =
    {
        sshfs, 0,
    };

    concat_argv(sshfs_argv, argv + 1);

    fixenv_and_execv(sshfs, sshfs_argv);
    return 1;
}

static int do_svc(int argc, char *argv[])
{
#define SSHFS_ARGS                      \
    "-f",                               \
    "-orellinks",                       \
    "-ofstypename=SSHFS",               \
    "-o ssh_command=/usr/bin/ssh.exe",  \
    "-oUserKnownHostsFile=/dev/null",   \
    "-oStrictHostKeyChecking=no"

    if (3 > argc || 200 < argc)
        usage();

    struct passwd *passwd = 0;
    char idmap[64], authmeth[64 + 1024], volpfx[256], portopt[256], remote[256];
    char escaped[1024];
    char *cls, *locuser, *locuser_nodom, *userhost, *port, *root, *path, *p;

    snprintf(volpfx, sizeof volpfx, "--VolumePrefix=%s", argv[1]);

    /* translate backslash to forward slash */
    for (p = argv[1]; *p; p++)
        if ('\\' == *p)
            *p = '/';

    /* skip class name */
    p = argv[1];
    while ('/' == *p)
        p++;
    cls = p;
    while (*p && '/' != *p)
        p++;
    if (*p)
        *p++ = '\0';
    while ('/' == *p)
        p++;
    root = 1 == reg_get(cls, "sshfs.rootdir") ? "/" : "";

    /* parse instance name (syntax: [locuser=]user@host[!port]) */
    locuser = locuser_nodom = 0;
    userhost = p;
    port = 0;
    while (*p && '/' != *p)
    {
        if ('=' == *p)
        {
            *p = '\0';
            locuser = userhost;
            userhost = p + 1;
        }
        else if ('!' == *p)
        {
            *p = '\0';
            port = p + 1;
        }
        p++;
    }
    if (*p)
        *p++ = '\0';
    path = p;
    if (port != 0)
    {
        opt_escape(port, escaped, sizeof escaped);
        snprintf(portopt, sizeof portopt, "-oPort=%s", escaped);
    }
    else
    {
        portopt[0] = '\0';
    }
    snprintf(remote, sizeof remote, "%s:%s%s", userhost, root, path);

    /* get local user name */
    if (0 == locuser)
    {
        if (3 >= argc)
        {
            p = userhost;
            while (*p && '@' != *p)
                p++;
            if (*p)
            {
                *p = '\0';
                locuser = userhost;
            }
        }
        else
        {
            locuser = argv[3];
            /* the Cygwin runtime does not remove double quotes around args with non-ASCII chars */
            if (locuser[0] == '"')
            {
                size_t len = strlen(locuser);
                if (len >= 2 && locuser[len - 1] == '"')
                {
                    locuser[len - 1] = '\0';    /* remove the trailing quote... */
                    ++locuser;                  /* ...and the heading one       */
                }
            }
            /* translate backslash to '+' */
            for (p = locuser; *p; p++)
                if ('\\' == *p)
                {
                    *p = '+';
                    locuser_nodom = p + 1;
                }
        }
    }

    snprintf(idmap, sizeof idmap, "-ouid=-1,gid=-1");
    if (0 != locuser)
    {
        /* get uid/gid from local user name */
        passwd = getpwnam(locuser);
        if (0 == passwd && 0 != locuser_nodom)
            passwd = getpwnam(locuser_nodom);
        if (0 != passwd)
            snprintf(idmap, sizeof idmap, "-ouid=%d,gid=%d", passwd->pw_uid, passwd->pw_gid);
    }

    if (1 == reg_get(cls, "Credentials"))
        snprintf(authmeth, sizeof authmeth,
            "-opassword_stdin,password_stdout");
    else if (0 != passwd)
    {
        opt_escape(passwd->pw_dir, escaped, sizeof escaped);
        snprintf(authmeth, sizeof authmeth,
            "-oPreferredAuthentications=publickey,IdentityFile=\"%s/.ssh/id_rsa\"", escaped);
    }
    else
        snprintf(authmeth, sizeof authmeth,
            "-oPreferredAuthentications=publickey");

    char *sshfs_argv[256] =
    {
        sshfs, SSHFS_ARGS, idmap, authmeth, volpfx, portopt, remote, argv[2], 0,
    };

    if ('\0' == portopt[0])
    {
        /* if not passing a port option, remove it from sshfs_argv */
        int portopt_idx = 0;
        while (sshfs_argv[portopt_idx] != portopt)
            portopt_idx++;
        while (sshfs_argv[portopt_idx] != 0)
        {
            sshfs_argv[portopt_idx] = sshfs_argv[portopt_idx + 1];
            portopt_idx++;
        }
    }

    if (4 <= argc)
        concat_argv(sshfs_argv, argv + 4);

    fixenv_and_execv(sshfs, sshfs_argv);
    return 1;

#undef SSHFS_ARGS
}

int main(int argc, char *argv[])
{
    const char *mode = argv[1];

    if (0 != mode && 0 == strcmp("cmd", mode))
        return do_cmd(argc - 1, argv + 1);
    if (0 != mode && 0 == strcmp("svc", mode))
        return do_svc(argc - 1, argv + 1);

    usage();
    return 1;
}
