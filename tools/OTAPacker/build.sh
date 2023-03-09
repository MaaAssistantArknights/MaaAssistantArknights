#!/usr/bin/env bash

source_repo=$1
releases_txt="$2"
arch=$3
source_repo_fallback=$4

working_dir="$(pwd)"
echo "working_dir: $working_dir"

script_dir="$(dirname $(realpath "$0"))"

latest_tag=$(head -n 1 "$releases_txt")

while read tag; do
    echo "::group::$tag"

    if [ ! -d "$tag"/content ]; then
        mkdir -pv "$tag"
        cd "$tag"
        echo "Downloading $tag"
        gh release download "$tag" --repo $source_repo --pattern "MAA-$tag-win-$arch.zip" --clobber \
            || gh release download "$tag" --repo $source_repo_fallback --pattern "MAA-$tag-win-$arch.zip" --clobber \
            || { echo "::warning:: win $arch not found in release $tag skipping."; echo "::endgroup::"; continue; }
        mkdir -pv 'content'
        unzip -q -O GB2312 -o "*.zip" -d 'content'
        rm -fv *.zip
        cd $working_dir
    else
        echo "$tag"/content/ already exists
    fi

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
        zip -q -9 -r "$working_dir"/"MAAComponent-OTA-${tag}_${latest_tag}-win-$arch.zip" .
        cd $working_dir
    fi

    echo "::endgroup::"
done < "$releases_txt"

