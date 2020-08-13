#ifndef ZeraTranslation_H
#define ZeraTranslation_H

#include <QQmlPropertyMap>
#include <QTranslator>
#include "zeratranslation_export.h"

QT_BEGIN_NAMESPACE
class QQmlEngine;
class QJSEngine;
QT_END_NAMESPACE

/**
 * @brief Translation mapper with builtin qml notifications
 */
class ZERATRANSLATION_EXPORT ZeraTranslation : public QQmlPropertyMap
{
    Q_OBJECT
public:
    /**
     * @brief getInstance get our singleton
     * @return
     */
    static ZeraTranslation *getInstance();
    /**
     * @brief getStaticInstance get our singleton - QmlEngine callback
     * @param t_engine
     * @param t_scriptEngine
     * @return
     */
    static QObject *getStaticInstance(QQmlEngine *t_engine, QJSEngine *t_scriptEngine);
    /**
     * @brief changeLanguage Request to change language (usually by QML)
     * @param t_language language name to switch to e.g en_GB / de_DE
     */
    Q_INVOKABLE void changeLanguage(const QString &t_language);
    /**
     * @brief TrValue Translate a single string
     * @param key for QQmlPropertyMap
     * @return Translated string or if not found in map key
     */
    QVariant TrValue(const QString &key);
signals:
    /**
     * @brief sigLanguageChanged Signal fired once translation map is reloaded typically after changeLanguage
     */
    void sigLanguageChanged();

public slots:

private:
    explicit ZeraTranslation(QObject *parent = nullptr);
    void setupTranslationFiles();
    void reloadStringTable();

    /**
     * @brief s_instance Pointer to the one and only instance
     */
    static ZeraTranslation *s_instance;

    QString m_currentLanguage;
    QTranslator m_translator;
    //key = locale name (e.g. en_US, de_DE)
    //value = absolute path
    QHash<QString, QString> m_translationFilesModel;
    QHash<QString, QString> m_translationFlagsModel;

    // QQmlPropertyMap interface
protected:
    QVariant updateValue(const QString &key, const QVariant &input) override;
};

#endif // ZeraTranslation_H
