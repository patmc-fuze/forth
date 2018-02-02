#!/bin/bash
pwd

make -C ForthLib -f Makefile.ubu86 all
rm ForthLinux/forth
make -C ForthLinux all

