#include "zeratranslationplugin.h"
#include "zeratranslation.h"
#include <QtQml/QtQml>

bool ZeraTranslationPlugin::wasRegistered = false;

void ZeraTranslationPlugin::registerQml()
{
    // we are a singleton so ensure we are registerd once only
    if(!wasRegistered) {
        ZeraTranslation* zeraTranslation = ZeraTranslation::getInstance();
        zeraTranslation->changeLanguage("C");

        // Register internal worker not intended for external use
        qmlRegisterSingletonType<ZeraTranslation>("ZeraTranslationBackend", 1, 0, "ZTR", ZeraTranslation::getStaticInstance);
        // Register our magic Z.tr("<text-to-translate>")
        qmlRegisterSingletonType(QUrl("qrc:/qml/TranslationHelper.qml"), "ZeraTranslation", 1, 0, "Z");

        wasRegistered = true;
    }
}
