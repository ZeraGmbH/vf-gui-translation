#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "${0} usage: $0 FLAGS_SOURCE_DIR DESTDIR_WITH_TRANSLATION" >&2
    exit 1
fi

if ! [ -d "$1" ]; then
    echo "${0}: $1 not a directory" >&2
    exit 1
fi
FLAGS_SOURCE_DIR="$1"

if ! [ -d "$2" ]; then
      echo "${0}: $2 not a directory" >&2
      exit 1
fi
DESTDIR_WITH_TRANSLATION="$2"

copy_flags()
{
    for file in `find "$DESTDIR_WITH_TRANSLATION" -name '*.qm'`; do
        filename=`basename $file`
        language=`echo $filename | sed -e 's:zera-gui_::' -e 's:.qm::'`
        region=`echo $language | tr -d [:lower:] | tr -d '_'`
        echo "Copy flag source to build for $format / $language / $region..."
        flagfile="$FLAGS_SOURCE_DIR/$format/${region}.$format"
        if [ ! -e "$flagfile" ]; then
            echo "Could not find flagfile $flagfile" >&2
            exit 1
        fi
        cp -f "$flagfile" "${DESTDIR_WITH_TRANSLATION}/flag_${language}.$format"
    done
}

format='svg'
copy_flags

format='png'
copy_flags

