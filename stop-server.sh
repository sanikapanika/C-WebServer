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

kill $(lsof -i :${port} | grep webserver | cut -d " " -f2)
make full-clean
