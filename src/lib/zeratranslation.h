#ifndef ZeraTranslation_H
#define ZeraTranslation_H

#include "zeratranslation_export.h"
#include <QQmlPropertyMap>
#include <QTranslator>
#include <QLocale>
#include <memory>

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
    Q_INVOKABLE void changeLanguage(const QString &language);
    static void setInitialLanguage(const QString &language);
    /**
     * @brief TrValue Translate a single string
     * @param key for QQmlPropertyMap
     * @return Translated string or if not found in map key
     */
    QVariant TrValue(const QString &key);

    QLocale getLocale() const;
    Q_PROPERTY(QLocale locale READ getLocale NOTIFY sigLanguageChanged)
    QString getDecimalPoint() const;
    Q_PROPERTY(QString decimalPoint READ getDecimalPoint NOTIFY sigLanguageChanged)

signals:
    void sigLanguageChanged();

    // QQmlPropertyMap interface
protected:
    QVariant updateValue(const QString &key, const QVariant &input) override;
private:
    explicit ZeraTranslation(QObject *parent = nullptr);
    void setupTranslationFiles();
    void reloadStringTable();

    static ZeraTranslation *s_instance;
    static QString m_initialLanguage;

    QString m_currentLanguage;
    QTranslator m_translator;
    //key = locale name (e.g. en_US, de_DE)
    //value = absolute path
    QHash<QString, QString> m_translationFilesModel;
    QHash<QString, QString> m_translationFlagsModel;
    std::unique_ptr<QLocale> m_locale = std::make_unique<QLocale>(m_initialLanguage);
};

#endif // ZeraTranslation_H
