// $Id: treenode.c,v 1.7 2005/03/14 13:30:46 lynx Exp $

#include <net.h>

volatile mixed value;
volatile object left, right;

getValue() {
    unless (clonep(ME)) return;
    return value;
}

setValue(x) {
    unless (clonep(ME)) return;
    value = x;
}

getLeft() {
    unless (clonep(ME)) return;
    return left;
}

setLeft(x) {
    unless (clonep(ME)) return;
    left = x;
}

getRight() {
    unless (clonep(ME)) return;
    return right;
}

setRight(x) {
    unless (clonep(ME)) return;
    right = x;
}

load() {
    return ME;
}

// in case you didnt guess so far,
// this file is a joke really..
