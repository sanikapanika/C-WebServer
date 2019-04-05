#!/usr/bin/env bash

make

if [[ ! -d "webserver" ]]; then
	mkdir server
	fi
	touch server/index.html
	echo "Edit file inside webserver folder"
	echo "After that run: ./webserver <port> server/"

make clean