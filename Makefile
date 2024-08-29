MyProductName = "SSHFS-Win"
MyCompanyName = "Navimatics LLC"
MyDescription = "SSHFS for Windows"
MyProductVersion = "2021.1 Beta2"
MyProductStage = "Beta"
MyVersion = 3.7.$(shell date '+%y%j')
ifeq ($(shell uname -m),x86_64)
	MyArch = x64
else
	MyArch = x86
endif

CertIssuer = "DigiCert"
CrossCert = "DigiCert High Assurance EV Root CA.crt"

PrjDir	= $(shell pwd)
BldDir	= .build/$(MyArch)
DistDir = $(BldDir)/dist
SrcDir	= $(BldDir)/src
RootDir	= $(BldDir)/root
WixDir	= $(BldDir)/wix
Status	= $(BldDir)/status
BinExtra= ssh #bash ls mount

export PATH := $(shell cygpath -au "$$WIX")/bin:$(PATH)

goal: $(Status) $(Status)/done

$(Status):
	mkdir -p $(Status)

$(Status)/done: $(Status)/dist
	touch $(Status)/done

$(Status)/dist: $(Status)/wix
	mkdir -p $(DistDir)
	cp $(shell cygpath -aw $(WixDir)/sshfs-win-$(MyVersion)-$(MyArch).msi) $(DistDir)
	tools/signtool sign \
		/ac tools/$(CrossCert) \
		/i $(CertIssuer) \
		/n $(MyCompanyName) \
		/d $(MyDescription) \
		/fd sha1 \
		/t http://timestamp.digicert.com \
		'$(shell cygpath -aw $(DistDir)/sshfs-win-$(MyVersion)-$(MyArch).msi)' || \
		echo "SIGNING FAILED! The product has been successfully built, but not signed." 1>&2
	touch $(Status)/dist

$(Status)/wix: $(Status)/sshfs-win sshfs-win.wxs
	mkdir -p $(WixDir)
	cp sshfs-win.wxs $(WixDir)/
	candle -nologo -arch $(MyArch) -pedantic\
		-dMyProductName=$(MyProductName)\
		-dMyCompanyName=$(MyCompanyName)\
		-dMyDescription=$(MyDescription)\
		-dMyProductVersion=$(MyProductVersion)\
		-dMyProductStage=$(MyProductStage)\
		-dMyVersion=$(MyVersion)\
		-dMyArch=$(MyArch)\
		-o "$(shell cygpath -aw $(WixDir)/sshfs-win.wixobj)"\
		"$(shell cygpath -aw $(WixDir)/sshfs-win.wxs)"
	heat dir $(shell cygpath -aw $(RootDir))\
		-nologo -dr INSTALLDIR -cg C.Main -srd -ke -sreg -gg -sfrag\
		-o $(shell cygpath -aw $(WixDir)/root.wxs)
	candle -nologo -arch $(MyArch) -pedantic\
		-dMyProductName=$(MyProductName)\
		-dMyCompanyName=$(MyCompanyName)\
		-dMyDescription=$(MyDescription)\
		-dMyProductVersion=$(MyProductVersion)\
		-dMyProductStage=$(MyProductStage)\
		-dMyVersion=$(MyVersion)\
		-dMyArch=$(MyArch)\
		-o "$(shell cygpath -aw $(WixDir)/root.wixobj)"\
		"$(shell cygpath -aw $(WixDir)/root.wxs)"
	light -nologo\
		-o $(shell cygpath -aw $(WixDir)/sshfs-win-$(MyVersion)-$(MyArch).msi)\
		-ext WixUIExtension\
		-b $(RootDir)\
		$(shell cygpath -aw $(WixDir)/root.wixobj)\
		$(shell cygpath -aw $(WixDir)/sshfs-win.wixobj)
	touch $(Status)/wix

$(Status)/sshfs-win: $(Status)/root sshfs-win.c
	gcc -o $(RootDir)/bin/sshfs-win sshfs-win.c
	strip $(RootDir)/bin/sshfs-win
	touch $(Status)/sshfs-win

$(Status)/root: $(Status)/make
	mkdir -p $(RootDir)/{bin,dev/{mqueue,shm},etc}
	(cygcheck $(SrcDir)/sshfs/build/sshfs; for f in $(BinExtra); do cygcheck /usr/bin/$$f; done) |\
		tr -d '\r' | tr '\\' / | xargs cygpath -au | grep '^/usr/bin/' | sort | uniq |\
		while read f; do cp $$f $(RootDir)/bin; done
	cp $(SrcDir)/sshfs/build/sshfs $(RootDir)/bin
	strip $(RootDir)/bin/sshfs
	for f in $(BinExtra); do cp /usr/bin/$$f $(RootDir)/bin; done
	cp -R $(PrjDir)/etc $(RootDir)
	touch $(Status)/root

$(Status)/make: $(Status)/config
	cd $(SrcDir)/sshfs/build && ninja
	touch $(Status)/make

$(Status)/config: $(Status)/patch
	mkdir -p $(SrcDir)/sshfs/build
	cd $(SrcDir)/sshfs/build && meson ..
	touch $(Status)/config

$(Status)/patch: $(Status)/clone
	cd $(SrcDir)/sshfs &&                      \
	for f in $(PrjDir)/patches/*.patch; do     \
		patch -p1          <$$f >/dev/null ||  \
		patch -p1 --binary <$$f;               \
	done
	touch $(Status)/patch

$(Status)/clone:
	mkdir -p $(SrcDir)
	git clone $(PrjDir)/sshfs $(SrcDir)/sshfs
	touch $(Status)/clone

clean:
	git clean -dffx
