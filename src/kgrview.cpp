/****************************************************************************
 *    Copyright 2012  Roney Gomes <roney477@gmail.com>                      *
 *                                                                          *
 *    This program is free software; you can redistribute it and/or         *
 *    modify it under the terms of the GNU General Public License as        *
 *    published by the Free Software Foundation; either version 2 of        *
 *    the License, or (at your option) any later version.                   *
 *                                                                          *
 *    This program is distributed in the hope that it will be useful,       *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *    GNU General Public License for more details.                          *
 *                                                                          *
 *    You should have received a copy of the GNU General Public License     *
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ****************************************************************************/

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


