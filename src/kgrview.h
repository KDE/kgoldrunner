/*
    SPDX-FileCopyrightText: 2012 Roney Gomes <roney477@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
    explicit KGrView (QWidget * parent);
    ~KGrView         () override;

    /*
     * Get a pointer to the game scene.
     */
    KGrScene *  gameScene   () const { return m_scene; }

Q_SIGNALS:
    void mouseClick (int);
    void mouseLetGo (int);

protected:
    void resizeEvent           (QResizeEvent   *) override;
    void mousePressEvent       (QMouseEvent * mouseEvent) override;
    void mouseDoubleClickEvent (QMouseEvent * mouseEvent) override;
    void mouseReleaseEvent     (QMouseEvent * mouseEvent) override;

private:
    KGrScene    * m_scene;
};

#endif // KGRVIEW_H
