#!/bin/bash

cp data/com.github.kinvolk.tcd.conf /etc/dbus-1/system.d/
systemctl reload dbus.service

