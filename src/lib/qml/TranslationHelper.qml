pragma Singleton

import QtQuick 2.0
import ZeraTranslationbackend 1.0

// Translation frontend implementing Z.tr("<text-to-translate>") and Z.changeLanguage(<new-language>)
Item {
    signal languageChanged();

    id: root
    function tr(key) {
        var translated = ""
        if(Array.isArray(key)) { //translating array e.g. text models
            var newkey = [];
            for(var i = 0; i < key.length; i++) {
                if(ZTR[key[i]] !== undefined) {
                    newkey[i] = ZTR[key[i]];
                } else {
                    newkey[i] = key[i];
                }
            }
            translated = newkey;
        } else { //translating normal text elements
            if(ZTR[key] !== undefined) {
                translated = ZTR[key];
            } else {
                translated = key;
            }
        }
        return translated
    }
    function changeLanguage(language) {
        ZTR.changeLanguage(language)
    }
}
