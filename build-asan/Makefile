## Automatically generated. http://www.rockbox.org/

export ROOTDIR=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu
export FIRMDIR=$(ROOTDIR)/firmware
export APPSDIR=$(ROOTDIR)/apps
export TOOLSDIR=$(ROOTDIR)/tools
export DOCSDIR=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/docs
export MANUALDIR=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/manual
export DEBUG=-DDEBUG
export MODELNAME=ipod6g
export FLASHFILE=
export TARGET_ID=71
export TARGET=-DIPOD_6G
export SYSFONT=08-Schumacher-Clean
export ARCH=arch_none
export ARCH_VERSION=
export ARCH_PROFILE=
export CPU=arm
export MANUFACTURER=s5l8702
export OBJDIR=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/build-asan
export BUILDDIR=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/build-asan
export RBCODEC_BLD=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/build-asan/lib/rbcodec
export VOICELANGUAGE=
export MEMORYSIZE=64
export BUILDDATE:=$(shell date -u +'-DYEAR=%Y -DMONTH=%m -DDAY=%d')
export MKFIRMWARE=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/tools/scramble -add=ip6g
export BMP2RB_MONO=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/tools/bmp2rb -f 0
export BMP2RB_NATIVE=/Volumes/Ricolinos/Codigo/GitHub/mi-ipod-os/.claude/worktrees/split-root-menu/tools/bmp2rb -f 4
export BMP2RB_REMOTEMONO=
export BMP2RB_REMOTENATIVE=
export BINARY=rockboxui
export APPEXTRA=recorder:gui:radio
export ENABLEDPLUGINS=yes
export EXTRA_DEFINES= -DSIMULATOR -DHAVE_TEST_PLUGINS
export HOSTCC=/usr/bin/gcc
export HOSTAR=/usr/bin/ar
export CC=/opt/homebrew/bin/gcc-15
export CPP=/opt/homebrew/bin/cpp-15
export LD=/usr/bin/ld
export AR=/opt/homebrew/bin/gcc-ar-15
export AS=/usr/bin/as
export OC=objcopy
export WINDRES=windres
export DLLTOOL=dlltool
export DLLWRAP=dllwrap
export RANLIB=gcc-ranlib
export PREFIX=
export PROFILE_OPTS=
export APP_TYPE=sdl-sim
export APPLICATION=
export SIMDIR=$(ROOTDIR)/uisimulator/sdl
export GCCOPTS=-W -Wall -Wextra -Wundef -Os -Wstrict-prototypes -pipe -std=gnu99 -funit-at-a-time -fno-delete-null-pointer-checks -fno-strict-overflow -fno-common -fno-builtin -g -fsanitize=address -fPIC -Wno-unused-result -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE -I$(SIMDIR) -Wno-pointer-sign -Wno-override-init -Wimplicit-fallthrough=0
export TARGET_INC=-I$(FIRMDIR)/target/arm/s5l8702/ipod6g -I$(FIRMDIR)/target/hosted/sdl -I$(FIRMDIR)/target/hosted -I$(FIRMDIR)/target/arm/s5l8702 -I$(FIRMDIR)/target/arm
export LOADADDRESS=
export SHARED_LDFLAGS=-dynamiclib -Wl,-no_warn_duplicate_libraries
export SHARED_CFLAGS=-fPIC -fvisibility=hidden
export LDOPTS= -fsanitize=address -lasan -lm -ldl -L/opt/homebrew/lib /opt/homebrew/lib/libSDL2.a -lm -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-weak_framework,CoreHaptics -Wl,-weak_framework,GameController -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-weak_framework,QuartzCore -Wl,-weak_framework,Metal
export GLOBAL_LDOPTS=
export LDMAP_OPT=-map
export GCCVER=15
export GCCNUM=1502
export UNAME=Darwin
export MANUALDEV=ipod6g
export TTS_OPTS=
export TTS_ENGINE=
export ENC_OPTS=
export ENCODER=
export USE_ELF=no
export PLUGIN_USE_ELF=no
export RBDIR=/.rockbox
export ROCKBOX_SHARE_PATH=
export ROCKBOX_BINARY_PATH=
export ROCKBOX_LIBRARY_PATH=
export SDLCONFIG=/opt/homebrew/bin/sdl2-config
export LCDORIENTATION=
export ANDROID_ARCH=
export ANDROID_NDK_PATH=
export ANDROID_SDK_PATH=
export FUNKEY_SDK_PATH=
export ANDROID_PLATFORM_VERSION=
export TOOLSET=rdf2binary convbdf codepages scramble bmp2rb
export PIPER_MODEL_DIR=



CONFIGURE_OPTIONS=  --sdl-threads --with-address-sanitizer --target=$(MODELNAME) --ram=$(MEMORYSIZE) --rbdir=$(RBDIR) --type=s

include $(TOOLSDIR)/root.make
