# To install:           Uncomment 'prerequisites' and the library name you'd like, 
#                       listed at the bottom of this file.  Then run build.sh.
#
# To uninstall:         Re-install your target library, then run the following
#                       command from the build directory.
#
#                       xargs rm < install_manifest.txt

set -e

#OS="Ubuntu"
OS="CentOS"
baseDir='/usr/local/VAPOR-Deps/2023-Mar-src'
installDir='/usr/local/VAPOR-Deps/current'
while getopts o:b:i flag
do
    case "${flag}" in
        b) baseDir=${OPTARG};;
        i) installDir=${OPTARG};;
        o) OS=${OPTARG};;
    esac
done



#OS="OSX"
#OS="M1"
#CC='clang'
#CXX='clang++'

osxPrerequisites() {
    CC='clang'
    CXX='clang++'
    brew install cmake
    brew install autoconf
    brew install atool
    brew install libtool
    brew install automake
    brew install xz zlib openssl
    brew install pkg-config openssl@1.1 xz gdbm tcl-tk # https://devguide.python.org/getting-started/setup-building/index.html#macos-and-os-x
}

ubuntuPrerequisites() {
    CC='gcc'
    CXX='g++'
    apt update -y
    apt upgrade -y

    # all for cmake
    apt-get update
    apt-get install -y gpg wget
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null
    DEBIAN_FRONTEND=noninteractive apt install -y software-properties-common
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
    apt install -y cmake --allow-unauthenticated

    apt install -y \
        m4 \
        libcurl4-openssl-dev \
        libxau-dev \
        autoconf \
        libtool \
        libxcb-xinerama0 \
        pkg-config \
        unzip \
        libssl-dev
    
    # Qt
    apt-get install -y \
        '^libxcb.*-dev' \
        libx11-xcb-dev \
        libglu1-mesa-dev \
        libxrender-dev \
        libxi-dev \
        libxkbcommon-dev \
        libxkbcommon-x11-dev
}

centosPrerequisites() {
    CC='gcc'
    CXX='g++'
	yum update -y
	yum install -y \
        epel-release \
        kernel-devel \
        gcc \
        gcc-c++ \
        cmake3 \
        make \
        xz-devel \
        zlib-devel \
        openssl-devel \
        expat-devel \
        libcurl-devel \
        qt5-qtbase-devel

    shopt -s expand_aliases
    echo alias cmake=\'cmake3\' >> ~/.bashrc
    . ~/.bashrc

    yum groupinstall -y "Development Tools"

	#yum install -y kernel-devel
	#yum install -y gcc
	#yum install -y cmake3 make
	#yum install -y xz-devel
	#yum install -y zlib-devel
	#yum install -y openssl-devel

	#curl -LO https://github.com/Kitware/CMake/releases/download/v3.26.0/cmake-3.26.0-linux-x86_64.tar.gz
	#tar -xvf cmake-3.26.0.tar.gz
	#mv cmake-3.26.0 /usr/local/cmake
	#echo 'export PATH="/usr/local/cmake/bin:$PATH"' >> ~/.bashrc
	#source ~/.bashrc
}

windowsPrerequisites() {
    choco install visualstudio2019-workload-vctools python cmake -y
    setx /M PATH "%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin"
    python -m pip install gdown
}

if [ $OS == "OSX" ]; then
    osxPrerequisites
elif [ $OS == "Ubuntu" ]; then
    ubuntuPrerequisites
elif [ $OS == "CentOS" ]; then
    centosPrerequisites
fi

libpng() {
    cd $baseDir
    local library='libpng-1.6.39'
    tar xvf $baseDir/$library.tar.xz 
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    make -j4 && make install
}

assimp() {
    cd $baseDir
    if [ $OS == "CentOS" ]; then
        local library='assimp-5.1.6'
    else
        local library='assimp-5.2.5' #requires c++17
    fi
    tar xvf $baseDir/$library.tar.gz
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake \
    -DCMAKE_INSTALL_PREFIX=$installDir \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -Wno-error=deprecated-declarations" \
    -DASSIMP_BUILD_TESTS=OFF \
    ..
    make -j4 && make install
}

zlib() {
    cd $baseDir
    local library='zlib-1.2.13'
    tar xvf $baseDir/$library.tar.gz
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    make -j4 && make install
}

#Note: after configuration, need to make sure both zlib and szlib are enabled!!
# How are we supposed do do that?  Configure does not indicate yes or no...
szip() {
    cd $baseDir
    local library='szip-2.1.1'
    tar xvf $baseDir/$library.tar.gz
    cd $baseDir/$library
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
}

#hdfVersion='1.14.0'
#hdfVersion='1.13.3'
hdfVersion='1.12.2'
hdf5() {
    cd $baseDir
    if [ $OS == "Ubuntu" ] || [ $OS == "CentOS" ]; then
        #tar xvf hdf5/hdf5-$hdfVersion-Std-centos7_64.tar.gz && cd hdf              # use this line for versions > 12.12.2
        tar xvf hdf5/hdf5-$hdfVersion-Std-centos7_64-7.2.0.tar.gz && cd hdf         # use this line for versions = 12.12.2
        ./HDF5-$hdfVersion-Linux.sh --prefix=$installDir --exclude-subdir --skip-license
    elif [ $OS == "OSX" ]; then
        tar xvf hdf5/hdf5-$hdfVersion-Std-macos11_64-clang.tar.gz && cd hdf
        ./HDF5-$hdfVersion-Darwin.sh --prefix=$installDir --exclude-subdir --skip-license
    elif [ $OS == "M1" ]; then
        tar xvf hdf5/hdf5-$hdfVersion-Std-macos11m1_64-clang.tar.gz && cd hdf
        ./HDF5-$hdfVersion-Darwin.sh --prefix=$installDir --exclude-subdir --skip-license
    fi

    ln -s $installDir/HDF_Group/HDF5/$hdfVersion/lib/plugin/ $installDir/share/plugins
}

netcdf() {
    cd $baseDir
    local library='netcdf-c-4.9.1'
    tar xvf $baseDir/$library.tar.gz
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build

    cmake \
    -DCMAKE_INSTALL_PREFIX=$installDir \
    -DCMAKE_PREFIX_PATH="$installDir/HDF_Group/HDF5/$hdfVersion" \
    -DENABLE_BYTERANGE=False \
    -DENABLE_DAP=False \
    -DCMAKE_BUILD_TYPE=Release ..

    make && make install
}

expat() {
    cd $baseDir
    local library='expat-2.5.0'
    tar xvf $baseDir/$library.tar.xz 
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    make -j4 && make install
}

udunits() {
    cd $baseDir
    local library='udunits-2.2.28'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library
    LDFLAGS=-L$installDir/lib/ \
    CPPFLAGS=-I$installDir/include/ \
    CC=$CC CXX=$CXX \
    ./configure \
    --prefix=$installDir
    make -j4 && make install
}

freetype() {
    cd $baseDir
    local library='freetype-2.13.0'
    tar xvf $baseDir/$library.tar.xz
    cd $baseDir/$library
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
}

#CC=clang CXX=clang++ ./configure --prefix=/usr/local/VAPOR-Deps/2019-Aug
jpeg() {
    cd $baseDir
    tar xvf $baseDir/jpegsrc.v9e.tar.gz
    cd $baseDir/jpeg-9e
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
}

tiff() {
    cd $baseDir
    local library='libtiff-v4.5.0'
    tar xvf $baseDir/$library.tar.gz
    cd $baseDir/$library
    if [ $OS == "OSX" ] || [ $OS == "M1" ]; then
        glibtoolize --force
    else
        libtoolize --force
    fi
    aclocal
    autoheader
    automake --force-missing --add-missing
    autoconf
    LDFLAGS=-L$installDir/lib \
    CPPFLAGS=-I$installDir/include \
    CC=$CC CXX=$CXX \
    ./configure \
    --prefix=$installDir \
    --disable-dap
    make -j4 && make install
}

sqlite() {
    cd $baseDir
    local library='sqlite-autoconf-3410000'
    tar xvf $baseDir/$library.tar.gz
    cd $baseDir/$library
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
}

proj() {
    cd $baseDir
    #local library='proj-9.1.0' # does not work
    #local library='proj-6.3.1' # works
    local library='proj-7.2.1' # ?
    tar xvf $baseDir/$library.tar.gz
    tar xvf proj-datumgrid-1.8.tar.gz -C $library/data
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    
    if [ $OS == "OSX" ] || [ $OS == "M1" ]; then
        local sqliteLib="-DSQLITE3_LIBRARY=$installDir/lib/libsqlite3.dylib"
    else
        local sqliteLib="-DSQLITE3_LIBRARY=$installDir/lib/libsqlite3.so"
    fi

    cmake \
    -DCMAKE_PREFIX_PATH=$installDir \
    -DEXE_SQLITE3=$installDir/bin/sqlite3 \
    -DSQLITE3_INCLUDE_DIR=$installDir/include \
    $sqliteLib \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_INSTALL_PREFIX=$installDir \
    -DPROJ_COMPILER_NAME=$CXX \
    ..

    make -j4 && make install
}

geotiff() {
    cd $baseDir
    local library='libgeotiff-1.7.1'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library

    LDFLAGS=-L$installDir/lib/ \
    CPPFLAGS=-I$installDir/include/ \
    CC=$CC CXX=$CXX \
    ./configure \
    --prefix=$installDir \
    --with-zlib=yes \
    --with-jpeg=yes \
    --with-proj=$installDir

    make -j4 && make install
}

xinerama() {
    cd $baseDir
    local library='xcb-proto-1.15.2'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library
    ./configure --prefix=$installDir
    make -j4 && make install

    cd $baseDir
    library='libxcb-1.15'
    export PYTHONPATH=$installDir/local/lib/python3.10/dist-packages
    tar xvf $baseDir/$library.tar.xz && cd $baseDir/$library
    PYTHON=python3 PKG_CONFIG_PATH=$installDir/share/pkgconfig ./configure --without-doxygen --docdir='${datadir}'/doc/libxcb-1.15 --prefix=$installDir
    make -j4 && make install
}

openssl() {
    cd $baseDir
    local library='openssl-1.1.1t'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library
    ./config shared --prefix=$installDir --openssldir=$installDir
    make -j4 && make install
}

python() {
    cd $baseDir
    local library='cpython-3.9.16'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library
    if [ $OS = "OSX" ] && [ $OS = "M1" ]; then
        export PKG_CONFIG_PATH="$(brew --prefix tcl-tk)/lib/pkgconfig"; \
        CC=$CC \
        CXX=$CXX \
        LDFLAGS="-L$installDir/lib -L$(brew --prefix zlib)/include -I$(brew --prefix openssl)/include" \
        CPPFLAGS="-I$installDir/include -I$(brew --prefix zlib)/include -I$(brew --prefix openssl)/include" \
        ./configure \
        --prefix=$installDir \
        --enable-shared \
        --with-ensurepip=install \
        --with-suffix=.vapor \
        --enable-optimizations \
        --with-openssl=$(brew --prefix openssl@1.1) \
        --with-tcltk-libs="$(pkg-config --libs tcl tk)" \
        --with-tcltk-includes="$(pkg-config --cflags tcl tk)"
    elif [ $OS = "CentOS" ]; then
        CPPFLAGS=-I$installDir/include \
        LDFLAGS="-L$installDir/lib -Wl,-rpath=$installDir/lib" \
        CC=$CC \
        CXX=$CXX \
        ./configure \
        --prefix=$installDir \
        --enable-shared \
        --with-ensurepip=install \
        --with-suffix=.vapor \
        --with-openssl=$installDir
    else
        CPPFLAGS=-I$installDir/include \
        LDFLAGS="-L$installDir/lib -Wl,-rpath=$installDir/lib" \
        CC=$CC \
        CXX=$CXX \
        ./configure \
        --prefix=$installDir \
        --enable-shared \
        --with-ensurepip=install \
        --with-suffix=.vapor \
        --enable-optimizations \
        --with-openssl=$installDir
    fi

    make -j4 && make install

    $installDir/bin/python3.9.vapor -m pip install --upgrade pip
    $installDir/bin/pip3 install --upgrade --target $installDir/lib/python3.9/site-packages numpy scipy matplotlib
}

ospray() {
    cd $baseDir
    if [ $OS != "OSX" ]; then
        local library='ospray-2.11.0.x86_64.linux'
        tar xvf $baseDir/ospray/$library.tar.gz && cd $baseDir/$library
    else
        local library='ospray-2.11.0.x86_64.macosx'
        unzip $baseDir/ospray/$library && cd $baseDir/$library
    fi
    mkdir -p $installDir/Ospray
    cp -r * $installDir/Ospray
}

glm() {
    cd $baseDir
    local library='glm-0.9.9.8'
    unzip $baseDir/$library.zip && cp -r $baseDir/glm/glm $installDir/include
    #unzip $library.zip && mkdir -p glm/build
    #cd glm/build
    #cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    #make -j4 && make install
}

gte() {
    cd $baseDir
    tar xvf GTE.tar.xz
    mv GTE $installDir/include
}

images() {
    cd $baseDir
    tar xvf images.tar.xz
    mv images $installDir/share
}

qt() {
    cd $baseDir
    if [ $OS == "CentOS" ]; then
        local library='qt-everywhere-src-5.13.2'
        yum install -y wget
        rm -rf /usr/local/VAPOR-Deps/2023-Mar-src/qt-everywhere-src-5.13.2.tar.xz
        wget https://download.qt.io/archive/qt/5.13/5.13.2/single/qt-everywhere-src-5.13.2.tar.xz
        tar xvf $baseDir/$library.tar.xz && cd $baseDir/$library
    else 
        local library='qt-everywhere-opensource-src-5.15.8'
        tar xvf $baseDir/$library.tar.xz && cd $baseDir/qt-everywhere-src-5.15.8
    fi

    CPPFLAGS=-I$installDir/include \
    LDFLAGS="-L$installDir/lib -Wl,-rpath=$installDir/lib" \
    CC=$CC \
    CXX=$CXX \
    ./configure \
    -prefix $installDir \
    -opensource \
    -confirm-license \
    -release \
    -nomake examples \
    -nomake tests
    
    if [ $OS == "CentOS" ]; then
        /usr/bin/make -j4 && make install # CentOS can't find make when building qt as of 4/10/20023, so give full path to make
    else
        make -j4 && make install
    fi
    #Qt/qt-unified-linux-x64-4.5.1-online.run --script Qt/qt-installer-noninteractive.qs
}


if [ $OS == "OSX" ]; then
    osxPrerequisites
elif [ $OS == "Ubuntu" ]; then
    ubuntuPrerequisites
elif [ $OS == "CentOS" ]; then
    centosPrerequisites
elif [ $OS == "Windows" ]; then
    windowsPrerequisites
fi

#openssl
#python
#zlib
#libpng
#assimp
#szip
#hdf5
#netcdf
#expat
#udunits
#freetype
#jpeg
#tiff
#sqlite
#proj
#geotiff
#if [ $OS == "Ubuntu" ] ; then
#   xinerama
#fi         
#ospray
#glm
#gte
#images
qt
