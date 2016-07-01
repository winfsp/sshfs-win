#include <stdio.h>
#include <unistd.h>

#define SSHFS_ARGS                      \
    "-f",                               \
    "-opassword_stdin",                 \
    "-opassword_stdout",                \
    "-oUserKnownHostsFile=/dev/null",   \
    "-oStrictHostKeyChecking=no"

int main(int argc, char *argv[])
{
    static const char *sshfs = "/bin/sshfs.exe";
    static const char *environ[] =
    {
        "PATH=/bin",
    };
    char idmap[64], volpfx[256], remote[256], *p, *q;

    if (3 != argc)
        return 2;

    snprintf(idmap, sizeof idmap, "-ouid=%d,gid=%d", -1, -1);

    snprintf(volpfx, sizeof volpfx, "--VolumePrefix=%s", argv[1]);

    p = argv[1];
    while ('\\' == *p)
        p++;
    while (*p && '\\' != *p)
        p++;
    while ('\\' == *p)
        p++;
    q = p;
    while (*q && '\\' != *q)
        q++;
    snprintf(remote, sizeof remote, "%.*s:%s", q - p, p, *q ? q + 1 : q);
    p = remote;
    while (*p)
    {
        if ('\\' == *p)
            *p = '/';
        p++;
    }

    execle(sshfs, sshfs, SSHFS_ARGS, idmap, volpfx, remote, argv[2], (void *)0, environ);

    return 1;
}
