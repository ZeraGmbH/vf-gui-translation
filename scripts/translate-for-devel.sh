#/bin/sh

# This script is based upon of autobuilder's weekly job
# 'vf-declarative-gui_translation' to lupdate *.ts from source code
# file(s).
# Autobuilder is fine for code with newly added Z.tr calls in master
# branch. But in case one wants to add (mass) translations in code that
# requires on demand testing it might be a better ideas to make changes
# on working branches before applying to master.
#
# The suggested workfow is:
#
# * create and checkout working branches (based upon latest master!)
#   * vf-qmllibs (suggested the one you are working on - not the copy in
#     submodule containe in this repo)
#   * in this repo
#   * the repo to add Z.tr (only if it is other than vf-qmllibs)
# * Add Z.tr in cpp/qml sources
# * Add new texts to translate in
#   vf-qmllibs/src/libs/zeraTranslation/src/zeratranslation.cpp
# * Push changes in vf-qmllibs
# * Fetch/check vf-qmllibs in submodule (src-folder)
# * Run this script
# * Open *.ts files with qtlinguist and check if the added texts appear
# * If there are cycles left do translations (or just mark as translated
#   for english texts) - but be aware not to tell C.Molke that you did
# * Run this script
# * Copy *.qm to the path translaton files are expected usually
#   /home/operator/translations
# * If you did translations test if they appear as expected and- VERY
#   IMPORTANT - don't ruin layouts
# * If all tests succeed: Send out PRs


# Fedora specifics
lupdate="lupdate-qt5"
lrelease="lrelease-qt5"

# move to base path of this repo
base_path="`dirname $0`/.."
cd "$base_path"

# To have one helper script only we do both lupdate & lrelease
find -maxdepth 1 -name '*.ts' | xargs $lupdate -no-obsolete -locations none -no-ui-lines -pro src/libs/zeraTranslation/lupdate.pro -ts
$lrelease *.ts
