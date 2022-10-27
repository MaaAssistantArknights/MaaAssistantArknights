#!/usr/bin/env bash

source_repo=$1
limit=${2:-6}

ghrepo() {
    gh $@ --repo $source_repo
}

working_dir=$(pwd)
echo "working_dir: $working_dir"

echo "fetching $limit release info from $source_repo"
ghrepo release list --limit=$limit | awk '{ print $1 }' | tee releases

latest_tag=$(head -n 1 ./releases)

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

    if [[ "$tag" != "$latest_tag" ]]; then
        echo "comparing files $tag...$latest_tag"
        rsync -ancv --delete --info=FLIST0 "$latest_tag"/content/ "$tag"/content/ \
            | grep -v '/$' \
            | head -n -3 \
            | tee "$tag"/files.txt

        echo "installing files"
        mkdir -pv "$tag"/pkg

        while read file; do
            if [[ $file == "deleting "* ]]; then
                echo ${file#"deleting "} >> "$tag"/pkg/deleted.txt
            else
                install -DCv "$latest_tag"/content/"$file" "$tag"/pkg/"$file"
            fi
        done < "$tag"/files.txt

        echo "Creating zip archive"
        cd "$tag"/pkg
        zip -q -9 -r "$working_dir"/"${tag}_${latest_tag}.zip" .
        cd $working_dir
    fi
done < ./releases

