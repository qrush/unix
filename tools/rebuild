#!/bin/sh

r() { cat pages/$2-* > rebuilt/$1.s; }

rebuild() {
    test -d rebuilt || mkdir rebuilt

    echo rebuilding...
    r u0 e00
    r u1 e01
    r u2 e02
    r u3 e03
    r u4 e04
    r u5 e05
    r u6 e06
    r u7 e07
    r u8 e08
    r u9 e09
    r ux e10
    r sh e11
    r init e12
}

corep() {
    for x in ../patches/core/*.patch ; do
        y=`basename $x .patch`
        echo ' ' $y; patch -s -p1 < $x
    done
}

p() { echo ' ' $1; patch -s -p1 <../patches/$1.patch; }

patches() {
    test -d build || mkdir build

    echo patching...
    cp rebuilt/* build
    cd build
    corep
    for x in "$@" ; do
        p $x
    done
}


rebuild
patches "$@"
