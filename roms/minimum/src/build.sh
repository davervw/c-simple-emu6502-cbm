#!/bin/sh -x
export ACME=${USERPROFILE}/Downloads/acme0.97win/acme
${ACME}/acme -o ../testmin.bin -l ../testmin.lbl -r ../testmin.lst testmin.asm
${ACME}/acme -o ../wozmon.bin -l ../wozmon.lbl -r ../wozmon.lst wozmon.asm
