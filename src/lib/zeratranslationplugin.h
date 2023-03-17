#ifndef ZERATRANSLATIONPLUGIN_H
#define ZERATRANSLATIONPLUGIN_H

#include <QObject>
#include "zeratranslation_export.h"

class ZERATRANSLATION_EXPORT ZeraTranslationPlugin : public QObject {
    Q_OBJECT
public:
    /**
     * @brief registerQml Register ZeraTranslationPlugin QML component (add 'import ZeraTranslation 1.0' in your QML
     * to use it
     */
    static void registerQml();
private:
    static bool wasRegistered;
};

#endif // ZERATRANSLATIONPLUGIN_H
