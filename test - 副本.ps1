$latest_tag=$(git describe --tags --abbrev=0)
$content=$(git log --pretty=format:"- %s @%an<br>" v5.2.0..$latest_tag)
echo "content="$content
pause