#!/bin/sh

configd-xgettext index.html | xgettext -L Lisp --keyword=_  --from-code utf-8 -o alterator-usbmount.pot -- -
xgettext -L Lisp  --join-existing --keyword=_ -o alterator-usbmount.pot ajax.scm
cat USBMount.desktop |  sed -n -e "s,^Name[[:space:]]*=[[:space:]]*\(.*\),(_ \"\1\"),p" -e "s,^Comment[[:space:]]*=[[:space:]]*\(.*\),(_ \"\1\"),p" | xgettext -L Lisp --join-existing --keyword=_ -o alterator-usbmount.pot -- -
msgmerge alterator-usbmount.po alterator-usbmount.pot -o alterator-usbmount.po