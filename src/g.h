// These are rough QGraphicsScene and QGraphicsView classes for testing the
// KGrRenderer class, but they illustrate points about KGoldrunner graphics.
//
// 1. The scene has examples of every type of SVG element used in KGr.
// 2. There is a brick marking each corner of the level-layout.
// 3. There is an empty border, at least two bricks wide, around the layout.
// 4. The border usually holds the scores and titles and acts as a buffer zone
//    for mouse moves.
// 5. The rectangle around the level-layout and border is there for testing
//    purposes only and is not part of KGoldrunner.
// 6. Again, purely for testing purposes, the sprites at the top left change
//    their animation frames as the view is resized.
// 7. The border and its contents have not been drawn.
// 8. Some borders (e.g. Egypt) contain border tiles (not drawn here).
// 9. Some (e.g. Default) contain display-tiles for scores (not drawn here).
//
#ifndef G_H
#define G_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QGraphicsRectItem>
#include <QVector>
#include <QPoint>
#include <KGameRenderedItem>

class KGrRenderer;
class KGameRenderer;

class GS : public QGraphicsScene
{
    Q_OBJECT
public:
    GS (QObject * parent = 0);
    ~GS();
    void redrawScene (QSize size);

public slots:
    void paintCell (const int i, const int j, const char type);

private:
    KGrRenderer   *     m_renderer;
    KGameRenderer *     m_renderSet;
    KGameRenderer *     m_renderActors;

    int                 m_tileSize;
    int                 m_tilesWide;
    int                 m_tilesHigh;

    QVector<KGameRenderedItem *> m_tiles;

    QPoint              m_gridTopLeft;
    QGraphicsRectItem * m_grid;

    KGameRenderedItem * m_background;

    KGameRenderedItem * m_hero;
    KGameRenderedItem * m_enemy1;
    KGameRenderedItem * m_enemy2;
    KGameRenderedItem * m_brick;

    void loadTestItems();
    void redrawTestItems (const int tileSize);
};



class GV : public QGraphicsView
{
    Q_OBJECT
public:
    GV(QWidget * parent = 0);
    ~GV();
    /* void setGridSize() {
        if (scene() != 0) {
            fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
        }
    }; */

protected:
    void resizeEvent(QResizeEvent* event) {
        if (scene() != 0) {
	    m_scene->redrawScene (event->size());
            // fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
        }
        QGraphicsView::resizeEvent(event);
    };

    GS * m_scene;
};
#endif G_H
