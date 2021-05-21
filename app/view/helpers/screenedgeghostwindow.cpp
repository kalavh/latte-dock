/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>

    This file is part of Latte-Dock

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "screenedgeghostwindow.h"

// local
#include "../view.h"

// Qt
#include <QDebug>
#include <QSurfaceFormat>
#include <QQuickView>
#include <QTimer>

// KDE
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>

// X11
#include <NETWM>

namespace Latte {
namespace ViewPart {

ScreenEdgeGhostWindow::ScreenEdgeGhostWindow(Latte::View *view) :
    SubWindow(view, QString("Screen Ghost Window"))
{
    if (m_debugMode) {
        m_showColor = QColor("purple");
        m_hideColor = QColor("blue");
    } else {
        m_showColor = QColor(Qt::transparent);
        m_hideColor = QColor(Qt::transparent);

        m_showColor.setAlpha(0);
        m_hideColor.setAlpha(1);
    }

    setColor(m_showColor);

    //! this timer is used in order to avoid fast enter/exit signals during first
    //! appearing after edge activation
    m_delayedMouseTimer.setSingleShot(true);
    m_delayedMouseTimer.setInterval(50);
    connect(&m_delayedMouseTimer, &QTimer::timeout, this, [this]() {
        if (m_delayedContainsMouse) {
            setContainsMouse(true);
        } else {
            setContainsMouse(false);
        }
    });

    updateGeometry();
    hideWithMask();
}

ScreenEdgeGhostWindow::~ScreenEdgeGhostWindow()
{
}

QString ScreenEdgeGhostWindow::validTitlePrefix() const
{
    return QString("#subghostedge#");
}

void ScreenEdgeGhostWindow::updateGeometry()
{
    if (m_latteView->positioner()->slideOffset() != 0) {
        return;
    }

    QRect newGeometry;

    if (KWindowSystem::compositingActive()) {
        m_thickness = 6;
    } else {
        m_thickness = 2;
    }

    int length{30};
    int lengthDifference{0};

    if (m_latteView->formFactor() == Plasma::Types::Horizontal) {
        //! set minimum length to be 25% of screen width
        length = qMax(m_latteView->screenGeometry().width()/4,qMin(m_latteView->absoluteGeometry().width(), m_latteView->screenGeometry().width() - 1));
        lengthDifference = qMax(0,length - m_latteView->absoluteGeometry().width());
    } else {
        //! set minimum length to be 25% of screen height
        length = qMax(m_latteView->screenGeometry().height()/4,qMin(m_latteView->absoluteGeometry().height(), m_latteView->screenGeometry().height() - 1));
        lengthDifference = qMax(0,length - m_latteView->absoluteGeometry().height());
    }

    if (m_latteView->location() == Plasma::Types::BottomEdge) {
        int xF = qMax(m_latteView->screenGeometry().left(), m_latteView->absoluteGeometry().left() - lengthDifference);
        newGeometry.moveLeft(xF);
        newGeometry.moveTop(m_latteView->screenGeometry().bottom() - m_thickness);
    } else if (m_latteView->location() == Plasma::Types::TopEdge) {
        int xF = qMax(m_latteView->screenGeometry().left(), m_latteView->absoluteGeometry().left() - lengthDifference);
        newGeometry.moveLeft(xF);
        newGeometry.moveTop(m_latteView->screenGeometry().top());
    } else if (m_latteView->location() == Plasma::Types::LeftEdge) {
        int yF = qMax(m_latteView->screenGeometry().top(), m_latteView->absoluteGeometry().top() - lengthDifference);
        newGeometry.moveLeft(m_latteView->screenGeometry().left());
        newGeometry.moveTop(yF);
    } else if (m_latteView->location() == Plasma::Types::RightEdge) {
        int yF = qMax(m_latteView->screenGeometry().top(), m_latteView->absoluteGeometry().top() - lengthDifference);
        newGeometry.moveLeft(m_latteView->screenGeometry().right() - m_thickness);
        newGeometry.moveTop(yF);
    }

    if (m_latteView->formFactor() == Plasma::Types::Horizontal) {
        newGeometry.setWidth(length);
        newGeometry.setHeight(m_thickness + 1);
    } else {
        newGeometry.setWidth(m_thickness + 1);
        newGeometry.setHeight(length);
    }

    m_calculatedGeometry = newGeometry;

    emit calculatedGeometryChanged();
}

bool ScreenEdgeGhostWindow::containsMouse() const
{
    return m_containsMouse;
}

void ScreenEdgeGhostWindow::setContainsMouse(bool contains)
{
    if (m_containsMouse == contains) {
        return;
    }

    m_containsMouse = contains;
    emit containsMouseChanged(contains);
}

bool ScreenEdgeGhostWindow::event(QEvent *e)
{
    if (e->type() == QEvent::DragEnter || e->type() == QEvent::DragMove) {
        if (!m_containsMouse) {
            m_delayedContainsMouse = false;
            m_delayedMouseTimer.stop();
            setContainsMouse(true);
            emit dragEntered();
        }
    } else if (e->type() == QEvent::Enter) {
        m_delayedContainsMouse = true;
        if (!m_delayedMouseTimer.isActive()) {
            m_delayedMouseTimer.start();
        }
    } else if (e->type() == QEvent::Leave || e->type() == QEvent::DragLeave) {
        m_delayedContainsMouse = false;
        if (!m_delayedMouseTimer.isActive()) {
            m_delayedMouseTimer.start();
        }
    }

    return SubWindow::event(e);
}

}
}
