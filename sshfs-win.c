#include <stdarg.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

#define SSHFS_ARGS                      \
    "-f",                               \
    "-opassword_stdin",                 \
    "-opassword_stdout",                \
    "-orellinks",                       \
    "-ofstypename=SSHFS",               \
    "-oUserKnownHostsFile=/dev/null",   \
    "-oStrictHostKeyChecking=no"

#if 0
#define execle pr_execl
static void pr_execl(const char *path, ...)
{
    va_list ap;
    const char *arg;

    va_start(ap, path);
    fprintf(stderr, "%s\n", path);
    while (0 != (arg = va_arg(ap, const char *)))
        fprintf(stderr, "    %s\n", arg);
    va_end(ap);
}
#endif

int main(int argc, char *argv[])
{
    static const char *sshfs = "/bin/sshfs.exe";
    static const char *environ[] = { "PATH=/bin", 0 };
    struct passwd *passwd;
    char idmap[64], volpfx[256], portopt[256], remote[256];
    char *locuser, *locuser_nodom, *userhost, *port, *path, *p;

    if (3 > argc || argc > 4)
        return 2;

    snprintf(volpfx, sizeof volpfx, "--VolumePrefix=%s", argv[1]);

    /* translate backslash to forward slash */
    for (p = argv[1]; *p; p++)
        if ('\\' == *p)
            *p = '/';

    /* skip class name (\\sshfs\) */
    p = argv[1];
    while ('/' == *p)
        p++;
    while (*p && '/' != *p)
        p++;
    while ('/' == *p)
        p++;

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
    snprintf(remote, sizeof remote, "%s:%s", userhost, path);

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

    execle(sshfs, sshfs, SSHFS_ARGS, idmap, volpfx, portopt, remote, argv[2], (void *)0, environ);

    return 1;
}
