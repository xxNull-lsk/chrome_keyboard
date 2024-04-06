#!/bin/bash
DIR=`pwd`

if [ ! -f ".major_version" ]; then
    echo "1" > .major_version
fi
major_version=`cat .major_version`
if [ ! -f ".minor_version" ]; then
    echo "0" > .minor_version
fi
minor_version=`cat .minor_version`
if [ ! -f ".fixed_version" ]; then
    echo "0" > .fixed_version
fi
fixed_version=`cat .fixed_version`
version=${major_version}.${minor_version}.${fixed_version}
name=chrome_keyboard

if [ ! -d ./build ]; then
    mkdir build
    cd build
    cmake ..
else
    cd build
fi

make -j8
cd $DIR

source /etc/os-release

mkdir -p ${DIR}/dist >/dev/null 2>&1
mkdir ${DIR}/tmp > /dev/null 2>&1
cp -rf ${DIR}/debian/* ${DIR}/tmp
mkdir -p ${DIR}/tmp/etc/profile.d > /dev/null 2>&1
echo "nohup chrome_keyboard_tray >/dev/null 2>&1 &" > ${DIR}/tmp/etc/profile.d/chrome_keyboard_tray.sh
mkdir -p ${DIR}/tmp/etc/systemd/system > /dev/null 2>&1
cp ${DIR}/*.service ${DIR}/tmp/etc/systemd/system
mkdir -p ${DIR}/tmp/usr/share/${name} > /dev/null 2>&1
cp -rf $DIR/res/* ${DIR}/tmp/usr/share/${name}
mkdir -p ${DIR}/tmp/sbin > /dev/null 2>&1
cp $DIR/build/chrome_keyboard ${DIR}/tmp/sbin
mkdir -p ${DIR}/tmp/usr/bin > /dev/null 2>&1
cp $DIR/build/chrome_keyboard_tray ${DIR}/tmp/usr/bin

sudo chown root:root -R ${DIR}/tmp/usr
sudo chown root:root -R ${DIR}/tmp/etc

architecture=`uname -m`
if [ "$architecture" == "x86_64" ]; then
    architecture="amd64"
fi
sed -i "s/^Version:.*/Version: ${version}/g" ${DIR}/tmp/DEBIAN/control
sed -i "s/^Architecture:.*/Architecture: ${architecture}/g" ${DIR}/tmp/DEBIAN/control
chmod 0555 ${DIR}/tmp/DEBIAN/postinst
dpkg -b tmp ${DIR}/dist/${name}_${version}_${ID}_${VERSION_ID}_${architecture}.deb

cd $DIR
sudo rm -rf tmp
next_fixed_version=$((fixed_version+1))
echo $next_fixed_version > .fixed_version
exit 0