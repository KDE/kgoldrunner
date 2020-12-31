/*
    SPDX-FileCopyrightText: 2012 Roney Gomes <roney477@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kgrview.h"
#include "kgrscene.h"
#include "kgrglobals.h"
#include "kgrrenderer.h"

#include "kgoldrunner_debug.h"

KGrView::KGrView    (QWidget * parent)
    :
    QGraphicsView   (parent),
    m_scene         (new KGrScene   (this))
{
    setScene        (m_scene);
}

KGrView::~KGrView ()
{
}

void KGrView::resizeEvent (QResizeEvent *)
{
    if (scene() != nullptr) {
        m_scene->changeSize ();
        fitInView (scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void KGrView::mousePressEvent (QMouseEvent * mouseEvent)
{
    Q_EMIT mouseClick (mouseEvent->button());
}

void KGrView::mouseDoubleClickEvent (QMouseEvent * mouseEvent)
{
    Q_EMIT mouseClick (mouseEvent->button());
}

void KGrView::mouseReleaseEvent (QMouseEvent * mouseEvent)
{
    Q_EMIT mouseLetGo (mouseEvent->button());
}


