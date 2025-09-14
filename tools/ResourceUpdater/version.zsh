#!/usr/bin/env zsh

has_changes=false
has_png_changes=false

git add .

all_modified_files=$(git diff --name-only HEAD 2>/dev/null)

git reset .

if [ -z "$all_modified_files" ]; then
    echo "No files to update."
else
    has_changes=true
    
    # Filter out files in tasks directories
    modified_files=$(echo "$all_modified_files" | grep -v '^resource\(/global/[^/]*/resource\)\?/tasks/')
    
    # Get unique directories
    directories=(${(f)$(echo "$modified_files" | xargs -I{} dirname {} | sort -u)})
    
    # Build list of directories containing version.json
    version_json_dirs=()
    
    for dir in "${directories[@]}"; do
        # If this is Arknights-Tile-Pos folder, use parent directory
        if [[ "$dir" =~ /Arknights-Tile-Pos$ ]]; then
            version_json_dirs+=("$(dirname "$dir")")
        else
            version_json_dirs+=("$dir")
        fi
    done
    
    # Remove duplicates
    typeset -U version_json_dirs
    
    # Update all version.json files
    for dir in "${version_json_dirs[@]}"; do
        version_file="$dir/version.json"
        
        if [ -f "$version_file" ]; then
            # Use jq to update the JSON with proper 4-space indentation
            current_date=$(date -u +"%Y-%m-%d %H:%M:%S.000")
            jq --arg date "$current_date" --indent 4 '.last_updated = $date' "$version_file" > "$version_file.tmp"
            mv "$version_file.tmp" "$version_file"
            
            echo "Updated: $version_file"
        fi
    done
    
    # Check for PNG changes
    if echo "$all_modified_files" | grep -q '\.png$'; then
        has_png_changes=true
    fi
fi

echo "Changes: $has_changes"
echo "PNG Changes: $has_png_changes"
echo "changes=$has_changes" >> $GITHUB_OUTPUT
echo "contains_png=$has_png_changes" >> $GITHUB_OUTPUT
