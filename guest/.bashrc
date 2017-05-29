#!/bin/sh

# if running interactively, then
if [ "$PS1" ]; then
        if [ -e /etc/profile ]; then
                source /etc/profile
        fi
        if [ -e ~/.bash_login ]; then
                source ~/.bash_login
        fi
fi
