/*
 * MySimpleSIP — janela de Configurações (port Qt do SettingsDlg do MicroSIP).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Port de SettingsDlg.cpp (MicroSIP 3.22.16) sobre o IDD_SETTINGS original (CDialogLayout).
// LISTBOX → QListWidget; botões de setas (msctls_updown32) → dois QToolButton; links "?"
// da ajuda ocultos enquanto não houver site de ajuda (_GLOBAL_HELP_WEBSITE vazio).
// Diferenças marcadas com "MySimpleSIP:".

#include "SettingsDlg.h"

#include "AAOptionsDlg.h"
#include "Dialer.h"
#include "FeatureCodesDlg.h"
#include "MessagesDlg.h"
#include "Preview.h"
#include "PulseAudioDev.h"
#include "UiCommon.h"
#include "langpack.h"
#include "mainDlg.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <cstring>
#include <cwchar>

static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

static CString defaultActionItems[] = {
_T(""),
_T("call"),
#ifdef _GLOBAL_VIDEO
_T("video"),
#endif
_T("message"),
};
static CString defaultActionValues[] = {
_T("Default"),
_T("Call"),
#ifdef _GLOBAL_VIDEO
_T("Video Call"),
#endif
_T("Message"),
};

bool msip_startup_get()
{
    const char* configHome = getenv("XDG_CONFIG_HOME");
    const char* home = getenv("HOME");
    QString dir;
    if (configHome && *configHome == '/') {
        dir = QString::fromLocal8Bit(configHome);
    }
    else if (home && *home) {
        dir = QString::fromLocal8Bit(home) + QStringLiteral("/.config");
    }
    else {
        return false;
    }
    QFile f(dir + QStringLiteral("/autostart/mysimplesip.desktop"));
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QString text = QString::fromUtf8(f.readAll()).toLower();
    return text.contains(QS(accountSettings.exeFile).toLower());
}

SettingsDlg::SettingsDlg(QWidget* pParent) : QWidget(pParent, Qt::Dialog)
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    m_layout.customFactory = [](const QString& id, const QString&, QWidget* p) -> QWidget* {
        if (id == QLatin1String("IDC_SETTINGS_AUDIO_CODECS_ALL") || id == QLatin1String("IDC_SETTINGS_AUDIO_CODECS")) {
            return new QListWidget(p);
        }
        if (id == QLatin1String("IDC_SETTINGS_SPIN_MODIFY") || id == QLatin1String("IDC_SETTINGS_SPIN_ORDER")) {
            // msctls_updown32: setas horizontais (habilitar/desabilitar) ou verticais (ordem)
            return CreateUpDown(p, id == QLatin1String("IDC_SETTINGS_SPIN_MODIFY"));
        }
        return nullptr;
    };
    m_layout.Build(this, QStringLiteral("IDD_SETTINGS"));
    m_codecsAll = static_cast<QListWidget*>(DLG_ITEM(m_layout, IDC_SETTINGS_AUDIO_CODECS_ALL));
    m_codecs = static_cast<QListWidget*>(DLG_ITEM(m_layout, IDC_SETTINGS_AUDIO_CODECS));
    OnInitDialog();
    show();
}

SettingsDlg::~SettingsDlg()
{
}

void SettingsDlg::OnInitDialog()
{
    QComboBox* combobox;
    unsigned count;
    int i;
    int n;
    bool found;
    CString str;

    setWindowIcon(LoadImageIcon(IDI_MAINFRAME));

    str.Format(_T("<a href=\"#\">%s</a>"), Translate(_T("Feature Codes")));
    m_layout.Label(QStringLiteral("IDC_SETTINGS_FEATURE_CODES"))->setText(QS(str));
    QObject::connect(m_layout.Label(QStringLiteral("IDC_SETTINGS_FEATURE_CODES")), &QLabel::linkActivated, this,
                     [this] { OnNMClickSyslinkFeatureCodes(); });

    aaOptionsDlg = NULL;
    featureCodesDlg = NULL;

    m_layout.TranslateDialog(this);

    // MySimpleSIP: links "?" (OpenHelp) ocultos — _GLOBAL_HELP_WEBSITE ainda vazio
    for (QWidget* w : findChildren<QWidget*>()) {
        if (w->objectName().startsWith(QLatin1String("IDC_SETTINGS_HELP_"))) {
            w->hide();
        }
    }

    m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE"))->setText(QS(accountSettings.ringtone));
    auto* volumeRing = static_cast<QSlider*>(DLG_ITEM(m_layout, IDC_SETTINGS_VOLUME_RING));
    volumeRing->setRange(0, 100);
    volumeRing->setValue(accountSettings.volumeRing);
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_RECORDING"))->setText(QS(accountSettings.recordingPath));
    auto* radioMp3 = static_cast<QRadioButton*>(DLG_ITEM(m_layout, IDC_SETTINGS_RECORDING_MP3));
    auto* radioWav = static_cast<QRadioButton*>(DLG_ITEM(m_layout, IDC_SETTINGS_RECORDING_WAV));
    if (accountSettings.recordingFormat == _T("wav")) {
        radioWav->setChecked(true);
    }
    else {
        radioMp3->setChecked(true);
    }
    m_layout.Check(QStringLiteral("IDC_SETTINGS_RECORDING_CHECKBOX"))->setChecked(accountSettings.autoRecording);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_RECORDING_BUTTON"))->setChecked(accountSettings.recordingButton);
    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_MICROPHONE"));
    combobox->addItem(TR(_T("Default")));
    combobox->setCurrentIndex(0);
    pjmedia_aud_dev_info aud_dev_info[PJMEDIA_AUD_MAX_DEVS];
    if (is_pjsua_running()) {
        count = PJMEDIA_AUD_MAX_DEVS;
        pjsua_enum_aud_devs(aud_dev_info, &count);
    }
    else {
        count = 0;
    }
    for (unsigned i = 0; i < count; i++)
    {
        if (aud_dev_info[i].input_count && strcmp(aud_dev_info[i].name, MSIP_PULSE_DEFAULT_DEVICE) != 0) {
            // MySimpleSIP: o pseudo-dispositivo do PulseAudio é o próprio "Padrão"
            CString audDevName = MSIP::Utf8DecodeUni(aud_dev_info[i].name);
            combobox->addItem(QS(audDevName));
            if (!accountSettings.audioInputDevice.Compare(audDevName)) {
                combobox->setCurrentIndex(combobox->count() - 1);
            }
        }
    }
    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_SPEAKERS"));
    combobox->addItem(TR(_T("Default")));
    combobox->setCurrentIndex(0);
    QComboBox* comboboxRing = m_layout.Combo(QStringLiteral("IDC_SETTINGS_RING"));
    comboboxRing->addItem(TR(_T("Default")));
    comboboxRing->setCurrentIndex(0);
    for (unsigned i = 0; i < count; i++)
    {
        if (aud_dev_info[i].output_count && strcmp(aud_dev_info[i].name, MSIP_PULSE_DEFAULT_DEVICE) != 0) {
            CString audDevName = MSIP::Utf8DecodeUni(aud_dev_info[i].name);
            combobox->addItem(QS(audDevName));
            comboboxRing->addItem(QS(audDevName));
            if (!accountSettings.audioOutputDevice.Compare(audDevName)) {
                combobox->setCurrentIndex(combobox->count() - 1);
            }
            if (!accountSettings.audioRingDevice.Compare(audDevName)) {
                comboboxRing->setCurrentIndex(comboboxRing->count() - 1);
            }
        }
    }
    m_layout.Check(QStringLiteral("IDC_SETTINGS_MIC_AMPLIF"))->setChecked(accountSettings.micAmplification);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_SW_ADJUST"))->setChecked(accountSettings.swLevelAdjustment);

    pjsua_codec_info codec_info[PJMEDIA_CODEC_MGR_MAX_CODECS];
    QListWidget* listbox = m_codecsAll;
    QListWidget* listbox2 = m_codecs;

    CList<CString> disabledCodecsList;
    if (is_pjsua_running()) {
        count = PJMEDIA_CODEC_MGR_MAX_CODECS;
        pjsua_enum_codecs(codec_info, &count);
    }
    else {
        count = 0;
    }
    for (unsigned i = 0; i < count; i++)
    {
        POSITION pos = MainDlg()->audioCodecList.Find(
            MSIP::PjToStr(&codec_info[i].codec_id)
        );
        if (!pos) {
            continue; // MySimpleSIP: codec sem legenda (no original, todos os codecs a têm)
        }
        CString key = MainDlg()->audioCodecList.GetNext(pos);
        CString value = MainDlg()->audioCodecList.GetNext(pos);
        if (codec_info[i].priority
            && (!accountSettings.audioCodecs.IsEmpty() || wcsstr(_T(_GLOBAL_CODECS_ENABLED), key))
            ) {
            if (listbox2->findItems(QS(value), Qt::MatchExactly).isEmpty()) {
                listbox2->addItem(QS(value));
            }
        }
        else {
            disabledCodecsList.AddTail(key);
        }
    }
    POSITION pos = MainDlg()->audioCodecList.GetHeadPosition();
    while (pos) {
        CString key = MainDlg()->audioCodecList.GetNext(pos);
        CString value = MainDlg()->audioCodecList.GetNext(pos);
        if (disabledCodecsList.Find(key)) {
            listbox->addItem(QS(value));
        }
    }
    listbox->installEventFilter(this);
    listbox2->installEventFilter(this);
    QWidget* spinModify = DLG_ITEM(m_layout, IDC_SETTINGS_SPIN_MODIFY);
    QObject::connect(spinModify->findChild<QToolButton*>(QStringLiteral("second")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinModify(-1); }); // → habilitar
    QObject::connect(spinModify->findChild<QToolButton*>(QStringLiteral("first")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinModify(1); });  // ← desabilitar
    QWidget* spinOrder = DLG_ITEM(m_layout, IDC_SETTINGS_SPIN_ORDER);
    QObject::connect(spinOrder->findChild<QToolButton*>(QStringLiteral("first")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinOrder(-1); });
    QObject::connect(spinOrder->findChild<QToolButton*>(QStringLiteral("second")), &QToolButton::clicked, this,
                     [this] { OnDeltaposSpinOrder(1); });

    m_layout.Check(QStringLiteral("IDC_SETTINGS_VAD"))->setChecked(accountSettings.vad);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_EC"))->setChecked(accountSettings.ec);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_OPUS_STEREO"))->setChecked(accountSettings.opusStereo);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_FORCE_CODEC"))->setChecked(accountSettings.forceCodec);

#ifdef _GLOBAL_VIDEO
    m_layout.Check(QStringLiteral("IDC_SETTINGS_DISABLE_VIDEO"))->setChecked(accountSettings.disableVideo);

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_VID_CAP_DEV"));
    combobox->addItem(TR(_T("Default")));
    combobox->setCurrentIndex(0);

    pjmedia_vid_dev_info vid_dev_info[PJMEDIA_VID_DEV_MAX_DEVS];
    if (is_pjsua_running()) {
        count = PJMEDIA_VID_DEV_MAX_DEVS;
        pjsua_vid_enum_devs(vid_dev_info, &count);
    }
    else {
        count = 0;
    }
    for (unsigned i = 0; i < count; i++)
    {
        if (vid_dev_info[i].fmt_cnt && (vid_dev_info[i].dir == PJMEDIA_DIR_ENCODING || vid_dev_info[i].dir == PJMEDIA_DIR_ENCODING_DECODING))
        {
            CString vidDevName = MSIP::Utf8DecodeUni(vid_dev_info[i].name);
            combobox->addItem(QS(vidDevName));
            if (!accountSettings.videoCaptureDevice.Compare(vidDevName))
            {
                combobox->setCurrentIndex(combobox->count() - 1);
            }
        }
    }

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_VIDEO_CODEC"));
    combobox->addItem(TR(_T("Default")));
    combobox->setCurrentIndex(0);
    if (is_pjsua_running()) {
        count = PJMEDIA_CODEC_MGR_MAX_CODECS;
        pjsua_vid_enum_codecs(codec_info, &count);
    }
    else {
        count = 0;
    }
    for (unsigned i = 0; i < count; i++)
    {
        combobox->addItem(QS(MSIP::PjToStr(&codec_info[i].codec_id)));
        if (!accountSettings.videoCodec.Compare(MSIP::PjToStr(&codec_info[i].codec_id)))
        {
            combobox->setCurrentIndex(combobox->count() - 1);
        }
    }

    m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_H264"))->setChecked(accountSettings.videoH264);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_H263"))->setChecked(accountSettings.videoH263);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_VP8"))->setChecked(accountSettings.videoVP8);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_VP9"))->setChecked(accountSettings.videoVP9);
    if (!accountSettings.videoBitrate) {
        if (is_pjsua_running()) {
            // MySimpleSIP: os IDs de vídeo do pjproject upstream incluem o payload type
            CString h264 = msip_vid_codec_id(_T("H264"));
            if (!h264.IsEmpty()) {
                CStringA id(h264);
                pj_str_t codec_id = pj_str(const_cast<char*>(id.GetString()));
                pjmedia_vid_codec_param param;
                if (pjsua_vid_codec_get_param(&codec_id, &param) == PJ_SUCCESS) {
                    accountSettings.videoBitrate = param.enc_fmt.det.vid.max_bps / 1000;
                }
            }
        }
    }
    str.Format(_T("%d"), accountSettings.videoBitrate);
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_VIDEO_BITRATE"))->setText(QS(str));
#endif

    m_layout.Check(QStringLiteral("IDC_SETTINGS_RPORT"))->setChecked(accountSettings.rport);
    str.Format(_T("%d"), accountSettings.sourcePort);
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_SOURCE_PORT"))->setText(QS(str));
    str.Format(_T("%d"), accountSettings.rtpPortMin);
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_RTP_PORT_MIN"))->setText(QS(str));
    str.Format(_T("%d"), accountSettings.rtpPortMax);
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_RTP_PORT_MAX"))->setText(QS(str));

    m_layout.Edit(QStringLiteral("IDC_SETTINGS_DNS_SRV_NS"))->setText(QS(accountSettings.dnsSrvNs));
    m_layout.Check(QStringLiteral("IDC_SETTINGS_DNS_SRV_CHECKBOX"))->setChecked(accountSettings.dnsSrv);

    m_layout.Edit(QStringLiteral("IDC_SETTINGS_STUN"))->setText(QS(accountSettings.stun));
    m_layout.Check(QStringLiteral("IDC_SETTINGS_STUN_CHECKBOX"))->setChecked(accountSettings.enableSTUN);

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_DTMF_METHOD"));
    combobox->addItem(TR(_T("Auto")));
    combobox->addItem(TR(_T("In-band")));
    combobox->addItem(TR(_T("RFC2833")));
    combobox->addItem(TR(_T("SIP-INFO")));
    combobox->setCurrentIndex(accountSettings.DTMFMethod);

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_AUTO_ANSWER"));
    combobox->addItem(TR(_T("No")));
    autoAnswerValues.Add(_T(""));
    combobox->addItem(TR(_T("Control Button")));
    autoAnswerValues.Add(_T("button"));
    combobox->addItem(TR(_T("SIP Header")));
    autoAnswerValues.Add(_T("header"));
    combobox->addItem(TR(_T("All Calls")));
    autoAnswerValues.Add(_T("all"));
    combobox->setCurrentIndex(0);
    for (i = 0; i < autoAnswerValues.GetCount(); i++) {
        if (accountSettings.autoAnswer == autoAnswerValues.GetAt(i)) {
            combobox->setCurrentIndex(i);
            break;
        }
    }

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_FWD"));
    combobox->addItem(TR(_T("No")));
    forwardingValues.Add(_T(""));
    combobox->setCurrentIndex(0);
    combobox->addItem(TR(_T("Control Button")));
    forwardingValues.Add(_T("button"));
    combobox->addItem(TR(_T("All Calls")));
    forwardingValues.Add(_T("all"));
    for (i = 0; i < forwardingValues.GetCount(); i++) {
        if (accountSettings.forwarding == forwardingValues.GetAt(i)) {
            combobox->setCurrentIndex(i);
            break;
        }
    }
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_FWD_NUMBER"))->setText(QS(accountSettings.forwardingNumber));
    str.Format(_T("%d"), accountSettings.forwardingDelay);
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_FWD_DELAY"))->setText(QS(str));

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_DENY_INCOMING"));
    combobox->addItem(TR(_T("No")));
    denyIncomingValues.Add(_T(""));
    combobox->addItem(TR(_T("Control Button")));
    denyIncomingValues.Add(_T("button"));
    combobox->addItem(TR(_T("Different User")));
    denyIncomingValues.Add(_T("user"));
    combobox->addItem(TR(_T("Different Domain")));
    denyIncomingValues.Add(_T("domain"));
    combobox->addItem(TR(_T("Different User or Domain")));
    denyIncomingValues.Add(_T("userdomain"));
    combobox->addItem(TR(_T("Different Remote Domain")));
    denyIncomingValues.Add(_T("remotedomain"));
    combobox->addItem(TR(_T("All Calls")));
    denyIncomingValues.Add(_T("all"));
    combobox->setCurrentIndex(0);
    for (i = 0; i < denyIncomingValues.GetCount(); i++) {
        if (accountSettings.denyIncoming == denyIncomingValues.GetAt(i)) {
            combobox->setCurrentIndex(i);
            break;
        }
    }

    m_layout.Edit(QStringLiteral("IDC_SETTINGS_DIRECTORY"))->setText(QS(accountSettings.usersDirectory));

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_DEFAULT_ACTION"));
    n = sizeof(defaultActionItems) / sizeof(defaultActionItems[0]);
    found = false;
    for (int i = 0; i < n; i++) {
        combobox->addItem(TR(defaultActionValues[i]));
        if (accountSettings.defaultAction == defaultActionItems[i]) {
            combobox->setCurrentIndex(i);
            found = true;
        }
    }
    if (!found) {
        combobox->setCurrentIndex(0);
    }

    m_layout.Check(QStringLiteral("IDC_SETTINGS_MEDIA_BUTTONS"))->setChecked(accountSettings.enableMediaButtons);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_HID"))->setChecked(accountSettings.headsetSupport);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_LOCAL_DTMF"))->setChecked(accountSettings.localDTMF);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_SINGLE_MODE"))->setChecked(accountSettings.singleMode);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_ENABLE_LOG"))->setChecked(accountSettings.enableLog);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_BRING_TO_FRONT"))->setChecked(accountSettings.bringToFrontOnIncoming);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_ANSWER_BOX_RANDOM"))->setChecked(accountSettings.randomAnswerBox);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_CALL_WAITING"))->setChecked(accountSettings.callWaiting);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_MULTI_MONITOR"))->setChecked(accountSettings.multiMonitor);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_NETWORK_CHANGES"))->setChecked(accountSettings.networkChanges);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_DISMESS"))->setChecked(accountSettings.disableMessaging);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_DISABLE_NAME_LOOKUP"))->setChecked(accountSettings.disableNameLookup);
    m_layout.Check(QStringLiteral("IDC_SETTINGS_ENABLE_LOCAL"))->setChecked(accountSettings.enableLocalAccount);

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_UPDATES_INTERVAL"));
    combobox->addItem(TR(_T("Daily")));
    combobox->addItem(TR(_T("Weekly")));
    combobox->addItem(TR(_T("Monthly")));
    combobox->addItem(TR(_T("Quarterly")));
    combobox->addItem(TR(_T("Never")));
    if (accountSettings.updatesInterval == _T("daily"))
    {
        i = 0;
    }
    else if (accountSettings.updatesInterval == _T("monthly"))
    {
        i = 2;
    }
    else if (accountSettings.updatesInterval == _T("quarterly"))
    {
        i = 3;
    }
    else if (accountSettings.updatesInterval == _T("never"))
    {
        i = 4;
    }
    else
    {
        i = 1;
    }
    combobox->setCurrentIndex(i);
    // MySimpleSIP: registro HKCU\...\Run → XDG Autostart
    m_layout.Check(QStringLiteral("IDC_SETTINGS_STARTUP"))->setChecked(msip_startup_get());

    // controles → handlers (mapa de mensagens)
    auto click = [this](const char* id, std::function<void()> fn) {
        if (auto* b = qobject_cast<QPushButton*>(m_layout.Item(QString::fromLatin1(id))))
            QObject::connect(b, &QPushButton::clicked, this, [fn] { fn(); });
    };
    click("IDOK", [this] { OnBnClickedOk(); });
    click("IDCANCEL", [this] { OnBnClickedCancel(); });
    click("IDC_SETTINGS_PREVIEW", [this] { OnBnClickedPreview(); });
    click("IDC_SETTINGS_BROWSE", [this] { OnBnClickedBrowse(); });
    click("IDC_SETTINGS_DEFAULT", [this] { OnBnClickedDefault(); });
    click("IDC_SETTINGS_RECORDING_BROWSE", [this] { OnBnClickedRecordingBrowse(); });
    click("IDC_SETTINGS_RECORDING_DEFAULT", [this] { OnBnClickedRecordingDefault(); });
    click("IDC_SETTINGS_AA_OPTIONS", [this] { OnBnClickedAAOptions(); });
    m_layout.Button(QStringLiteral("IDOK"))->setDefault(true);
    QObject::connect(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE")), &QLineEdit::textChanged, this, [this] { OnChangeRingtone(); });
    QObject::connect(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RECORDING")), &QLineEdit::textChanged, this, [this] { OnEnChangeRecording(); });
    QObject::connect(volumeRing, &QSlider::sliderReleased, this, [this] { OnRingVolumeReleased(); });
    OnChangeRingtone();
    OnEnChangeRecording();
}

void SettingsDlg::closeEvent(QCloseEvent* event)
{
    if (!m_destroyed && !g_bClosingApp) {
        OnBnClickedCancel(); // fechar a janela = Cancelar
    }
    event->accept();
}

void SettingsDlg::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        OnBnClickedCancel();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool SettingsDlg::eventFilter(QObject* watched, QEvent* event)
{
    // OnVKeyToItem: Espaço habilita o codec selecionado; Delete desabilita
    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (watched == m_codecsAll && m_codecsAll->currentRow() != -1 && ke->key() == Qt::Key_Space) {
            OnDeltaposSpinModify(-1);
            return true;
        }
        if (watched == m_codecs && m_codecs->currentRow() != -1 && ke->key() == Qt::Key_Delete) {
            OnDeltaposSpinModify(1);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SettingsDlg::OnClose()
{
    // DestroyWindow → OnDestroy (mainDlg->settingsDlg = nullptr) → delete
    if (m_destroyed) {
        return;
    }
    m_destroyed = true;
    if (MainDlg()->settingsDlg == this) {
        MainDlg()->settingsDlg = nullptr;
    }
    hide();
    deleteLater();
}

void SettingsDlg::OnBnClickedCancel()
{
    MainDlg()->PlayerStop();
    OnClose();
}

void SettingsDlg::OnBnClickedOk()
{
    hide();

    statistics.settingsTime = CTime::GetCurrentTime().GetTime();

    MainDlg()->PJDestroy();
    // PostMessage(UM_UPDATE_SETTINGS): callbacks must be processed before a new session starts
    QTimer::singleShot(0, this, [this] { OnUpdateSettings(0, 0); });
}

LRESULT SettingsDlg::OnUpdateSettings(WPARAM, LPARAM)
{
    CString str;

    QComboBox* combobox;
    int i;
    accountSettings.audioInputDevice = CS(m_layout.Combo(QStringLiteral("IDC_SETTINGS_MICROPHONE"))->currentText());
    if (accountSettings.audioInputDevice == Translate(_T("Default")))
    {
        accountSettings.audioInputDevice = _T("");
    }

    accountSettings.audioOutputDevice = CS(m_layout.Combo(QStringLiteral("IDC_SETTINGS_SPEAKERS"))->currentText());
    if (accountSettings.audioOutputDevice == Translate(_T("Default")))
    {
        accountSettings.audioOutputDevice = _T("");
    }
    accountSettings.audioRingDevice = CS(m_layout.Combo(QStringLiteral("IDC_SETTINGS_RING"))->currentText());
    if (accountSettings.audioRingDevice == Translate(_T("Default")))
    {
        accountSettings.audioRingDevice = _T("");
    }
    accountSettings.micAmplification = m_layout.Check(QStringLiteral("IDC_SETTINGS_MIC_AMPLIF"))->isChecked();
    accountSettings.swLevelAdjustment = m_layout.Check(QStringLiteral("IDC_SETTINGS_SW_ADJUST"))->isChecked();

    accountSettings.vad = m_layout.Check(QStringLiteral("IDC_SETTINGS_VAD"))->isChecked();
    accountSettings.ec = m_layout.Check(QStringLiteral("IDC_SETTINGS_EC"))->isChecked();
    accountSettings.opusStereo = m_layout.Check(QStringLiteral("IDC_SETTINGS_OPUS_STEREO"))->isChecked();
    accountSettings.forceCodec = m_layout.Check(QStringLiteral("IDC_SETTINGS_FORCE_CODEC"))->isChecked();

    bool hasStereo = false;
    // MySimpleSIP: sem pjsua rodando a lista fica vazia — mantém os codecs atuais
    if (m_codecs->count() || m_codecsAll->count()) {
        accountSettings.audioCodecs = _T("");
        for (int i = 0; i < m_codecs->count(); i++)
        {
            CString value = CS(m_codecs->item(i)->text());
            POSITION pos = MainDlg()->audioCodecList.Find(value);
            if (pos) {
                MainDlg()->audioCodecList.GetPrev(pos);
                CString key = MainDlg()->audioCodecList.GetPrev(pos);
                accountSettings.audioCodecs += key + _T(" ");
                if (!hasStereo && key.Right(2) == _T("/2") && key.Left(4) != _T("opus")) {
                    hasStereo = true;
                }
            }
        }
        accountSettings.audioCodecs.Trim();
    }
    if (hasStereo && accountSettings.ec) {
        QMessageBox::information(MainDlg(), QStringLiteral(_GLOBAL_NAME_NICE), QStringLiteral("Echo Canceler enabled. Stereo will be converted to Mono."));
    }

#ifdef _GLOBAL_VIDEO
    accountSettings.disableVideo = m_layout.Check(QStringLiteral("IDC_SETTINGS_DISABLE_VIDEO"))->isChecked();
    accountSettings.videoCaptureDevice = CS(m_layout.Combo(QStringLiteral("IDC_SETTINGS_VID_CAP_DEV"))->currentText());
    if (accountSettings.videoCaptureDevice == Translate(_T("Default")))
    {
        accountSettings.videoCaptureDevice = _T("");
    }
    accountSettings.videoCodec = CS(m_layout.Combo(QStringLiteral("IDC_SETTINGS_VIDEO_CODEC"))->currentText());
    if (accountSettings.videoCodec == Translate(_T("Default")))
    {
        accountSettings.videoCodec = _T("");
    }
    accountSettings.videoH264 = m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_H264"))->isChecked();
    accountSettings.videoH263 = m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_H263"))->isChecked();
    accountSettings.videoVP8 = m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_VP8"))->isChecked();
    accountSettings.videoVP9 = m_layout.Check(QStringLiteral("IDC_SETTINGS_VIDEO_VP9"))->isChecked();
    str = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_VIDEO_BITRATE"))->text());
    accountSettings.videoBitrate = _wtoi(str);
#endif

    accountSettings.rport = m_layout.Check(QStringLiteral("IDC_SETTINGS_RPORT"))->isChecked();
    str = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_SOURCE_PORT"))->text());
    accountSettings.sourcePort = _wtoi(str);
    str = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RTP_PORT_MIN"))->text());
    accountSettings.rtpPortMin = _wtoi(str);
    str = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RTP_PORT_MAX"))->text());
    accountSettings.rtpPortMax = _wtoi(str);

    accountSettings.dnsSrvNs = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_DNS_SRV_NS"))->text());
    accountSettings.dnsSrvNs.Trim();
    if (!accountSettings.dnsSrvNs.IsEmpty()) {
        accountSettings.dnsSrv = m_layout.Check(QStringLiteral("IDC_SETTINGS_DNS_SRV_CHECKBOX"))->isChecked();
    }
    else {
        accountSettings.dnsSrv = false;
    }

    accountSettings.stun = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_STUN"))->text());
    accountSettings.stun.Trim();
    if (!accountSettings.stun.IsEmpty()) {
        accountSettings.enableSTUN = m_layout.Check(QStringLiteral("IDC_SETTINGS_STUN_CHECKBOX"))->isChecked();
    }
    else {
        accountSettings.enableSTUN = false;
    }

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_DTMF_METHOD"));
    accountSettings.DTMFMethod = combobox->currentIndex();

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_AUTO_ANSWER"));
    accountSettings.autoAnswer = autoAnswerValues.GetAt(combobox->currentIndex());

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_FWD"));
    accountSettings.forwarding = forwardingValues.GetAt(combobox->currentIndex());
    accountSettings.forwardingNumber = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_FWD_NUMBER"))->text());
    str = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_FWD_DELAY"))->text());
    accountSettings.forwardingDelay = _wtoi(str);

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_DENY_INCOMING"));
    accountSettings.denyIncoming = denyIncomingValues.GetAt(combobox->currentIndex());

    accountSettings.usersDirectory = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_DIRECTORY"))->text());
    accountSettings.usersDirectory.Trim();
    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_DEFAULT_ACTION"));
    accountSettings.defaultAction = defaultActionItems[combobox->currentIndex()];
    accountSettings.enableMediaButtons = m_layout.Check(QStringLiteral("IDC_SETTINGS_MEDIA_BUTTONS"))->isChecked();
    accountSettings.headsetSupport = m_layout.Check(QStringLiteral("IDC_SETTINGS_HID"))->isChecked();
    accountSettings.localDTMF = m_layout.Check(QStringLiteral("IDC_SETTINGS_LOCAL_DTMF"))->isChecked();
    accountSettings.singleMode = m_layout.Check(QStringLiteral("IDC_SETTINGS_SINGLE_MODE"))->isChecked();
    accountSettings.enableLog = m_layout.Check(QStringLiteral("IDC_SETTINGS_ENABLE_LOG"))->isChecked();
    accountSettings.bringToFrontOnIncoming = m_layout.Check(QStringLiteral("IDC_SETTINGS_BRING_TO_FRONT"))->isChecked();
    accountSettings.randomAnswerBox = m_layout.Check(QStringLiteral("IDC_SETTINGS_ANSWER_BOX_RANDOM"))->isChecked();
    accountSettings.callWaiting = m_layout.Check(QStringLiteral("IDC_SETTINGS_CALL_WAITING"))->isChecked();
    accountSettings.multiMonitor = m_layout.Check(QStringLiteral("IDC_SETTINGS_MULTI_MONITOR"))->isChecked();
    accountSettings.networkChanges = m_layout.Check(QStringLiteral("IDC_SETTINGS_NETWORK_CHANGES"))->isChecked();
    accountSettings.disableMessaging = m_layout.Check(QStringLiteral("IDC_SETTINGS_DISMESS"))->isChecked();
    accountSettings.disableNameLookup = m_layout.Check(QStringLiteral("IDC_SETTINGS_DISABLE_NAME_LOOKUP"))->isChecked();
    accountSettings.ringtone = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE"))->text());
    accountSettings.volumeRing = static_cast<QSlider*>(DLG_ITEM(m_layout, IDC_SETTINGS_VOLUME_RING))->value();
    accountSettings.recordingPath = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RECORDING"))->text());
    accountSettings.recordingPath.Trim();
    accountSettings.recordingFormat = static_cast<QRadioButton*>(DLG_ITEM(m_layout, IDC_SETTINGS_RECORDING_MP3))->isChecked() ? _T("mp3") : _T("wav");
    accountSettings.autoRecording = m_layout.Check(QStringLiteral("IDC_SETTINGS_RECORDING_CHECKBOX"))->isChecked();
    accountSettings.recordingButton = m_layout.Check(QStringLiteral("IDC_SETTINGS_RECORDING_BUTTON"))->isChecked();
    accountSettings.enableLocalAccount = m_layout.Check(QStringLiteral("IDC_SETTINGS_ENABLE_LOCAL"))->isChecked();

    combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_UPDATES_INTERVAL"));
    i = combobox->currentIndex();
    switch (i) {
    case 0:
        accountSettings.updatesInterval = _T("daily");
        break;
    case 2:
        accountSettings.updatesInterval = _T("monthly");
        break;
    case 3:
        accountSettings.updatesInterval = _T("quarterly");
        break;
    case 4:
        accountSettings.updatesInterval = _T("never");
        break;
    default:
        accountSettings.updatesInterval = _T("");
    }

    msip_startup_set(m_layout.Check(QStringLiteral("IDC_SETTINGS_STARTUP"))->isChecked());

    accountSettings.SettingsSave();

    if (accountSettings.singleMode) {
        MainDlg()->messagesDlg->OnClose();
    }
#ifdef _GLOBAL_VIDEO
    MainDlg()->messagesDlg->UpdateCallButton();
#endif
    if (QWidget* message = MainDlg()->messagesDlg->findChild<QWidget*>(QStringLiteral("IDC_MESSAGE"))) {
        message->setEnabled(!accountSettings.disableMessaging);
    }

    MainDlg()->pageDialer->RebuildButtons();
    MainDlg()->pageDialer->UpdateCallButton();
    MainDlg()->OnRecreate(0, 0);
    OnClose();
    return 0;
}

void SettingsDlg::OnBnClickedBrowse()
{
    QString file = QFileDialog::getOpenFileName(this, TR(_T("Ringtone")), QString(), QStringLiteral("WAV Files (*.wav)"));
    if (!file.isEmpty()) {
        // arquivo na pasta atual → só o nome (como no original)
        if (QFileInfo(file).absolutePath() == QDir::currentPath()) {
            m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE"))->setText(QFileInfo(file).fileName());
        }
        else {
            m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE"))->setText(file);
        }
    }
}

void SettingsDlg::OnChangeRingtone()
{
    CString str = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE"))->text());
    DLG_ITEM(m_layout, IDC_SETTINGS_DEFAULT)->setEnabled(str.GetLength() > 0);
}

void SettingsDlg::OnBnClickedDefault()
{
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE"))->setText(QString());
}

void SettingsDlg::OnRingVolumeReleased()
{
    MainDlg()->PlayerStop();
    int volumeRingOld = accountSettings.volumeRing;
    CString audioRingDeviceOld = accountSettings.audioRingDevice;
    accountSettings.volumeRing = static_cast<QSlider*>(DLG_ITEM(m_layout, IDC_SETTINGS_VOLUME_RING))->value();
    accountSettings.audioRingDevice = CS(m_layout.Combo(QStringLiteral("IDC_SETTINGS_RING"))->currentText());
    MainDlg()->UpdateSoundDevicesIds();
    CString ringtone = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RINGTONE"))->text());
    if (ringtone.IsEmpty()) {
        ringtone = _T("ringtone.wav");
    }
    MainDlg()->onPlayerPlay(MSIP_SOUND_CUSTOM_NOLOOP, (LPARAM)&ringtone);
    accountSettings.volumeRing = volumeRingOld;
    accountSettings.audioRingDevice = audioRingDeviceOld;
    MainDlg()->UpdateSoundDevicesIds();
}

void SettingsDlg::OnBnClickedRecordingBrowse()
{
    QString str = m_layout.Edit(QStringLiteral("IDC_SETTINGS_RECORDING"))->text();
    QString strOutFolder;
    if (str.isEmpty() || QDir::isRelativePath(str)) {
        strOutFolder = QDir::currentPath();
        if (!str.isEmpty()) {
            strOutFolder += QLatin1Char('/') + str;
        }
    }
    else {
        strOutFolder = str;
    }
    // CShellManager::BrowseForFolder → QFileDialog
    QString folder = QFileDialog::getExistingDirectory(this, TR(_T("Call Recording")), strOutFolder);
    if (!folder.isEmpty()) {
        m_layout.Edit(QStringLiteral("IDC_SETTINGS_RECORDING"))->setText(folder);
    }
}

void SettingsDlg::OnEnChangeRecording()
{
    CString str = CS(m_layout.Edit(QStringLiteral("IDC_SETTINGS_RECORDING"))->text());
    DLG_ITEM(m_layout, IDC_SETTINGS_RECORDING_DEFAULT)->setEnabled(str.GetLength() > 0);
}

void SettingsDlg::OnBnClickedRecordingDefault()
{
    m_layout.Edit(QStringLiteral("IDC_SETTINGS_RECORDING"))->setText(QString());
}

void SettingsDlg::OnBnClickedAAOptions()
{
    if (!aaOptionsDlg) {
        aaOptionsDlg = new AAOptionsDlg(this);
    }
    else {
        aaOptionsDlg->raise();
        aaOptionsDlg->activateWindow();
    }
}

void SettingsDlg::OnDeltaposSpinModify(int iDelta)
{
    QListWidget* listbox = m_codecsAll;
    QListWidget* listbox2 = m_codecs;
    if (iDelta == -1) {
        //add
        int selected = listbox->currentRow();
        if (selected != -1)
        {
            QString str = listbox->item(selected)->text();
            listbox2->addItem(str);
            delete listbox->takeItem(selected);
            listbox->setCurrentRow(selected < listbox->count() ? selected : selected - 1);
        }
    }
    else {
        //remove
        int selected = listbox2->currentRow();
        if (selected != -1)
        {
            QString str = listbox2->item(selected)->text();
            listbox->addItem(str);
            delete listbox2->takeItem(selected);
            listbox2->setCurrentRow(selected < listbox2->count() ? selected : selected - 1);
        }
    }
}

void SettingsDlg::OnDeltaposSpinOrder(int iDelta)
{
    QListWidget* listbox2 = m_codecs;
    int selected = listbox2->currentRow();
    if (selected != -1)
    {
        if (iDelta == -1) {
            //up
            if (selected > 0)
            {
                QListWidgetItem* item = listbox2->takeItem(selected);
                listbox2->insertItem(selected - 1, item);
                listbox2->setCurrentRow(selected - 1);
            }
        }
        else {
            //down
            if (selected < listbox2->count() - 1)
            {
                QListWidgetItem* item = listbox2->takeItem(selected);
                listbox2->insertItem(selected + 1, item);
                listbox2->setCurrentRow(selected + 1);
            }
        }
    }
}

void SettingsDlg::OnNMClickSyslinkFeatureCodes()
{
    if (!featureCodesDlg) {
        featureCodesDlg = new FeatureCodesDlg(this);
    }
    else {
        featureCodesDlg->raise();
        featureCodesDlg->activateWindow();
    }
}

void SettingsDlg::OnBnClickedPreview()
{
    QComboBox* combobox = m_layout.Combo(QStringLiteral("IDC_SETTINGS_VID_CAP_DEV"));
    CString name = CS(combobox->currentText());
    if (!MainDlg()->previewWin) {
        MainDlg()->previewWin = new Preview(MainDlg());
    }
    MainDlg()->previewWin->Start(MainDlg()->VideoCaptureDeviceId(name));
}
