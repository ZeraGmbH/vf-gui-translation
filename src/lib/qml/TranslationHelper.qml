pragma Singleton

import QtQuick 2.0
import ZeraTranslationBackend 1.0

// Translation frontend implementing Z.tr("<text-to-translate>") and Z.changeLanguage(<new-language>)
Item {
    property var notFound: []
    function tr(key) {
        var translated = ""
        if(ZTR[key] !== undefined)
            translated = ZTR[key]
        else {
            if(!notFound.includes(key)) {
                console.warn("Z.tr / Translation not found: " + key)
                notFound.push(key)
            }
            translated = key
        }
        return translated
    }
    function trArr(arr) {
        let ret = []
        for(let i=0; i<arr.length; i++)
            ret.push(tr(arr[i]))
        return ret
    }
    function changeLanguage(language) {
        ZTR.changeLanguage(language)
    }
    function trDateTimeShort(dateTimeString) {
        return ZTR.trDateTimeShort(dateTimeString)
    }
}
