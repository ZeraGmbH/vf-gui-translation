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

flag_width=256

create_pngs_from_svgs()
{
    for file in `find "$DESTDIR_WITH_TRANSLATION" -name '*.qm'`; do
        filename=`basename $file`
        language=`echo $filename | sed -e 's:zera-gui_::' -e 's:.qm::'`
        region=`echo $language | tr -d [:lower:] | tr -d '_'`
        echo "Create png from svg $language / $region..."
        svgfile="$FLAGS_SOURCE_DIR/svg/${region}.svg"
        if [ ! -e "$svgfile" ]; then
            echo "Could not find $svgfile" >&2
            exit 1
        fi
        if ! rsvg-convert --width=${flag_width} --format=png $svgfile > "${DESTDIR_WITH_TRANSLATION}/flag_${language}.png"; then
            echo "Could not create flag_${language}.png from $svgfile" >&2
            exit 1
        fi
    done
}


format='svg'
copy_flags

create_pngs_from_svgs

