## SerenityOS build instructions

### Prerequisites

#### Linux prerequisites
Make sure you have all the dependencies installed (`ninja` is optional, but is faster in practice):

**Debian / Ubuntu**
```bash
sudo apt install build-essential cmake curl libmpfr-dev libmpc-dev libgmp-dev e2fsprogs ninja-build qemu-system-i386 qemu-utils
```

**Fedora**
```bash
sudo dnf install curl cmake mpfr-devel libmpc-devel gmp-devel e2fsprogs ninja-build patch @"C Development Tools and Libraries" @Virtualization
```

**openSUSE**
```bash
sudo zypper install curl cmake mpfr-devel mpc-devel ninja gmp-devel e2fsprogs patch qemu-x86 qemu-audio-pa gcc gcc-c++ patterns-devel-C-C++-devel_C_C++
```

**Arch Linux / Manjaro**
```bash
sudo pacman -S --needed base-devel cmake curl mpfr libmpc gmp e2fsprogs ninja qemu qemu-arch-extra
```

**ALT Linux**
```bash
apt-get install curl cmake libmpc-devel gmp-devel e2fsprogs libmpfr-devel ninja-build patch gcc
```

Ensure your gcc version is >= 10 with `gcc --version`. Otherwise, install it.

On Ubuntu it's in the repositories of 20.04 (Focal) and later - add the `ubuntu-toolchain-r/test` PPA if you're running an older version:
```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
```

On Debian you can use the Debian testing branch:
```bash
sudo echo "deb http://http.us.debian.org/debian/ testing non-free contrib main" >> /etc/apt/sources.list
sudo apt update
```

Now on Ubuntu or Debian you can install gcc-10 with apt like this:
```bash
sudo apt install gcc-10 g++-10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 900 --slave /usr/bin/g++ g++ /usr/bin/g++-10
```

If you don't want to stay on the testing branch you can switch back by running:
```bash
sudo sed -i '$d' /etc/apt/sources.list
sudo apt update
```

Ensure your CMake version is >= 3.16 with `cmake --version`. If your system doesn't provide a suitable version of CMake, you can download a binary release from the [CMake website](https://cmake.org/download).

**NixOS**

You can use a `nix-shell` script like the following to set up the correct environment:

myshell.nix:
```
with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "cpp-env";
  nativeBuildInputs = [
    gcc10
    curl
    cmake
    mpfr
    ninja
    gmp
    libmpc
    e2fsprogs
    patch

    # Example Build-time Additional Dependencies
    pkgconfig
  ];
  buildInputs = [
    # Example Run-time Additional Dependencies
    openssl
    x11
    # glibc
  ];
  hardeningDisable = [ "format" "fortify" ];
}
```

Then use this script: `nix-shell myshell.nix`.

Once you're in nix-shell, you should be able to follow the build directions.

#### macOS prerequisites
Make sure you have all the dependencies installed:
```bash
brew install coreutils qemu e2fsprogs m4 autoconf libtool automake bash gcc@10 ninja
brew install --cask osxfuse
Toolchain/BuildFuseExt2.sh
```

Notes: 
- fuse-ext2 is not available as brew formula so it must be installed using `BuildFuseExt2.sh`
- Xcode and `xcode-tools` must be installed (`git` is required by some scripts)
- coreutils is needed to build gcc cross compiler
- qemu is needed to run the compiled OS image. You can also build it using the `BuildQemu.sh` script
- osxfuse, e2fsprogs, m4, autoconf, automake, libtool and `BuildFuseExt2.sh` are needed if you want to build the root filesystem disk image natively on macOS. This allows mounting an EXT2 fs and also installs commands like `mke2fs` that are not available on stock macOS. 
- Installing osxfuse for the first time requires enabling its system extension in System Preferences and then restarting your machine. The output from installing osxfuse with brew says this, but it's easy to miss.
- bash is needed because the default version installed on macOS doesn't support globstar
- If you install some commercial EXT2 macOS fs handler instead of osxfuse and fuse-ext2, you will need to `brew install e2fsprogs` to obtain `mke2fs` anyway.
- As of 2020-08-06, you might need to tell the build system about your newer host compiler. Once you've built the toolchain, navigate to `Build/`, `rm -rf *`, then run `cmake .. -G Ninja -DCMAKE_C_COMPILER=gcc-10 -DCMAKE_CXX_COMPILER=g++-10`, then continue with `ninja install` as usual.

#### OpenBSD prerequisites
```
$ doas pkg_add bash cmake g++ gcc git gmake gmp ninja
```

To use `ninja image` and `ninja run`, you'll need Qemu and other utilities:
```
$ doas pkg_add coreutils qemu sudo
```

#### FreeBSD prerequisites
```
$ pkg add bash coreutils git gmake ninja sudo
```

#### Windows
For Windows, you will require Windows Subsystem for Linux 2 (WSL2). [Follow the WSL2 instructions here.](https://github.com/SerenityOS/serenity/blob/master/Documentation/NotesOnWSL.md)
Do note the ```Hardware acceleration``` and ```Note on filesystems``` sections, otherwise performance will be terrible.
Once you have installed a distro for WSL2, follow the Linux prerequisites above for the distro you installed, then continue as normal.

You may also want to install [ninja](https://github.com/ninja-build/ninja/releases)

### Build
Go into the `Toolchain/` directory and run the **BuildIt.sh** script:
```bash
$ cd Toolchain
$ ./BuildIt.sh
```

Building the toolchain will also automatically create a `Build/` directory for the build to live in.

Once the toolchain has been built, go into the `Build/` directory and run the commands. Note that while `ninja` seems to be faster, you can also just use GNU make, by omitting `-G Ninja` and calling `make` instead of `ninja`:
```bash
$ cd ..
$ cd Build
$ cmake .. -G Ninja
$ ninja
$ ninja install
```

This will compile all of SerenityOS and install the built files into `Root/` inside the build tree. `ninja install` actually pulls in the regular `ninja` (`ninja all`) automatically, so there isn't really a need to run it explicitly. `ninja` will automatically build as many jobs in parallel as it detects processors; `make` builds only one job in parallel. (Use the `-j` option with an argument if you want to change this.)

Now to build a disk image, run `ninja image`, and take it for a spin by using `ninja run`.
```bash
$ ninja image
$ ninja run
```

Note that the `anon` user is able to become `root` without password by default, as a development convenience.
To prevent this, remove `anon` from the `wheel` group and he will no longer be able to run `/bin/su`.

On Linux, QEMU is significantly faster if it's able to use KVM. The run script will automatically enable KVM if `/dev/kvm` exists and is readable+writable by the current user.

Bare curious users may even consider sourcing suitable hardware to [install Serenity on a physical PC.](https://github.com/SerenityOS/serenity/blob/master/Documentation/INSTALL.md)

Outside of QEMU, Serenity will run on VirtualBox. If you're curious, see how to [install Serenity on VirtualBox.](https://github.com/SerenityOS/serenity/blob/master/Documentation/VirtualBox.md)

Later on, when you `git pull` to get the latest changes, there's (usually) no need to rebuild the toolchain. You can simply run `ninja install`, `ninja image`, and `ninja run` again. CMake will only rebuild those parts that have been updated.

#### Ports
To add a package from the ports collection to Serenity, for example curl, go into `Ports/curl/` and run **./package.sh**. The sourcecode for the package will be downloaded and the package will be built. After that, run **make image** from the `Build/` directory to update the disk image. The next time you start Serenity with **make run**, `curl` will be available.

#### Keymap

Create a file with the exact name `sync-local.sh` in the project root (the same directory as `.clang-format`), with content like this:

```
#!/bin/sh

set -e

cat << 'EOF' >> mnt/etc/SystemServer.ini

[keymap]
Executable=/bin/keymap
Arguments=de
User=anon
EOF
```

This will configure your keymap to German (`de`) instead of US English. See [`Base/res/keymaps/`](../Base/res/keymaps/) for a full list.
