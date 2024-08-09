#!/usr/bin/env bash
set -e

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

    if [ ! -f "$tag"/MAA-$tag-win-$arch.zip ]; then
        mkdir -pv "$tag"
        echo "Downloading $tag"
        gh release download "$tag" --repo $source_repo --pattern "MAA-$tag-win-$arch.zip" --clobber \
            || gh release download "$tag" --repo $source_repo_fallback --pattern "MAA-$tag-win-$arch.zip" --clobber \
            || { echo "::warning:: win $arch not found in release $tag, skipping."; rm -vf MAA-$tag-win-$arch.zip ; echo "::endgroup::"; continue; }
        mv MAA-$tag-win-$arch.zip $tag
    else
        echo "$tag"/MAA-$tag-win-$arch.zip already exists
    fi

    if [[ "$tag" == "$latest_tag" ]]; then
        find "$latest_tag"
    else
        "$script_dir"/zipota.sh \
            $tag/MAA-$tag-win-$arch.zip \
            $latest_tag/MAA-$latest_tag-win-$arch.zip \
            "$working_dir"/"MAAComponent-OTA-${tag}_${latest_tag}-win-$arch.zip"

        rm -fv $tag/MAA-$tag-win-$arch.zip

        cd $working_dir
    fi

    echo "::endgroup::"
done < "$releases_txt"

