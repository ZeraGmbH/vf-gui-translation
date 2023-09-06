pragma Singleton

import QtQuick 2.0
import ZeraTranslationBackend 1.0

// Translation frontend implementing Z.tr("<text-to-translate>") and Z.changeLanguage(<new-language>)
Item {
    property var notFound: []
    function tr(key) {
        var translated = ""
        if(Array.isArray(key)) { //translating array e.g. text models
            console.info("Z.tr / array mode: " + key) // is this ever used???
            var newkey = []
            for(var i = 0; i < key.length; i++) {
                if(ZTR[key[i]] !== undefined)
                    newkey[i] = ZTR[key[i]]
                else {
                    if(!notFound.includes(key[i])) {
                        console.warn("Z.tr / Translation not found: " + key)
                        notFound.push(key[i])
                    }
                    newkey[i] = key[i]
                }
            }
            translated = newkey
        }
        else { //translating normal text elements
            if(ZTR[key] !== undefined)
                translated = ZTR[key]
            else {
                if(!notFound.includes(key)) {
                    console.warn("Z.tr / Translation not found: " + key)
                    notFound.push(key)
                }
                translated = key
            }
        }
        return translated
    }
    function changeLanguage(language) {
        ZTR.changeLanguage(language)
    }
}
