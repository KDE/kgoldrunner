#ifndef KGRVIEW_H
#define KGRVIEW_H

#include <QResizeEvent>
#include <QGraphicsView>

#include "kgrscene.h"

class KGrView : public QGraphicsView
{
    Q_OBJECT
public:
    KGrView (QWidget * parent = 0);
    ~KGrView ();

protected:
    virtual void keyReleaseEvent    (QKeyEvent * event);
    virtual void resizeEvent        (QResizeEvent * event);

private:
    KGrScene * m_scene;
};

#endif // KGRVIEW_H
