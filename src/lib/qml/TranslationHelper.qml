pragma Singleton

import QtQuick 2.0
import ZeraTranslationBackend 1.0

Item {
    function tr(key) {
        // just for language change notification
        // inspired by: https://pusling.com/blog/?p=376
        let language = ZTR.language
        return ZTR.trValue(key)
    }
    function trArr(arr) {
        let ret = []
        for(let i=0; i<arr.length; i++)
            ret.push(tr(arr[i]))
        return ret
    }
    function changeLanguage(language) {
        ZTR.setLanguage(language)
    }
    function trDateTimeShort(dateTimeString) {
        return ZTR.trDateTimeShort(dateTimeString)
    }
    function trDateTimeLong(dateTimeString) {
        return ZTR.trDateTimeLong(dateTimeString)
    }
    readonly property var localesModel: ZTR.localesModel
    readonly property var flagsModel: ZTR.flagsModel
}
