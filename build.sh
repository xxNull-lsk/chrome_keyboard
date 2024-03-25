#!/bin/bash

if [ ! -d ./build ]; then
    mkdir build
    cd build
    cmake ..
else
    cd build
fi

make -j8
sudo make install
sudo cp chrome_keyboard.service /etc/systemd/system
sudo systemctl daemon-reload
sudo systemctl start chrome_keyboard
sudo systemctl enable chrome_keyboard