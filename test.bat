$latest_tag=$(git describe --tags --abbrev=0)
$content=$(git log --pretty=format:"%s @%an" v5.1.0..$latest_tag)
echo $content
pause