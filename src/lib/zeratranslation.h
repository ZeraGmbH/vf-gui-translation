#ifndef ZeraTranslation_H
#define ZeraTranslation_H

#include <QTranslator>
#include <QMap>
#include <QHash>
#include <QVariant>
#include "zeratranslation_export.h"

QT_BEGIN_NAMESPACE
class QQmlEngine;
class QJSEngine;
QT_END_NAMESPACE

class ZERATRANSLATION_EXPORT ZeraTranslation : public QObject
{
    Q_OBJECT
public:
    static ZeraTranslation *getInstance();
    static QObject *getStaticInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

    static void setInitialLanguage(const QString &language);
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY sigLanguageChanged)
    QString getLanguage();
    Q_INVOKABLE void setLanguage(const QString &language);

    Q_INVOKABLE QVariant trValue(const QString &key);
    Q_INVOKABLE QString trDateTimeShort(const QString &dateTime);

    Q_PROPERTY(QStringList localesModel READ getLocalesModel CONSTANT)
    QStringList getLocalesModel();
    Q_PROPERTY(QStringList flagsModel READ getFlagsModel CONSTANT)
    QStringList getFlagsModel();

signals:
    void sigLanguageChanged();

private:
    explicit ZeraTranslation();
    void setupTranslationFiles();
    void reloadStringTable();
    static const QVariantHash loadTranslationHash();

    static ZeraTranslation *m_instance;
    static QString m_initialLanguage;

    QString m_currentLanguage;
    QVariantHash m_currentTranslations;
    QTranslator m_translator;

    //key = locale name (e.g. en_US, de_DE)
    //value = absolute path
    QMap<QString, QString> m_translationFilesModel;
    QMap<QString, QString> m_translationFlagsModel;
};

#endif // ZeraTranslation_H
