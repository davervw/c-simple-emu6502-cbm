#!/bin/sh
export ACME=${USERPROFILE}/Downloads/acme0.97win/acme
${ACME}/acme -o ../testmin.bin -l ../testmin.lbl testmin.asm
