#!/usr/bin/env bash

make

echo "Name your server root dir: "
read rootdir

if [[ ! -d "webserver" ]]; then
	mkdir
	fi
touch server/index.html
echo "Edit file inside ". ${rootdir} ." folder"
echo "After that run: ./webserver <port> ". ${rootdir}

make clean