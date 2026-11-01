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

// MySimpleSIP: portado de Preview.h do MicroSIP 3.22.16 (janela "Vídeo local", prévia da
// câmera). Alterações marcadas com "MySimpleSIP:".

#pragma once

#include "global.h"

#ifdef _GLOBAL_VIDEO

#include "DialogLayout.h"

#include <pjsua-lib/pjsua.h>

#include <QWidget>

class Preview : public QWidget // MySimpleSIP: CThemeDialog → QWidget (o Qt segue o tema)
{
public:
    Preview(QWidget* pParent = NULL);
    virtual ~Preview();

    void Start(int id);
    void OnClose();
    // MySimpleSIP: janela do renderizador encaixada (testes)
    QWidget* VideoWidget() const { return m_video; }
private:
    pjmedia_vid_dev_index widx;
    CDialogLayout m_layout;
    QWidget* m_video = nullptr;
    bool m_destroyed = false;
protected:
    void OnInitDialog();
    void OnDestroy();
    void closeEvent(QCloseEvent* event) override;
};
#endif
