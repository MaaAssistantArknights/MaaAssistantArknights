# usage: 
#   makeota.sh UPDATE_FROM UPDATE_TO WRITE_TO
# warning:
#   mind trailing slash issue of rsync

tmpfile=$(mktemp /tmp/makeota-files.XXX)
exec 3>"$tmpfile"
exec 4<"$tmpfile"
rm "$tmpfile"

ota_from="$1"
ota_to="$2"
out_dir="$3"

rsync -ancv --delete --info=FLIST0 "$ota_to"/ "$ota_from"/ \
    | grep -v '/$' \
    | head -n -3 \
    > "$tmpfile"

while read file; do
    if [[ $file == "deleting "* ]]; then
        echo ${file#"deleting "} >> "$out_dir"/removelist.txt
    else
        install -DCv "$ota_to"/"$file" "$out_dir"/"$file"
    fi
done < "$tmpfile"
