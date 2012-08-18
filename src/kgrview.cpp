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

#include <QDebug>

KGrView::KGrView    (QWidget * parent)
    :
    QGraphicsView   (parent),
    m_mouse         (new QCursor    ()),
    m_scene         (new KGrScene   (this))
{
    setScene        (m_scene);
}

KGrView::~KGrView ()
{
    delete m_mouse;
}

void KGrView::getMousePos (int & i, int & j)
{
    QPointF pos =  mapToScene (mapFromGlobal (m_mouse->pos()));

    i = pos.x();
    j = pos.y();

    // The window lacks keyboard focus or is minimized.
    if (! isActiveWindow()) {
        i = -2;
        j = -2;
        return;
    }

    // The pointer is outside the level layout.
    if (i < 1 || i > FIELDWIDTH || j < 1 || j > FIELDHEIGHT) {
        i = -1;
        j = -1;
        return;
    }
}

void KGrView::setMousePos (const int i, const int j)
{
    QPoint pos  = mapFromScene (i, j);

    // Puts the cursor at the middle of the starting cell.
    int s       = m_scene->tileSize().width() / 2;
    int x       = pos.x() + s;
    int y       = pos.y() + s;

    m_mouse->setPos( mapToGlobal ( QPoint (x, y)));
}

void KGrView::resizeEvent (QResizeEvent *)
{
    if (scene() != 0) {
        m_scene->changeSize ();
        fitInView (scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void KGrView::mousePressEvent (QMouseEvent * mouseEvent)
{
    emit mouseClick (mouseEvent->button());
}

void KGrView::mouseReleaseEvent (QMouseEvent * mouseEvent)
{
    emit mouseLetGo (mouseEvent->button());
}

#include "kgrview.moc"
