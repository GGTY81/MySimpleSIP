/*
 * MySimpleSIP — renderizador de vídeo do pjmedia em janelas Qt (no lugar do SDL).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: no Windows o MicroSIP mostra o vídeo nas janelas do renderizador SDL do
// pjmedia (vídeo remoto em janela própria, com borda e redimensionável) e encaixa a janela da
// prévia no diálogo IDD_PREVIEW com SetParent. No Wayland uma janela do SDL não pode ser
// encaixada em outra, então o renderizador é um dispositivo de vídeo próprio do pjmedia
// ("Qt renderer") cujas janelas são QWidgets: o pjsua continua criando, mostrando,
// escondendo e redimensionando as janelas pelas mesmas APIs (pjsua_vid_win_*), e
// pjsua_vid_win_info.hwnd.info.window aponta para o QWidget (o "HWND" da janela).
//
// Os quadros chegam na thread do relógio de vídeo do pjmedia já convertidos para BGRA (o
// vid_port converte do formato do decodificador ou da câmera) e são copiados para um
// buffer; a janela é atualizada na thread da interface.

#pragma once

#include <QImage>
#include <QPointer>
#include <QWidget>

#include <memory>
#include <mutex>

struct VideoSink;

class VideoWindow : public QWidget {
    Q_OBJECT
public:
    explicit VideoWindow(std::shared_ptr<VideoSink> sink, QWidget* parent = nullptr);
    // último quadro mostrado (testes)
    QImage currentFrame() const;
    int framesShown() const { return m_framesShown; }

protected:
    void paintEvent(QPaintEvent* event) override;
    // fechar só esconde (a chamada continua; o pjsua pode mostrar de novo)
    void closeEvent(QCloseEvent* event) override;

private:
    friend struct VideoSink;
    std::shared_ptr<VideoSink> m_sink;
    int m_framesShown = 0;
};

struct VideoSink {
    std::mutex mutex;
    QImage frame; // BGRA = QImage::Format_RGB32 (little-endian)
    bool hide = false;
    bool border = true;
    bool resizable = true;
    QSize size;
    QPointer<VideoWindow> window;
    bool updatePending = false;
};

// Registra o renderizador no pjmedia e retira o do SDL (depois do pjsua_init, como o
// backend de áudio); o padrão de renderização passa a ser o "Qt renderer".
bool msip_qt_video_install();
// Janelas de vídeo abertas (testes)
int msip_qt_video_window_count();
