#!/usr/bin/env bash
set -e

script_dir="$(dirname $(realpath "$0"))"

from_zip="$1"
to_zip="$2"
out_zip="$3"

from_list="$("$script_dir"/ziplist.sh "$from_zip" | sort)"
to_list="$("$script_dir"/ziplist.sh "$to_zip" | sort)"

# files with same name and content
comm_list="$(comm -12 <(echo "$from_list") <(echo "$to_list"))"

from_fn="$(echo "$from_list" | cut -d\  -f2- | sort)"
to_fn="$(echo "$to_list" | cut -d\  -f2- | sort)"

cp -v "$to_zip" "$out_zip"

echo "$comm_list" | cut -d\  -f2- | xargs zip --delete "$out_zip"

tmpdir=$(mktemp -d /tmp/zipota-files.XXX)

comm -23 <(echo "$from_fn") <(echo "$to_fn") > "$tmpdir"/removelist.txt
echo "$to_fn" > "$tmpdir"/filelist.txt

zip -X -r -j "$out_zip" "$tmpdir"/removelist.txt "$tmpdir"/filelist.txt

rm -rf $tmpdir
