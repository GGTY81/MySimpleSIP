/*
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// MySimpleSIP: portado de Preview.cpp do MicroSIP 3.22.16. Alterações marcadas com
// "MySimpleSIP:". A janela da prévia é do renderizador Qt (VideoWindow), encaixada aqui como
// o original encaixa a janela do SDL com SetParent.

#include "Preview.h"

#ifdef _GLOBAL_VIDEO

#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>

// MySimpleSIP: a janela principal da interface (mainDlg é o CmainDlgCore do núcleo)
static CmainDlg* MainDlg()
{
    return static_cast<CmainDlg*>(mainDlg);
}

Preview::Preview(QWidget* pParent /*=NULL*/)
    : QWidget(pParent, Qt::Window) // MySimpleSIP: Create(IDD, pParent) → janela Qt
{
    if (langPack.rtl) {
        setLayoutDirection(Qt::RightToLeft);
    }
    m_layout.Build(this, QStringLiteral("IDD_PREVIEW"));
    widx = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
    OnInitDialog();
}


Preview::~Preview(void)
{
    // MySimpleSIP: destruída junto com a janela principal sem passar pelo OnClose
    if (!m_destroyed) {
        OnDestroy();
    }
}

void Preview::OnInitDialog()
{
    m_layout.TranslateDialog(this);
}

void Preview::closeEvent(QCloseEvent* event)
{
    OnClose();
    event->accept();
}

void Preview::OnClose()
{
    // DestroyWindow → OnDestroy → PostNcDestroy (delete this)
    if (m_destroyed) {
        return;
    }
    OnDestroy();
    hide();
    deleteLater();
}

void Preview::OnDestroy()
{
    m_destroyed = true;
    if (MainDlg() && MainDlg()->previewWin == this) {
        MainDlg()->previewWin = nullptr;
    }
    if (is_pjsua_running()) {
        videoUsedTime = CTime::GetCurrentTime().GetTime();
        pjsua_vid_preview_stop(widx);
    }
}

void Preview::Start(int id)
{
    if (!is_pjsua_running()) {
        OnClose();
        return;
    }
    videoUsedTime = CTime::GetCurrentTime().GetTime();
    pjsua_vid_win_info wi;
    pjsua_vid_win_id wid = pjsua_vid_preview_get_win(widx);
    if (wid != PJSUA_INVALID_ID) {
        if (widx != id) {
            pjsua_vid_preview_stop(widx);
        }
        else {
            return;
        }
    }
    widx = id;
    pjsua_vid_preview_param pre_param;
    pj_status_t status;
    const pjmedia_coord pos = { 0, 0 };
    pjsua_vid_preview_param_default(&pre_param);
    //pre_param.show = PJ_FALSE;
    status = pjsua_vid_preview_start(widx, &pre_param);
    if (status != PJ_SUCCESS) {
        OnClose();
        return;
    }
    wid = pjsua_vid_preview_get_win(widx);
    pjsua_vid_win_get_info(wid, &wi);
    // MySimpleSIP: a moldura fica com o compositor; só a área de vídeo é dimensionada
    QRect screenRect = QGuiApplication::primaryScreen()->availableGeometry();
    //int w = wi.size.w + ptDiff.x;
    //int h = wi.size.h + ptDiff.y;
    int w = 320;
    int h = 240;
    int x = screenRect.right() + 1 - w;
    int y = screenRect.top();
    setFixedSize(w, h);
    move(x, y); // MySimpleSIP: no Wayland o compositor decide a posição
    show();
    pjsua_vid_win_set_pos(wid, &pos);
    //pjsua_vid_win_set_show(wid, PJ_TRUE);
    const pjmedia_rect_size size = { 320, 240 };
    pjsua_vid_win_set_size(wid, &size);
    // ::SetParent(hwnd, m_hWnd): a janela do renderizador Qt vira filha deste diálogo
    m_video = static_cast<QWidget*>(wi.hwnd.info.window);
    if (m_video) {
        m_video->setParent(this, Qt::Widget);
        m_video->setGeometry(0, 0, w, h);
        m_video->show();
    }
}
#endif
