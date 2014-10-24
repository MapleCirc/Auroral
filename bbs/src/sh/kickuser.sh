#!/bin/sh
#踢掉站上所有的使用者

kill -15 `ps auxwww | grep bbsd | awk '{print $2}'`

