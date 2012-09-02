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

#include <QCursor>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QGraphicsView>

class KGrScene;

class KGrView : public QGraphicsView
{
    Q_OBJECT
public:
    KGrView     (QWidget * parent);
    ~KGrView    ();

    /*
     * Get the cursor's position in this widget coordinates.
     */
    QPointF     mousePos    ();

    /*
     * Get a pointer to the game scene.
     */
    KGrScene *  gameScene   () const { return m_scene; }

public slots:
    void getMousePos (int & i, int & j);
    void setMousePos (const int i, const int j);

signals:
    void mouseClick (int);
    void mouseLetGo (int);

protected:
    virtual void resizeEvent           (QResizeEvent   *);
    virtual void mousePressEvent       (QMouseEvent * mouseEvent);
    virtual void mouseDoubleClickEvent (QMouseEvent * mouseEvent);
    virtual void mouseReleaseEvent     (QMouseEvent * mouseEvent);

private:
    QCursor     * m_mouse;
    KGrScene    * m_scene;
};

#endif // KGRVIEW_H
