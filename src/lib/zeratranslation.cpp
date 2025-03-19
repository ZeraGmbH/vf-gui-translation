#include "zeratranslation.h"
#include <QLocale>
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
    const QString dateFormat = locale.dateFormat(QLocale::ShortFormat);
    QString timeFormat = locale.timeFormat(QLocale::ShortFormat);
    // hack in seconds - short format is missing them
    if(!timeFormat.contains("ss"))
        timeFormat.replace("mm", "mm:ss");
    const QString formatStr = dateFormat + QLatin1Char(' ') + timeFormat;
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

QDateTime ZeraTranslation::getDateTimeNow()
{
    return QDateTime::currentDateTime();
}

QStringList ZeraTranslation::getLocalesModel()
{
    return m_translationFlagsModel.keys();
}

QStringList ZeraTranslation::getFlagsModel()
{
    return m_translationFlagsModel.values();
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
    QStringList ignoreList = QStringList();

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
    qInfo("Translation string table with %i entries reloaded within %lldms.", tmpTranslations.count(), elapsed.elapsed());

    for(auto iter=tmpTranslations.cbegin(); iter!=tmpTranslations.cend(); iter++)
        m_currentTranslations.insert(iter.key(), iter.value());
    qInfo("Translation strings added to current translation map within %lldms.", elapsed.elapsed());

    emit sigLanguageChanged();
    qInfo("Language change notification within %lldms.", elapsed.elapsed());
}

const QVariantHash ZeraTranslation::loadTranslationHash()
{
    //tmpTranslations.insert("something %1", tr("something %1"))...

    QVariantHash tmpTranslations;
    // common translations
    tmpTranslations.insert("AC", tr("AC", "Alternating current"));
    tmpTranslations.insert("U", tr("U", "Voltage"));
    tmpTranslations.insert("W", tr("W", "Watt unit active"));
    tmpTranslations.insert("VA", tr("VA", "Watt unit reactive"));
    tmpTranslations.insert("Var", tr("Var", "Watt unit apparent"));

    tmpTranslations.insert("4LW", tr("4LW", "4 Leiter Wirkleistung = 4 wire active power"));
    tmpTranslations.insert("3LW", tr("3LW", "3 Leiter Wirkleistung = 3 wire active power"));
    tmpTranslations.insert("2LW", tr("2LW", "2 Leiter Wirkleistung = 2 wire active power"));

    tmpTranslations.insert("4LB", tr("4LB", "4 Leiter Blindleistung = 4 wire reactive power"));
    tmpTranslations.insert("4LBK", tr("4LBK", "4 Leiter Blindleistung künstlicher Nulleiter = 4 wire reactive power artificial neutral conductor"));
    tmpTranslations.insert("3LB", tr("3LB", "3 Leiter Blindleistung = 3 wire reactive power"));
    tmpTranslations.insert("2LB", tr("2LB", "2 Leiter Blindleistung = 2 wire reactive power"));

    tmpTranslations.insert("4LS", tr("4LS", "4 Leiter Scheinleistung = 4 wire apparent power"));
    tmpTranslations.insert("4LSg", tr("4LSg", "4 Leiter Scheinleistung geometrisch = 4 wire apparent power geometric"));
    tmpTranslations.insert("2LS", tr("2LS", "2 Leiter Scheinleistung = 2 wire apparent power"));
    tmpTranslations.insert("2LSg", tr("2LSg", "2 Leiter Scheinleistung geometrisch = 2 wire apparent power geometric"));

    tmpTranslations.insert("XLW", tr("XLW", "X Leiter Wirkleistung = X wire active power"));
    tmpTranslations.insert("XLB", tr("XLB", "X Leiter Blindleistung = X wire reactive power"));
    tmpTranslations.insert("XLS", tr("XLS", "X Leiter Scheinleistung = X wire apparent power"));
    tmpTranslations.insert("XLSg", tr("XLSg", "X Leiter Scheinleistung geometrisch = X wire apparent power geometric"));

    tmpTranslations.insert("QREF", tr("QREF", "Referenz-Modus = reference mode"));

    tmpTranslations.insert("ind", tr("ind", "Power factor inductive load type"));
    tmpTranslations.insert("cap", tr("cap", "Power factor capacitive load type"));

    tmpTranslations.insert("L1", tr("L1", "measuring system 1"));
    tmpTranslations.insert("L2", tr("L2", "measuring system 2"));
    tmpTranslations.insert("L3", tr("L3", "measuring system 3"));
    tmpTranslations.insert("AUX", tr("AUX", "auxiliary measuring system"));

    tmpTranslations.insert("Phase1", tr("Phase1", "Letter or number for phase e.g euro: 1 / us: A"));
    tmpTranslations.insert("Phase2", tr("Phase2", "Letter or number for phase e.g euro: 2 / us: B"));
    tmpTranslations.insert("Phase3", tr("Phase3", "Letter or number for phase e.g euro: 3 / us: C"));

    tmpTranslations.insert("P 1", tr("P 1", "Meter test power phase 1"));
    tmpTranslations.insert("P 2", tr("P 2", "Meter test power phase 2"));
    tmpTranslations.insert("P 3", tr("P 3", "Meter test power phase 3"));
    tmpTranslations.insert("P AUX", tr("P AUX", "Meter test power phase AUX"));
    tmpTranslations.insert("P AC", tr("P AC", "Meter test power AC"));
    tmpTranslations.insert("P DC", tr("P DC", "Meter test power DC"));

    tmpTranslations.insert("REF1", tr("REF1", "reference channel 1"));
    tmpTranslations.insert("REF2", tr("REF2", "reference channel 2"));
    tmpTranslations.insert("REF3", tr("REF3", "reference channel 3"));
    tmpTranslations.insert("REF4", tr("REF4", "reference channel 4"));
    tmpTranslations.insert("REF5", tr("REF5", "reference channel 5"));
    tmpTranslations.insert("REF6", tr("REF6", "reference channel 6"));

    tmpTranslations.insert("UPN", tr("UPN","voltage pase to neutral"));
    tmpTranslations.insert("UPP", tr("UPP","voltage phase to phase"));
    tmpTranslations.insert("kU", tr("kU","harmonic distortion on voltage"));
    tmpTranslations.insert("I", tr("I","current"));
    tmpTranslations.insert("kI", tr("kI","harmonic distortion on current"));
    tmpTranslations.insert("∠U", tr("∠U","phase difference of voltage to reference channel"));
    tmpTranslations.insert("∠I", tr("∠I","phase difference of current to reference channel"));
    tmpTranslations.insert("∠UI", tr("∠UI","phase difference"));
    tmpTranslations.insert("λ", tr("λ","power factor"));
    //: needs to be short enough to fit
    tmpTranslations.insert("P", tr("P","active power"));
    //: needs to be short enough to fit
    tmpTranslations.insert("Q", tr("Q","reactive power"));
    //: needs to be short enough to fit
    tmpTranslations.insert("S", tr("S","apparent power"));
    tmpTranslations.insert("F", tr("F","frequency"));

    tmpTranslations.insert("Sb", tr("Sb", "standard burden"));
    tmpTranslations.insert("cos(β)", tr("cos(β)", "cosinus beta"));
    tmpTranslations.insert("Sn", tr("Sn", "operating burden in %, relative to the nominal burden"));
    tmpTranslations.insert("BRD1", tr("BRD1", "burden system name"));
    tmpTranslations.insert("BRD2", tr("BRD2", "burden system name"));
    tmpTranslations.insert("BRD3", tr("BRD3", "burden system name"));

    //PagePathView.qml
    //: as in "close this view"
    tmpTranslations.insert("OK", tr("OK"));
    tmpTranslations.insert("Close", tr("Close", "not open"));
    tmpTranslations.insert("Accept", tr("Accept"));
    tmpTranslations.insert("Cancel", tr("Cancel"));
    tmpTranslations.insert("Save", tr("Save"));
    //: default session
    tmpTranslations.insert("Default", tr("Default"));
    //: changing energy direction session
    tmpTranslations.insert("Changing energy direction", tr("Changing energy direction"));
    //: changing energy direction session
    tmpTranslations.insert("Reference", tr("Reference"));
    tmpTranslations.insert("3 Systems / 2 Wires", tr("3 Systems / 2 Wires"));
    tmpTranslations.insert("DC: 4*Voltage / 1*Current", tr("DC: 4*Voltage / 1*Current"));

    //RangeMenu.qml
    //: used for a yes / no configuration element
    tmpTranslations.insert("Range automatic", tr("Range automatic"));
    //: when selected, the corresponding phases are inverted
    tmpTranslations.insert("Invert", tr("Invert"));
    //: measurement channel range overload, e.g. the range is configured for 5V measurement and the measured input voltage is >5V
    tmpTranslations.insert("Overload", tr("Overload"));
    //: used for a yes / no configuration element
    tmpTranslations.insert("Range grouping", tr("Range grouping"));
    //: header text - can be multiline
    tmpTranslations.insert("Measurement modes", tr("Measurement modes"));
    //: Ratio checkbox
    tmpTranslations.insert("Ratio", tr("Ratio"));

    //Settings.qml
    //: settings specific to the GUI application
    tmpTranslations.insert("Application", tr("Application"));
    tmpTranslations.insert("Timezone:", tr("Timezone:"));
    //: used for a yes / no configuration element
    tmpTranslations.insert("Relative to fundamental", tr("Relative to fundamental"));
    tmpTranslations.insert("Show angles", tr("Show angles"));
    //: max number total decimals for displayed values
    tmpTranslations.insert("Max decimals total:", tr("Max decimals total:"));
    //: max number of decimals after the decimal separator
    tmpTranslations.insert("Max places after the decimal point:", tr("Max places after the decimal point:"));
    //: Remote web label
    tmpTranslations.insert("Web-Server:", tr("Web-Server:"));

    //: used for the selection of language via country flag
    tmpTranslations.insert("Language:", tr("Language:"));
    //: used for the selection of GUI display mode
    tmpTranslations.insert("Display:", tr("Display:"));
    //: GUI display mode fullscreen
    tmpTranslations.insert("Fullscreen", tr("Fullscreen"));
    //: GUI display mode desktop
    tmpTranslations.insert("Desktop (slow)", tr("Desktop (slow)"));
    //: settings specific to the hardware
    tmpTranslations.insert("Device", tr("Device"));
    //: settings specific to the network
    tmpTranslations.insert("Network", tr("Network"));
    //: settings specific to the Bluetooth sensors
    tmpTranslations.insert("BLE sensor", tr("BLE sensor"));
    //: measurement channel the phase locked loop uses as base
    tmpTranslations.insert("PLL channel:", tr("PLL channel:"));
    //: automatic phase locked loop channel selection
    tmpTranslations.insert("PLL channel automatic:", tr("PLL channel automatic:"));
    //: dft phase reference channel
    tmpTranslations.insert("DFT reference channel:", tr("DFT reference channel:"));
    tmpTranslations.insert("System colors:", tr("System colors:"));
    //: Settings show/hide AUX phases
    tmpTranslations.insert("Show AUX phase values", tr("Show AUX phase values"));
    //: Color themes popup adjust brightness for current
    tmpTranslations.insert("Brightness currents:", tr("Brightness currents:"));
    //: Color themes popup adjust brightness for current
    tmpTranslations.insert("Brightness black:", tr("Brightness black:"));
    tmpTranslations.insert("SCPI sequential mode:", tr("SCPI sequential mode:"));
    tmpTranslations.insert("Channel ignore limit [% of range]:", tr("Channel ignore limit [% of range]:"));

    //SettingsInterval.qml
    //: time based integration interval
    tmpTranslations.insert("Integration time interval", tr("Integration time interval"));
    //: measurement period based integration interval
    tmpTranslations.insert("Integration period interval:", tr("Integration period interval:"));
    //displayed under settings
    tmpTranslations.insert("seconds", tr("seconds", "integration time interval unit"));
    tmpTranslations.insert("periods", tr("periods", "integration period interval unit"));

    // Network settings
    //: AvailableApDialog.qml Wifi password access point
    tmpTranslations.insert("Wifi Password", tr("Wifi Password"));
    //: AvailableApDialog.qml Networks?
    tmpTranslations.insert("Networks:", tr("Networks:"));
    //: EthernetSettings.qml Header text
    tmpTranslations.insert("Ethernet Connection Settings", tr("Ethernet Connection Settings"));
    //: EthernetSettings.qml Ethernet connection name
    tmpTranslations.insert("Connection name:", tr("Connection name:"));
    //: EthernetSettings.qml IPv4 sub-header
    tmpTranslations.insert("IPv4", tr("IPv4"));
    //: connection mode DHCP/Manual
    tmpTranslations.insert("Mode:", tr("Mode:"));
    //: connection Manual fixed IP address
    tmpTranslations.insert("IP:", tr("IP:"));
    //: connection Manual fixed IP subnetmask
    tmpTranslations.insert("Subnetmask:", tr("Subnetmask:"));
    //: EthernetSettings.qml IPv6 sub-header
    tmpTranslations.insert("IPv6", tr("IPv6"));
    //: EthernetSettings.qml IPv6 error field IP
    tmpTranslations.insert("IPV6 IP", tr("IPV6 IP"));
    //: EthernetSettings.qml IPv6 error field subnetmask
    tmpTranslations.insert("IPV6 Subnetmask", tr("IPV6 Subnetmask"));
    //: EthernetSettings.qml IPv4 error field IP
    tmpTranslations.insert("IPV4 IP", tr("IPV4 IP"));
    //: EthernetSettings.qml IPv4 error field subnetmask
    tmpTranslations.insert("IPV4 Subnetmask", tr("IPV4 Subnetmask"));

    //: SmartConnect.qml Wifi password header
    tmpTranslations.insert("Wifi password", tr("Wifi password"));
    //: SmartConnect.qml Wifi password
    tmpTranslations.insert("Password:", tr("Password:"));
    //: SmartConnect.qml Wifi password
    tmpTranslations.insert("Device:", tr("Device:"));

    //: WifiSettings.qml Header text hotspot
    tmpTranslations.insert("Hotspot Settings", tr("Hotspot Settings"));
    //: WifiSettings.qml Header text wifi
    tmpTranslations.insert("Wifi Settings", tr("Wifi Settings"));
    //: WifiSettings.qml SSID field
    tmpTranslations.insert("SSID:", tr("SSID:"));
    //: WifiSettings.qml SSID select dialog header
    tmpTranslations.insert("Wifi SSID", tr("Wifi SSID"));
    //: WifiSettings.qml Mode ComboBox
    tmpTranslations.insert("Hotspot", tr("Hotspot"));
    //: EthernetSettings.qml error SSID
    tmpTranslations.insert("SSID", tr("SSID"));
    //: EthernetSettings.qml error SSID
    tmpTranslations.insert("Password", tr("Password"));
    //: EthernetSettings.qml Mode ComboBox
    tmpTranslations.insert("Automatic (DHCP)", tr("Automatic (DHCP)"));
    //: EthernetSettings.qml Mode ComboBox
    tmpTranslations.insert("Manual", tr("Manual"));
    //: SUggested default
    tmpTranslations.insert("Wifi", tr("Wifi"));
    //: Checkbox for auto-connection
    tmpTranslations.insert("Autoconnect:", tr("Autoconnect:"));

    //: ConnectionTree.qml group Ethernet
    tmpTranslations.insert("ETHERNET", tr("Ethernet"));
    //: ConnectionTree.qml group Wifi
    tmpTranslations.insert("WIFI", tr("WIFI"));
    //: ConnectionTree.qml group Hotspot
    tmpTranslations.insert("HOTSPOT", tr("Hotspot"));

    //: ConnectionInfo.qml header
    tmpTranslations.insert("Connection Information", tr("Connection Information"));
    //: ConnectionTree.qml netmask header
    tmpTranslations.insert("Netmask:", tr("Netmask:"));
    //: ConnectionTree.qml add entry
    tmpTranslations.insert("Add Ethernet...", tr("Add Ethernet..."));
    //: ConnectionTree.qml add entry
    tmpTranslations.insert("Add Hotspot...", tr("Add Hotspot..."));
    //: ConnectionTree.qml add entry
    tmpTranslations.insert("Add Wifi...", tr("Add Wifi..."));

    //MainToolBar.qml
    tmpTranslations.insert("Battery low !\nPlease charge the device before it turns down", tr("Battery low !\nPlease charge the device before it turns down"));

    //main.qml
    tmpTranslations.insert("Loading...", tr("Loading..."));
    //: progress of loading %1 of %2 objects
    tmpTranslations.insert("Loading: %1/%2", tr("Loading: %1/%2"));
    //: settings for range automatic etc.
    tmpTranslations.insert("Range", tr("Range", "measuring range"));

    //ErrorCalculatorModulePage.qml
    //: switch between time based and period based measurement
    tmpTranslations.insert("Mode:", tr("Mode:", "error calculator"));
    //: reference channel selection
    tmpTranslations.insert("Reference input:", tr("Reference input:"));
    //: device input selection (e.g. scanning head)
    tmpTranslations.insert("Device input:", tr("Device input:"));
    //: device under test constant
    tmpTranslations.insert("Constant", tr("Constant"));
    //: energy to compare
    tmpTranslations.insert("Energy:", tr("Energy:"));
    //: value based on the DUT constant
    tmpTranslations.insert("Pulse:", tr("Pulse:"));
    tmpTranslations.insert("Start", tr("Start"));
    tmpTranslations.insert("Stop", tr("Stop"));
    tmpTranslations.insert("Lower error limit:", tr("Lower error limit:"));
    tmpTranslations.insert("Upper error limit:", tr("Upper error limit:"));
    tmpTranslations.insert("continuous", tr("continuous"));
    tmpTranslations.insert("Count / Pause:", tr("Count / Pause:"));

    //ErrorRegisterModulePage.qml
    tmpTranslations.insert("Duration:", tr("Duration:"));
    tmpTranslations.insert("Start/Stop", tr("Start/Stop"));
    tmpTranslations.insert("Duration", tr("Duration"));
    tmpTranslations.insert("Start value:", tr("Start value:"));
    tmpTranslations.insert("End value:", tr("End value:"));

    //ErrorCalculatorModulePage.qml
    tmpTranslations.insert("Frequency:", tr("Frequency:"));

    //ErrorRegisterModulePage.qml
    tmpTranslations.insert("Count:", tr("Count:"));
    tmpTranslations.insert("Passed:", tr("Passed:"));
    tmpTranslations.insert("Failed:", tr("Failed:"));
    tmpTranslations.insert("Mean:", tr("Mean:"));
    tmpTranslations.insert("Range:", tr("Range:"));
    tmpTranslations.insert("Stddev. n:", tr("Stddev. n:"));
    tmpTranslations.insert("Stddev. n-1:", tr("Stddev. n-1:"));

    //FftTabPage.qml
    //: text must be short enough to fit
    tmpTranslations.insert("Amp", tr("Amp", "Amplitude of the phasor"));
    //: text must be short enough to fit
    tmpTranslations.insert("Phase", tr("Phase","Phase of the phasor"));
    //: total harmonic distortion with noise
    tmpTranslations.insert("THDN:", tr("THDN:"));
    tmpTranslations.insert("Harmonic table", tr("Harmonic table", "Tab text harmonic table"));
    tmpTranslations.insert("Harmonic chart", tr("Harmonic chart", "Tab text harmonic chart"));

    //VectorModulePage.qml
    //: ComboBox: Scale to max. range
    tmpTranslations.insert("Ranges", tr("Ranges"));
    //: ComboBox: Scale to max. value
    tmpTranslations.insert("Maximum", tr("Maximum"));

    //PowerModulePage.qml
    tmpTranslations.insert("Ext.", tr("Ext."));

    //HarmonicPowerTabPage.qml
    tmpTranslations.insert("Harmonic power table", tr("Harmonic power table", "Tab text harmonic power table"));
    tmpTranslations.insert("Harmonic power chart", tr("Harmonic power chart", "Tab text harmonic power chart"));


    //: text must be short enough to fit
    tmpTranslations.insert("UL1", tr("UL1", "channel name"));
    //: text must be short enough to fit
    tmpTranslations.insert("UL2", tr("UL2", "channel name"));
    //: text must be short enough to fit
    tmpTranslations.insert("UL3", tr("UL3", "channel name"));
    //: text must be short enough to fit
    tmpTranslations.insert("IL1", tr("IL1", "channel name"));
    //: text must be short enough to fit
    tmpTranslations.insert("IL2", tr("IL2", "channel name"));
    //: text must be short enough to fit
    tmpTranslations.insert("IL3", tr("IL3", "channel name"));
    //: text must be short enough to fit
    tmpTranslations.insert("UAUX", tr("UAUX", "channel name"));
    //: text must be short enough to fit
    tmpTranslations.insert("IAUX", tr("IAUX", "channel name"));

    //: text must be short enough to fit
    tmpTranslations.insert("P1", tr("P1", "harmonic power label active / phase 1"));
    //: text must be short enough to fit
    tmpTranslations.insert("Q1", tr("Q1", "harmonic power label reactive / phase 1"));
    //: text must be short enough to fit
    tmpTranslations.insert("S1", tr("S1", "harmonic power label aparent / phase 1"));
    //: text must be short enough to fit
    tmpTranslations.insert("P2", tr("P2", "harmonic power label active / phase 2"));
    //: text must be short enough to fit
    tmpTranslations.insert("Q2", tr("Q2", "harmonic power label reactive / phase 2"));
    //: text must be short enough to fit
    tmpTranslations.insert("S2", tr("S2", "harmonic power label aparent / phase 2"));
    //: text must be short enough to fit
    tmpTranslations.insert("P3", tr("P3", "harmonic power label active / phase 3"));
    //: text must be short enough to fit
    tmpTranslations.insert("Q3", tr("Q3", "harmonic power label reactive / phase 3"));
    //: text must be short enough to fit
    tmpTranslations.insert("S3", tr("S3", "harmonic power label aparent / phase 3"));

    //MeasurementPageModel.qml
    //: polar (amplitude and phase) phasor diagram
    tmpTranslations.insert("Vector diagram", tr("Vector diagram"));
    tmpTranslations.insert("Actual values", tr("Actual values"));
    tmpTranslations.insert("Actual values DC", tr("Actual values DC"));
    tmpTranslations.insert("Actual values & Meter tests", tr("Actual values & Meter tests"));
    tmpTranslations.insert("Oscilloscope plot", tr("Oscilloscope plot"));
    //: FFT bar diagrams or tables that show the harmonic component distribution of the measured values
    tmpTranslations.insert("Harmonics & Curves", tr("Harmonics & Curves"));
    //: measuring mode dependent power values
    tmpTranslations.insert("Power values", tr("Power values"));
    //: FFT tables that show the real and imaginary parts of the measured power values
    tmpTranslations.insert("Harmonic power values", tr("Harmonic power values"));
    //: shows the deviation of measured energy between the reference device and the device under test
    tmpTranslations.insert("Error calculator", tr("Error calculator"));
    //: shows energy comparison between the reference device and the device under test's registers/display
    tmpTranslations.insert("Comparison measurements", tr("Comparison measurements"));
    //: control one or more source devices
    tmpTranslations.insert("Source control", tr("Source control"));

    //ComparisonTabsView.qml
    //: Comparison tabs label texts
    tmpTranslations.insert("Meter test", tr("Meter test"));
    tmpTranslations.insert("Energy comparison", tr("Energy comparison"));
    tmpTranslations.insert("Energy register", tr("Energy register"));
    tmpTranslations.insert("Power register", tr("Power register"));

    tmpTranslations.insert("Burden values", tr("Burden values"));
    tmpTranslations.insert("Transformer values", tr("Transformer values"));
    //: effective values
    tmpTranslations.insert("RMS values", tr("RMS values"));

    //BurdenModulePage.qml
    tmpTranslations.insert("Voltage Burden", tr("Voltage Burden", "Tab title current burden"));
    tmpTranslations.insert("Current Burden", tr("Current Burden", "Tab title current burden"));
    tmpTranslations.insert("Nominal burden:", tr("Nominal burden:"));
    tmpTranslations.insert("Nominal range:", tr("Nominal range:"));
    tmpTranslations.insert("Wire crosssection:", tr("Wire crosssection:"));
    tmpTranslations.insert("Wire length:", tr("Wire length:"));

    //ReferencePageModel.qml
    tmpTranslations.insert("DC reference values", tr("DC reference values"));
    //QuartzModulePage.qml
    tmpTranslations.insert("Quartz reference measurement", tr("Quartz reference measurement"));

    //CEDPageModel.qml
    tmpTranslations.insert("CED power values", tr("CED power values"));

    //HarmonicPowerTabPage.qml
    tmpTranslations.insert("Measuring modes:", tr("Measuring modes:", "label for measuring mode selectors"));

    //TransformerModulePage.qml
    tmpTranslations.insert("TR1", tr("TR1", "transformer system 1"));
    tmpTranslations.insert("X-Prim:", tr("X-Prim:"));
    tmpTranslations.insert("X-Sec:", tr("X-Sec:"));
    tmpTranslations.insert("Ms-Prim:", tr("Ms-Prim:"));
    tmpTranslations.insert("Ms-Sec:", tr("Ms-Sec:"));
    tmpTranslations.insert("Mp-Prim:", tr("Mp-Prim:"));
    tmpTranslations.insert("Mp-Sec:", tr("Mp-Sec:"));
    tmpTranslations.insert("X-Ratio", tr("X-Ratio"));
    tmpTranslations.insert("N-Sec", tr("N-Sec"));
    tmpTranslations.insert("X-Prim", tr("X-Prim"));
    tmpTranslations.insert("X-Sec", tr("X-Sec"));
    tmpTranslations.insert("crad", tr("crad", "centiradian"));
    tmpTranslations.insert("arcmin", tr("arcmin", "arcminute"));

    // SourceModulePage.qml
    tmpTranslations.insert("Actual", tr("Actual", "Source/Vector view indicator label: actual values mode"));
    tmpTranslations.insert("Target", tr("Target", "Source/Vector view indicator label: target values mode"));

    //DeviceInformation.qml
    tmpTranslations.insert("Device info", tr("Device info"));
    tmpTranslations.insert("Device log", tr("Device log"));
    tmpTranslations.insert("License information", tr("License information"));
    tmpTranslations.insert("Save logfile to USB", tr("Save logfile to USB"));
    tmpTranslations.insert("Serial number", tr("Serial number"));
    tmpTranslations.insert("Operating system version", tr("Operating system version"));
    tmpTranslations.insert("Emob PCB version", tr("Emob PCB version"));
    tmpTranslations.insert("Accu PCB version", tr("Accu PCB version"));
    tmpTranslations.insert("Relay PCB version", tr("Relay PCB version"));
    tmpTranslations.insert("System PCB version", tr("System PCB version"));
    tmpTranslations.insert("PCB server version", tr("PCB server version"));
    tmpTranslations.insert("DSP server version", tr("DSP server version"));
    tmpTranslations.insert("DSP firmware version", tr("DSP firmware version"));
    tmpTranslations.insert("FPGA firmware version", tr("FPGA firmware version"));
    tmpTranslations.insert("System controller version", tr("System controller version"));
    tmpTranslations.insert("Relay controller version", tr("Relay controller version"));
    tmpTranslations.insert("Emob controller version", tr("Emob controller version"));
    tmpTranslations.insert("Accu controller version", tr("Accu controller version"));
    tmpTranslations.insert("Adjustment status", tr("Adjustment status"));
    tmpTranslations.insert("Not adjusted", tr("Not adjusted"));
    tmpTranslations.insert("Wrong version", tr("Wrong version"));
    tmpTranslations.insert("Wrong serial number", tr("Wrong serial number"));
    tmpTranslations.insert("CPU-board number", tr("CPU-board number"));
    tmpTranslations.insert("CPU-board assembly", tr("CPU-board assembly"));
    tmpTranslations.insert("CPU-board date", tr("CPU-board date"));
    tmpTranslations.insert("Saving logs and dumps to external drive...", tr("Saving logs and dumps to external drive..."));
    tmpTranslations.insert("Could not save logs and dumps", tr("Could not save logs and dumps"));
    tmpTranslations.insert("Starting update...", tr("Starting update..."));
    tmpTranslations.insert("Could not update. Please check if necessary files are available.", tr("Could not update. Please check if necessary files are available."));
    tmpTranslations.insert("Update failed. Please save logs and send them to service@zera.de.", tr("Update failed. Please save logs and send them to service@zera.de."));

    //LoggerSettings.qml
    tmpTranslations.insert("Database Logging", tr("Database Logging"));
    tmpTranslations.insert("Logging enabled:", tr("Logging enabled:"));
    tmpTranslations.insert("DB location:", tr("DB location:"));
    //: describes the duration of the recording
    tmpTranslations.insert("Logging Duration [hh:mm:ss]:", tr("Logging Duration [hh:mm:ss]:"));
    tmpTranslations.insert("Logger status:", tr("Logger status:"));
    //: describes the ongoing task of recording data into a database
    tmpTranslations.insert("Logging data", tr("Logging data"));
    tmpTranslations.insert("Logging disabled", tr("Logging disabled"));
    tmpTranslations.insert("No database selected", tr("No database selected"));
    tmpTranslations.insert("Database loaded", tr("Database loaded"));
    tmpTranslations.insert("Database error", tr("Database error"));
    tmpTranslations.insert("Manage customer data:", tr("Manage customer data:"));
    tmpTranslations.insert("Snapshot", tr("Snapshot"));
    //: when the system disabled the customer data management, the brackets are for visual distinction from other text
    tmpTranslations.insert("[customer data is not available]", tr("[customer data is not available]"));
    //: when the customer id is empty, the brackets are for visual distinction from other text
    tmpTranslations.insert("[customer id is not set]", tr("[customer id is not set]"));
    //: when the customer number is empty, the brackets are for visual distinction from other text
    tmpTranslations.insert("[customer number is not set]", tr("[customer number is not set]"));
    //: placeholder text for the database filename
    tmpTranslations.insert("filename", tr("filename"));
    //: popup create database header
    tmpTranslations.insert("Create database", tr("Create database"));
    tmpTranslations.insert("Show", tr("Show"));
    tmpTranslations.insert("Basic", tr("Basic"));

    //LoggerDbSearchDialog.qml
    //:Select db location: internal storage
    tmpTranslations.insert("internal", tr("internal"));
    //:Select db location: external storage e.g memory stick
    tmpTranslations.insert("external", tr("external"));
    //:Delete database confirmation text - %1 is replaced by database filename
    tmpTranslations.insert("Delete database <b>'%1'</b>?", tr("Delete database <b>'%1'</b>?"));

    //LoggerSessionNameSelector.qml
    //: displayed in logger session name popup, visible when the user presses start or snapshot in the logger
    //: the session name is a database field that the user can use to search / filter different sessions
    tmpTranslations.insert("Select session name", tr("Select session name"));
    //: label for current session name
    tmpTranslations.insert("Current name:", tr("Current name:"));
    //: header for list of existing session names. Operator can select one of them to make it current session name
    tmpTranslations.insert("Select existing:", tr("Select existing:"));
    tmpTranslations.insert("Preview", tr("Preview"));
    //: delete session confirmation popup header
    tmpTranslations.insert("Confirmation", tr("Confirmation"));
    //: delete session confirmation popup header
    tmpTranslations.insert("Delete session <b>'%1'</b>?", tr("Delete session <b>'%1'</b>?"));

    // LoggerSessionNew.qml
    //: header view 'Add new session'
    tmpTranslations.insert("Add new session", tr("Add new session"));
    //: add new session view: label session name
    tmpTranslations.insert("Session name:", tr("Session name:"));
    //: add new session view: label list cutomer data to select entry from
    tmpTranslations.insert("Select customer data:", tr("Select customer data:"));
    //: add new session view: list entry to select no customer data
    tmpTranslations.insert("-- no customer --", tr("-- no customer --"));
    tmpTranslations.insert("no customer", tr("no customer"));

    // LoggerSessionNameWithMacrosPopup.qml
    //: default prefix for auto session name
    tmpTranslations.insert("Session ", tr("Session "));

    //LoggerCustomDataSelector.qml
    //: logger custom data selector view header
    tmpTranslations.insert("Select custom data contents", tr("Select custom data contents"));
    //: button to select content-set actual values
    tmpTranslations.insert("ZeraActualValues", tr("Actual values"));
    //: button to select content-set harmonic values
    tmpTranslations.insert("ZeraHarmonics", tr("Harmonic values"));
    //: button to select content-set sample values
    tmpTranslations.insert("ZeraCurves", tr("Sample values"));
    tmpTranslations.insert("ZeraComparison", tr("Comparison measurements"));
    //: button to select content-set burden values
    tmpTranslations.insert("ZeraBurden", tr("Burden values"));
    //: button to select content-set transformer values
    tmpTranslations.insert("ZeraTransformer", tr("Transformer values"));
    //: button to select content-set dc-reference values
    tmpTranslations.insert("ZeraDCReference", tr("DC reference values"));
    //: button to select content-set dc-reference values
    tmpTranslations.insert("ZeraQuartzReference", tr("Quartz reference values"));
    //: button to select content-set dc-reference values
    tmpTranslations.insert("ZeraAll", tr("All"));
    //: logger custom data selector get back to menu button
    tmpTranslations.insert("Back", tr("Back"));

    //LoggerMenu.qml
    //: displayed when user presses logger button
    //: text displayed in session name menu entry when no session was set yet
    tmpTranslations.insert("-- no session --", tr("-- no session --"));
    //: take snapshot menu entry
    tmpTranslations.insert("Take snapshot", tr("Take snapshot"));
    //: save snapshot error message
    tmpTranslations.insert("Could not store snapshot in database. Please save logs and send them to service@zera.de.", tr("Could not store snapshot in database. Please save logs and send them to service@zera.de."));
    //: start logging menu entry
    tmpTranslations.insert("Start logging", tr("Start logging"));
    //: stop logging menu entry
    tmpTranslations.insert("Stop logging", tr("Stop logging"));
    //: menu entry to open logger settings
    tmpTranslations.insert("Settings...", tr("Settings..."));
    //: Logger menu entry export
    tmpTranslations.insert("Export...", tr("Export..."));
    //: menu radio button to select content-set actual values
    tmpTranslations.insert("MenuZeraActualValues", tr("Actual values only"));
    //: menu radio button to select content-set harmonic values
    tmpTranslations.insert("MenuZeraHarmonics", tr("Harmonic values only"));
    //: menu radio button to select content-set sample values
    tmpTranslations.insert("MenuZeraCurves", tr("Sample values only"));
    //: menu radio button to select content-set comparison measurement values
    tmpTranslations.insert("MenuZeraComparison", tr("Comparison measurements only"));
    //: menu radio button to select content-set burden values
    tmpTranslations.insert("MenuZeraBurden", tr("Burden values only"));
    //: menu radio button to select content-set transformer values
    tmpTranslations.insert("MenuZeraTransformer", tr("Transformer values only"));
    //: menu radio button to select content-set dc-reference values
    tmpTranslations.insert("MenuZeraDCReference", tr("DC reference values only"));
    //: menu radio button to select content-set dc-reference values
    tmpTranslations.insert("MenuZeraQuartzReference", tr("Quartz reference values only"));
    //: menu radio button to select all values
    tmpTranslations.insert("MenuZeraAll", tr("All data"));
    //: menu radio button to select custom content-sets values
    tmpTranslations.insert("MenuZeraCustom", tr("Custom data"));

    //CustomerDataEntry.qml
    tmpTranslations.insert("Customer", tr("Customer"));
    //: power meter, not distance
    tmpTranslations.insert("Meter information", tr("Meter information"));
    tmpTranslations.insert("Location", tr("Location"));
    tmpTranslations.insert("Power grid", tr("Power grid"));
    tmpTranslations.insert("PAR_DatasetIdentifier", tr("Data Identifier:"));
    tmpTranslations.insert("PAR_DatasetComment", tr("Data Comment:"));
    tmpTranslations.insert("PAR_CustomerNumber", tr("Customer number:"));
    tmpTranslations.insert("PAR_CustomerFirstName", tr("Customer First name:"));
    tmpTranslations.insert("PAR_CustomerLastName", tr("Customer Last name:"));
    tmpTranslations.insert("PAR_CustomerCountry", tr("Customer Country:"));
    tmpTranslations.insert("PAR_CustomerCity", tr("Customer City:"));
    tmpTranslations.insert("PAR_CustomerPostalCode", tr("Customer ZIP code:", "Postal code"));
    tmpTranslations.insert("PAR_CustomerStreet", tr("Customer Street:"));
    tmpTranslations.insert("PAR_CustomerComment", tr("Customer Comment:"));
    tmpTranslations.insert("PAR_LocationNumber", tr("Location Identifier:"));
    tmpTranslations.insert("PAR_LocationFirstName", tr("Location First name:"));
    tmpTranslations.insert("PAR_LocationLastName", tr("Location Last name:"));
    tmpTranslations.insert("PAR_LocationCountry", tr("LocationCountry:"));
    tmpTranslations.insert("PAR_LocationCity", tr("Location City:"));
    tmpTranslations.insert("PAR_LocationPostalCode", tr("Location ZIP code:", "Postal code"));
    tmpTranslations.insert("PAR_LocationStreet", tr("Location Street:"));
    tmpTranslations.insert("PAR_LocationComment", tr("Location Comment:"));
    tmpTranslations.insert("PAR_MeterFactoryNumber", tr("Meter Factory number:"));
    tmpTranslations.insert("PAR_MeterManufacturer", tr("Meter Manufacturer:"));
    tmpTranslations.insert("PAR_MeterOwner", tr("Meter Owner:"));
    tmpTranslations.insert("PAR_MeterComment", tr("Meter Comment:"));
    tmpTranslations.insert("PAR_PowerGridOperator", tr("Power grid Operator:"));
    tmpTranslations.insert("PAR_PowerGridSupplier", tr("Power grid Supplier:"));
    tmpTranslations.insert("PAR_PowerGridComment", tr("Power grid Comment:"));

    //CustomerDataBrowser.qml
    //: header text customer data browser
    tmpTranslations.insert("Customer data", tr("Customer data"));
    //: button import cutomer data
    tmpTranslations.insert("Import", tr("Import"));
    //: button import cutomer data
    tmpTranslations.insert("Export", tr("Export"));
    //: label text add customer data
    tmpTranslations.insert("File name:", tr("File name:", "customerdata filename"));
    tmpTranslations.insert("Search", tr("Search", "search for customerdata files"));
    //: header import customer data popup
    tmpTranslations.insert("Import customer data", tr("Import customer data"));
    //: label import customer data device select combo
    tmpTranslations.insert("Files found from device:", tr("Files found from device:"));
    //: import customer data popup option checkbox text
    tmpTranslations.insert("Delete current files first", tr("Delete current files first"));
    //: import customer data popup option checkbox text
    tmpTranslations.insert("Overwrite current files with imported ones", tr("Overwrite current files with imported ones"));
    //: customer data delete file confirmation popup header
    tmpTranslations.insert("Confirmation", tr("Confirmation"));
    //: customer data delete file confirmation text
    tmpTranslations.insert("Delete <b>'%1'</b>?", tr("Delete <b>'%1'</b>?"));
    //: customer data delete file button text
    tmpTranslations.insert("Delete", tr("Delete"));
    tmpTranslations.insert("Delete transaction ?", tr("Delete transaction ?"));

    //: header text popup new customer data
    tmpTranslations.insert("Create Customer data file", tr("Create Customer data file"));

    // LoggerExport.qml
    //: header text view export data
    tmpTranslations.insert("Export stored data", tr("Export stored data"));
    //: label combobox export type (MTVisXML/SQLiteDB...)
    tmpTranslations.insert("Export type:", tr("Export type:"));
    //: entry combobox export type MTVis Part 1
    tmpTranslations.insert("MtVis XML", tr("MtVis XML"));
    //: entry combobox export type MTVis Part 2
    tmpTranslations.insert("Session:", tr("Session:"));
    //: entry combobox export type in case no sessions stored yet
    tmpTranslations.insert("MtVis XML - requires stored sessions", tr("MtVis XML - requires stored sessions"));
    //: label edit field export name
    tmpTranslations.insert("Export name:", tr("Export name:"));
    //: edit field export name: Text displayed in case MTVis export is selected and field is empty (placeholder text)
    tmpTranslations.insert("Name of export path", tr("Name of export path"));
    //: label combobox target drive (visible only if multiple sicks / partitions are mounted)
    tmpTranslations.insert("Target drive:", tr("Target drive:"));
    //: button text MTVis export type selected but no session active currently
    tmpTranslations.insert("Please select a session first...", tr("Please select a session first..."));
    //: Text set in case MTVis export failed
    tmpTranslations.insert("Export failed - drive full or removed?", tr("Export failed - drive full or removed?"));
    //: Text set in case MTVis export failed
    tmpTranslations.insert("Import failed - drive removed?", tr("Import failed - drive removed?"));
    //: Text set in case SQLite file export (copy) failed
    tmpTranslations.insert("Copy failed - drive full or removed?", tr("Copy failed - drive full or removed?"));
    //: Header text in wait export popup
    tmpTranslations.insert("Exporting customer data...", tr("Exporting customer data..."));
    //: Header text in wait import popup
    tmpTranslations.insert("Importing customer data...", tr("Importing customer data..."));
    //: Header text in wait export popup
    tmpTranslations.insert("Exporting MTVis XML...", tr("Exporting MTVis XML..."));
    //: Header text in wait export popup
    tmpTranslations.insert("Exporting database...", tr("Exporting database..."));
    //: warning prefix
    tmpTranslations.insert("Warning:", tr("Warning:"));
    //: error prefix
    tmpTranslations.insert("Error:", tr("Error:"));
    //: Transaction table: transaction type
    tmpTranslations.insert("Recording", tr("Recording"));

    // MountedDrivesCombo.qml
    //: in drive select combo for partitions without name
    tmpTranslations.insert("unnamed", tr("unnamed"));
    //: in drive select combo free memory label
    tmpTranslations.insert("free", tr("free"));

    // SerialSettings.qml
    tmpTranslations.insert("Not connected", tr("Not connected"));
    tmpTranslations.insert("Serial SCPI", tr("Serial SCPI"));
    tmpTranslations.insert("Serial SCPI", tr("Serial SCPI"));
    tmpTranslations.insert("Source device", tr("Source device"));
    tmpTranslations.insert("Scanning for source device...", tr("Scanning for source device..."));
    tmpTranslations.insert("Opening SCPI serial...", tr("Opening SCPI serial..."));
    tmpTranslations.insert("Disconnect source...", tr("Disconnect source..."));
    tmpTranslations.insert("Disconnect SCPI serial...", tr("Disconnect SCPI serial..."));
    tmpTranslations.insert("No source found", tr("No source found"));
    tmpTranslations.insert("Source switch off failed", tr("Source switch off failed"));
    tmpTranslations.insert("Switch on failed", tr("Switch on failed"));
    tmpTranslations.insert("Switch off failed", tr("Switch off failed"));
    tmpTranslations.insert("Switching on %1...", tr("Switching on %1..."));
    tmpTranslations.insert("Switching off %1...", tr("Switching off %1..."));
    tmpTranslations.insert("none", tr("none"));
    tmpTranslations.insert("On", tr("On"));
    tmpTranslations.insert("symmetric", tr("symmetric"));
    tmpTranslations.insert("Off", tr("Off"));
    tmpTranslations.insert("Frequency:", tr("Frequency:"));

    // SensorSettings.qml
    tmpTranslations.insert("Bluetooth:", tr("Bluetooth:"));
    tmpTranslations.insert("MAC address:", tr("MAC address:"));
    tmpTranslations.insert("Temperature [°C]:", tr("Temperature [°C]:"));
    tmpTranslations.insert("T [°C]", tr("T [°C]"));
    tmpTranslations.insert("Temperature [°F]:", tr("Temperature [°F]:"));
    tmpTranslations.insert("Humidity [%]:", tr("Humidity [%]:"));
    tmpTranslations.insert("Air pressure [hPa]:", tr("Air pressure [hPa]:"));

    tmpTranslations.insert("No USB-stick inserted", tr("No USB-stick inserted"));
    tmpTranslations.insert("Screenshot taken and saved on USB-stick", tr("Screenshot taken and saved on USB-stick"));

    //: Timezone with no region info are marked as "\<other\>"
    tmpTranslations.insert("other", tr("other"));
    //: Measurememt mode / fixed frequency. Note too long texts ruin layout!!!
    tmpTranslations.insert("Fixed Freq.", tr("Fixed Freq."));


    return tmpTranslations;
}
