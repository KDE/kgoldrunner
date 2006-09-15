/*  Originally created for KBoard
    Copyright 2006 Maurizio Monge <maurizio.monge@gmail.com>

BSD License
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __KGRGAMECANVAS_H__
#define __KGRGAMECANVAS_H__

/*
 *  Author: Maurizio Monge <maurizio.monge@gmail.com>
 */

// KGr temporary namespace changes: mauricio@tabuleiro.com
// TODO find a module to add these classes to other kdegames modules (maybe libkdegames? or own subdirectory?)

#include <QList>
#include <QPoint>
#include <QPicture>
#include <QPixmap>
#include <QPainter>
#include <QRect>
#include <QRegion>
#include <QWidget>

class KGrGameCanvasItem;

/**
    \class KGrGameCanvasAbstract canvas.h <canvas.h>
    \brief Container class.

    A KGrGameCanvasAbstract is a set of canvas items.
*/
class KGrGameCanvasAbstract
{
protected:
    friend class KGrGameCanvasItem;

    QList<KGrGameCanvasItem*> m_items;
    QList<KGrGameCanvasItem*> m_animated_items;

public:
    /** The constructor */
    KGrGameCanvasAbstract();

    virtual ~KGrGameCanvasAbstract();

    /** Returns a const pointer to the list holding all the items in the canvas */
    const QList<KGrGameCanvasItem*>* items() const { return &m_items; };

    /** Helper function to retrieve the topmost item at the given position */
    KGrGameCanvasItem* itemAt(QPoint pos) const;

    /** Overload, same as above */
    KGrGameCanvasItem* itemAt(int x, int y) const { return itemAt(QPoint(x,y)); }

    /** Helper function to retrieve all the items at the given position,
        starting from the topmost one. */
    QList<KGrGameCanvasItem*> itemsAt(QPoint pos) const;

    /** Overload, same as above */
    QList<KGrGameCanvasItem*> itemsAt(int x, int y) const { return itemsAt(QPoint(x,y)); }

    /** Virtual function to let know the canvas that it has animated items in it */
    virtual void ensureAnimating() = 0;

    /** Virtual function to ensure an update is pending, called from children */
    virtual void ensurePendingUpdate() = 0;

    /** Virtual function to update a rect */
    virtual void invalidate(const QRect& r, bool translate = true) = 0;

    /** Virtual function to update a region */
    virtual void invalidate(const QRegion& r, bool translate = true) = 0;

    /** Returns the toplevel non-group KGrGameCanvasWidget object */
    virtual class KGrGameCanvasWidget* topLevelCanvas() = 0;
};


class KGrGameCanvasAbstract;

/**
    \class KGrGameCanvasItem canvas.h <canvas.h>
    \brief An abstract item.

    A KGrGameCanvasItem is an abstract class to represent a generic item that can be
    put in a canvas.
*/
class KGrGameCanvasItem
{
private:
    friend class KGrGameCanvasAbstract;
    friend class KGrGameCanvasWidget;
    friend class KGrGameCanvasGroup;

    bool m_visible;
    bool m_animated;
    int  m_opacity;
    QPoint m_pos;
    KGrGameCanvasAbstract *m_canvas;

    bool m_changed;
    QRect m_last_rect;

    static QPixmap* transparence_pixmap_cache;
    static QPixmap* getTransparenceCache(QSize s);
    virtual void paintInternal(QPainter* p, const QRect& prect, const QRegion& preg,
                                          QPoint delta, double cumulative_opacity);

    void updateAfterRestack(int from, int to);

    /* function to update pending changes, called from parent */
    virtual void updateChanges();
    virtual void changedInternal();

public:
    /** Constructor, it allows you to specify the reference canvas or to create
        an orphan item that will be put into a canvas in a second moment.
        The item is always hidden after beeing created.
        Note that the restacking functions are quite intelligent and will only
        repaint if there is an actual need of doing it. So if you call raise on
        an item that is already (locally) on the top, no redraw will take place */
    KGrGameCanvasItem(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasItem();

    /** schedule an update if the item */
    void changed() { changedInternal(); }

    /** Returns true if the item is visible */
    bool visible() const { return m_visible; }

    /** Set the item as visible or hidden */
    void setVisible(bool v);

    /** Returns true if the item is animated */
    bool animated() const { return m_animated; }

    /** Set the item as animated or not */
    void setAnimated(bool a);

    /** Returns the opacity of the item */
    int opacity() const { return m_opacity; }

    /** Set the item's opacity value (int the 0-255 range) */
    void setOpacity(int o);

    /** Hides the item */
    void hide(){ setVisible(false); }

    /** Shows the item */
    void show(){ setVisible(true); }

    /** Restacks the item on the top of the canvas */
    void raise();

    /** Restacks the item on the bottom of the canvas */
    void lower();

    /** Restacks the item immediately over ref */
    void stackOver(KGrGameCanvasItem* ref);

    /** Restacks the item immediately under ref */
    void stackUnder(KGrGameCanvasItem* ref);

    /** Returns the canvas that is actually "owning" the item */
    KGrGameCanvasAbstract *canvas() const { return m_canvas; }

    /** Returns the toplevel canvas widget, or NULL */
    KGrGameCanvasWidget *topLevelCanvas() const { return m_canvas ? m_canvas->topLevelCanvas() : NULL; }

    /** Lets you specify the owning canvas. Call this function with canvas
        set to NULL to remove the item from the current canvas. */
    void putInCanvas(KGrGameCanvasAbstract *canvas);

    /** Returns the position of the item */
    QPoint pos() const { return m_pos; }

    /** Sets a new position. Note that an update will be posted to the parent
        canvas, and if you move an item twice in very little time, a region
        bigger than needed will be updated, causing a possible inefficiency */
    void moveTo(QPoint newpos);

    /** Overload, same as above */
    void moveTo(int x, int y) { moveTo( QPoint(x,y)); }

    /** Override this function to draw the item with the painter */
    virtual void paint(QPainter* p) = 0;

    /** Override this function to return the rect the item will be drawn into */
    virtual QRect rect() const = 0;

    /** Override this function to specify if the painting operations will paint over
        each other. If not, the item will be drawn more quickly when opacity is != 255,
        because it does not have to be painted onto a pixmap first. If you don't care
        about the item's opacity, don't care about this function as well. */
    virtual bool layered() const;

    /** Override this function to handle animations, the default function does nothing.
        The argument is the number of milliseconds from the creation of the canvas, so
        that you use it to handle the animation. */
    virtual void advance(int msecs);
};


/**
    \class KGrGameCanvasDummy canvas.h <canvas.h>
    \brief A dummy (empty) item.

    A KGrGameCanvasDummy is an empty, invisible item.
    You can use it as reference for stacking items in the canvas using the
    stackOver and stackUnder functions.
*/
class KGrGameCanvasDummy : public KGrGameCanvasItem
{
private:
    /** This function does nothing (of course) */
    virtual void paintInternal(QPainter* p, const QRect& prect,
            const QRegion& preg, QPoint delta, double cumulative_opacity);

public:
    /** Constructor */
    KGrGameCanvasDummy(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasDummy();

    /** This function does nothing (of course) */
    virtual void paint(QPainter* p);

    /** This returns an empty rectangle */
    virtual QRect rect() const;
};


/**
    \class KGrGameCanvasGroup canvas.h <canvas.h>
    \brief An item containing other items.

    A KGrGameCanvasGroup is an KGrGameCanvasItem, but it is also a KGrGameCanvasAbstract,
    so you can add children items to it. Just an inner canvas, if you prefer.
*/
class KGrGameCanvasGroup : public KGrGameCanvasItem, public KGrGameCanvasAbstract
{
private:
    mutable bool m_child_rect_changed;
    mutable QRect m_last_child_rect;

    virtual void paintInternal(QPainter* p, const QRect& prect, const QRegion& preg,
                                          QPoint delta, double cumulative_opacity);

    virtual void ensureAnimating();
    virtual void ensurePendingUpdate();
    virtual void invalidate(const QRect& r, bool translate = true);
    virtual void invalidate(const QRegion& r, bool translate = true);
    virtual void updateChanges();
    virtual void changedInternal();

public:
    /** Constructor */
    KGrGameCanvasGroup(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasGroup();

    /** This paints all the children */
    virtual void paint(QPainter* p);

    /** This returns the bouding rect of all children */
    virtual QRect rect() const;

    /** Animations step, updates the animation for the children */
    virtual void advance(int msecs);

    /** returns the toplevel canvas (or null if it is in an orphan tree) */
    KGrGameCanvasWidget* topLevelCanvas();
};

/**
    \class KGrGameCanvasPicture canvas.h <canvas.h>
    \brief A picture, ie a collection of paint operations.

    A KGrGameCanvasPicture is a picture that can be replayed on the canvas.
*/
class KGrGameCanvasPicture : public KGrGameCanvasItem
{
private:
    QPicture m_picture;

public:
    /** Constructor, specifying the picture to use */
    KGrGameCanvasPicture(const QPicture& picture, KGrGameCanvasAbstract* canvas = NULL);

    /** Constructor, creating with an empty picture */
    KGrGameCanvasPicture(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasPicture();

    /** Returns the picture */
    QPicture picture() const { return m_picture; }

    /** Sets the picture of the sprite */
    void setPicture(const QPicture& picture);

    virtual void paint(QPainter* p);
    virtual QRect rect() const;
};

/**
    \class KGrGameCanvasPixmap canvas.h <canvas.h>
    \brief A pixmap (sprite).

    A KGrGameCanvasPixmap is a pixmap that can be put in the canvas.
*/
class KGrGameCanvasPixmap : public KGrGameCanvasItem
{
private:
    QPixmap m_pixmap;

    /** optimization of this special case, it is a bit faster */
#if QT_VERSION >= 0x040200
    virtual void paintInternal(QPainter* p, const QRect& prect, const QRegion& preg,
                                          QPoint delta, double cumulative_opacity);
#endif

public:
    /** Constructor, specifying the pixmap to use */
    KGrGameCanvasPixmap(const QPixmap& pixmap, KGrGameCanvasAbstract* canvas = NULL);

    /** Constructor, creating with an empty pixmap */
    KGrGameCanvasPixmap(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasPixmap();

    /** Returns the pixmap */
    QPixmap pixmap() const { return m_pixmap; }

    /** Sets the pixmap of the sprite */
    void setPixmap(const QPixmap& pixmap);

    virtual void paint(QPainter* p);
    virtual QRect rect() const;
};

/**
    \class KGrGameCanvasTiledPixmap canvas.h <canvas.h>
    \brief A tiled pixmap (brush).

    A KGrGameCanvasPixmap is a pixmap that can be put in the canvas.
*/
class KGrGameCanvasTiledPixmap : public KGrGameCanvasItem
{
private:
    QPixmap m_pixmap;
    QSize m_size;
    QPoint m_origin;
    bool m_move_orig;

    /** optimization of this special case, it is a bit faster */
#if QT_VERSION >= 0x040200
    virtual void paintInternal(QPainter* p, const QRect& prect, const QRegion& preg,
                                          QPoint delta, double cumulative_opacity);
#endif

public:
    /** Constructor, specifying the pixmap and the parameters to use */
    KGrGameCanvasTiledPixmap(const QPixmap& pixmap, QSize size, QPoint origin,
                            bool move_orig, KGrGameCanvasAbstract* canvas = NULL);

    /** Constructor, creating with an empty pixmap */
    KGrGameCanvasTiledPixmap(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasTiledPixmap();

    /** Returns the pixmap */
    QPixmap pixmap() const { return m_pixmap; }

    /** Sets the pixmap of the tile */
    void setPixmap(const QPixmap& pixmap);

    /** Sets the size */
    void setSize(QSize size);

    /** The origin */
    QPoint origin() const { return m_move_orig ? m_origin + pos() : m_origin; }

    /** Sets the origin of the tiles */
    void setOrigin(QPoint size);

    /** If the origin is moved */
    bool moveOrigin(){ return m_move_orig; }

    /** Sets if the origin of the brush will be moved with the pixmap */
    void setMoveOrigin(bool move_orig);

    virtual void paint(QPainter* p);
    virtual QRect rect() const;
};


/**
    \class KGrGameCanvasRectangle canvas.h <canvas.h>
    \brief A solid rectangle.

    A KGrGameCanvasPixmap is a pixmap that can be put in the canvas.
*/
class KGrGameCanvasRectangle : public KGrGameCanvasItem
{
private:
    QColor m_color;
    QSize m_size;

    /** optimization of this special case, it is much faster */
    virtual void paintInternal(QPainter* p, const QRect& prect, const QRegion& preg,
                                          QPoint delta, double cumulative_opacity);

public:
    /** Constructor, specifying the pixmap and the parameters to use */
    KGrGameCanvasRectangle(const QColor& color, QSize size, KGrGameCanvasAbstract* canvas = NULL);

    /** Constructor, creating with an empty pixmap */
    KGrGameCanvasRectangle(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasRectangle();

    /** Returns the color */
    QColor color() const { return m_color; }

    /** Sets the color */
    void setColor(const QColor& color);

    /** Sets the size */
    void setSize(QSize size);

    virtual void paint(QPainter* p);
    virtual QRect rect() const;
};

/**
    \class KGrGameCanvasText canvas.h <canvas.h>
    \brief KGrGameCanvasText.

    A KGrGameCanvasText is a text that can be put in the canvas.
*/
class KGrGameCanvasText : public KGrGameCanvasItem
{
public:
    /** Specifies the meaning of the x coordinate of the item. It can
        refer to the start of the text, of the left, center, right of
        the bounding rectangle. */
    enum HPos {
        HStart,
        HLeft,
        HRight,
        HCenter
    };

    /** Specifies the meaning of the y coordinate of the item. It can
        refer to the baseline of the text, of the top, center, bottom of
        the bounding rectangle. */
    enum VPos {
        VBaseline,
        VTop,
        VBottom,
        VCenter
    };

private:
    QString m_text;
    QColor m_color;
    QFont m_font;
    HPos m_hpos;
    VPos m_vpos;
    QRect m_bounding_rect;

    QPoint offsetToDrawPos() const;
    void calcBoundingRect();

    /** optimization of this special case, it is much faster */
    virtual void paintInternal(QPainter* p, const QRect& prect, const QRegion& preg,
                                          QPoint delta, double cumulative_opacity);

public:
    /** Constructor, specifying the text and the parameters to use */
    KGrGameCanvasText(const QString& text, const QColor& color,
                    const QFont& font, HPos hp, VPos vp,
                    KGrGameCanvasAbstract* canvas = NULL);

    /** Constructor, creating with an empty text */
    KGrGameCanvasText(KGrGameCanvasAbstract* canvas = NULL);

    virtual ~KGrGameCanvasText();

    /** Returns the text */
    QString text() const { return m_text; }

    /** Sets the text */
    void setText(const QString& text);

    /** Returns the color */
    QColor color() const { return m_color; }

    /** Sets the color */
    void setColor(const QColor& color);

    /** Returns the font */
    QFont font() const { return m_font; }

    /** Sets the font */
    void setFont(const QFont& font);

    /** Returns the horizontal positioning style */
    HPos hPositioning() const { return m_hpos; }

    /** Returns the vertical positioning style */
    VPos vPositioning() const { return m_vpos; }

    /** Sets the positioning style */
    void setPositioning(HPos hp, VPos vp);

    /** Sets the size */
    void setSize(QSize size);

    virtual void paint(QPainter* p);
    virtual QRect rect() const;
};

/**
    \class KGrGameCanvasWidget canvas.h <canvas.h>
    \brief Container widget.

    A KGrGameCanvasWidget is a widget that can contain many KGrGameCanvasItem (images, rectangles, lines, etc).
    Portions of the widget are automatically redrawn to update the changes made to the items.
*/
class KGrGameCanvasWidget : public QWidget, public KGrGameCanvasAbstract
{
Q_OBJECT
private:
    friend class KGrGameCanvasItem;
    friend class AnimationNotifier;

    class  KGrGameCanvasWidgetPrivate *priv;

    virtual void ensureAnimating();
    virtual void ensurePendingUpdate();
    virtual void invalidate(const QRect& r, bool translate = true);
    virtual void invalidate(const QRegion& r, bool translate = true);

    virtual void paintEvent(QPaintEvent *event);

private slots:
    void processAnimations();
    void updateChanges();

public:
    /** The constructor */
    KGrGameCanvasWidget(QWidget* parent = NULL);

    virtual ~KGrGameCanvasWidget();

    /** Set the delay of the animation, in milliseconds */
    void setAnimationDelay(int d);

    /** Return the number of millisecons from the creation of the canvas
        (see also KGrGameCanvasItem::advance)*/
    int mSecs();

    /** returns 'this' */
    KGrGameCanvasWidget* topLevelCanvas();
};


#endif //__KGRGAMECANVAS_H__
