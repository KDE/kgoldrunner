
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
