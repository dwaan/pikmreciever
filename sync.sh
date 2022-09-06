#!/bin/bash
echo "Remote syncing..."
rsync -rlptzv --progress --delete --exclude=build/* ./ pi@themoon.local:~/pikmreciever/ > /dev/null

echo "Remote building..."
ssh pi@themoon.local ./build

echo "Remote executing..."
ssh pi@themoon.local sudo ./pikmreciever/build/pikmreciever