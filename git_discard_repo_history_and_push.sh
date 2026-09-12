git checkout --orphan temp-branch
git add -A
git commit -m "Replace previous history with single commit"
git branch -D trunk
git branch -m trunk
git push -f origin trunk
git push -f github trunk
