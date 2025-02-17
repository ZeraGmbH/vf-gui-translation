#ifndef ZeraTranslation_H
#define ZeraTranslation_H

#include <QQmlPropertyMap>
#include <QTranslator>
#include "zeratranslation_export.h"

QT_BEGIN_NAMESPACE
class QQmlEngine;
class QJSEngine;
QT_END_NAMESPACE

class ZERATRANSLATION_EXPORT ZeraTranslation : public QQmlPropertyMap
{
    Q_OBJECT
public:
    static const QVariantHash loadTranslationHash();
    static ZeraTranslation *getInstance();
    static QObject *getStaticInstance(QQmlEngine *t_engine, QJSEngine *t_scriptEngine);
    Q_INVOKABLE void changeLanguage(const QString &language);
    static void setInitialLanguage(const QString &language);
    QString getCurrentLanguage();
    QVariant TrValue(const QString &key);
    Q_INVOKABLE QString trDateTimeShort(const QString &dateTime);

signals:
    void sigLanguageChanged();

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
};

#endif // ZeraTranslation_H
