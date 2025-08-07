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


### Windows linguist download:
https://github.com/thurask/Qt-Linguist/releases

### Create a new translation 'language_REGION' e.g 'de_AT':
Following 3 steps should be done:
#### 1. vf-gui-translation
* Create a document '''zera-gui_language_REGION.ts''' with content:

```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="language_REGION">
</TS>
```

* Run CMake
* Compile
* As long as the translation is not ready for public, append "language_REGION" to https://github.com/ZeraGmbH/vf-gui-translation/blob/7e37dd7b40ae4537b043656a89b9b6d8d4b665ab/src/lib/zeratranslation.cpp#L152 as
```cpp
   QStringList ignoreList = QStringList() << "language_REGION";
```
* Once translation is ready for the wild don't forget to remove it from ```ignoreList```

#### 2. vf-qmllibs
* Update [Timezone translations](https://github.com/ZeraGmbH/vf-qmllibs/blob/master/libs/datetime-setter/timezone-translations/lib/README.md)

#### 3. reportjs-vue
* See [this document](https://github.com/ZeraGmbH/reportjs-vue?tab=readme-ov-file#translations) for more information
