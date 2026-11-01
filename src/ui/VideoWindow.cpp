/*
 * MySimpleSIP — renderizador de vídeo do pjmedia em janelas Qt (no lugar do SDL).
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "VideoWindow.h"

#include "UiCommon.h"

#include <QApplication>
#include <QCloseEvent>
#include <QPainter>
#include <QThread>

#include <functional>
#include <new>
#include <set>

#include <pjmedia-videodev/videodev_imp.h>
#include <pjmedia/videodev.h>
#include <pjsua-lib/pjsua.h>

#define THIS_FILE "VideoWindow.cpp"

// renderizador SDL do pjproject (sdl_dev.c), retirado ao registrar o nosso
extern "C" pjmedia_vid_dev_factory* pjmedia_sdl_factory(pj_pool_factory* pf);

namespace {

std::set<VideoWindow*> g_windows; // só na thread da interface

// executa na thread da interface: direto se já estiver nela, senão enfileirado
void RunOnGui(std::function<void()> fn)
{
    if (!qApp) {
        return;
    }
    if (QThread::currentThread() == qApp->thread()) {
        fn();
    }
    else {
        QMetaObject::invokeMethod(qApp, std::move(fn), Qt::QueuedConnection);
    }
}

void ApplyWindowState(const std::shared_ptr<VideoSink>& sink)
{
    VideoWindow* w = sink->window;
    if (!w) {
        return;
    }
    bool hide;
    QSize size;
    {
        std::lock_guard<std::mutex> lock(sink->mutex);
        hide = sink->hide;
        size = sink->size;
    }
    if (!w->parentWidget() && size.isValid()) {
        w->resize(size);
    }
    w->setVisible(!hide);
}

// ------------------------------------------------------------------ fábrica do pjmedia

constexpr unsigned kClockRate = 90000;
constexpr int kDefaultWidth = 640;
constexpr int kDefaultHeight = 480;
constexpr int kDefaultFps = 25;

struct QtFactory {
    pjmedia_vid_dev_factory base;
    pj_pool_t* pool;
    pj_pool_factory* pf;
    pjmedia_vid_dev_info info;
};

struct QtStream {
    pjmedia_vid_dev_stream base;
    pj_pool_t* pool;
    pjmedia_vid_dev_param param;
    std::shared_ptr<VideoSink>* sink; // no pool: shared_ptr alocado com new
};

pj_status_t factory_init(pjmedia_vid_dev_factory* f)
{
    auto* qf = reinterpret_cast<QtFactory*>(f);
    pj_bzero(&qf->info, sizeof(qf->info));
    pj_ansi_strxcpy(qf->info.name, "Qt renderer", sizeof(qf->info.name));
    pj_ansi_strxcpy(qf->info.driver, "Qt", sizeof(qf->info.driver));
    qf->info.dir = PJMEDIA_DIR_RENDER;
    qf->info.has_callback = PJ_FALSE;
    qf->info.caps = PJMEDIA_VID_DEV_CAP_FORMAT | PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW | PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE |
                    PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION | PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE |
                    PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS | PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN;
    qf->info.fmt_cnt = 1;
    pjmedia_format_init_video(&qf->info.fmt[0], PJMEDIA_FORMAT_BGRA, kDefaultWidth, kDefaultHeight, kDefaultFps, 1);
    PJ_LOG(4, (THIS_FILE, "Qt video renderer initialized"));
    return PJ_SUCCESS;
}

pj_status_t factory_destroy(pjmedia_vid_dev_factory* f)
{
    auto* qf = reinterpret_cast<QtFactory*>(f);
    pj_pool_t* pool = qf->pool;
    qf->pool = nullptr;
    pj_pool_release(pool);
    return PJ_SUCCESS;
}

pj_status_t factory_refresh(pjmedia_vid_dev_factory*)
{
    return PJ_SUCCESS;
}

unsigned factory_get_dev_count(pjmedia_vid_dev_factory*)
{
    return 1;
}

pj_status_t factory_get_dev_info(pjmedia_vid_dev_factory* f, unsigned index, pjmedia_vid_dev_info* info)
{
    PJ_ASSERT_RETURN(index == 0, PJMEDIA_EVID_INVDEV);
    pj_memcpy(info, &reinterpret_cast<QtFactory*>(f)->info, sizeof(*info));
    return PJ_SUCCESS;
}

pj_status_t factory_default_param(pj_pool_t*, pjmedia_vid_dev_factory* f, unsigned index, pjmedia_vid_dev_param* param)
{
    PJ_ASSERT_RETURN(index == 0, PJMEDIA_EVID_INVDEV);
    pj_bzero(param, sizeof(*param));
    param->dir = PJMEDIA_DIR_RENDER;
    param->rend_id = index;
    param->cap_id = PJMEDIA_VID_INVALID_DEV;
    param->flags = PJMEDIA_VID_DEV_CAP_FORMAT;
    param->clock_rate = kClockRate;
    pj_memcpy(&param->fmt, &reinterpret_cast<QtFactory*>(f)->info.fmt[0], sizeof(param->fmt));
    return PJ_SUCCESS;
}

void SetFormat(QtStream* s, const pjmedia_format* fmt)
{
    pjmedia_format_copy(&s->param.fmt, fmt);
}

void SetWindowFlags(VideoSink& sink, unsigned flags)
{
    sink.border = flags & PJMEDIA_VID_DEV_WND_BORDER;
    sink.resizable = flags & PJMEDIA_VID_DEV_WND_RESIZABLE;
}

pj_status_t stream_get_cap(pjmedia_vid_dev_stream* strm, pjmedia_vid_dev_cap cap, void* pval);

pj_status_t stream_get_param(pjmedia_vid_dev_stream* strm, pjmedia_vid_dev_param* pi)
{
    auto* s = reinterpret_cast<QtStream*>(strm);
    pj_memcpy(pi, &s->param, sizeof(*pi));
    // pjsua_vid_win_get_info lê janela, visibilidade e tamanho daqui
    stream_get_cap(strm, PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW, &pi->window);
    stream_get_cap(strm, PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE, &pi->window_hide);
    stream_get_cap(strm, PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE, &pi->disp_size);
    pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW | PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE | PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE;
    return PJ_SUCCESS;
}

pj_status_t stream_get_cap(pjmedia_vid_dev_stream* strm, pjmedia_vid_dev_cap cap, void* pval)
{
    auto* s = reinterpret_cast<QtStream*>(strm);
    VideoSink& sink = **s->sink;
    switch (cap) {
    case PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW: {
        auto* hwnd = static_cast<pjmedia_vid_dev_hwnd*>(pval);
        pj_bzero(hwnd, sizeof(*hwnd));
        hwnd->type = PJMEDIA_VID_DEV_HWND_TYPE_NONE;
        // a janela só existe já se o fluxo foi criado na thread da interface (prévia)
        hwnd->info.window = (QThread::currentThread() == qApp->thread()) ? static_cast<QWidget*>(sink.window.data()) : nullptr;
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE: {
        std::lock_guard<std::mutex> lock(sink.mutex);
        *static_cast<pj_bool_t*>(pval) = sink.hide;
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE: {
        std::lock_guard<std::mutex> lock(sink.mutex);
        auto* size = static_cast<pjmedia_rect_size*>(pval);
        size->w = sink.size.width();
        size->h = sink.size.height();
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION:
        // MySimpleSIP: no Wayland o compositor decide a posição
        pj_bzero(pval, sizeof(pjmedia_coord));
        return PJ_SUCCESS;
    case PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS: {
        std::lock_guard<std::mutex> lock(sink.mutex);
        *static_cast<unsigned*>(pval) = (sink.border ? PJMEDIA_VID_DEV_WND_BORDER : 0) |
                                        (sink.resizable ? PJMEDIA_VID_DEV_WND_RESIZABLE : 0);
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_FORMAT:
        pjmedia_format_copy(static_cast<pjmedia_format*>(pval), &s->param.fmt);
        return PJ_SUCCESS;
    default:
        return PJMEDIA_EVID_INVCAP;
    }
}

pj_status_t stream_set_cap(pjmedia_vid_dev_stream* strm, pjmedia_vid_dev_cap cap, const void* pval)
{
    auto* s = reinterpret_cast<QtStream*>(strm);
    std::shared_ptr<VideoSink> sink = *s->sink;
    switch (cap) {
    case PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE: {
        {
            std::lock_guard<std::mutex> lock(sink->mutex);
            sink->hide = *static_cast<const pj_bool_t*>(pval);
        }
        s->param.window_hide = *static_cast<const pj_bool_t*>(pval);
        RunOnGui([sink] { ApplyWindowState(sink); });
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE: {
        const auto* size = static_cast<const pjmedia_rect_size*>(pval);
        {
            std::lock_guard<std::mutex> lock(sink->mutex);
            sink->size = QSize((int) size->w, (int) size->h);
        }
        s->param.disp_size = *size;
        RunOnGui([sink] { ApplyWindowState(sink); });
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION:
        s->param.window_pos = *static_cast<const pjmedia_coord*>(pval);
        return PJ_SUCCESS;
    case PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS: {
        std::lock_guard<std::mutex> lock(sink->mutex);
        SetWindowFlags(*sink, *static_cast<const unsigned*>(pval));
        s->param.window_flags = *static_cast<const unsigned*>(pval);
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_OUTPUT_FULLSCREEN: {
        const bool full = *static_cast<const pjmedia_vid_dev_fullscreen_flag*>(pval) != PJMEDIA_VID_DEV_WINDOWED;
        RunOnGui([sink, full] {
            if (VideoWindow* w = sink->window) {
                full ? w->showFullScreen() : w->showNormal();
            }
        });
        return PJ_SUCCESS;
    }
    case PJMEDIA_VID_DEV_CAP_FORMAT: {
        const auto* fmt = static_cast<const pjmedia_format*>(pval);
        if (fmt->id != PJMEDIA_FORMAT_BGRA) {
            return PJMEDIA_EVID_BADFORMAT;
        }
        SetFormat(s, fmt);
        return PJ_SUCCESS;
    }
    default:
        return PJMEDIA_EVID_INVCAP;
    }
}

pj_status_t stream_start(pjmedia_vid_dev_stream*)
{
    return PJ_SUCCESS;
}

pj_status_t stream_stop(pjmedia_vid_dev_stream*)
{
    return PJ_SUCCESS;
}

pj_status_t stream_put_frame(pjmedia_vid_dev_stream* strm, const pjmedia_frame* frame)
{
    auto* s = reinterpret_cast<QtStream*>(strm);
    if (frame->type != PJMEDIA_FRAME_TYPE_VIDEO || !frame->buf || !frame->size) {
        return PJ_SUCCESS;
    }
    const int w = (int) s->param.fmt.det.vid.size.w;
    const int h = (int) s->param.fmt.det.vid.size.h;
    if (w <= 0 || h <= 0 || frame->size < (pj_size_t) w * h * 4) {
        return PJ_SUCCESS;
    }
    std::shared_ptr<VideoSink> sink = *s->sink;
    bool post;
    {
        std::lock_guard<std::mutex> lock(sink->mutex);
        if (sink->frame.width() != w || sink->frame.height() != h) {
            sink->frame = QImage(w, h, QImage::Format_RGB32);
        }
        for (int y = 0; y < h; y++) {
            memcpy(sink->frame.scanLine(y), static_cast<const uchar*>(frame->buf) + (size_t) y * w * 4, (size_t) w * 4);
        }
        post = !sink->updatePending;
        sink->updatePending = true;
    }
    if (post) {
        RunOnGui([sink] {
            {
                std::lock_guard<std::mutex> lock(sink->mutex);
                sink->updatePending = false;
            }
            if (VideoWindow* w = sink->window) {
                w->update();
            }
        });
    }
    return PJ_SUCCESS;
}

pj_status_t stream_destroy(pjmedia_vid_dev_stream* strm)
{
    auto* s = reinterpret_cast<QtStream*>(strm);
    std::shared_ptr<VideoSink> sink = *s->sink;
    delete s->sink;
    s->sink = nullptr;
    RunOnGui([sink] {
        if (VideoWindow* w = sink->window) {
            w->hide();
            w->deleteLater();
        }
        sink->window = nullptr;
    });
    pj_pool_release(s->pool);
    return PJ_SUCCESS;
}

pjmedia_vid_dev_stream_op stream_op = {
    &stream_get_param, &stream_get_cap, &stream_set_cap, &stream_start,
    nullptr, // get_frame (renderizador)
    &stream_put_frame, &stream_stop, &stream_destroy,
};

pj_status_t factory_create_stream(pjmedia_vid_dev_factory* f, pjmedia_vid_dev_param* param, const pjmedia_vid_dev_cb*,
                                  void*, pjmedia_vid_dev_stream** p_vid_strm)
{
    auto* qf = reinterpret_cast<QtFactory*>(f);
    PJ_ASSERT_RETURN(param->dir == PJMEDIA_DIR_RENDER, PJ_EINVAL);
    if (param->fmt.id != PJMEDIA_FORMAT_BGRA) {
        return PJMEDIA_EVID_BADFORMAT;
    }
    pj_pool_t* pool = pj_pool_create(qf->pf, "qt-vid%p", 1024, 1024, nullptr);
    PJ_ASSERT_RETURN(pool, PJ_ENOMEM);
    auto* s = PJ_POOL_ZALLOC_T(pool, QtStream);
    s->pool = pool;
    pj_memcpy(&s->param, param, sizeof(*param));
    auto sink = std::make_shared<VideoSink>();
    sink->size = QSize((int) param->fmt.det.vid.size.w, (int) param->fmt.det.vid.size.h);
    if (param->flags & PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE) {
        sink->size = QSize((int) param->disp_size.w, (int) param->disp_size.h);
    }
    if (param->flags & PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE) {
        sink->hide = param->window_hide;
    }
    if (param->flags & PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS) {
        SetWindowFlags(*sink, param->window_flags);
    }
    s->sink = new std::shared_ptr<VideoSink>(sink);
    RunOnGui([sink] {
        sink->window = new VideoWindow(sink);
        ApplyWindowState(sink);
    });
    s->base.op = &stream_op;
    *p_vid_strm = &s->base;
    return PJ_SUCCESS;
}

pjmedia_vid_dev_factory_op factory_op = {
    &factory_init, &factory_destroy, &factory_get_dev_count, &factory_get_dev_info,
    &factory_default_param, &factory_create_stream, &factory_refresh,
};

pjmedia_vid_dev_factory* qt_factory_create(pj_pool_factory* pf)
{
    pj_pool_t* pool = pj_pool_create(pf, "qt-video", 512, 512, nullptr);
    if (!pool) {
        return nullptr;
    }
    auto* f = PJ_POOL_ZALLOC_T(pool, QtFactory);
    f->pf = pf;
    f->pool = pool;
    f->base.op = &factory_op;
    return &f->base;
}

} // namespace

// ------------------------------------------------------------------ janela

VideoWindow::VideoWindow(std::shared_ptr<VideoSink> sink, QWidget* parent) : QWidget(parent), m_sink(std::move(sink))
{
    g_windows.insert(this);
    connect(this, &QObject::destroyed, [this] { g_windows.erase(this); });
    setAttribute(Qt::WA_OpaquePaintEvent);
    setWindowTitle(QStringLiteral("%1 — MySimpleSIP").arg(TR(_T("Video Call"))));
    setWindowIcon(QApplication::windowIcon());
    Qt::WindowFlags flags = Qt::Window;
    if (!m_sink->border) {
        flags |= Qt::FramelessWindowHint;
    }
    setWindowFlags(flags);
    if (!m_sink->resizable && m_sink->size.isValid()) {
        setFixedSize(m_sink->size);
    }
    setMinimumSize(64, 48);
}

QImage VideoWindow::currentFrame() const
{
    std::lock_guard<std::mutex> lock(m_sink->mutex);
    return m_sink->frame.copy();
}

void VideoWindow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    std::lock_guard<std::mutex> lock(m_sink->mutex);
    if (m_sink->frame.isNull()) {
        return;
    }
    // como o SDL, ocupa a janela; aqui mantendo a proporção do quadro
    QSize s = m_sink->frame.size().scaled(size(), Qt::KeepAspectRatio);
    QRect target(QPoint((width() - s.width()) / 2, (height() - s.height()) / 2), s);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(target, m_sink->frame);
    m_framesShown++;
}

void VideoWindow::closeEvent(QCloseEvent* event)
{
    // MySimpleSIP: o SDL encerra o renderizador ao fechar (PJMEDIA_EVENT_WND_CLOSING, que o
    // pjsua não trata); aqui a janela só se esconde
    event->ignore();
    hide();
    {
        std::lock_guard<std::mutex> lock(m_sink->mutex);
        m_sink->hide = true;
    }
}

bool msip_qt_video_install()
{
    if (pjmedia_vid_register_factory(&qt_factory_create, nullptr) != PJ_SUCCESS) {
        return false;
    }
    // o padrão de renderização é o primeiro renderizador da lista: retira o SDL
    pjmedia_vid_unregister_factory(&pjmedia_sdl_factory, nullptr);
    pjmedia_vid_dev_refresh();
    return true;
}

int msip_qt_video_window_count()
{
    return (int) g_windows.size();
}
