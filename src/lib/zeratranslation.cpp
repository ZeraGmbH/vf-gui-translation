#include "zeratranslation.h"
#include <QDir>
#include <QUrl>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDateTime>
#include <QDebug>

ZeraTranslation *ZeraTranslation::m_instance = nullptr;
QString ZeraTranslation::m_initialLanguage = "en_GB";

ZeraTranslation::ZeraTranslation()
{
    setupTranslationFiles();
    setLanguage(m_initialLanguage);
    connect(&m_dateTimePollTimer, &QTimer::timeout,
            this, &ZeraTranslation::ZeraTranslation::onDateTimePoll);
    m_dateTimePollTimer.setInterval(200);
    m_dateTimePollTimer.setSingleShot(false);
    m_dateTimePollTimer.start();
}

ZeraTranslation *ZeraTranslation::getInstance()
{
    if(!m_instance)
        m_instance = new ZeraTranslation();
    return  m_instance;
}

QObject *ZeraTranslation::getStaticInstance(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return getInstance();
}

void ZeraTranslation::setLanguage(const QString &language)
{
    if(m_currentLanguage != language) {
        m_currentLanguage = language;
        QLocale locale = QLocale(m_currentLanguage);
        QString languageName = QLocale::languageToString(locale.language());
        if(m_translationFilesModel.contains(language) || language == "C") {
            QCoreApplication::instance()->removeTranslator(&m_translator);
            const QString filename = m_translationFilesModel.value(language);
            qInfo("Load translation file %s...", qPrintable(filename));
            if(m_translator.load(filename)) {
                qInfo("Install translator...");
                QCoreApplication::instance()->installTranslator(&m_translator);
                qInfo() << "Current Language changed to" << languageName << locale << language;
                reloadStringTable();
            }
            else {
                if(language != "C")
                    qWarning() << "Language not found:" << language << filename.arg(language);
                reloadStringTable();
            }
        }
        else if(language != "C")
            qWarning() << "Language not found for locale:" << language;
    }
}

void ZeraTranslation::setInitialLanguage(const QString &language)
{
    m_initialLanguage = language;
}

QString ZeraTranslation::getLanguage()
{
    return m_currentLanguage;
}

QVariant ZeraTranslation::trValue(const QString &key)
{
    const auto iter = m_currentTranslations.constFind(key);
    if(iter == m_currentTranslations.constEnd()) {
        qWarning("Translation not found for '%s'", qPrintable(key));
        m_currentTranslations.insert(key, key);
        return key;
    }
    return iter.value();
}

QString ZeraTranslation::trDateTimeShort(const QString &dateTime)
{
    QDateTime dTime = QDateTime::fromString(dateTime);
    QLocale locale(m_currentLanguage);
    const QString formatStr = getTimeDateFormatShort(locale);
    return locale.toString(dTime, formatStr);
}

QString ZeraTranslation::trDateTimeTz(const QString &dateTime)
{
    QDateTime dTime = QDateTime::fromString(dateTime);
    QLocale locale(m_currentLanguage);
    const QString formatStr = "t";
    return locale.toString(dTime, formatStr);
}

QString ZeraTranslation::trDateTimeLong(const QString &dateTime)
{
    QDateTime dTime = QDateTime::fromString(dateTime);
    QLocale locale(m_currentLanguage);
    const QString dateFormat = locale.dateFormat(QLocale::LongFormat);
    QString timeFormat = locale.timeFormat(QLocale::LongFormat);
    const QString formatStr = dateFormat + QLatin1Char(' ') + timeFormat;
    return locale.toString(dTime, formatStr);
}

QString ZeraTranslation::getDateSeparator() const
{
    QLocale locale(m_currentLanguage);
    QString dateFormat = locale.dateFormat(QLocale::ShortFormat);
    QString dateSeparator = dateFormat
                                .remove("d", Qt::CaseInsensitive)
                                .remove("m", Qt::CaseInsensitive)
                                .remove("y", Qt::CaseInsensitive);
    if(dateSeparator.length() == 2)
        return dateSeparator.at(1);
    return "/";
}

QDateTime ZeraTranslation::getDateTimeNow()
{
    return QDateTime::currentDateTime();
}

QStringList ZeraTranslation::getSupportedLanguages() const
{
    return m_translationFilesModel.keys();
}

QStringList ZeraTranslation::getLocalesModel()
{
    return m_translationFlagsModel.keys();
}

QStringList ZeraTranslation::getFlagsModel()
{
    return m_translationFlagsModel.values();
}

QStringList ZeraTranslation::getFlagsModelSvg()
{
    QStringList flagsModelSvg = getFlagsModel();
    for (QString &flag : flagsModelSvg)
        flag.replace(".png", ".svg");
    return flagsModelSvg;
}

void ZeraTranslation::onDateTimePoll()
{
    QTime nowTime = getDateTimeNow().time();
    int seconds = nowTime.second();
    if(m_secondsStored != seconds) {
        m_secondsStored = seconds;
        emit sigDateTimeNowSecondChanged();
    }
}

void ZeraTranslation::setupTranslationFiles()
{
    // Append translations not yet good enough to ship to ignoreList
    QStringList ignoreList = QStringList() << "it_IT" << "el_GR";

    QDir searchDir(QString(ZERA_TRANSLATION_PATH));
    if(searchDir.exists() && searchDir.isReadable()) {
        const auto qmList = searchDir.entryInfoList({"*.qm"}, QDir::Files);
        for(const QFileInfo &qmFileInfo : qmList) {
            const QString localeName = qmFileInfo.fileName().replace("zera-gui_","").replace(".qm","");
            if(ignoreList.contains(localeName))
                continue;
            if(!m_translationFilesModel.contains(localeName)) {
                QFileInfo flagFileInfo;
                flagFileInfo.setFile(QString("%1/flag_%2.png").arg(qmFileInfo.path(), localeName));//currently only supports .png (.svg rasterization is too slow)
                if(flagFileInfo.exists()) {
                    m_translationFilesModel.insert(localeName, qmFileInfo.absoluteFilePath());
                    const QUrl flagUrl = QUrl::fromLocalFile(flagFileInfo.absoluteFilePath());
                    m_translationFlagsModel.insert(localeName, flagUrl.toString()); //qml image needs url form (qrc:<...> or file://<...>)
                }
                else
                    qWarning() << "Flag file for translation:" << qmFileInfo.absoluteFilePath() << "doesn't exist, skipping translation!";
            }
            else
                qWarning() << "Skipping duplicate translation:" << qmFileInfo.absoluteFilePath() << "already loaded file from:" << m_translationFilesModel.value(localeName);
        }
    }
}

void ZeraTranslation::reloadStringTable()
{
    QElapsedTimer elapsed;
    elapsed.start();

    qInfo("Reload translation string table...");
    QVariantHash tmpTranslations = loadTranslationHash();
    int translationSize = tmpTranslations.count(); // Qt6 returns qsizetype => force int to avoid version macro & if else
    qInfo("Translation string table with %i entries reloaded within %lldms.", translationSize, elapsed.elapsed());

    for(auto iter=tmpTranslations.cbegin(); iter!=tmpTranslations.cend(); iter++)
        m_currentTranslations.insert(iter.key(), iter.value());
    qInfo("Translation strings added to current translation map within %lldms.", elapsed.elapsed());

    emit sigLanguageChanged();
    qInfo("Language change notification within %lldms.", elapsed.elapsed());
}

QString ZeraTranslation::getTimeDateFormatShort(const QLocale &locale)
{
    const QString dateFormat = locale.dateFormat(QLocale::ShortFormat);
    QString timeFormat = locale.timeFormat(QLocale::ShortFormat);
    // hack in seconds - short format is missing them
    if(!timeFormat.contains("ss"))
        timeFormat.replace("mm", "mm:ss");
    const QString formatStr = dateFormat + QLatin1Char(' ') + timeFormat;
    return formatStr;
}

void ZeraTranslation::addTranslation(QVariantHash &translationHash, const QString &labelInSoure, const QString &labelInTranslation)
{
    if (translationHash.contains(labelInSoure))
        qWarning("Translation: Double entry '%s'", qPrintable(labelInSoure));
    translationHash.insert(labelInSoure, labelInTranslation);
}

const QVariantHash ZeraTranslation::loadTranslationHash()
{
    //addTranslation(tmpTranslations, "something %1", tr("something %1"))...

    QVariantHash tmpTranslations;
    // common translations
    addTranslation(tmpTranslations, "AC", tr("AC", "Alternating current"));
    addTranslation(tmpTranslations, "U", tr("U", "Voltage"));
    addTranslation(tmpTranslations, "W", tr("W", "Watt unit active"));
    addTranslation(tmpTranslations, "VA", tr("VA", "Watt unit reactive"));
    addTranslation(tmpTranslations, "Var", tr("Var", "Watt unit apparent"));

    addTranslation(tmpTranslations, "4LW", tr("4LW", "4 Leiter Wirkleistung = 4 wire active power"));
    addTranslation(tmpTranslations, "3LW", tr("3LW", "3 Leiter Wirkleistung = 3 wire active power"));
    addTranslation(tmpTranslations, "2LW", tr("2LW", "2 Leiter Wirkleistung = 2 wire active power"));

    addTranslation(tmpTranslations, "4LB", tr("4LB", "4 Leiter Blindleistung = 4 wire reactive power"));
    addTranslation(tmpTranslations, "4LBK", tr("4LBK", "4 Leiter Blindleistung künstlicher Nulleiter = 4 wire reactive power artificial neutral conductor"));
    addTranslation(tmpTranslations, "3LB", tr("3LB", "3 Leiter Blindleistung = 3 wire reactive power"));
    addTranslation(tmpTranslations, "2LB", tr("2LB", "2 Leiter Blindleistung = 2 wire reactive power"));

    addTranslation(tmpTranslations, "4LS", tr("4LS", "4 Leiter Scheinleistung = 4 wire apparent power"));
    addTranslation(tmpTranslations, "4LSg", tr("4LSg", "4 Leiter Scheinleistung geometrisch = 4 wire apparent power geometric"));
    addTranslation(tmpTranslations, "3LS", tr("3LS", "3 Leiter Scheinleistung = 3 wire apparent power"));
    addTranslation(tmpTranslations, "3LSg", tr("3LSg", "3 Leiter Scheinleistung geometrisch = 3 wire apparent power geometric"));
    addTranslation(tmpTranslations, "2LS", tr("2LS", "2 Leiter Scheinleistung = 2 wire apparent power"));
    addTranslation(tmpTranslations, "2LSg", tr("2LSg", "2 Leiter Scheinleistung geometrisch = 2 wire apparent power geometric"));

    addTranslation(tmpTranslations, "XLW", tr("XLW", "X Leiter Wirkleistung = X wire active power"));
    addTranslation(tmpTranslations, "XLB", tr("XLB", "X Leiter Blindleistung = X wire reactive power"));
    addTranslation(tmpTranslations, "XLS", tr("XLS", "X Leiter Scheinleistung = X wire apparent power"));
    addTranslation(tmpTranslations, "XLSg", tr("XLSg", "X Leiter Scheinleistung geometrisch = X wire apparent power geometric"));

    addTranslation(tmpTranslations, "QREF", tr("QREF", "Referenz-Modus = reference mode"));

    addTranslation(tmpTranslations, "ind", tr("ind", "Power factor inductive load type"));
    addTranslation(tmpTranslations, "cap", tr("cap", "Power factor capacitive load type"));

    addTranslation(tmpTranslations, "L1", tr("L1", "measuring system 1"));
    addTranslation(tmpTranslations, "L2", tr("L2", "measuring system 2"));
    addTranslation(tmpTranslations, "L3", tr("L3", "measuring system 3"));
    addTranslation(tmpTranslations, "AUX", tr("AUX", "auxiliary measuring system"));

    addTranslation(tmpTranslations, "Phase1", tr("Phase1", "Letter or number for phase e.g euro: 1 / us: A"));
    addTranslation(tmpTranslations, "Phase2", tr("Phase2", "Letter or number for phase e.g euro: 2 / us: B"));
    addTranslation(tmpTranslations, "Phase3", tr("Phase3", "Letter or number for phase e.g euro: 3 / us: C"));

    addTranslation(tmpTranslations, "P 1", tr("P 1", "Meter test power phase 1"));
    addTranslation(tmpTranslations, "P 2", tr("P 2", "Meter test power phase 2"));
    addTranslation(tmpTranslations, "P 3", tr("P 3", "Meter test power phase 3"));
    addTranslation(tmpTranslations, "P AUX", tr("P AUX", "Meter test power phase AUX"));
    addTranslation(tmpTranslations, "P AC", tr("P AC", "Meter test power AC"));
    addTranslation(tmpTranslations, "P DC", tr("P DC", "Meter test power DC"));

    addTranslation(tmpTranslations, "REF1", tr("REF1", "reference channel 1"));
    addTranslation(tmpTranslations, "REF2", tr("REF2", "reference channel 2"));
    addTranslation(tmpTranslations, "REF3", tr("REF3", "reference channel 3"));
    addTranslation(tmpTranslations, "REF4", tr("REF4", "reference channel 4"));
    addTranslation(tmpTranslations, "REF5", tr("REF5", "reference channel 5"));
    addTranslation(tmpTranslations, "REF6", tr("REF6", "reference channel 6"));

    addTranslation(tmpTranslations, "UPN", tr("UPN","voltage pase to neutral"));
    addTranslation(tmpTranslations, "UPP", tr("UPP","voltage phase to phase"));
    addTranslation(tmpTranslations, "kU", tr("kU","harmonic distortion on voltage"));
    addTranslation(tmpTranslations, "I", tr("I","current"));
    addTranslation(tmpTranslations, "kI", tr("kI","harmonic distortion on current"));
    addTranslation(tmpTranslations, "∠U", tr("∠U","phase difference of voltage to reference channel"));
    addTranslation(tmpTranslations, "∠I", tr("∠I","phase difference of current to reference channel"));
    addTranslation(tmpTranslations, "∠UI", tr("∠UI","phase difference"));
    addTranslation(tmpTranslations, "λ", tr("λ","power factor"));
    //: needs to be short enough to fit
    addTranslation(tmpTranslations, "P", tr("P","active power"));
    //: needs to be short enough to fit
    addTranslation(tmpTranslations, "Q", tr("Q","reactive power"));
    //: needs to be short enough to fit
    addTranslation(tmpTranslations, "S", tr("S","apparent power"));
    addTranslation(tmpTranslations, "F", tr("F","frequency"));

    addTranslation(tmpTranslations, "Sb", tr("Sb", "standard burden"));
    addTranslation(tmpTranslations, "cos(β)", tr("cos(β)", "cosinus beta"));
    addTranslation(tmpTranslations, "Sn", tr("Sn", "operating burden in %, relative to the nominal burden"));
    addTranslation(tmpTranslations, "BRD1", tr("BRD1", "burden system name"));
    addTranslation(tmpTranslations, "BRD2", tr("BRD2", "burden system name"));
    addTranslation(tmpTranslations, "BRD3", tr("BRD3", "burden system name"));

    //: In error calculator, switch between time based and period based measurement. In Network settings, connection mode DHCP/Manual
    addTranslation(tmpTranslations, "Mode:", tr("Mode:"));

    //PagePathView.qml
    //: as in "close this view"
    addTranslation(tmpTranslations, "OK", tr("OK"));
    addTranslation(tmpTranslations, "Close", tr("Close", "not open"));
    addTranslation(tmpTranslations, "Accept", tr("Accept"));
    addTranslation(tmpTranslations, "Cancel", tr("Cancel"));
    addTranslation(tmpTranslations, "Save", tr("Save"));
    //: default session
    addTranslation(tmpTranslations, "Default", tr("Default"));
    //: changing energy direction session
    addTranslation(tmpTranslations, "Changing energy direction", tr("Changing energy direction"));
    //: changing energy direction session
    addTranslation(tmpTranslations, "Reference", tr("Reference"));
    addTranslation(tmpTranslations, "3 Systems / 2 Wires", tr("3 Systems / 2 Wires"));
    addTranslation(tmpTranslations, "DC: 4*Voltage / 1*Current", tr("DC: 4*Voltage / 1*Current"));

    //RangeMenu.qml
    //: used for a yes / no configuration element
    addTranslation(tmpTranslations, "Range automatic", tr("Range automatic"));
    //: when selected, the corresponding phases are inverted
    addTranslation(tmpTranslations, "Invert", tr("Invert"));
    //: measurement channel range overload, e.g. the range is configured for 5V measurement and the measured input voltage is >5V
    addTranslation(tmpTranslations, "Overload", tr("Overload"));
    //: used for a yes / no configuration element
    addTranslation(tmpTranslations, "Range grouping", tr("Range grouping"));
    //: header text - can be multiline
    addTranslation(tmpTranslations, "Measurement modes", tr("Measurement modes"));
    //: Ratio checkbox
    addTranslation(tmpTranslations, "Ratio", tr("Ratio"));

    //Settings.qml
    //: settings specific to the GUI application
    addTranslation(tmpTranslations, "Application", tr("Application"));
    addTranslation(tmpTranslations, "Timezone:", tr("Timezone:"));
    //: used for a yes / no configuration element
    addTranslation(tmpTranslations, "Relative to fundamental", tr("Relative to fundamental"));
    addTranslation(tmpTranslations, "Show angles", tr("Show angles"));
    //: max number total decimals for displayed values
    addTranslation(tmpTranslations, "Max decimals total:", tr("Max decimals total:"));
    //: max number of decimals after the decimal separator
    addTranslation(tmpTranslations, "Max places after the decimal point:", tr("Max places after the decimal point:"));
    //: Remote web label
    addTranslation(tmpTranslations, "Web-Server:", tr("Web-Server:"));
    //: Remote API label
    addTranslation(tmpTranslations, "API-Access:", tr("API-Access:"));

    //: used for the selection of language via country flag
    addTranslation(tmpTranslations, "Language:", tr("Language:"));
    //: used for the selection of GUI display mode
    addTranslation(tmpTranslations, "Display:", tr("Display:"));
    //: GUI display mode fullscreen
    addTranslation(tmpTranslations, "Fullscreen", tr("Fullscreen"));
    //: GUI display mode desktop
    addTranslation(tmpTranslations, "Desktop (slow)", tr("Desktop (slow)"));
    //: settings specific to the hardware
    addTranslation(tmpTranslations, "Device", tr("Device"));
    //: settings specific to the network
    addTranslation(tmpTranslations, "Network", tr("Network"));
    //: settings specific to the Bluetooth sensors
    addTranslation(tmpTranslations, "BLE sensor", tr("BLE sensor"));
    //: measurement channel the phase locked loop uses as base
    addTranslation(tmpTranslations, "PLL channel:", tr("PLL channel:"));
    //: automatic phase locked loop channel selection
    addTranslation(tmpTranslations, "PLL channel automatic:", tr("PLL channel automatic:"));
    //: dft phase reference channel
    addTranslation(tmpTranslations, "DFT reference channel:", tr("DFT reference channel:"));
    addTranslation(tmpTranslations, "Colors:", tr("Colors:"));
    //: Settings show/hide AUX phases
    addTranslation(tmpTranslations, "Show AUX phase values", tr("Show AUX phase values"));
    //: Color themes popup adjust brightness for current
    addTranslation(tmpTranslations, "Brightness currents:", tr("Brightness currents:"));
    //: Color themes popup adjust brightness for current
    addTranslation(tmpTranslations, "Brightness black:", tr("Brightness black:"));
    addTranslation(tmpTranslations, "Channel ignore limit [% of range]:", tr("Channel ignore limit [% of range]:"));

    //SettingsInterval.qml
    //: time based integration interval
    addTranslation(tmpTranslations, "Integration time interval", tr("Integration time interval"));
    //: measurement period based integration interval
    addTranslation(tmpTranslations, "Integration period interval:", tr("Integration period interval:"));
    //displayed under settings
    addTranslation(tmpTranslations, "seconds", tr("seconds", "integration time interval unit"));
    addTranslation(tmpTranslations, "periods", tr("periods", "integration period interval unit"));

    //: ZDeleteConfirmPopup.qml Delete confirmation popup header
    addTranslation(tmpTranslations, "Confirmation", tr("Confirmation"));

    // Network settings
    //: AvailableApDialog.qml Wifi password access point
    addTranslation(tmpTranslations, "Wifi Password", tr("Wifi Password"));
    //: AvailableApDialog.qml Networks?
    addTranslation(tmpTranslations, "Networks:", tr("Networks:"));
    //: EthernetSettings.qml Header text
    addTranslation(tmpTranslations, "Ethernet Connection Settings", tr("Ethernet Connection Settings"));
    //: EthernetSettings.qml Ethernet connection name
    addTranslation(tmpTranslations, "Connection name:", tr("Connection name:"));
    //: EthernetSettings.qml Ethernet connection name hint
    addTranslation(tmpTranslations, "Enter connection name displayed", tr("Enter connection name displayed"));
    //: EthernetSettings.qml IPv4 sub-header
    addTranslation(tmpTranslations, "IPv4", tr("IPv4"));
    //: connection Manual fixed IP address
    addTranslation(tmpTranslations, "IP:", tr("IP:"));
    //: connection Manual fixed IP address hint
    addTranslation(tmpTranslations, "Enter IP address e.g 192.168.1.1", tr("Enter IP address e.g 192.168.1.1"));
    //: connection Manual fixed IP subnetmask
    addTranslation(tmpTranslations, "Subnetmask:", tr("Subnetmask:"));
    //: connection Manual fixed IP subnetmask hint
    addTranslation(tmpTranslations, "Enter subnet mask e.g 255.255.255.0", tr("Enter subnet mask e.g 255.255.255.0"));
    //: EthernetSettings.qml IPv6 sub-header
    addTranslation(tmpTranslations, "IPv6", tr("IPv6"));
    //: EthernetSettings.qml IPv6 error field IP
    addTranslation(tmpTranslations, "IPV6 IP", tr("IPV6 IP"));
    //: EthernetSettings.qml IPv6 error field subnetmask
    addTranslation(tmpTranslations, "IPV6 Subnetmask", tr("IPV6 Subnetmask"));
    //: EthernetSettings.qml IPv4 error field IP
    addTranslation(tmpTranslations, "IPV4 IP", tr("IPV4 IP"));
    //: EthernetSettings.qml IPv4 error field subnetmask
    addTranslation(tmpTranslations, "IPV4 Subnetmask", tr("IPV4 Subnetmask"));

    //: SmartConnect.qml Wifi password header
    addTranslation(tmpTranslations, "Wifi password", tr("Wifi password"));
    //: SmartConnect.qml Wifi password
    addTranslation(tmpTranslations, "Password:", tr("Password:"));
    //: SmartConnect.qml Wifi password
    addTranslation(tmpTranslations, "Device:", tr("Device:"));

    //: WifiSettings.qml Header text hotspot
    addTranslation(tmpTranslations, "Hotspot Settings", tr("Hotspot Settings"));
    //: WifiSettings.qml Header text wifi
    addTranslation(tmpTranslations, "Wifi Settings", tr("Wifi Settings"));
    //: WifiSettings.qml SSID field
    addTranslation(tmpTranslations, "SSID:", tr("SSID:"));
    //: WifiSettings.qml SSID field hint
    addTranslation(tmpTranslations, "SSID: the public hotspot name", tr("SSID: the public hotspot name"));
    //: WifiSettings.qml SSID select dialog header
    addTranslation(tmpTranslations, "Wifi SSID", tr("Wifi SSID"));
    //: WifiSettings.qml Mode ComboBox
    addTranslation(tmpTranslations, "Hotspot", tr("Hotspot"));
    //: EthernetSettings.qml error SSID
    addTranslation(tmpTranslations, "SSID", tr("SSID"));
    //: EthernetSettings.qml password
    addTranslation(tmpTranslations, "Password", tr("Password"));
    //: EthernetSettings.qml password hint
    addTranslation(tmpTranslations, "Enter password (minimum 8 characters)", tr("Enter password (minimum 8 characters)"));
    //: EthernetSettings.qml Mode ComboBox
    addTranslation(tmpTranslations, "Automatic (DHCP)", tr("Automatic (DHCP)"));
    //: EthernetSettings.qml Mode ComboBox
    addTranslation(tmpTranslations, "Manual", tr("Manual"));
    //: SUggested default
    addTranslation(tmpTranslations, "Wifi", tr("Wifi"));
    //: Checkbox for auto-connection
    addTranslation(tmpTranslations, "Autoconnect:", tr("Autoconnect:"));

    //: ConnectionTree.qml group Ethernet
    addTranslation(tmpTranslations, "ETHERNET", tr("Ethernet"));
    //: ConnectionTree.qml group Wifi
    addTranslation(tmpTranslations, "WIFI", tr("WIFI"));
    //: ConnectionTree.qml group Hotspot
    addTranslation(tmpTranslations, "HOTSPOT", tr("Hotspot"));
    //: ConnectionTree.qml type in connection delete confirm popup
    addTranslation(tmpTranslations, "Delete network connection <b>'%1'</b>?", tr("Delete network connection <b>'%1'</b>?"));

    //: ConnectionInfo.qml header
    addTranslation(tmpTranslations, "Connection Information", tr("Connection Information"));
    //: ConnectionTree.qml netmask header
    addTranslation(tmpTranslations, "Netmask:", tr("Netmask:"));
    //: ConnectionTree.qml add entry
    addTranslation(tmpTranslations, "Add Ethernet...", tr("Add Ethernet..."));
    //: ConnectionTree.qml add entry
    addTranslation(tmpTranslations, "Add Hotspot...", tr("Add Hotspot..."));
    //: ConnectionTree.qml add entry
    addTranslation(tmpTranslations, "Add Wifi...", tr("Add Wifi..."));

    //MainToolBar.qml
    addTranslation(tmpTranslations, "Battery low !\nPlease charge the device before it turns down", tr("Battery low !\nPlease charge the device before it turns down"));

    //main.qml
    addTranslation(tmpTranslations, "Loading...", tr("Loading..."));
    //: progress of loading %1 of %2 objects
    addTranslation(tmpTranslations, "Loading: %1/%2", tr("Loading: %1/%2"));
    //: settings for range automatic etc.
    addTranslation(tmpTranslations, "Range", tr("Range", "measuring range"));
    addTranslation(tmpTranslations, "Please wait...", tr("Please wait..."));
    addTranslation(tmpTranslations, "Save/Send logs", tr("Save/Send logs"));
    addTranslation(tmpTranslations, "Something went wrong", tr("Something went wrong"));
    addTranslation(tmpTranslations, "Firmware update is running.\nDo not switch off the device!", tr("Firmware update is running.\nDo not switch off the device!"));

    //ErrorCalculatorModulePage.qml
    //: reference channel selection
    addTranslation(tmpTranslations, "Reference input:", tr("Reference input:"));
    //: device input selection (e.g. scanning head)
    addTranslation(tmpTranslations, "Device input:", tr("Device input:"));
    //: device under test constant
    addTranslation(tmpTranslations, "Constant", tr("Constant"));
    //: energy to compare
    addTranslation(tmpTranslations, "Energy:", tr("Energy:"));
    //: value based on the DUT constant
    addTranslation(tmpTranslations, "Pulse:", tr("Pulse:"));
    addTranslation(tmpTranslations, "Start", tr("Start"));
    addTranslation(tmpTranslations, "Stop", tr("Stop"));
    addTranslation(tmpTranslations, "Lower error limit:", tr("Lower error limit:"));
    addTranslation(tmpTranslations, "Upper error limit:", tr("Upper error limit:"));
    addTranslation(tmpTranslations, "continuous", tr("continuous"));
    addTranslation(tmpTranslations, "Count / Pause:", tr("Count / Pause:"));
    //: popup if range automatic active in POWER/ENERGY REGISTER
    addTranslation(tmpTranslations, "Switch off 'Range automatic'", tr("Switch off 'Range automatic'"));
    addTranslation(tmpTranslations, "Select a matching range", tr("Select a matching range"));

    //ErrorRegisterModulePage.qml
    addTranslation(tmpTranslations, "Duration:", tr("Duration:"));
    addTranslation(tmpTranslations, "Start/Stop", tr("Start/Stop"));
    addTranslation(tmpTranslations, "Duration", tr("Duration"));
    addTranslation(tmpTranslations, "Start value:", tr("Start value:"));
    addTranslation(tmpTranslations, "End value:", tr("End value:"));

    //ErrorCalculatorModulePage.qml
    addTranslation(tmpTranslations, "Frequency:", tr("Frequency:"));

    //ErrorRegisterModulePage.qml
    addTranslation(tmpTranslations, "Count:", tr("Count:"));
    addTranslation(tmpTranslations, "Passed:", tr("Passed:"));
    addTranslation(tmpTranslations, "Failed:", tr("Failed:"));
    addTranslation(tmpTranslations, "Mean:", tr("Mean:"));
    addTranslation(tmpTranslations, "Range:", tr("Range:"));
    addTranslation(tmpTranslations, "Stddev. n:", tr("Stddev. n:"));
    addTranslation(tmpTranslations, "Stddev. n-1:", tr("Stddev. n-1:"));

    //FftTabPage.qml
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "Amp", tr("Amp", "Amplitude of the phasor"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "Phase", tr("Phase","Phase of the phasor"));
    //: total harmonic distortion with noise
    addTranslation(tmpTranslations, "THDN:", tr("THDN:"));
    addTranslation(tmpTranslations, "Harmonic table", tr("Harmonic table", "Tab text harmonic table"));
    addTranslation(tmpTranslations, "Harmonic chart", tr("Harmonic chart", "Tab text harmonic chart"));

    //VectorModulePage.qml
    //: ComboBox: Scale to max. range
    addTranslation(tmpTranslations, "Ranges", tr("Ranges"));
    //: ComboBox: Scale to max. value
    addTranslation(tmpTranslations, "Maximum", tr("Maximum"));

    //PowerModulePage.qml
    addTranslation(tmpTranslations, "Ext.", tr("Ext."));

    //HarmonicPowerTabPage.qml
    addTranslation(tmpTranslations, "Harmonic power table", tr("Harmonic power table", "Tab text harmonic power table"));
    addTranslation(tmpTranslations, "Harmonic power chart", tr("Harmonic power chart", "Tab text harmonic power chart"));


    //: text must be short enough to fit
    addTranslation(tmpTranslations, "UL1", tr("UL1", "channel name"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "UL2", tr("UL2", "channel name"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "UL3", tr("UL3", "channel name"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "IL1", tr("IL1", "channel name"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "IL2", tr("IL2", "channel name"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "IL3", tr("IL3", "channel name"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "UAUX", tr("UAUX", "channel name"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "IAUX", tr("IAUX", "channel name"));

    //: text must be short enough to fit
    addTranslation(tmpTranslations, "P1", tr("P1", "harmonic power label active / phase 1"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "Q1", tr("Q1", "harmonic power label reactive / phase 1"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "S1", tr("S1", "harmonic power label aparent / phase 1"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "P2", tr("P2", "harmonic power label active / phase 2"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "Q2", tr("Q2", "harmonic power label reactive / phase 2"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "S2", tr("S2", "harmonic power label aparent / phase 2"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "P3", tr("P3", "harmonic power label active / phase 3"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "Q3", tr("Q3", "harmonic power label reactive / phase 3"));
    //: text must be short enough to fit
    addTranslation(tmpTranslations, "S3", tr("S3", "harmonic power label aparent / phase 3"));

    //MeasurementPageModel.qml
    //: polar (amplitude and phase) phasor diagram
    addTranslation(tmpTranslations, "Vector diagram", tr("Vector diagram"));
    addTranslation(tmpTranslations, "Actual values", tr("Actual values"));
    addTranslation(tmpTranslations, "Actual values AC", tr("Actual values AC"));
    addTranslation(tmpTranslations, "Actual values DC", tr("Actual values DC"));
    addTranslation(tmpTranslations, "Actual values & Meter tests", tr("Actual values & Meter tests"));
    addTranslation(tmpTranslations, "Oscilloscope plot", tr("Oscilloscope plot"));
    //: FFT bar diagrams or tables that show the harmonic component distribution of the measured values
    addTranslation(tmpTranslations, "Harmonics & Curves", tr("Harmonics & Curves"));
    //: measuring mode dependent power values
    addTranslation(tmpTranslations, "Power values", tr("Power values"));
    //: FFT tables that show the real and imaginary parts of the measured power values
    addTranslation(tmpTranslations, "Harmonic power values", tr("Harmonic power values"));
    //: shows the deviation of measured energy between the reference device and the device under test
    addTranslation(tmpTranslations, "Error calculator", tr("Error calculator"));
    //: shows energy comparison between the reference device and the device under test's registers/display
    addTranslation(tmpTranslations, "Comparison measurements", tr("Comparison measurements"));
    //: control one or more source devices
    addTranslation(tmpTranslations, "Source control", tr("Source control"));

    // EnergyGraphs.qml
    //: Energy / charge plot: Select phases to display label
    addTranslation(tmpTranslations, "Select phase to display:", tr("Select phase to display:"));

    //ComparisonTabsView.qml
    //: Comparison tabs label texts
    addTranslation(tmpTranslations, "Meter test", tr("Meter test"));
    addTranslation(tmpTranslations, "Energy comparison", tr("Energy comparison"));
    addTranslation(tmpTranslations, "Energy register", tr("Energy register"));
    addTranslation(tmpTranslations, "Power register", tr("Power register"));

    addTranslation(tmpTranslations, "Burden values", tr("Burden values"));
    addTranslation(tmpTranslations, "Transformer values", tr("Transformer values"));
    //: effective values
    addTranslation(tmpTranslations, "RMS values", tr("RMS values"));

    //BurdenModulePage.qml
    addTranslation(tmpTranslations, "Voltage Burden", tr("Voltage Burden", "Tab title current burden"));
    addTranslation(tmpTranslations, "Current Burden", tr("Current Burden", "Tab title current burden"));
    addTranslation(tmpTranslations, "Nominal burden:", tr("Nominal burden:"));
    addTranslation(tmpTranslations, "Nominal range:", tr("Nominal range:"));
    addTranslation(tmpTranslations, "Wire crosssection:", tr("Wire crosssection:"));
    addTranslation(tmpTranslations, "Wire length:", tr("Wire length:"));

    //ReferencePageModel.qml
    addTranslation(tmpTranslations, "DC reference values", tr("DC reference values"));
    //QuartzModulePage.qml
    addTranslation(tmpTranslations, "Quartz reference measurement", tr("Quartz reference measurement"));

    //CEDPageModel.qml
    addTranslation(tmpTranslations, "CED power values", tr("CED power values"));

    //HarmonicPowerTabPage.qml
    addTranslation(tmpTranslations, "Measuring modes:", tr("Measuring modes:", "label for measuring mode selectors"));

    //TransformerModulePage.qml
    addTranslation(tmpTranslations, "TR1", tr("TR1", "transformer system 1"));
    addTranslation(tmpTranslations, "X-Prim:", tr("X-Prim:"));
    addTranslation(tmpTranslations, "X-Sec:", tr("X-Sec:"));
    addTranslation(tmpTranslations, "Ms-Prim:", tr("Ms-Prim:"));
    addTranslation(tmpTranslations, "Ms-Sec:", tr("Ms-Sec:"));
    addTranslation(tmpTranslations, "Mp-Prim:", tr("Mp-Prim:"));
    addTranslation(tmpTranslations, "Mp-Sec:", tr("Mp-Sec:"));
    addTranslation(tmpTranslations, "X-Ratio", tr("X-Ratio"));
    addTranslation(tmpTranslations, "N-Sec", tr("N-Sec"));
    addTranslation(tmpTranslations, "X-Prim", tr("X-Prim"));
    addTranslation(tmpTranslations, "X-Sec", tr("X-Sec"));
    addTranslation(tmpTranslations, "crad", tr("crad", "centiradian"));
    addTranslation(tmpTranslations, "arcmin", tr("arcmin", "arcminute"));

    // SourceModulePage.qml
    addTranslation(tmpTranslations, "Actual", tr("Actual", "Source/Vector view indicator label: actual values mode"));
    addTranslation(tmpTranslations, "Target", tr("Target", "Source/Vector view indicator label: target values mode"));

    //DeviceInformation.qml
    addTranslation(tmpTranslations, "Device info", tr("Device info"));
    addTranslation(tmpTranslations, "Device log", tr("Device log"));
    addTranslation(tmpTranslations, "License information", tr("License information"));
    addTranslation(tmpTranslations, "Serial number", tr("Serial number"));
    addTranslation(tmpTranslations, "Operating system version", tr("Operating system version"));
    addTranslation(tmpTranslations, "Emob PCB version", tr("Emob PCB version"));
    addTranslation(tmpTranslations, "Accu PCB version", tr("Accu PCB version"));
    addTranslation(tmpTranslations, "Relay PCB version", tr("Relay PCB version"));
    addTranslation(tmpTranslations, "System PCB version", tr("System PCB version"));
    addTranslation(tmpTranslations, "PCB server version", tr("PCB server version"));
    addTranslation(tmpTranslations, "DSP server version", tr("DSP server version"));
    addTranslation(tmpTranslations, "DSP firmware version", tr("DSP firmware version"));
    addTranslation(tmpTranslations, "FPGA firmware version", tr("FPGA firmware version"));
    addTranslation(tmpTranslations, "System controller version", tr("System controller version"));
    addTranslation(tmpTranslations, "Relay controller version", tr("Relay controller version"));
    addTranslation(tmpTranslations, "Emob controller version", tr("Emob controller version"));
    addTranslation(tmpTranslations, "Accu controller version", tr("Accu controller version"));
    addTranslation(tmpTranslations, "Adjustment status", tr("Adjustment status"));
    addTranslation(tmpTranslations, "Not adjusted", tr("Not adjusted"));
    addTranslation(tmpTranslations, "Wrong version", tr("Wrong version"));
    addTranslation(tmpTranslations, "Wrong serial number", tr("Wrong serial number"));
    addTranslation(tmpTranslations, "CPU-board number", tr("CPU-board number"));
    addTranslation(tmpTranslations, "CPU-board assembly", tr("CPU-board assembly"));
    addTranslation(tmpTranslations, "CPU-board date", tr("CPU-board date"));
    addTranslation(tmpTranslations, "Saving logs and dumps to external drive...", tr("Saving logs and dumps to external drive..."));
    addTranslation(tmpTranslations, "Could not save logs and dumps", tr("Could not save logs and dumps"));
    addTranslation(tmpTranslations, "Starting update...", tr("Starting update..."));
    addTranslation(tmpTranslations, "Could not update. Please check if necessary files are available.", tr("Could not update. Please check if necessary files are available."));
    addTranslation(tmpTranslations, "Update failed. Please save logs and send them to service@zera.de.", tr("Update failed. Please save logs and send them to service@zera.de."));

    // ServiceSupport.qml
    addTranslation(tmpTranslations, "Service Support", tr("Service Support"));
    addTranslation(tmpTranslations, "Save logfile to USB", tr("Save logfile to USB"));
    addTranslation(tmpTranslations, "Start Update", tr("Start Update"));

    //LoggerSettings.qml
    addTranslation(tmpTranslations, "Database Logging", tr("Database Logging"));
    addTranslation(tmpTranslations, "Logging enabled:", tr("Logging enabled:"));
    addTranslation(tmpTranslations, "DB location:", tr("DB location:"));
    //: describes the duration of the recording
    addTranslation(tmpTranslations, "Logging Duration [hh:mm:ss]:", tr("Logging Duration [hh:mm:ss]:"));
    addTranslation(tmpTranslations, "Logger status:", tr("Logger status:"));
    //: describes the ongoing task of recording data into a database
    addTranslation(tmpTranslations, "Logging data", tr("Logging data"));
    addTranslation(tmpTranslations, "Logging disabled", tr("Logging disabled"));
    addTranslation(tmpTranslations, "No database selected", tr("No database selected"));
    addTranslation(tmpTranslations, "Database loaded", tr("Database loaded"));
    addTranslation(tmpTranslations, "Database error", tr("Database error"));
    addTranslation(tmpTranslations, "Manage customer data:", tr("Manage customer data:"));
    addTranslation(tmpTranslations, "Snapshot", tr("Snapshot"));
    //: when the system disabled the customer data management, the brackets are for visual distinction from other text
    addTranslation(tmpTranslations, "[customer data is not available]", tr("[customer data is not available]"));
    //: when the customer id is empty, the brackets are for visual distinction from other text
    addTranslation(tmpTranslations, "[customer id is not set]", tr("[customer id is not set]"));
    //: when the customer number is empty, the brackets are for visual distinction from other text
    addTranslation(tmpTranslations, "[customer number is not set]", tr("[customer number is not set]"));
    //: placeholder text for the database filename
    addTranslation(tmpTranslations, "filename", tr("filename"));
    //: popup create database header
    addTranslation(tmpTranslations, "Create database", tr("Create database"));
    addTranslation(tmpTranslations, "Show", tr("Show"));
    addTranslation(tmpTranslations, "Basic", tr("Basic"));

    //LoggerDbSearchDialog.qml
    //:Select db location: internal storage
    addTranslation(tmpTranslations, "internal", tr("internal"));
    //:Select db location: external storage e.g memory stick
    addTranslation(tmpTranslations, "external", tr("external"));
    //:Delete database confirmation text - %1 is replaced by database filename
    addTranslation(tmpTranslations, "Delete database <b>'%1'</b>?", tr("Delete database <b>'%1'</b>?"));

    //LoggerSessionNameSelector.qml
    //: displayed in logger session name popup, visible when the user presses start or snapshot in the logger
    //: the session name is a database field that the user can use to search / filter different sessions
    addTranslation(tmpTranslations, "Select session name", tr("Select session name"));
    //: label for current session name
    addTranslation(tmpTranslations, "Current name:", tr("Current name:"));
    //: header for list of existing session names. Operator can select one of them to make it current session name
    addTranslation(tmpTranslations, "Select existing:", tr("Select existing:"));
    addTranslation(tmpTranslations, "Preview", tr("Preview"));
    //: delete session confirmation popup header
    addTranslation(tmpTranslations, "Delete session <b>'%1'</b>?", tr("Delete session <b>'%1'</b>?"));

    // LoggerSessionNew.qml
    //: header view 'Add new session'
    addTranslation(tmpTranslations, "Add new session", tr("Add new session"));
    //: add new session view: label session name
    addTranslation(tmpTranslations, "Session name:", tr("Session name:"));
    //: add new session view: label list cutomer data to select entry from
    addTranslation(tmpTranslations, "Select customer data:", tr("Select customer data:"));
    //: add new session view: list entry to select no customer data
    addTranslation(tmpTranslations, "-- no customer --", tr("-- no customer --"));
    addTranslation(tmpTranslations, "no customer", tr("no customer"));

    // LoggerSessionNameWithMacrosPopup.qml
    //: default prefix for auto session name
    addTranslation(tmpTranslations, "Session ", tr("Session "));

    //LoggerCustomDataSelector.qml
    //: logger custom data selector view header
    addTranslation(tmpTranslations, "Select custom data contents", tr("Select custom data contents"));
    //: button to select content-set actual values
    addTranslation(tmpTranslations, "ZeraActualValues", tr("Actual values"));
    //: button to select content-set harmonic values
    addTranslation(tmpTranslations, "ZeraHarmonics", tr("Harmonic values"));
    //: button to select content-set sample values
    addTranslation(tmpTranslations, "ZeraCurves", tr("Sample values"));
    addTranslation(tmpTranslations, "ZeraComparison", tr("Comparison measurements"));
    //: button to select content-set burden values
    addTranslation(tmpTranslations, "ZeraBurden", tr("Burden values"));
    //: button to select content-set transformer values
    addTranslation(tmpTranslations, "ZeraTransformer", tr("Transformer values"));
    //: button to select content-set dc-reference values
    addTranslation(tmpTranslations, "ZeraDCReference", tr("DC reference values"));
    //: button to select content-set dc-reference values
    addTranslation(tmpTranslations, "ZeraQuartzReference", tr("Quartz reference values"));
    //: button to select content-set dc-reference values
    addTranslation(tmpTranslations, "ZeraAll", tr("All"));
    //: logger custom data selector get back to menu button
    addTranslation(tmpTranslations, "Back", tr("Back"));

    //LoggerMenu.qml
    //: displayed when user presses logger button
    //: text displayed in session name menu entry when no session was set yet
    addTranslation(tmpTranslations, "-- no session --", tr("-- no session --"));
    //: take snapshot menu entry
    addTranslation(tmpTranslations, "Take snapshot", tr("Take snapshot"));
    //: save snapshot error message
    addTranslation(tmpTranslations, "Could not store snapshot in database. Please save logs and send them to service@zera.de.", tr("Could not store snapshot in database. Please save logs and send them to service@zera.de."));
    //: start logging menu entry
    addTranslation(tmpTranslations, "Start logging", tr("Start logging"));
    //: stop logging menu entry
    addTranslation(tmpTranslations, "Stop logging", tr("Stop logging"));
    //: menu entry to open logger settings
    addTranslation(tmpTranslations, "Settings...", tr("Settings..."));
    //: Logger menu entry export
    addTranslation(tmpTranslations, "Export...", tr("Export..."));
    //: menu radio button to select content-set actual values
    addTranslation(tmpTranslations, "MenuZeraActualValues", tr("Actual values only"));
    //: menu radio button to select content-set harmonic values
    addTranslation(tmpTranslations, "MenuZeraHarmonics", tr("Harmonic values only"));
    //: menu radio button to select content-set sample values
    addTranslation(tmpTranslations, "MenuZeraCurves", tr("Sample values only"));
    //: menu radio button to select content-set comparison measurement values
    addTranslation(tmpTranslations, "MenuZeraComparison", tr("Comparison measurements only"));
    //: menu radio button to select content-set burden values
    addTranslation(tmpTranslations, "MenuZeraBurden", tr("Burden values only"));
    //: menu radio button to select content-set transformer values
    addTranslation(tmpTranslations, "MenuZeraTransformer", tr("Transformer values only"));
    //: menu radio button to select content-set dc-reference values
    addTranslation(tmpTranslations, "MenuZeraDCReference", tr("DC reference values only"));
    //: menu radio button to select content-set dc-reference values
    addTranslation(tmpTranslations, "MenuZeraQuartzReference", tr("Quartz reference values only"));
    //: menu radio button to select all values
    addTranslation(tmpTranslations, "MenuZeraAll", tr("All data"));
    //: menu radio button to select custom content-sets values
    addTranslation(tmpTranslations, "MenuZeraCustom", tr("Custom data"));

    //CustomerDataEntry.qml
    addTranslation(tmpTranslations, "Customer", tr("Customer"));
    //: power meter, not distance
    addTranslation(tmpTranslations, "Meter information", tr("Meter information"));
    addTranslation(tmpTranslations, "Location", tr("Location"));
    addTranslation(tmpTranslations, "Power grid", tr("Power grid"));
    addTranslation(tmpTranslations, "PAR_DatasetIdentifier", tr("Data Identifier:"));
    addTranslation(tmpTranslations, "PAR_DatasetComment", tr("Data Comment:"));
    addTranslation(tmpTranslations, "PAR_CustomerNumber", tr("Customer number:"));
    addTranslation(tmpTranslations, "PAR_CustomerFirstName", tr("Customer First name:"));
    addTranslation(tmpTranslations, "PAR_CustomerLastName", tr("Customer Last name:"));
    addTranslation(tmpTranslations, "PAR_CustomerCountry", tr("Customer Country:"));
    addTranslation(tmpTranslations, "PAR_CustomerCity", tr("Customer City:"));
    addTranslation(tmpTranslations, "PAR_CustomerPostalCode", tr("Customer ZIP code:", "Postal code"));
    addTranslation(tmpTranslations, "PAR_CustomerStreet", tr("Customer Street:"));
    addTranslation(tmpTranslations, "PAR_CustomerComment", tr("Customer Comment:"));
    addTranslation(tmpTranslations, "PAR_LocationNumber", tr("Location Identifier:"));
    addTranslation(tmpTranslations, "PAR_LocationFirstName", tr("Location First name:"));
    addTranslation(tmpTranslations, "PAR_LocationLastName", tr("Location Last name:"));
    addTranslation(tmpTranslations, "PAR_LocationCountry", tr("LocationCountry:"));
    addTranslation(tmpTranslations, "PAR_LocationCity", tr("Location City:"));
    addTranslation(tmpTranslations, "PAR_LocationPostalCode", tr("Location ZIP code:", "Postal code"));
    addTranslation(tmpTranslations, "PAR_LocationStreet", tr("Location Street:"));
    addTranslation(tmpTranslations, "PAR_LocationComment", tr("Location Comment:"));
    addTranslation(tmpTranslations, "PAR_MeterFactoryNumber", tr("Meter Factory number:"));
    addTranslation(tmpTranslations, "PAR_MeterManufacturer", tr("Meter Manufacturer:"));
    addTranslation(tmpTranslations, "PAR_MeterOwner", tr("Meter Owner:"));
    addTranslation(tmpTranslations, "PAR_MeterComment", tr("Meter Comment:"));
    addTranslation(tmpTranslations, "PAR_PowerGridOperator", tr("Power grid Operator:"));
    addTranslation(tmpTranslations, "PAR_PowerGridSupplier", tr("Power grid Supplier:"));
    addTranslation(tmpTranslations, "PAR_PowerGridComment", tr("Power grid Comment:"));

    //CustomerDataBrowser.qml
    //: header text customer data browser
    addTranslation(tmpTranslations, "Customer data", tr("Customer data"));
    //: button import cutomer data
    addTranslation(tmpTranslations, "Import", tr("Import"));
    //: button import cutomer data
    addTranslation(tmpTranslations, "Export", tr("Export"));
    //: label text add customer data
    addTranslation(tmpTranslations, "File name:", tr("File name:", "customerdata filename"));
    addTranslation(tmpTranslations, "Search", tr("Search", "search for customerdata files"));
    //: header import customer data popup
    addTranslation(tmpTranslations, "Import customer data", tr("Import customer data"));
    //: label import customer data device select combo
    addTranslation(tmpTranslations, "Files found from device:", tr("Files found from device:"));
    //: import customer data popup option checkbox text
    addTranslation(tmpTranslations, "Delete current files first", tr("Delete current files first"));
    //: import customer data popup option checkbox text
    addTranslation(tmpTranslations, "Overwrite current files with imported ones", tr("Overwrite current files with imported ones"));
    //: customer data delete file confirmation text
    addTranslation(tmpTranslations, "Delete <b>'%1'</b>?", tr("Delete <b>'%1'</b>?"));
    //: customer data delete file button text
    addTranslation(tmpTranslations, "Delete", tr("Delete"));
    addTranslation(tmpTranslations, "Delete transaction ?", tr("Delete transaction ?"));

    //: header text popup new customer data
    addTranslation(tmpTranslations, "Create Customer data file", tr("Create Customer data file"));

    // LoggerExport.qml
    //: header text view export data
    addTranslation(tmpTranslations, "Export stored data", tr("Export stored data"));
    //: label combobox export type (MTVisXML/SQLiteDB...)
    addTranslation(tmpTranslations, "Export type:", tr("Export type:"));
    //: entry combobox export type MTVis Part 1
    addTranslation(tmpTranslations, "MtVis XML", tr("MtVis XML"));
    //: entry combobox export type MTVis Part 2
    addTranslation(tmpTranslations, "Session:", tr("Session:"));
    //: entry combobox export type in case no sessions stored yet
    addTranslation(tmpTranslations, "MtVis XML - requires stored sessions", tr("MtVis XML - requires stored sessions"));
    //: label edit field export name
    addTranslation(tmpTranslations, "Export name:", tr("Export name:"));
    //: edit field export name: Text displayed in case MTVis export is selected and field is empty (placeholder text)
    addTranslation(tmpTranslations, "Name of export path", tr("Name of export path"));
    //: label combobox target drive (visible only if multiple sicks / partitions are mounted)
    addTranslation(tmpTranslations, "Target drive:", tr("Target drive:"));
    //: button text MTVis export type selected but no session active currently
    addTranslation(tmpTranslations, "Please select a session first...", tr("Please select a session first..."));
    //: Text set in case MTVis export failed
    addTranslation(tmpTranslations, "Export failed - drive full or removed?", tr("Export failed - drive full or removed?"));
    //: Text set in case MTVis export failed
    addTranslation(tmpTranslations, "Import failed - drive removed?", tr("Import failed - drive removed?"));
    //: Text set in case SQLite file export (copy) failed
    addTranslation(tmpTranslations, "Copy failed - drive full or removed?", tr("Copy failed - drive full or removed?"));
    //: Header text in wait export popup
    addTranslation(tmpTranslations, "Exporting customer data...", tr("Exporting customer data..."));
    //: Header text in wait import popup
    addTranslation(tmpTranslations, "Importing customer data...", tr("Importing customer data..."));
    //: Header text in wait export popup
    addTranslation(tmpTranslations, "Exporting MTVis XML...", tr("Exporting MTVis XML..."));
    //: Header text in wait export popup
    addTranslation(tmpTranslations, "Exporting database...", tr("Exporting database..."));
    //: warning prefix
    addTranslation(tmpTranslations, "Warning:", tr("Warning:"));
    //: error prefix
    addTranslation(tmpTranslations, "Error:", tr("Error:"));
    //: Transaction table: transaction type
    addTranslation(tmpTranslations, "Recording", tr("Recording"));

    // MountedDrivesCombo.qml
    //: in drive select combo for partitions without name
    addTranslation(tmpTranslations, "unnamed", tr("unnamed"));
    //: in drive select combo free memory label
    addTranslation(tmpTranslations, "free", tr("free"));

    // SerialSettings.qml
    addTranslation(tmpTranslations, "Not connected", tr("Not connected"));
    addTranslation(tmpTranslations, "Serial SCPI", tr("Serial SCPI"));
    addTranslation(tmpTranslations, "Source device", tr("Source device"));
    addTranslation(tmpTranslations, "Scanning for source device...", tr("Scanning for source device..."));
    addTranslation(tmpTranslations, "Opening SCPI serial...", tr("Opening SCPI serial..."));
    addTranslation(tmpTranslations, "Disconnect source...", tr("Disconnect source..."));
    addTranslation(tmpTranslations, "Disconnect SCPI serial...", tr("Disconnect SCPI serial..."));
    addTranslation(tmpTranslations, "No source found", tr("No source found"));
    addTranslation(tmpTranslations, "Source switch off failed", tr("Source switch off failed"));
    addTranslation(tmpTranslations, "Switch on failed", tr("Switch on failed"));
    addTranslation(tmpTranslations, "Switch off failed", tr("Switch off failed"));
    addTranslation(tmpTranslations, "Switching on %1...", tr("Switching on %1..."));
    addTranslation(tmpTranslations, "Switching off %1...", tr("Switching off %1..."));
    addTranslation(tmpTranslations, "none", tr("none"));
    addTranslation(tmpTranslations, "On", tr("On"));
    addTranslation(tmpTranslations, "symmetric", tr("symmetric"));
    addTranslation(tmpTranslations, "Off", tr("Off"));

    // SensorSettings.qml
    addTranslation(tmpTranslations, "Bluetooth:", tr("Bluetooth:"));
    addTranslation(tmpTranslations, "MAC address:", tr("MAC address:"));
    addTranslation(tmpTranslations, "Temperature [°C]:", tr("Temperature [°C]:"));
    addTranslation(tmpTranslations, "T [°C]", tr("T [°C]"));
    addTranslation(tmpTranslations, "Temperature [°F]:", tr("Temperature [°F]:"));
    addTranslation(tmpTranslations, "Humidity [%]:", tr("Humidity [%]:"));
    addTranslation(tmpTranslations, "Air pressure [hPa]:", tr("Air pressure [hPa]:"));

    addTranslation(tmpTranslations, "No USB-stick inserted", tr("No USB-stick inserted"));
    addTranslation(tmpTranslations, "Screenshot taken and saved on USB-stick", tr("Screenshot taken and saved on USB-stick"));

    //: Timezone with no region info are marked as "\<other\>"
    addTranslation(tmpTranslations, "other", tr("other"));
    addTranslation(tmpTranslations, "Time:", tr("Time:"));
    addTranslation(tmpTranslations, "Date:", tr("Date:"));
    addTranslation(tmpTranslations, "Internal timebase", tr("Internal timebase"));
    addTranslation(tmpTranslations, "Synchronize from network", tr("Synchronize from network"));
    addTranslation(tmpTranslations, "Set timezone failed!", tr("Set timezone failed!"));
    addTranslation(tmpTranslations, "NTP activate failed!", tr("NTP activate failed!"));
    addTranslation(tmpTranslations, "NTP deactivate failed!", tr("NTP deactivate failed!"));
    addTranslation(tmpTranslations, "Set date/time failed!", tr("Set date/time failed!"));

    addTranslation(tmpTranslations, "Date/Time:", tr("Date/Time:"));

    //: Measurememt mode / fixed frequency. Note too long texts ruin layout!!!
    addTranslation(tmpTranslations, "Fixed Freq.", tr("Fixed Freq."));

    // ApiConfirmationPopup.qml
    addTranslation(tmpTranslations, "API Trust requested", tr("API Trust requested"));
    addTranslation(tmpTranslations, "Name:", tr("Name:"));
    addTranslation(tmpTranslations, "Type:", tr("Type:"));
    addTranslation(tmpTranslations, "Identity:", tr("Identity:"));
    addTranslation(tmpTranslations, "Deny", tr("Deny"));
    addTranslation(tmpTranslations, "Allow", tr("Allow"));

    // ApiInfoPopup.qml
    addTranslation(tmpTranslations, "API SSL Certificate (SHA1)", tr("API SSL Certificate (SHA1)"));
    addTranslation(tmpTranslations, "No SSL Certificate available.", tr("No SSL Certificate available."));

    // TrustDeletePopup.qml
    addTranslation(tmpTranslations, "Delete this Trust", tr("Delete this Trust"));

    // TrustListPopup.qml
    addTranslation(tmpTranslations, "Trusted API Clients", tr("Trusted API Clients"));
    addTranslation(tmpTranslations, "No trusted clients yet.", tr("No trusted clients yet."));

    //: add nominal frequency
    addTranslation(tmpTranslations, "NF (Nominal Frequency)", tr("NF (Nominal Frequency)"));
    addTranslation(tmpTranslations, "NF:", tr("NF:"));

    // Keys for reportjs-vue project
    addTranslation(tmpTranslations, "Remote control", tr("Remote control"));
    addTranslation(tmpTranslations, "Download update", tr("Download update"));
    addTranslation(tmpTranslations, "ZERA SCPI documentation", tr("ZERA SCPI documentation"));
    addTranslation(tmpTranslations, "Database schema", tr("Database schema"));
    addTranslation(tmpTranslations, "Reports", tr("Reports"));
    addTranslation(tmpTranslations, "Download logfiles", tr("Download logfiles"));
    addTranslation(tmpTranslations, "Content", tr("Content"));
    addTranslation(tmpTranslations, "Display all", tr("Display all"));
    addTranslation(tmpTranslations, "Export as PDF", tr("Export as PDF"));
    addTranslation(tmpTranslations, "Back to overview", tr("Back to overview"));
    addTranslation(tmpTranslations, "Web Server for ZENUX Devices", tr("Web Server for ZENUX Devices"));
    addTranslation(tmpTranslations, "Error occurred !", tr("Error occurred !"));

    addTranslation(tmpTranslations, "Settings", tr("Settings"));
    addTranslation(tmpTranslations, "Value", tr("Value"));

    addTranslation(tmpTranslations, "Status", tr("Status"));

    addTranslation(tmpTranslations, "Measurement mode", tr("Measurement mode"));
    addTranslation(tmpTranslations, "Start time", tr("Start time"));
    addTranslation(tmpTranslations, "Stop time", tr("Stop time"));
    addTranslation(tmpTranslations, "Measurements", tr("Measurements"));
    addTranslation(tmpTranslations, "Measurement duration", tr("Measurement duration"));
    addTranslation(tmpTranslations, "Measurement time", tr("Measurement time"));
    addTranslation(tmpTranslations, "Pulse count", tr("Pulse count"));
    addTranslation(tmpTranslations, "Power", tr("Power"));
    addTranslation(tmpTranslations, "Mean value", tr("Mean value"));
    addTranslation(tmpTranslations, "Multiple measurements", tr("Multiple measurements"));
    addTranslation(tmpTranslations, "Unfinished", tr("Unfinished"));

    addTranslation(tmpTranslations, "Report all transactions in ", tr("Report all transactions in "));
    addTranslation(tmpTranslations, "Report one transaction in ", tr("Report one transaction in "));

    addTranslation(tmpTranslations, "Harmonic UI Charts", tr("Harmonic UI Charts"));
    addTranslation(tmpTranslations, "Energy charts", tr("Energy charts"));

    return tmpTranslations;
}
