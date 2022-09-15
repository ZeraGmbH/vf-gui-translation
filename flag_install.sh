#!/bin/sh

if [ "$#" -ne 1 ]; then
  echo "${0} usage: $0 DESTDIR" >&2
  exit 1
fi
if ! [ -e "$1" ]; then
  echo "${0}: $1 not found" >&2
  exit 1
fi
if ! [ -d "$1" ]; then
  echo "${0}: $1 not a directory" >&2
  exit 1
fi

# Changes name to required format "flag_<locale>.png" (where locale could be locale -> "de" or locale_country -> "en_US")
# Example 1: flag_en.png for zera-gui_en.ts
# Example 2: flag_en_US.png for zera-gui_en_US.ts
# Example 3: flag_en_GB.png for zera-gui_en_GB.ts

install -m 0644 ./region-flags/png/DE.png $1/flag_de_DE.png
install -m 0644 ./region-flags/png/GB.png $1/flag_en_GB.png
install -m 0644 ./region-flags/png/US.png $1/flag_en_US.png
install -m 0644 ./region-flags/png/FR.png $1/flag_fr_FR.png
