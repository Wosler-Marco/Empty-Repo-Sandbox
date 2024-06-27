#!/usr/bin/env bash
rm -r ./build

if [ $1 -eq "DEBUG" ]
then
    ccmake -S . -B ./build -DCMAKE_PREFIX_PATH=/usr/local/Qt-6.5.3/lib/cmake/Qt6 -DQt6GuiTools_DIR=/usr/local/Qt-6.5.3/lib/cmake/Qt6GuiTools -DQt6WidgetsTools_DIR=/usr/local/Qt-6.5.3/lib/cmake/Qt6WidgetsTools -DQt6Widgets_DIR=/usr/local/Qt-6.5.3/lib/cmake/Qt6Widgets -DENABLE_DEBUG=ON -DCMAKE_FIND_DEBUG_MODE=TRUE -DQT_DEBUG_FIND_PACKAGE=ON 
else
    cmake -S . -B ./build
fi
make
