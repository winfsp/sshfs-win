/**
 * sshfs-win.c
 *
 * Copyright 2015-2019 Bill Zissimopoulos
 */
/*
 * This file is part of SSHFS-Win.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *sshfs = "/bin/sshfs.exe";
static char *sshfs_environ[] = { "PATH=/bin", 0 };
static const char *KEY_OPTION = "k";
static const char *ROOT_OPTION = "r";

struct UNC_data
{
    int key;
    int root;
};

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
        "                        \\sshfs[.r][.k]\\[LOCUSER=]REMUSER@HOST[!PORT][\\PATH]\n"
        "                        sshfs: remote home; sshfs.r: remote root\n"
        "                        sshfs.k: remote home with key authentication\n"
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

static int use_pass(const char *cls)
{
    char regpath[256];
    int regfd;
    uint32_t value = -1;

    snprintf(regpath, sizeof regpath,
        "/proc/registry32/HKEY_LOCAL_MACHINE/Software/WinFsp/Services/%s/Credentials", cls);

    regfd = open(regpath, O_RDONLY);
    if (-1 != regfd)
    {
        read(regfd, &value, sizeof value);
        close(regfd);
    }

    return 1 == value;
}

/**
 * Enable an element of UNC_data structure according to the opt passed in parameter.
 * @param opt The option to enable in UNC_data structure if recognized.
 * @param options The UNC_data structure to modify according to the opt.
 */
static void determine_option(const char *opt, struct UNC_data *options)
{
    if (strncmp(KEY_OPTION, opt, strlen(KEY_OPTION)) == 0)
        options->key = 1;
    else if (strncmp(ROOT_OPTION, opt, strlen(ROOT_OPTION)) == 0)
        options->root = 1;
}

/**
 * Read the string passed in parameter until /, ., or NULL character is read.
 * @param stringToParse A pointer to the character pointer to modify while parsing the segment.
 * @return The position of the beginning of the option (The position initialy pointed by stringToParse).
 */
static char* parse_segment(char **stringToParse)
{
    char *p = *stringToParse;
    while (*p && '.' != *p && '/' != *p)
        p++;

    char *option = *stringToParse;
    *stringToParse = p;
    return option;
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

    execve(sshfs, sshfs_argv, sshfs_environ);
    return 1;
}

static int do_svc(int argc, char *argv[])
{
#define SSHFS_ARGS                      \
    "-f",                               \
    "-orellinks",                       \
    "-ofstypename=SSHFS",               \
    "-oUserKnownHostsFile=/dev/null",   \
    "-oStrictHostKeyChecking=no"

    if (3 > argc || 200 < argc)
        usage();

    struct passwd *passwd = 0;
    struct UNC_data options = { .key = 0, .root = 0 };
    char idmap[64], authmeth[256], volpfx[256], portopt[256], remote[256];
    char *cls, *locuser, *locuser_nodom, *userhost, *port, *path, *p;

    snprintf(volpfx, sizeof volpfx, "--VolumePrefix=%s", argv[1]);

    /* translate backslash to forward slash */
    for (p = argv[1]; *p; p++)
        if ('\\' == *p)
            *p = '/';

    /* parse options (syntax: //sshfs[.r][.k]/) */
    /* skip first slash */
    p = argv[1];
    while (*p && '/' == *p)
        p++;
    /* Assign cls and fill UNC_data structure */
    cls = parse_segment(&p);
    while ('.' == *p)
    {
        p++;
        determine_option(parse_segment(&p), &options);
    }
    *p++ = '\0';

    /* parse instance name (syntax: [locuser=]user@host!port) */
    locuser = locuser_nodom = 0;
    userhost = p;
    port = "22";
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

    snprintf(portopt, sizeof portopt, "-oPort=%s", port);
    snprintf(remote, sizeof remote, "%s:%s%s", userhost, options.root ? "/" : "", path);

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
            /* translate backslash to '+' */
            locuser = argv[3];
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

    if (use_pass(cls))
        snprintf(authmeth, sizeof authmeth,
            "-opassword_stdin,password_stdout");
    else if (0 != passwd)
        snprintf(authmeth, sizeof authmeth,
            "-oPreferredAuthentications=publickey,IdentityFile=%s/.ssh/id_rsa", passwd->pw_dir);
    else
        snprintf(authmeth, sizeof authmeth,
            "-oPreferredAuthentications=publickey");

    char *sshfs_argv[256] =
    {
        sshfs, SSHFS_ARGS, idmap, authmeth, volpfx, portopt, remote, argv[2], 0,
    };

    if (4 <= argc)
        concat_argv(sshfs_argv, argv + 4);

    execve(sshfs, sshfs_argv, sshfs_environ);
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
