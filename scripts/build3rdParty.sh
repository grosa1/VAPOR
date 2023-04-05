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


if [ $OS == "OSX" ]; then
    CC='clang'
    CXX='clang++'
    osxPrerequisites
elif [ $OS == "Ubuntu" ]; then
    ubuntuPrerequisites
    CC='gcc'
    CXX='g++'
elif [ $OS == "CentOS" ]; then
    centosPrerequisites
    CC='gcc'
    CXX='g++'
fi

#OS="OSX"
#OS="M1"
#CC='clang'
#CXX='clang++'

osxPrerequisites() {
    brew install cmake
    brew install autoconf
    brew install atool
    brew install libtool
    brew install automake
    brew install xz zlib openssl
    brew install pkg-config openssl@1.1 xz gdbm tcl-tk # https://devguide.python.org/getting-started/setup-building/index.html#macos-and-os-x
}

ubuntuPrerequisites() {
    sudo apt update
    sudo apt upgrade

    # All of this for updating to cmake > 3.16
    sudo apt remove --purge --auto-remove cmake
    sudo apt install -y software-properties-common lsb-release && \
    sudo apt clean all
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
    sudo apt install kitware-archive-keyring
    sudo rm /etc/apt/trusted.gpg.d/kitware.gpg
    sudo apt update
    sudo apt install cmake

    #sudo apt install -y curl
    sudo apt install -y m4 libcurl4-openssl-dev libxau-dev autoconf libtool libxcb-xinerama0
    #sudo apt -y install 
    #sudo apt install autoconf
    #sudo apt install libtool
    #sudo apt install libxcb-xinerama0

    sudo apt-get install libssl-dev
    # Qt
    sudo apt-get install -y '^libxcb.*-dev' libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev
}

centosPrerequisites() {
	sudo yum update -y
	sudo yum install -y epel-release
	sudo yum install -y kernel-devel
	sudo yum install -y gcc
	sudo yum install -y cmake3
	sudo yum install -y xz-devel
	sudo yum install -y zlib-devel
	sudo yum install -y openssl-devel

	#curl -LO https://github.com/Kitware/CMake/releases/download/v3.26.0/cmake-3.26.0-linux-x86_64.tar.gz
	#tar -xvf cmake-3.26.0.tar.gz
	#mv cmake-3.26.0 /usr/local/cmake
	#echo 'export PATH="/usr/local/cmake/bin:$PATH"' >> ~/.bashrc
	#source ~/.bashrc
}

libpng() {
    local library='libpng-1.6.39'
    tar xvf $baseDir/$library.tar.xz 
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    make -j4 && make install
    cd $baseDir
}

assimp() {
    local library='assimp-5.2.5'
    tar xvf $baseDir/$library.tar.gz
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG -Wno-error=deprecated-declarations" -Wno-error=deprecated-declarations ..
    make -j4 && make install
    cd $baseDir
}

zlib() {
    local library='zlib-1.2.13'
    tar xvf $baseDir/$library.tar.gz
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    make -j4 && make install
    cd $baseDir
}

#Note: after configuration, need to make sure both zlib and szlib are enabled!!
# How are we supposed do do that?  Configure does not indicate yes or no...
szip() {
    local library='szip-2.1.1'
    tar xvf $baseDir/$library.tar.gz
    cd $baseDir/$library
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
    cd $baseDir
}

#hdfVersion='1.14.0'
#hdfVersion='1.13.3'
hdfVersion='1.12.2'
hdf5() {
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
    
    cd $baseDir
}

netcdf() {
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
    cd $baseDir
}

expat() {
    local library='expat-2.5.0'
    tar xvf $baseDir/$library.tar.xz 
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    make -j4 && make install
    cd $baseDir
}

udunits() {
    local library='udunits-2.2.28'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library
    LDFLAGS=-L$installDir/lib/ CPPFLAGS=-I$installDir/include/ CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
    cd $baseDir
}

freetype() {
    local library='freetype-2.13.0'
    tar xvf $baseDir/$library.tar.xz
    cd $baseDir/$library
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
    cd $baseDir
}

#CC=clang CXX=clang++ ./configure --prefix=/usr/local/VAPOR-Deps/2019-Aug
jpeg() {
    tar xvf $baseDir/jpegsrc.v9e.tar.gz
    cd $baseDir/jpeg-9e
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
    cd $baseDir
}

tiff() {
    #sudo apt install autoconf
    #sudo apt install libtool
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
    LDFLAGS=-L$installDir/lib CPPFLAGS=-I$installDir/include CC=$CC CXX=$CXX ./configure --prefix=$installDir --disable-dap
    make -j4 && make install

    cd $baseDir
}

tiffCmake() { # Does not work
    #sudo apt install autoconf
    #sudo apt install libtool
    local library='libtiff-v4.5.0'
    #local library='libtiff-v4.4.0'
    tar xvf $baseDir/$library.tar.gz
    mkdir -p $baseDir/$library/build && cd $baseDir/$library/build
    
    if [ $OS == "OSX" ] || [ $OS == "M1" ]; then
        local jpegLib="-DSQLITE3_LIBRARY=$installDir/lib/libjpeg.dylib"
    else
        local jpegLib="-DSQLITE3_LIBRARY=$installDir/lib/libjpeg.so"
    fi

    cmake \
    -DCMAKE_PREFIX_PATH="$installDir" \
    -Djpeg=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DJPEG_INCLUDE_DIR="$installDir/include" \
    -DJPEG_LIBRARY="/usr/local/VAPOR-Deps/current/lib/libjpeg.so.9" \
    -DCMAKE_INSTALL_PREFIX=$installDir \
    -DCMAKE_BUILD_TYPE=Release \
    ..
    #LDFLAGS=-L$installDir/lib/ CPPFLAGS=-I$installDir/include/ CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
    cd $baseDir
}

    #-DJPEG_LIBRARY="/usr/local/VAPOR-Deps/current/lib/libjpeg.so.9" \
    #-DJPEG_LIBRARY="$installDir/lib/libjpeg.so.9" \
    #-DJPEG_LIBRARY="$installDir/lib" \                  /usr/bin/ld: cannot find /usr/local/VAPOR-Deps/current/lib: file format not recognized
    #-DJPEG_LIBRARY="$installDir/lib/libjpeg.so" \       ./build.sh: line 127: -DJPEG_LIBRARY=/usr/local/VAPOR-Deps/current/lib/libjpeg.so: No such file or directory
    
sqlite() {
    local library='sqlite-autoconf-3410000'
    tar xvf $baseDir/$library.tar.gz
    cd $baseDir/$library
    CC=$CC CXX=$CXX ./configure --prefix=$installDir
    make -j4 && make install
    cd $baseDir
}

proj() {
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
    -DCMAKE_INSTALL_PREFIX=$installDir \
    -DPROJ_COMPILER_NAME=$CXX \
    ..

    make -j4 && make install
    cd $baseDir
}

geotiff() {
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
    cd $baseDir
}

xinerama() {
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
    cd $baseDir
}

openssl() {
    local library='openssl-1.1.1t'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library
    ./config shared --prefix=$installDir
    make -j4 && make install
    cd $baseDir
}

python() {
    local library='cpython-3.9.16'
    tar xvf $baseDir/$library.tar.gz && cd $baseDir/$library
    if [ $OS != "OSX" ] && [ $OS != "M1" ]; then
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
    else
        #LDFLAGS="-L$installDir/lib -Wl,-rpath=$installDir/lib" \
        #LDFLAGS="-L$installDir/lib" \
        #LDFLAGS="-L$installDir/lib -rpath=$installDir/lib" \
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
        #CPPFLAGS=-I$installDir/include \
        #LDFLAGS="-L$installDir/lib -Wl,-rpath=$installDir/lib" \
        #./configure \
        #--prefix=$installDir \
        #--enable-shared \
        #--with-ensurepip=install \
        #--with-suffix=.vapor \
        #--enable-optimizations \
        #--with-openssl=$installDir
    fi

    make -j4 && make install

    $installDir/bin/python3.9.vapor -m pip install --upgrade pip
    $installDir/bin/pip3 install --upgrade --target $installDir/lib/python3.9/site-packages numpy scipy matplotlib

    cd $baseDir
}

ospray() {
    if [ $OS != "OSX" ]; then
        local library='ospray-2.11.0.x86_64.linux'
        tar xvf $baseDir/ospray/$library.tar.gz && cd $baseDir/$library
    else
        local library='ospray-2.11.0.x86_64.macosx'
        unzip $baseDir/ospray/$library && cd $baseDir/$library
    fi
    mkdir -p $installDir/Ospray
    cp -r * $installDir/Ospray
    cd $baseDir
}

glm() {
    local library='glm-0.9.9.8'
    unzip $baseDir/$library.zip && cp -r $baseDir/glm/glm $installDir/include
    #unzip $library.zip && mkdir -p glm/build
    #cd glm/build
    #cmake -DCMAKE_INSTALL_PREFIX=$installDir -DCMAKE_BUILD_TYPE=Release ..
    #make -j4 && make install
    cd $baseDir
}

gte() {
    tar xvf GTE.tar.xz
    mv GTE $installDir/include
    cd $baseDir
}

images() {
    tar xvf images.tar.xz
    mv images $installDir/share
    cd $baseDir
}

qt() {
    if [ $OS == "CentOS" ]; then
        local library='qt-everywhere-src-5.13.2'
    else 
        local library='qt-everywhere-opensource-src-5.15.8'
    fi
    tar xvf $baseDir/$library.tar.xz && cd $baseDir/$library

    CPPFLAGS=-I$installDir/include \
    LDFLAGS="-L$installDir/lib -Wl,-rpath=$installDir/lib" \
    CC=$CC \
    CXX=$CXX \
    ./configure \
    -prefix $installDir \
    -opensource \
    -confirm-license \
    -release
    
    make -j4 && make install
    #Qt/qt-unified-linux-x64-4.5.1-online.run --script Qt/qt-installer-noninteractive.qs
}


#if [ $OS == "OSX" ]; then
#    osxPrerequisites
#elif [ $OS == "Ubuntu" ]; then
#    ubuntuPrerequisites
#elif [ $OS == "CentOS" ]; then
#    centosPrerequisites
#fi

zlib
libpng
assimp
szip
hdf5
netcdf
expat
udunits
freetype
jpeg
tiff
sqlite
proj
geotiff
if [ $OS == "Ubuntu" ] ; then
   xinerama
fi         
openssl
python
ospray
glm
gte
images
qt
