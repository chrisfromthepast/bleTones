#!/bin/bash
# Script to create GitHub release for v1.0.0-python
# Run this script after the PR is merged to create the GitHub release

# Ensure we're on the main branch with latest changes
git checkout main
git pull origin main

# Create and push the tag
git tag -a v1.0.0-python -m "Python-heavy version release before major refactor

This release captures the fully functional Python-based version of bleTones featuring:
- Python desktop app with eel + bleak (main.py)
- SuperCollider integration with OSC bridge (supercollider/bridge.py)
- Electron desktop app with noble BLE scanning
- Web Audio synthesis with 12 instruments across 4 flavors
- Comprehensive documentation and multiple deployment options

This version is being archived before transitioning to a new language implementation."

git push origin v1.0.0-python

echo ""
echo "✅ Tag v1.0.0-python created and pushed!"
echo ""
echo "📦 Next steps:"
echo "1. Go to https://github.com/chrisfromthepast/bleTones/releases/new"
echo "2. Choose tag: v1.0.0-python"
echo "3. Release title: bleTones v1.0.0-python - Python Archive Release"
echo "4. Description: Use content from RELEASE_NOTES.md"
echo "5. Check 'Set as a pre-release' (optional, since this is an archive)"
echo "6. Click 'Publish release'"
echo ""
echo "Or use GitHub CLI:"
echo "gh release create v1.0.0-python --title 'bleTones v1.0.0-python - Python Archive Release' --notes-file RELEASE_NOTES.md"
