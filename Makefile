PrjDir = $(shell pwd)
BldDir = .build
SrcDir = $(BldDir)/src
RootDir = $(BldDir)/root
Status = $(BldDir)/status
BinExtra = ssh #bash ls mount

goal: $(Status) $(Status)/done

$(Status):
	mkdir -p $(Status)

$(Status)/done: $(Status)/run-sshfs
	touch $(Status)/done

$(Status)/run-sshfs: $(Status)/root run-sshfs.c
	gcc -o $(RootDir)/bin/run-sshfs run-sshfs.c
	strip $(RootDir)/bin/run-sshfs
	touch $(Status)/run-sshfs

$(Status)/root: $(Status)/make
	mkdir -p $(RootDir)/bin $(RootDir)/dev/{mqueue,shm}
	(ldd $(SrcDir)/sshfs/sshfs; for f in $(BinExtra); do ldd /usr/bin/$$f; done) |\
		sed -n 's@^.*/usr/bin/\([^ ]*\).*$$@\1@p' |\
		while read f; do cp /usr/bin/$$f $(RootDir)/bin; done
	cp $(SrcDir)/sshfs/sshfs $(RootDir)/bin
	strip $(RootDir)/bin/sshfs
	for f in $(BinExtra); do cp /usr/bin/$$f $(RootDir)/bin; done
	touch $(Status)/root

$(Status)/make: $(Status)/config
	cd $(SrcDir)/sshfs && make
	touch $(Status)/make

$(Status)/config: $(Status)/reconf
	cd $(SrcDir)/sshfs && ./configure
	touch $(Status)/config

$(Status)/reconf: $(Status)/patch
	cd $(SrcDir)/sshfs && autoreconf -i
	touch $(Status)/reconf

$(Status)/patch: $(Status)/clone
	cd $(SrcDir)/sshfs && for f in $(PrjDir)/patches/*.patch; do patch -p1 <$$f; done
	touch $(Status)/patch

$(Status)/clone:
	mkdir -p $(SrcDir)
	git clone $(PrjDir)/sshfs $(SrcDir)/sshfs
	touch $(Status)/clone

clean:
	git clean -dffx
