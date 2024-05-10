#!/bin/bash

cd "$(dirname "$0")"
if [ "$1" = clean ]; then
    target=Common/CommonResource.h
    grep -q '^#define EDCB_VERSION_EXTRA ' $target
    if [ $? -eq 0 ]; then
        grep -v '^#define EDCB_VERSION_EXTRA ' $target | tr -d '\r' | sed 's/$/\r/' >$target.new
        mv $target.new $target
    fi

    target=EpgTimer/EpgTimer/AppVersion.cs
    grep -q '^ *const string VERSION_EXTRA = "";' $target
    if [ $? -ne 0 ]; then
        grep -v '^ *\(}\|const string VERSION_EXTRA =\)' $target | tr -d '\r' | sed 's/$/\r/' >$target.new
        echo -e "        const string VERSION_EXTRA = \"\";\r\n    }\r\n}\r" >>$target.new
        mv $target.new $target
    fi
elif [ "$1" = gen ]; then
    git_hash=$(git rev-parse --short=7 HEAD)
    if [ $? -eq 0 ]; then
        target=Common/CommonResource.h
        grep -qF "$git_hash" $target
        if [ $? -ne 0 ]; then
            echo -e "#define EDCB_VERSION_EXTRA \"[$git_hash]\"\r" >$target.new
            grep -v '^#define EDCB_VERSION_EXTRA ' $target | tr -d '\r' | sed 's/$/\r/' >>$target.new
            mv $target.new $target
        fi

        target=EpgTimer/EpgTimer/AppVersion.cs
        grep -qF "$git_hash" $target
        if [ $? -ne 0 ]; then
            grep -v '^ *\(}\|const string VERSION_EXTRA =\)' $target | tr -d '\r' | sed 's/$/\r/' >$target.new
            echo -e "        const string VERSION_EXTRA = \"[$git_hash]\";\r\n    }\r\n}\r" >>$target.new
            mv $target.new $target
        fi
    fi
elif [ "$1" = edittag ]; then
    target=Common/CommonResource.h
    if [ -z "$2" ]; then
        sed "0,/^\\/*#define EDCB_VERSION_TAG / s|^/*\\(#define EDCB_VERSION_TAG \\).*|//\\1\"\"|" $target | tr -d '\r' | sed 's/$/\r/' >$target.new
    else
        sed "0,/^\\/*#define EDCB_VERSION_TAG / s|^/*\\(#define EDCB_VERSION_TAG \\).*|\\1\"$2\"|" $target | tr -d '\r' | sed 's/$/\r/' >$target.new
    fi
    mv $target.new $target

    target=EpgTimer/EpgTimer/AppVersion.cs
    sed "s|^\\( *const string VERSION_TAG = \\).*|\\1\"$2\";|" $target | tr -d '\r' | sed 's/$/\r/' >$target.new
    mv $target.new $target
else
    echo 'Usage: versionhash.sh clean|gen|edittag [<tag>]'
    exit 2
fi
