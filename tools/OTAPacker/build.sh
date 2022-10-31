#!/usr/bin/env bash

source_repo=$1
releases_txt="$2"

ghrepo() {
    gh $@ --repo $source_repo
}

working_dir="$(pwd)"
echo "working_dir: $working_dir"

script_dir="$(dirname $(realpath "$0"))"

latest_tag=$(head -n 1 "$releases_txt")

while read tag; do
    (
        mkdir -pv "$tag"
        cd "$tag"
        echo "Downloading $tag"
        ghrepo release download "$tag" --pattern "MAA-$tag-win-x64.zip" --pattern "MaaBundle-$tag.zip" --clobber
        mkdir -pv 'content'
        echo "Unzip" *.zip
        unzip -qq -O gbk -o "*.zip" -d 'content'
        rm -fv *.zip
    )

    cd $working_dir

    if [[ "$tag" == "$latest_tag" ]]; then
        cd "$latest_tag"/content/
        find * -type f -exec md5sum '{}' \; > md5sum.txt
        cd $working_dir
    else
        echo "Comparing and installing files $tag...$latest_tag"
        mkdir -pv "$tag"/pkg
        "$script_dir"/makeota.sh "$tag"/content "$latest_tag"/content "$tag"/pkg
        echo "Creating zip archive"
        cd "$tag"/pkg
        zip -q -9 -r "$working_dir"/"MAAComponent-OTA-${tag}_${latest_tag}-win-x64.zip" .
        cd $working_dir
    fi
done < "$releases_txt"

