/****************************************************************************
 *    Copyright 2012  Ian Wadham <iandw.au@gmail.com>                       *
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
#include "kgrrenderer.h"

#include <QDebug>

KGrView::KGrView (QWidget * parent)
    :
    QGraphicsView   (parent),
    m_scene         (new KGrScene (parent))
{
    setScene (m_scene);
}

KGrView::~KGrView ()
{
}

void KGrView::keyReleaseEvent (QKeyEvent * event)
{
    if (event->key() == Qt::Key_T)
        m_scene->renderer()->selectTheme();
}

void KGrView::resizeEvent (QResizeEvent * event)
{
    if (scene() != 0) {
        m_scene->redrawScene ();
        fitInView (scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}
