#include "zeratranslation.h"
#include <QLocale>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QCoreApplication>
#include <QDebug>

ZeraTranslation *ZeraTranslation::s_instance = nullptr;
QString ZeraTranslation::m_initialLanguage = "en_GB";

ZeraTranslation::ZeraTranslation(QObject *parent) : QQmlPropertyMap(this, parent)
{
    setupTranslationFiles();
    changeLanguage(m_initialLanguage);
}

ZeraTranslation *ZeraTranslation::getInstance()
{
    if(s_instance == nullptr)
        s_instance = new ZeraTranslation();
    return  s_instance;
}

QObject *ZeraTranslation::getStaticInstance(QQmlEngine *t_engine, QJSEngine *t_scriptEngine)
{
  Q_UNUSED(t_engine)
  Q_UNUSED(t_scriptEngine)

  return getInstance();
}

void ZeraTranslation::changeLanguage(const QString &language)
{
    if(m_currentLanguage != language) {
        m_currentLanguage = language;
        QLocale locale = QLocale(m_currentLanguage);
        QString languageName = QLocale::languageToString(locale.language());
        if(m_translationFilesModel.contains(language) || language == "C") {
            QCoreApplication::instance()->removeTranslator(&m_translator);
            const QString filename = m_translationFilesModel.value(language);
            if(m_translator.load(filename)) {
                QCoreApplication::instance()->installTranslator(&m_translator);
                qDebug() << "Current Language changed to" << languageName << locale << language;
                reloadStringTable();
            }
            else {
                if(language != "C")
                    qWarning() << "Language not found:" << language << filename.arg(language);
                reloadStringTable();
            }
        }
        else if(language != "C") {
            qWarning() << "Language not found for locale:" << language;
        }
    }
}

void ZeraTranslation::setInitialLanguage(const QString &language)
{
    m_initialLanguage = language;
}

QString ZeraTranslation::getCurrentLanguage()
{
    return m_currentLanguage;
}

QVariant ZeraTranslation::TrValue(const QString &key)
{
    if(!contains(key)) {
        qWarning("Translation not found for '%s'", qPrintable(key));
        insert(key, key);
    }
    return value(key);
}

void ZeraTranslation::setupTranslationFiles()
{
    QDir searchDir(QString(ZERA_TRANSLATION_PATH));
    if(searchDir.exists() && searchDir.isReadable()) {
        const auto qmList = searchDir.entryInfoList({"*.qm"}, QDir::Files);
        for(const QFileInfo &qmFileInfo : qmList) {
            const QString localeName = qmFileInfo.fileName().replace("zera-gui_","").replace(".qm","");
            if(m_translationFilesModel.contains(localeName) == false) {
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
    //export available languages to qml
    insert("TRANSLATION_LOCALES", QVariant::fromValue<QStringList>(m_translationFlagsModel.keys()));
    insert("TRANSLATION_FLAGS", QVariant::fromValue<QStringList>(m_translationFlagsModel.values()));
}

void ZeraTranslation::reloadStringTable()
{
    //insert("something %1", tr("something %1"))...

    // common translations
    insert("AC", tr("AC", "Alternating current"));
    insert("U", tr("U", "Voltage"));
    insert("W", tr("W", "Watt unit active"));
    insert("VA", tr("VA", "Watt unit reactive"));
    insert("Var", tr("Var", "Watt unit apparent"));

    insert("4LW", tr("4LW", "4 Leiter Wirkleistung = 4 wire active power"));
    insert("3LW", tr("3LW", "3 Leiter Wirkleistung = 3 wire active power"));
    insert("2LW", tr("2LW", "2 Leiter Wirkleistung = 2 wire active power"));

    insert("4LB", tr("4LB", "4 Leiter Blindleistung = 4 wire reactive power"));
    insert("4LBK", tr("4LBK", "4 Leiter Blindleistung künstlicher Nulleiter = 4 wire reactive power artificial neutral conductor"));
    insert("3LB", tr("3LB", "3 Leiter Blindleistung = 3 wire reactive power"));
    insert("2LB", tr("2LB", "2 Leiter Blindleistung = 2 wire reactive power"));

    insert("4LS", tr("4LS", "4 Leiter Scheinleistung = 4 wire apparent power"));
    insert("4LSg", tr("4LSg", "4 Leiter Scheinleistung geometrisch = 4 wire apparent power geometric"));
    insert("2LS", tr("2LS", "2 Leiter Scheinleistung = 2 wire apparent power"));
    insert("2LSg", tr("2LSg", "2 Leiter Scheinleistung geometrisch = 2 wire apparent power geometric"));

    insert("XLW", tr("XLW", "X Leiter Wirkleistung = X wire active power"));
    insert("XLB", tr("XLB", "X Leiter Blindleistung = X wire reactive power"));
    insert("XLS", tr("XLS", "X Leiter Scheinleistung = X wire apparent power"));
    insert("XLSg", tr("XLSg", "X Leiter Scheinleistung geometrisch = X wire apparent power geometric"));

    insert("QREF", tr("QREF", "Referenz-Modus = reference mode"));

    insert("L1", tr("L1", "measuring system 1"));
    insert("L2", tr("L2", "measuring system 2"));
    insert("L3", tr("L3", "measuring system 3"));
    insert("AUX", tr("AUX", "auxiliary measuring system"));

    insert("Phase1", tr("Phase1", "Letter or number for phase e.g euro: 1 / us: A"));
    insert("Phase2", tr("Phase2", "Letter or number for phase e.g euro: 2 / us: B"));
    insert("Phase3", tr("Phase3", "Letter or number for phase e.g euro: 3 / us: C"));

    insert("P 1", tr("P 1", "Meter test power phase 1"));
    insert("P 2", tr("P 2", "Meter test power phase 2"));
    insert("P 3", tr("P 3", "Meter test power phase 3"));
    insert("P AUX", tr("P AUX", "Meter test power phase AUX"));
    insert("P AC", tr("P AC", "Meter test power AC"));
    insert("P DC", tr("P DC", "Meter test power DC"));

    insert("REF1", tr("REF1", "reference channel 1"));
    insert("REF2", tr("REF2", "reference channel 2"));
    insert("REF3", tr("REF3", "reference channel 3"));
    insert("REF4", tr("REF4", "reference channel 4"));
    insert("REF5", tr("REF5", "reference channel 5"));
    insert("REF6", tr("REF6", "reference channel 6"));

    insert("UPN", tr("UPN","voltage pase to neutral"));
    insert("UPP", tr("UPP","voltage phase to phase"));
    insert("kU", tr("kU","harmonic distortion on voltage"));
    insert("I", tr("I","current"));
    insert("kI", tr("kI","harmonic distortion on current"));
    insert("∠U", tr("∠U","phase difference of voltage to reference channel"));
    insert("∠I", tr("∠I","phase difference of current to reference channel"));
    insert("∠UI", tr("∠UI","phase difference"));
    insert("λ", tr("λ","power factor"));
    //: needs to be short enough to fit
    insert("P", tr("P","active power"));
    //: needs to be short enough to fit
    insert("Q", tr("Q","reactive power"));
    //: needs to be short enough to fit
    insert("S", tr("S","apparent power"));
    insert("F", tr("F","frequency"));

    insert("Sb", tr("Sb", "standard burden"));
    insert("cos(β)", tr("cos(β)", "cosinus beta"));
    insert("Sn", tr("Sn", "operating burden in %, relative to the nominal burden"));
    insert("BRD1", tr("BRD1", "burden system name"));
    insert("BRD2", tr("BRD2", "burden system name"));
    insert("BRD3", tr("BRD3", "burden system name"));

    //PagePathView.qml
    //: as in "close this view"
    insert("OK", tr("OK"));
    insert("Close", tr("Close", "not open"));
    insert("Accept", tr("Accept"));
    insert("Cancel", tr("Cancel"));
    insert("Save", tr("Save"));
    //: default session
    insert("Default", tr("Default"));
    //: changing energy direction session
    insert("Changing energy direction", tr("Changing energy direction"));
    //: changing energy direction session
    insert("Reference", tr("Reference"));
    insert("3 Systems / 2 Wires", tr("3 Systems / 2 Wires"));
    insert("DC: 4*Voltage / 1*Current", tr("DC: 4*Voltage / 1*Current"));

    //RangeMenu.qml
    //: used for a yes / no configuration element
    insert("Range automatic", tr("Range automatic"));
    //: measurement channel range overload, e.g. the range is configured for 5V measurement and the measured input voltage is >5V
    insert("Overload", tr("Overload"));
    //: used for a yes / no configuration element
    insert("Range grouping", tr("Range grouping"));
    //: header text - can be multiline
    insert("Measurement modes", tr("Measurement modes"));
    //: Ratio checkbox
    insert("Ratio", tr("Ratio"));

    //Settings.qml
    //: settings specific to the GUI application
    insert("Application Settings", tr("Application Settings"));
    //: used for a yes / no configuration element
    insert("Display harmonic tables relative to the fundamental oscillation:", tr("Display harmonic tables relative to the fundamental oscillation:"));
    //: max number total decimals for displayed values
    insert("Max decimals total:", tr("Max decimals total:"));
    //: max number of decimals after the decimal separator
    insert("Max places after the decimal point:", tr("Max places after the decimal point:"));
    //: WebGL IPs 'or'separator
    insert("or", tr("or"));
    //: WebGL inactive label
    insert("Remote web (experimental):", tr("Remote web (experimental):"));
    //: WebGL active label
    insert("Browser addresses:", tr("Browser addresses:"));

    //: used for the selection of language via country flag
    insert("Language:", tr("Language:"));
    //: settings specific to the hardware
    insert("Device settings", tr("Device settings"));
    //: settings specific to the network
    insert("Network settings", tr("Network settings"));
    //: settings specific to the Bluetooth sensors
    insert("BLE sensor settings", tr("BLE sensor settings"));
    //: measurement channel the phase locked loop uses as base
    insert("PLL channel:", tr("PLL channel:"));
    //: automatic phase locked loop channel selection
    insert("PLL channel automatic:", tr("PLL channel automatic:"));
    //: dft phase reference channel
    insert("DFT reference channel:", tr("DFT reference channel:"));
    //: System = measuring system
    insert("System colors:", tr("System colors:"));
    insert("Frequency input/output configuration:", tr("Frequency input/output configuration:"));
    //: Settings show/hide AUX phases
    insert("Show AUX phase values:", tr("Show AUX phase values:"));
    //: Color themes popup adjust brightness for current
    insert("Brightness currents:", tr("Brightness currents:"));
    //: Color themes popup adjust brightness for current
    insert("Brightness black:", tr("Brightness black:"));
    insert("SCPI sequential mode:", tr("SCPI sequential mode:"));


    //: Displayed in Frequency input/output configuration
    insert("Nominal frequency:", tr("Nominal frequency:"));
    //: Displayed in Frequency input/output configuration
    insert("Frequency output constant:", tr("Frequency output constant:"));

    //SettingsInterval.qml
    //: time based integration interval
    insert("Integration time interval", tr("Integration time interval"));
    //: measurement period based integration interval
    insert("Integration period interval:", tr("Integration period interval:"));
    //displayed under settings
    insert("seconds", tr("seconds", "integration time interval unit"));
    insert("periods", tr("periods", "integration period interval unit"));

    // Network settings
    //: available devices AvailableDevDialog.qml
    insert("Device Binding", tr("Device Binding"));
    //: list of devices header
    insert("Devices:", tr("Devices:"));
    //: AvailableApDialog.qml Wifi password access point
    insert("Wifi Password", tr("Wifi Password"));
    //: AvailableApDialog.qml Networks?
    insert("Networks:", tr("Networks:"));
    //: EthernetSettings.qml Header text
    insert("Ethernet Connection Settings", tr("Ethernet Connection Settings"));
    //: EthernetSettings.qml Ethernet connection name
    insert("Connection name:", tr("Connection name:"));
    //: EthernetSettings.qml IPv4 sub-header
    insert("IPv4", tr("IPv4"));
    //: connection mode DHCP/Manual
    insert("Mode:", tr("Mode:"));
    //: connection Manual fixed IP address
    insert("IP:", tr("IP:"));
    //: connection Manual fixed IP subnetmask
    insert("Subnetmask:", tr("Subnetmask:"));
    //: EthernetSettings.qml IPv6 sub-header
    insert("IPv6", tr("IPv6"));
    //: EthernetSettings.qml IPv6 error field IP
    insert("IPV6 IP", tr("IPV6 IP"));
    //: EthernetSettings.qml IPv6 error field subnetmask
    insert("IPV6 Subnetmask", tr("IPV6 Subnetmask"));
    //: EthernetSettings.qml IPv4 error field IP
    insert("IPV4 IP", tr("IPV4 IP"));
    //: EthernetSettings.qml IPv4 error field subnetmask
    insert("IPV4 Subnetmask", tr("IPV4 Subnetmask"));
    //: EthernetSettings.qml error header
    insert("Network settings", tr("Network settings"));
    //: EthernetSettings.qml error text intro
    insert("invalid settings in field: ", tr("invalid settings in field: "));

    //: DeviceDialog.qml header
    insert("Select device", tr("Select device"));

    //: SmartConnect.qml Wifi password header
    insert("Wifi password", tr("Wifi password"));
    //: SmartConnect.qml Wifi password
    insert("Password:", tr("Password:"));
    //: SmartConnect.qml Wifi password
    insert("Device:", tr("Device:"));

    //: WifiSettings.qml Header text hotspot
    insert("Hotspot Settings", tr("Hotspot Settings"));
    //: WifiSettings.qml Header text wifi
    insert("Wifi Settings", tr("Wifi Settings"));
    //: WifiSettings.qml SSID field
    insert("SSID:", tr("SSID:"));
    //: WifiSettings.qml SSID select dialog header
    insert("Wifi SSID", tr("Wifi SSID"));
    //: WifiSettings.qml Mode ComboBox
    insert("Client", tr("Client"));
    //: WifiSettings.qml Mode ComboBox
    insert("Hotspot", tr("Hotspot"));
    //: EthernetSettings.qml error SSID
    insert("SSID", tr("SSID"));
    //: EthernetSettings.qml error SSID
    insert("Password", tr("Password"));
    //: EthernetSettings.qml Mode ComboBox
    insert("Automatic (DHCP)", tr("Automatic (DHCP)"));
    //: EthernetSettings.qml Mode ComboBox
    insert("Manual", tr("Manual"));
    //: SUggested default
    insert("Wifi", tr("Wifi"));
    //: Checkbox for auto-connection
    insert("Autoconnect:", tr("Autoconnect:"));

    //: ConnectionTree.qml group Ethernet
    insert("ETHERNET", tr("Ethernet"));
    //: ConnectionTree.qml group Ethernet
    insert("WIFI", tr("WIFI"));
    //: ConnectionTree.qml group Ethernet
    insert("HOTSPOT", tr("Hotspot"));
    //: ConnectionTree.qml group Ethernet
    insert("VPN", tr("VPN"));
    //: ConnectionTree.qml group Ethernet
    insert("BLUETOOTH", tr("Bluetooth"));

    //: ConnectionInfo.qml header
    insert("Connection Information", tr("Connection Information"));
    //: ConnectionTree.qml netmask header
    insert("Netmask:", tr("Netmask:"));
    //: ConnectionTree.qml add entry
    insert("Add Ethernet...", tr("Add Ethernet..."));
    //: ConnectionTree.qml add entry
    insert("Add Hotspot...", tr("Add Hotspot..."));
    //: ConnectionTree.qml add entry
    insert("Add Wifi...", tr("Add Wifi..."));

    //MainToolBar.qml
    insert("Battery low !\nPlease charge the device before it turns down", tr("Battery low !\nPlease charge the device before it turns down"));

    //main.qml
    insert("Loading...", tr("Loading..."));
    //: progress of loading %1 of %2 objects
    insert("Loading: %1/%2", tr("Loading: %1/%2"));
    //: the measurement view pages e.g. "Vector Diagram", "Oscilloscope view", etc.
    insert("Pages", tr("Pages", "view pages"));
    //: settings for range automatic etc.
    insert("Range", tr("Range", "measuring range"));

    //ErrorCalculatorModulePage.qml
    insert("Idle", tr("Idle"));
    //: the state where the device waits for the first pulse / edge to be triggered
    insert("Armed", tr("Armed"));
    insert("Started", tr("Started", "measurement started"));
    //: like finished
    insert("Ready", tr("Ready"));
    insert("Aborted", tr("Aborted", "measurement was aborted"));
    insert("Result:", tr("Result:", "error calculator result"));
    //: switch between time based and period based measurement
    insert("Mode:", tr("Mode:", "error calculator"));
    //: reference channel selection
    insert("Reference input:", tr("Reference input:"));
    //: device input selection (e.g. scanning head)
    insert("Device input:", tr("Device input:"));
    //: device under test constant
    insert("Constant", tr("Constant"));
    //: energy to compare
    insert("Energy:", tr("Energy:"));
    //: value based on the DUT constant
    insert("Pulse:", tr("Pulse:"));
    insert("Start", tr("Start"));
    insert("Stop", tr("Stop"));
    insert("energy", tr("energy"));
    insert("mrate", tr("mrate"));
    insert("Lower error limit:", tr("Lower error limit:"));
    insert("Upper error limit:", tr("Upper error limit:"));
    insert("continuous", tr("continuous"));
    insert("Count / Pause:", tr("Count / Pause:"));

    //ErrorRegisterModulePage.qml
    insert("Duration:", tr("Duration:"));
    insert("Start/Stop", tr("Start/Stop"));
    insert("Duration", tr("Duration"));
    insert("Start value:", tr("Start value:"));
    insert("End value:", tr("End value:"));

    //ErrorCalculatorModulePage.qml
    insert("Frequency:", tr("Frequency:"));

    //ErrorRegisterModulePage.qml
    insert("Count:", tr("Count:"));
    insert("Passed:", tr("Passed:"));
    insert("Failed:", tr("Failed:"));
    insert("Mean:", tr("Mean:"));
    insert("Range:", tr("Range:"));
    insert("Stddev. n:", tr("Stddev. n:"));
    insert("Stddev. n-1:", tr("Stddev. n-1:"));

    //FftTabPage.qml
    //: text must be short enough to fit
    insert("Amp", tr("Amp", "Amplitude of the phasor"));
    //: text must be short enough to fit
    insert("Phase", tr("Phase","Phase of the phasor"));
    //: total harmonic distortion with noise
    insert("THDN:", tr("THDN:"));
    insert("Harmonic table", tr("Harmonic table", "Tab text harmonic table"));
    insert("Harmonic chart", tr("Harmonic chart", "Tab text harmonic chart"));

    //VectorModulePage.qml
    //: ComboBox: I-vector ON
    insert("ON", tr("ON"));
    //: ComboBox: I-vector ON
    insert("OFF", tr("OFF"));
    //: ComboBox: Scale to max. range
    insert("Ranges", tr("Ranges"));
    //: ComboBox: Scale to max. value
    insert("Maximum", tr("Maximum"));

    //PowerModulePage.qml
    insert("Ext.", tr("Ext."));

    //HarmonicPowerTabPage.qml
    insert("Harmonic power table", tr("Harmonic power table", "Tab text harmonic power table"));
    insert("Harmonic power chart", tr("Harmonic power chart", "Tab text harmonic power chart"));


    //: text must be short enough to fit
    insert("UL1", tr("UL1", "channel name"));
    //: text must be short enough to fit
    insert("UL2", tr("UL2", "channel name"));
    //: text must be short enough to fit
    insert("UL3", tr("UL3", "channel name"));
    //: text must be short enough to fit
    insert("IL1", tr("IL1", "channel name"));
    //: text must be short enough to fit
    insert("IL2", tr("IL2", "channel name"));
    //: text must be short enough to fit
    insert("IL3", tr("IL3", "channel name"));
    //: text must be short enough to fit
    insert("UAUX", tr("UAUX", "channel name"));
    //: text must be short enough to fit
    insert("IAUX", tr("IAUX", "channel name"));

    //: text must be short enough to fit
    insert("P1", tr("P1", "harmonic power label active / phase 1"));
    //: text must be short enough to fit
    insert("Q1", tr("Q1", "harmonic power label reactive / phase 1"));
    //: text must be short enough to fit
    insert("S1", tr("S1", "harmonic power label aparent / phase 1"));
    //: text must be short enough to fit
    insert("P2", tr("P2", "harmonic power label active / phase 2"));
    //: text must be short enough to fit
    insert("Q2", tr("Q2", "harmonic power label reactive / phase 2"));
    //: text must be short enough to fit
    insert("S2", tr("S2", "harmonic power label aparent / phase 2"));
    //: text must be short enough to fit
    insert("P3", tr("P3", "harmonic power label active / phase 3"));
    //: text must be short enough to fit
    insert("Q3", tr("Q3", "harmonic power label reactive / phase 3"));
    //: text must be short enough to fit
    insert("S3", tr("S3", "harmonic power label aparent / phase 3"));

    //MeasurementPageModel.qml
    //: polar (amplitude and phase) phasor diagram
    insert("Vector diagram", tr("Vector diagram"));
    insert("Actual values", tr("Actual values"));
    insert("Actual values DC", tr("Actual values DC"));
    insert("Oscilloscope plot", tr("Oscilloscope plot"));
    //: FFT bar diagrams or tables that show the harmonic component distribution of the measured values
    insert("Harmonics & Curves", tr("Harmonics & Curves"));
    //: measuring mode dependent power values
    insert("Power values", tr("Power values"));
    //: FFT tables that show the real and imaginary parts of the measured power values
    insert("Harmonic power values", tr("Harmonic power values"));
    //: shows the deviation of measured energy between the reference device and the device under test
    insert("Error calculator", tr("Error calculator"));
    //: shows energy comparison between the reference device and the device under test's registers/display
    insert("Comparison measurements", tr("Comparison measurements"));
    //: control one or more source devices
    insert("Source control", tr("Source control"));

    //ComparisonTabsView.qml
    //: Comparison tabs label texts
    insert("Meter test", tr("Meter test"));
    insert("Energy comparison", tr("Energy comparison"));
    insert("Energy register", tr("Energy register"));
    insert("Power register", tr("Power register"));

    insert("Burden values", tr("Burden values"));
    insert("Transformer values", tr("Transformer values"));
    //: effective values
    insert("RMS values", tr("RMS values"));

    //BurdenModulePage.qml
    insert("Voltage Burden", tr("Voltage Burden", "Tab title current burden"));
    insert("Current Burden", tr("Current Burden", "Tab title current burden"));
    insert("Nominal burden:", tr("Nominal burden:"));
    insert("Nominal range:", tr("Nominal range:"));
    insert("Wire crosssection:", tr("Wire crosssection:"));
    insert("Wire length:", tr("Wire length:"));

    //ReferencePageModel.qml
    insert("DC reference values", tr("DC reference values"));
    //QuartzModulePage.qml
    insert("Quartz reference measurement", tr("Quartz reference measurement"));

    //CEDPageModel.qml
    insert("CED power values", tr("CED power values"));

    //HarmonicPowerTabPage.qml
    insert("Measuring modes:", tr("Measuring modes:", "label for measuring mode selectors"));

    //TransformerModulePage.qml
    insert("TR1", tr("TR1", "transformer system 1"));
    insert("X-Prim:", tr("X-Prim:"));
    insert("X-Sec:", tr("X-Sec:"));
    insert("Ms-Prim:", tr("Ms-Prim:"));
    insert("Ms-Sec:", tr("Ms-Sec:"));
    insert("Mp-Prim:", tr("Mp-Prim:"));
    insert("Mp-Sec:", tr("Mp-Sec:"));
    insert("X-Ratio", tr("X-Ratio"));
    insert("N-Sec", tr("N-Sec"));
    insert("X-Prim", tr("X-Prim"));
    insert("X-Sec", tr("X-Sec"));
    insert("crad", tr("crad", "centiradian"));
    insert("arcmin", tr("arcmin", "arcminute"));

    // SourceModulePage.qml
    insert("Actual", tr("Actual", "Source/Vector view indicator label: actual values mode"));
    insert("Target", tr("Target", "Source/Vector view indicator label: target values mode"));

    //Zera Classes -> zera-basemodule -> errormessages.h
    //: RESMAN = resource manager
    insert("RESMAN ident error", tr("RESMAN ident error"));
    //: RESMAN = resource manager
    insert("RESMAN resourcetype not avail", tr("RESMAN resourcetype not avail"));
    //: RESMAN = resource manager
    insert("RESMAN resource not avail", tr("RESMAN resource not avail"));
    //: RESMAN = resource manager
    insert("RESMAN resource Info error", tr("RESMAN resource Info error"));
    //: RESMAN = resource manager
    insert("RESMAN set resource failed", tr("RESMAN set resource failed"));
    //: RESMAN = resource manager
    insert("RESMAN free resource failed", tr("RESMAN free resource failed"));

    insert("PCB dsp channel read failed", tr("PCB dsp channel read failed"));
    insert("PCB alias read failed", tr("PCB alias read failed"));
    insert("PCB sample rate read failed", tr("PCB sample rate read failed"));
    insert("PCB unit read failed", tr("PCB unit read failed"));
    insert("PCB range list read failed", tr("PCB range list read failed"));
    insert("PCB range alias read failed", tr("PCB range alias read failed"));
    insert("PCB range type read failed", tr("PCB range type read failed"));
    insert("PCB range urvalue read failed", tr("PCB range urvalue read failed"));
    insert("PCB range rejection read failed", tr("PCB range rejection read failed"));
    insert("PCB range overload rejection failed", tr("PCB range overload rejection failed"));
    insert("PCB range avail info read failed", tr("PCB range avail info read failed"));
    insert("PCB set range failed", tr("PCB set range failed"));
    insert("PCB get range failed", tr("PCB get range failed"));
    insert("PCB set measuring mode failed", tr("PCB set measuring mode failed"));
    insert("PCB read gain correction failed", tr("PCB read gain correction failed"));
    insert("PCB set gain node failed", tr("PCB set gain node failed"));
    insert("PCB read offset correction failed", tr("PCB read offset correction failed"));
    insert("PCB set offset node failed", tr("PCB set offset node failed"));
    insert("PCB read phase correction failed", tr("PCB read phase correction failed"));
    insert("PCB set phase node failed", tr("PCB set phase node failed"));
    insert("PCB read channel status failed", tr("PCB read channel status failed"));
    insert("PCB reset channel status failed", tr("PCB reset channel status failed"));
    insert("PCB formfactor read failed", tr("PCB formfactor read failed"));
    insert("PCB register notifier failed", tr("PCB register notifier failed"));
    insert("PCB unregister notifier failed", tr("PCB unregister notifier failed"));
    insert("PCB set pll failed", tr("PCB set pll failed"));
    insert("PCB muxchannel read failed", tr("PCB muxchannel read failed"));
    insert("PCB reference constant read failed", tr("PCB reference constant read failed"));
    insert("PCB adjustment status read failed", tr("PCB adjustment status read failed"));
    insert("PCB adjustment chksum read failed", tr("PCB adjustment chksum read failed"));
    insert("PCB version read failed", tr("PCB version read failed"));
    insert("PCB server version read failed", tr("PCB server version read failed"));
    insert("PCB Controler version read failed", tr("PCB Controler version read failed"));
    insert("PCB FPGA version read failed", tr("PCB FPGA version read failed"));
    insert("PCB FPGA version read failed", tr("PCB FPGA version read failed"));
    insert("PCB error messages read failed", tr("PCB error messages read failed"));
    insert("PCB adjustment computation failed", tr("PCB adjustment computation failed"));
    insert("PCB adjustment storage failed", tr("PCB adjustment storage failed"));
    insert("PCB adjustment status setting failed", tr("PCB adjustment status setting failed"));
    insert("PCB adjustment init failed", tr("PCB adjustment init failed"));

    insert("DSP read gain correction failed", tr("DSP read gain correction failed"));
    insert("DSP read phase correction failed", tr("DSP read phase correction failed"));
    insert("DSP read offset correction failed", tr("DSP read offset correction failed"));
    insert("DSP write gain correction failed", tr("DSP write gain correction failed"));
    insert("DSP write phase correction failed", tr("DSP write phase correction failed"));
    insert("DSP write offset corredction failed", tr("DSP write offset corredction failed"));
    insert("DSP write varlist failed", tr("DSP write varlist failed"));
    insert("DSP write cmdlist failed", tr("DSP write cmdlist failed"));
    insert("DSP measure activation failed", tr("DSP measure activation failed"));
    insert("DSP measure deactivation failed", tr("DSP measure deactivation failed"));
    insert("DSP data acquisition failed", tr("DSP data acquisition failed"));
    insert("DSP memory write failed", tr("DSP memory write failed"));
    insert("DSP subdc write failed", tr("DSP subdc write failed"));
    insert("DSP server version read failed", tr("DSP server version read failed"));
    insert("DSP program version read failed", tr("DSP program version read failed"));
    insert("DSP setting sampling system failed", tr("DSP setting sampling system failed"));

    //: SEC = standard error calculator
    insert("SEC fetch ecalculator failed", tr("SEC fetch ecalculator failed"));
    //: SEC = standard error calculator
    insert("SEC free ecalculator failed", tr("SEC free ecalculator failed"));
    //: SEC = standard error calculator
    insert("SEC read register failed", tr("SEC read register failed"));
    //: SEC = standard error calculator
    insert("SEC write register failed", tr("SEC write register failed"));
    //: SEC = standard error calculator
    insert("SEC set sync failed", tr("SEC set sync failed"));
    //: SEC = standard error calculator
    insert("SEC set mux failed", tr("SEC set mux failed"));
    //: SEC = standard error calculator, cmdid = command id
    insert("SEC set cmdid failed", tr("SEC set cmdid failed"));
    //: SEC = standard error calculator
    insert("SEC start measure failed", tr("SEC start measure failed"));
    //: SEC = standard error calculator
    insert("SEC stop measure failed", tr("SEC stop measure failed"));

    insert("Interface JSON Document strange", tr("Interface JSON Document strange"));
    insert("Ethernet interface listen failed", tr("Ethernet interface listen failed"));
    insert("Serial interface not connected", tr("Serial interface not connected"));

    insert("Actual Values not found", tr("Actual Values not found"));
    insert("Release number not found", tr("Release number not found"));

    //DeviceInformation.qml
    insert("Device info", tr("Device info"));
    insert("Device log", tr("Device log"));
    insert("License information", tr("License information"));
    insert("Serial number:", tr("Serial number:"));
    insert("Operating system version:", tr("Operating system version:"));
    insert("Emob PCB version", tr("Emob PCB version"));
    insert("Relay PCB version", tr("Relay PCB version"));
    insert("System PCB version", tr("System PCB version"));
    insert("PCB server version:", tr("PCB server version:"));
    insert("DSP server version:", tr("DSP server version:"));
    insert("DSP firmware version:", tr("DSP firmware version:"));
    insert("FPGA firmware version:", tr("FPGA firmware version:"));
    insert("System controller version", tr("System controller version"));
    insert("Relay controller version", tr("Relay controller version"));
    insert("Emob controller version", tr("Emob controller version"));
    insert("Adjustment status:", tr("Adjustment status:"));
    insert("Not adjusted", tr("Not adjusted"));
    insert("Wrong version", tr("Wrong version"));
    insert("Wrong serial number", tr("Wrong serial number"));
    insert("Adjustment checksum:", tr("Adjustment checksum:"));
    insert("CPU-board number", tr("CPU-board number"));
    insert("CPU-board assembly", tr("CPU-board assembly"));
    insert("CPU-board date", tr("CPU-board date"));

    //Notifications.qml
    insert("Device notifications", tr("Device notifications"));

    //LoggerSettings.qml
    insert("Database Logging", tr("Database Logging"));
    insert("Logging enabled:", tr("Logging enabled:"));
    insert("DB location:", tr("DB location:"));
    insert("Database filename:", tr("Database filename:"));
    insert("Scheduled logging enabled:", tr("Scheduled logging enabled:"));
    //: describes the duration of the recording
    insert("Logging Duration [hh:mm:ss]:", tr("Logging Duration [hh:mm:ss]:"));
    insert("Logger status:", tr("Logger status:"));
    //: describes the ongoing task of recording data into a database
    insert("Logging data", tr("Logging data"));
    insert("Logging disabled", tr("Logging disabled"));
    insert("No database selected", tr("No database selected"));
    insert("Database loaded", tr("Database loaded"));
    insert("Database error", tr("Database error"));
    //: the user can make a selection of values he wants to log into a database
    insert("Select recorded values:", tr("Select recorded values:"));
    insert("Manage customer data:", tr("Manage customer data:"));
    insert("Snapshot", tr("Snapshot"));
    //: when the system disabled the customer data management, the brackets are for visual distinction from other text
    insert("[customer data is not available]", tr("[customer data is not available]"));
    //: when the customer id is empty, the brackets are for visual distinction from other text
    insert("[customer id is not set]", tr("[customer id is not set]"));
    //: when the customer number is empty, the brackets are for visual distinction from other text
    insert("[customer number is not set]", tr("[customer number is not set]"));
    //: placeholder text for the database filename
    insert("filename", tr("filename"));
    //: popup create database header
    insert("Create database", tr("Create database"));

    //LoggerDbSearchDialog.qml
    //: database search / delete view header
    insert("Databases", tr("Databases"));
    //:Select db location: internal storage
    insert("internal", tr("internal"));
    //:Select db location: external storage e.g memory stick
    insert("external", tr("external"));
    //:Delete database confirmation text - %1 is replaced by database filename
    insert("Delete database <b>'%1'</b>?", tr("Delete database <b>'%1'</b>?"));

    //LoggerSessionNameSelector.qml
    //: displayed in logger session name popup, visible when the user presses start or snapshot in the logger
    //: the session name is a database field that the user can use to search / filter different sessions
    insert("Select session name", tr("Select session name"));
    //: label for current session name
    insert("Current name:", tr("Current name:"));
    //: header for list of existing session names. Operator can select one of them to make it current session name
    insert("Select existing:", tr("Select existing:"));
    insert("Preview", tr("Preview"));
    //: delete session confirmation popup header
    insert("Confirmation", tr("Confirmation"));
    //: delete session confirmation popup header
    insert("Delete session <b>'%1'</b>?", tr("Delete session <b>'%1'</b>?"));

    // LoggerSessionNew.qml
    //: header view 'Add new session'
    insert("Add new session", tr("Add new session"));
    //: add new session view: label session name
    insert("Session name:", tr("Session name:"));
    //: add new session view: label list cutomer data to select entry from
    insert("Select customer data:", tr("Select customer data:"));
    //: add new session view: list entry to select no customer data
    insert("-- no customer --", tr("-- no customer --"));

    // LoggerSessionNameWithMacrosPopup.qml
    //: default prefix for auto session name
    insert("Session ", tr("Session "));

    //LoggerCustomDataSelector.qml
    //: logger custom data selector view header
    insert("Select custom data contents", tr("Select custom data contents"));
    //: button to select content-set actual values
    insert("ZeraActualValues", tr("Actual values"));
    //: button to select content-set harmonic values
    insert("ZeraHarmonics", tr("Harmonic values"));
    //: button to select content-set sample values
    insert("ZeraCurves", tr("Sample values"));
    //: button to select content-set comparison measurement values
    insert("ZeraComparison", tr("Comparison measurement results"));
    //: button to select content-set burden values
    insert("ZeraBurden", tr("Burden values"));
    //: button to select content-set transformer values
    insert("ZeraTransformer", tr("Transformer values"));
    //: button to select content-set dc-reference values
    insert("ZeraDCReference", tr("DC reference values"));
    //: button to select content-set dc-reference values
    insert("ZeraQuartzReference", tr("Quartz reference values"));
    //: button to select content-set dc-reference values
    insert("ZeraAll", tr("All"));
    //: logger custom data selector get back to menu button
    insert("Back", tr("Back"));

    //LoggerMenu.qml
    //: displayed when user presses logger button
    //: text displayed in session name menu entry when no session was set yet
    insert("-- no session --", tr("-- no session --"));
    //: take snapshot menu entry
    insert("Take snapshot", tr("Take snapshot"));
    //: start logging menu entry
    insert("Start logging", tr("Start logging"));
    //: stop logging menu entry
    insert("Stop logging", tr("Stop logging"));
    //: menu entry to open logger settings
    insert("Settings...", tr("Settings..."));
    //: Logger menu entry export
    insert("Export...", tr("Export..."));
    //: menu radio button to select content-set actual values
    insert("MenuZeraActualValues", tr("Actual values only"));
    //: menu radio button to select content-set harmonic values
    insert("MenuZeraHarmonics", tr("Harmonic values only"));
    //: menu radio button to select content-set sample values
    insert("MenuZeraCurves", tr("Sample values only"));
    //: menu radio button to select content-set comparison measurement values
    insert("MenuZeraComparison", tr("Comparison measurement results only"));
    //: menu radio button to select content-set burden values
    insert("MenuZeraBurden", tr("Burden values only"));
    //: menu radio button to select content-set transformer values
    insert("MenuZeraTransformer", tr("Transformer values only"));
    //: menu radio button to select content-set dc-reference values
    insert("MenuZeraDCReference", tr("DC reference values only"));
    //: menu radio button to select content-set dc-reference values
    insert("MenuZeraQuartzReference", tr("Quartz reference values only"));
    //: menu radio button to select all values
    insert("MenuZeraAll", tr("All data"));
    //: menu radio button to select custom content-sets values
    insert("MenuZeraCustom", tr("Custom data"));

    //CustomerDataEntry.qml
    insert("Customer", tr("Customer"));
    //: power meter, not distance
    insert("Meter information", tr("Meter information"));
    insert("Location", tr("Location"));
    insert("Power grid", tr("Power grid"));
    insert("PAR_DatasetIdentifier", tr("Data Identifier:"));
    insert("PAR_DatasetComment", tr("Data Comment:"));
    insert("PAR_CustomerNumber", tr("Customer number:"));
    insert("PAR_CustomerFirstName", tr("Customer First name:"));
    insert("PAR_CustomerLastName", tr("Customer Last name:"));
    insert("PAR_CustomerCountry", tr("Customer Country:"));
    insert("PAR_CustomerCity", tr("Customer City:"));
    insert("PAR_CustomerPostalCode", tr("Customer ZIP code:", "Postal code"));
    insert("PAR_CustomerStreet", tr("Customer Street:"));
    insert("PAR_CustomerComment", tr("Customer Comment:"));
    insert("PAR_LocationNumber", tr("Location Identifier:"));
    insert("PAR_LocationFirstName", tr("Location First name:"));
    insert("PAR_LocationLastName", tr("Location Last name:"));
    insert("PAR_LocationCountry", tr("LocationCountry:"));
    insert("PAR_LocationCity", tr("Location City:"));
    insert("PAR_LocationPostalCode", tr("Location ZIP code:", "Postal code"));
    insert("PAR_LocationStreet", tr("Location Street:"));
    insert("PAR_LocationComment", tr("Location Comment:"));
    insert("PAR_MeterFactoryNumber", tr("Meter Factory number:"));
    insert("PAR_MeterManufacturer", tr("Meter Manufacturer:"));
    insert("PAR_MeterOwner", tr("Meter Owner:"));
    insert("PAR_MeterComment", tr("Meter Comment:"));
    insert("PAR_PowerGridOperator", tr("Power grid Operator:"));
    insert("PAR_PowerGridSupplier", tr("Power grid Supplier:"));
    insert("PAR_PowerGridComment", tr("Power grid Comment:"));

    //CustomerDataBrowser.qml
    //: header text customer data browser
    insert("Customer data", tr("Customer data"));
    //: button import cutomer data
    insert("Import", tr("Import"));
    //: button import cutomer data
    insert("Export", tr("Export"));
    //: label text add customer data
    insert("File name:", tr("File name:", "customerdata filename"));
    insert("Search", tr("Search", "search for customerdata files"));
    //: header import customer data popup
    insert("Import customer data", tr("Import customer data"));
    //: label import customer data device select combo
    insert("Files found from device:", tr("Files found from device:"));
    //: import customer data popup option checkbox text
    insert("Delete current files first", tr("Delete current files first"));
    //: import customer data popup option checkbox text
    insert("Overwrite current files with imported ones", tr("Overwrite current files with imported ones"));
    //: customer data delete file confirmation popup header
    insert("Confirmation", tr("Confirmation"));
    //: customer data delete file confirmation text
    insert("Delete <b>'%1'</b>?", tr("Delete <b>'%1'</b>?"));
    //: customer data delete file button text
    insert("Delete", tr("Delete"));

    //: header text popup new customer data
    insert("Create Customer data file", tr("Create Customer data file"));

    // LoggerExport.qml
    //: header text view export data
    insert("Export stored data", tr("Export stored data"));
    //: label combobox export type (MTVisXML/SQLiteDB...)
    insert("Export type:", tr("Export type:"));
    //: entry combobox export type MTVis Part 1
    insert("MtVis XML", tr("MtVis XML"));
    //: entry combobox export type MTVis Part 2
    insert("Session:", tr("Session:"));
    //: entry combobox export type in case no sessions stored yet
    insert("MtVis XML - requires stored sessions", tr("MtVis XML - requires stored sessions"));
    //: entry combobox export type complete SQLite database
    insert("SQLite DB (complete)", tr("SQLite DB (complete)"));
    //: label edit field export name
    insert("Export name:", tr("Export name:"));
    //: edit field export name: Text displayed in case MTVis export is selected and field is empty (placeholder text)
    insert("Name of export path", tr("Name of export path"));
    //: label combobox target drive (visible only if multiple sicks / partitions are mounted)
    insert("Target drive:", tr("Target drive:"));
    //: button text MTVis export type selected but no session active currently
    insert("Please select a session first...", tr("Please select a session first..."));
    //: Text set in case MTVis export failed
    insert("Export failed - drive full or removed?", tr("Export failed - drive full or removed?"));
    //: Text set in case MTVis export failed
    insert("Import failed - drive removed?", tr("Import failed - drive removed?"));
    //: Text set in case SQLite file export (copy) failed
    insert("Copy failed - drive full or removed?", tr("Copy failed - drive full or removed?"));
    //: Header text in wait export popup
    insert("Exporting customer data...", tr("Exporting customer data..."));
    //: Header text in wait import popup
    insert("Importing customer data...", tr("Importing customer data..."));
    //: Header text in wait export popup
    insert("Exporting MTVis XML...", tr("Exporting MTVis XML..."));
    //: Header text in wait export popup
    insert("Exporting database...", tr("Exporting database..."));
    //: warning prefix
    insert("Warning:", tr("Warning:"));
    //: error prefix
    insert("Error:", tr("Error:"));

    // MountedDrivesCombo.qml
    //: in drive select combo for partitions without name
    insert("unnamed", tr("unnamed"));
    //: in drive select combo free memory label
    insert("free", tr("free"));

    // SerialSettings.qml
    insert("Not connected", tr("Not connected"));
    insert("Serial SCPI", tr("Serial SCPI"));
    insert("Serial SCPI", tr("Serial SCPI"));
    insert("Source device", tr("Source device"));
    insert("Scanning for source device...", tr("Scanning for source device..."));
    insert("Opening SCPI serial...", tr("Opening SCPI serial..."));
    insert("Disconnect source...", tr("Disconnect source..."));
    insert("Disconnect SCPI serial...", tr("Disconnect SCPI serial..."));
    insert("No source found", tr("No source found"));
    insert("Source switch off failed", tr("Source switch off failed"));
    insert("Switch on failed", tr("Switch on failed"));
    insert("Switch off failed", tr("Switch off failed"));
    insert("Switching on %1...", tr("Switching on %1..."));
    insert("Switching off %1...", tr("Switching off %1..."));
    insert("none", tr("none"));
    insert("On", tr("On"));
    insert("symmetric", tr("symmetric"));
    insert("Off", tr("Off"));
    insert("Frequency:", tr("Frequency:"));

    // SensorSettings.qml
    insert("Bluetooth:", tr("Bluetooth:"));
    insert("MAC address:", tr("MAC address:"));
    insert("Temperature [°C]:", tr("Temperature [°C]:"));
    insert("T [°C]", tr("T [°C]"));
    insert("Temperature [°F]:", tr("Temperature [°F]:"));
    insert("Humidity [%]:", tr("Humidity [%]:"));
    insert("Air pressure [hPa]:", tr("Air pressure [hPa]:"));

    emit sigLanguageChanged();
}

QVariant ZeraTranslation::updateValue(const QString &key, const QVariant &input)
{
    Q_ASSERT(false); //do not change the values from QML
    Q_UNUSED(input)
    return value(key);
}
