#!/bin/bash

rm logs/*
sudo killall -9 python ; sudo python startapp.py &