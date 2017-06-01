/****************************************************************************
 *    Copyright 2012 Roney Gomes <roney477@gmail.com>                       *
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

#ifndef KGRVIEW_H
#define KGRVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QResizeEvent>

class KGrScene;

class KGrView : public QGraphicsView
{
    Q_OBJECT
public:
    KGrView     (QWidget * parent);
    ~KGrView    ();

    /*
     * Get a pointer to the game scene.
     */
    KGrScene *  gameScene   () const { return m_scene; }

signals:
    void mouseClick (int);
    void mouseLetGo (int);

protected:
    void resizeEvent           (QResizeEvent   *) Q_DECL_OVERRIDE;
    void mousePressEvent       (QMouseEvent * mouseEvent) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent (QMouseEvent * mouseEvent) Q_DECL_OVERRIDE;
    void mouseReleaseEvent     (QMouseEvent * mouseEvent) Q_DECL_OVERRIDE;

private:
    KGrScene    * m_scene;
};

#endif // KGRVIEW_H
