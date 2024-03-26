# ZENUX central translation lib

### The suggested workflow is:

* create and checkout working branches (based upon latest master!)
    * the repo(s) to add Z.tr
    * in this repo
* Add Z.tr in cpp/qml in sources
* Add new texts to translate in this repo / src/lib/zeratranslation.cpp
* Build this repo
* Open *.ts files with qtlinguist and check if the added texts appear
* If there are cycles left do translations (or just mark as translated
 for English texts)

### Suggested tests:
* Build this repo in qt-creator
    * do they translations changed appear as expected?
    * VERY IMPORTANT: Check in all languages for ruined layouts

If all tests succeed: commit & push


Windows linguist download:
https://github.com/thurask/Qt-Linguist/releases
