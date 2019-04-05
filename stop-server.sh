#!/usr/bin/env bash
trap "exit" INT

echo "Stopping server."

echo "Enter the port you hosted the server on: "
while :
do
  read port
  len=${#port}
  testvar=$(echo "$port" | tr -dc '[:digit:]')
  if [[ $len -ne ${#testvar} ]] ; then
      echo "$port is not a number"
  else
      break
  fi
done

echo "Have you changed the server root dir name (webserver) y/n"
read yn
if [[ ${yn} == "y" ]]; then
    echo "What is the name of the server? "
    read dirname
    kill $(lsof -i :${port} | grep ${dirname} | cut -d " " -f2)
    exit
fi
kill $(lsof -i :${port} | grep webserver | cut -d " " -f2)
