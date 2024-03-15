#!/bin/sh

configd-xgettext index.html | xgettext -L Lisp --keyword=_ -o alterator-usbguard.pot -- -
xgettext -L Lisp  --join-existing --keyword=_ -o alterator-usbguard.pot ajax.scm
cat USBGuard.desktop |  sed -n -e "s,^Name[[:space:]]*=[[:space:]]*\(.*\),(_ \"\1\"),p" -e "s,^Comment[[:space:]]*=[[:space:]]*\(.*\),(_ \"\1\"),p" | xgettext -L Lisp --join-existing --keyword=_ -o alterator-usbguard.pot -- -
msgmerge alterator-usbguard.po alterator-usbguard.pot -o alterator-usbguard.po