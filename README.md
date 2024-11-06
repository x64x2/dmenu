# laiin - WORK IN PROGRESS (ON HOLD)


laiin is a decentralized peer-to-peer marketplace for trading goods and services with [**Monero**](https://getmonero.org/)


> __Disclaimer: The project is comprised of a single developer that operates independently
> and is not affiliated, associated, authorized, endorsed by, or in any way officially connected
> with the Monero project, Monero team or any other organization.__


## Table of contents
<!-- - [The history behind laiin](#about)-->
- [Demo](#demo)
- [Project Status](#project-status) <!-- - [Documentation](#documentation)-->
- [Building laiin](#building-laiin)
  - [Dependencies](#dependencies)
  - [Compiling laiin from source](#compiling-laiin-from-source)
- [Contributing](#contributing) <!-- - [Bug Bounty Program]-->
- [License](#license)
- [Donations](#donations)
- [Resources](#resources)
- [Thanks](#thanks)

# Background

Laiin is an implementation that leverages what I learned in the Ethereum Hook Incubator. My goal was to create smart contract interactions where I could minimize the technical complexity for the end-user. I used variations of signed messages, hashes, proofs to streamline how people can trade without compromising their privacy.

## Building laiin

### Dependencies
:heavy_check_mark: = Currently in use | :o: = Optional | :x: = Marked for deprecation or removed | :grey_question: = Not in use, but may be considered 

:white_square_button: = For CLI only | :package: = Bundled

|      Library                                                       | Minimum Ver.       |         Purpose                                                        | Status                                             |
|--------------------------------------------------------------------|--------------------|------------------------------------------------------------------------|----------------------------------------------------|
| [monero-cpp](https://github.com/woodser/monero-cpp)                | latest             | monero wallet and payment system                                       | :heavy_check_mark: :package:                       |
| [sqlite3](https://sqlite.org/)                                     | 3.38.0             | database management                                                    | :heavy_check_mark: :package:                       |
| [QR Code generator](https://github.com/nayuki/QR-Code-generator)   | ?                  | qr code generation                                                     | :heavy_check_mark: :package:                       |
| [json](https://github.com/nlohmann/json/)                          | ?                  | json parsing and msgpack                                               | :heavy_check_mark: :package:                       |
| [curl](https://github.com/curl/curl)                               | ?                  | currency conversion                                                    | :heavy_check_mark: :white_square_button:           |
| [openssl](https://github.com/openssl/openssl)                      | 1.1.1              | for curl, sha256 sum and message encryption                            | :heavy_check_mark:                                 |
| [Qt](https://www.qt.io/)                                           | 5.15.x             | graphical user interface                                               | :heavy_check_mark:                                 |
| [stduuid](https://github.com/mariusbancila/stduuid)                | ?                  | unique id generation                                                   | :heavy_check_mark: :white_square_button: :package: |
| [linenoise](https://github.com/antirez/linenoise)                  | ?                  | command line interface                                                 | :heavy_check_mark: :white_square_button: :package: |
| [lua](https://www.lua.org/)                                        | 5.1.5              | configuration script                                                   | :heavy_check_mark: :package:                       |
| [cxxopts](https://github.com/jarro2783/cxxopts)                    | ?                  | command line option parser                                             | :heavy_check_mark: :package:                       |
| [libi2pd](https://github.com/PurpleI2P/i2pd)                       | latest             | network proxy                                                          | :heavy_check_mark: :package:                       |
| [i2psam](https://github.com/i2p/i2psam)                            | ?                  | network proxy                                                          | :grey_question: :package:                          |

### Compiling laiin from source
**0. Install prerequisites**

Debian/Ubuntu
```bash
sudo apt install build-essential cmake git
```
Arch
```bash
sudo pacman -Sy --needed base-devel cmake git
```
Fedora
```bash
sudo dnf install gcc gcc-c++ make cmake git
```


**1. Clone laiin (and its submodules)**
```bash
git clone --recurse-submodules https://github.com/layters/testshop.git
```
```bash
cd testshop
```

**2. Install dependencies**

Debian/Ubuntu
```bash
# laiin
sudo apt install libcurl4-openssl-dev libssl-dev qtdeclarative5-dev qml-module-qt-labs-platform qml-module-qtquick-controls qml-module-qtquick-controls2 qml-module-qtquick-shapes qml-module-qtquick-dialogs
# monero-cpp (monero)
sudo apt update && sudo apt install pkg-config libssl-dev libzmq3-dev libunbound-dev libsodium-dev libunwind8-dev liblzma-dev libreadline6-dev libexpat1-dev libpgm-dev qttools5-dev-tools libhidapi-dev libusb-1.0-0-dev libprotobuf-dev protobuf-compiler libudev-dev libboost-chrono-dev libboost-date-time-dev libboost-filesystem-dev libboost-locale-dev libboost-program-options-dev libboost-regex-dev libboost-serialization-dev libboost-system-dev libboost-thread-dev python3 ccache
```
Arch (missing Qt/QML libraries)
```bash
# laiin
sudo pacman -Sy --needed curl openssl qt5-declarative
# monero-cpp (monero)
sudo pacman -Syu --needed boost openssl zeromq libpgm unbound libsodium libunwind xz readline expat gtest python3 ccache qt5-tools hidapi libusb protobuf systemd
```
Fedora (missing Qt/QML libraries)
```bash
# laiin
sudo dnf install libcurl-devel openssl-devel
# monero-cpp (monero)
sudo dnf install boost-static libstdc++-static pkgconf boost-devel openssl-devel zeromq-devel openpgm-devel unbound-devel libsodium-devel libunwind-devel xz-devel readline-devel expat-devel gtest-devel ccache qt5-linguist hidapi-devel libusbx-devel protobuf-devel protobuf-compiler systemd-devel
```


**3. Update monero-cpp submodules**
```bash
cd external/monero-cpp && ./bin/update_submodules.sh
```
```bash
cd external/monero-project
```


**4. Install expat and unbound (May be required to build monero-project on Debian/Ubuntu otherwise, this step can be skipped):**
```bash
wget https://github.com/libexpat/libexpat/releases/download/R_2_4_8/expat-2.4.8.tar.bz2
tar -xf expat-2.4.8.tar.bz2
sudo rm expat-2.4.8.tar.bz2
cd expat-2.4.8
./configure --enable-static --disable-shared
make
sudo make install
cd ../
```

```bash
wget https://www.nlnetlabs.nl/downloads/unbound/unbound-1.19.0.tar.gz
tar xzf unbound-1.19.0.tar.gz
sudo apt update
sudo apt install -y build-essential
sudo apt install -y libssl-dev
sudo apt install -y libexpat1-dev
sudo apt-get install -y bison
sudo apt-get install -y flex
cd unbound-1.19.0
./configure --with-libexpat=/usr --with-ssl=/usr --enable-static-exe
make
sudo make install
cd ../
```

<!-- git submodule update --init --force --recursive --> <!-- <= call this before building monero -->

**5. Build monero-project to create .a libraries**
```bash
make release-static -j$(nproc)
```
```bash
cd ../../../../
```


**6. Build laiin**

To build with [**CMake**](https://cmake.org/):

```bash
# Build external libraries
cd external/
cmake .
make -j$(nproc)
cd ..
```

```bash
# Build laiin
cd build
cmake .. #-Dlaiin_BUILD_CLI=1 #-Dlaiin_BUILD_TESTS=1
make -j$(nproc)
```


**7. Run laiin**
```bash
# Run laiin
./laiin
```



## Donations
**Monero (XMR):**
```
49FCj7XzyRVR3i7dwyqF6WabujydDBy1o957KHFgNbC7XsM4Kd7Duj8fxVCYeP7iU29hrG1pADh9eJusSAwWqwf9Jzpjt9S
```

## Resources
- Website: [laiin.org](https://laiin.org/)
- Matrix Rooms: 
    - [#laiin:matrix.org](https://matrix.to/#/#laiin:matrix.org)
    - [#laiin-dev:matrix.org](https://matrix.to/#/#laiin-dev:matrix.org)


## Thanks
* [yuriio147](https://github.com/yuriio147)
* [u/EchoingCat](https://www.reddit.com/user/EchoingCat/)

[//]: # (./clean.sh)
[//]: # (git checkout -b main)
[//]: # (git add .gitignore .gitmodules assets/ cmake/ CMakeLists.txt external/ LICENSE meson.build meson.options qml/ qml.qrc README.md src/ tests/)
[//]: # (git commit -m"..."    or    git commit -a --allow-empty-message -m "")
[//]: # (git push -u origin backup --force)
[//]: # (https://git.wownero.com/layter/laiin/settings => Mirror Settings => Synchronize Now)
[//]: # (removing an external lib from submodules index: git rm --cached path/to/submodule)
