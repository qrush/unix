#!/bin/sh
# rebuild sources in a manner that lets you see the page splits.
# useful for editing the pages to get the inter-page flow to line
# up properly.

xcat() { 
    for fn in $*; do
        cat $fn
        echo "******* $fn end *******"
    done
}

m() { xcat pages/$2 > rebuilt/$1.s; }

test -d rebuilt || mkdir rebuilt

m u0 e00-*
#m u1 e01-*
m u2 e02-*
#m u3 e03-*
#m u4 e04-*
m u5 e05-*
m u6 e06-*
m u7 e07-*
#m u8 e08-*
m u9 e09-*
m ux e10-*
#m sh e11-*
#m ini e12-*
