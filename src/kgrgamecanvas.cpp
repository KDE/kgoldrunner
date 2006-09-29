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

// KGr temporary namespace changes: mauricio@tabuleiro.com
// TODO find a module to add these classes to other kdegames modules (maybe libkdegames? or own subdirectory?)

#include <QPaintEvent>
#include <QPainter>
#include <QRegion>
#include <QApplication>
#include <QTimer>
#include <QTime>
#include "kgrgamecanvas.h"

/*
  TODO:
    - (maybe) allow an item to be destroyed while calling KGrGameCanvasItem::advance.
    - When a group is hidden/destroyed should only update items (optimize for sparse groups)
*/

#define DEBUG_DONT_MERGE_UPDATES 0
#define DEBUG_CANVAS_PAINTS      0

/*
    KGrGameCanvasAbstract
*/
KGrGameCanvasAbstract::KGrGameCanvasAbstract() {

}

KGrGameCanvasAbstract::~KGrGameCanvasAbstract() {
  for(int i=0;i<m_items.size();i++)
    m_items[i]->m_canvas = NULL;
}

KGrGameCanvasItem* KGrGameCanvasAbstract::itemAt(QPoint pt) const {
  for(int i=m_items.size()-1;i>=0;i--) {
    KGrGameCanvasItem *el = m_items[i];
    if(el->m_visible && el->rect().contains(pt))
      return el;
  }
  return NULL;
}

QList<KGrGameCanvasItem*> KGrGameCanvasAbstract::itemsAt(QPoint pt) const {
  QList<KGrGameCanvasItem*> retv;

  for(int i=m_items.size()-1;i>=0;i--) {
    KGrGameCanvasItem *el = m_items[i];
    if(el->m_visible && el->rect().contains(pt))
      retv.append(el);
  }

  return retv;
}


/*
    KGrGameCanvasWidget
*/
class KGrGameCanvasWidgetPrivate {
public:
  QTimer m_anim_timer;
  QTime m_anim_time;
  bool m_pending_update;
  QRegion m_pending_update_reg;

#if DEBUG_CANVAS_PAINTS
  bool debug_paints;
#endif //DEBUG_CANVAS_PAINTS

  KGrGameCanvasWidgetPrivate()
  : m_pending_update(false)
#if DEBUG_CANVAS_PAINTS
  , debug_paints(false)
#endif //DEBUG_CANVAS_PAINTS
  {}
};

KGrGameCanvasWidget::KGrGameCanvasWidget(QWidget* parent)
: QWidget(parent)
, priv(new KGrGameCanvasWidgetPrivate()) {
  priv->m_anim_time.start();
  connect(&priv->m_anim_timer, SIGNAL(timeout()), this, SLOT(processAnimations()));
}

KGrGameCanvasWidget::~KGrGameCanvasWidget() {
  delete priv;
}

void KGrGameCanvasWidget::ensureAnimating() {
  if(!priv->m_anim_timer.isActive() )
      priv->m_anim_timer.start();
}

void KGrGameCanvasWidget::ensurePendingUpdate() {
  if(priv->m_pending_update)
    return;
  priv->m_pending_update = true;

#if DEBUG_DONT_MERGE_UPDATES
  updateChanges();
#else //DEBUG_DONT_MERGE_UPDATES
  QTimer::singleShot( 0, this, SLOT(updateChanges()) );
#endif //DEBUG_DONT_MERGE_UPDATES
}

void KGrGameCanvasWidget::updateChanges() {
  for(int i=0;i<m_items.size();i++) {
    KGrGameCanvasItem *el = m_items[i];

    if(el->m_changed)
      el->updateChanges();
  }
  priv->m_pending_update = false;

#if DEBUG_CANVAS_PAINTS
  repaint();
  priv->debug_paints = true;
  repaint( priv->m_pending_update_reg );
  QApplication::syncX();
  priv->debug_paints = false;
  usleep(100000);
  repaint( priv->m_pending_update_reg );
  QApplication::syncX();
  usleep(100000);
#else //DEBUG_CANVAS_PAINTS
  repaint( priv->m_pending_update_reg );
#endif //DEBUG_CANVAS_PAINTS

  priv->m_pending_update_reg = QRegion();
}

void KGrGameCanvasWidget::invalidate(const QRect& r, bool /*translate*/) {
  priv->m_pending_update_reg |= r;
  ensurePendingUpdate();
}

void KGrGameCanvasWidget::invalidate(const QRegion& r, bool /*translate*/) {
  priv->m_pending_update_reg |= r;
  ensurePendingUpdate();
}

void KGrGameCanvasWidget::paintEvent(QPaintEvent *event) {
#if DEBUG_CANVAS_PAINTS
  if(priv->debug_paints)
  {
    QPainter p(this);
    p.fillRect(event->rect(), Qt::magenta);
    return;
  }
#endif //DEBUG_CANVAS_PAINTS

  {QPainter p(this);
  QRect evr = event->rect();
  QRegion evreg = event->region();

  for(int i=0;i<m_items.size();i++) {
    KGrGameCanvasItem *el = m_items[i];
    if( el->m_visible && evr.intersects( el->rect() )
        && evreg.contains( el->rect() ) ) {
      el->m_last_rect = el->rect();
      el->paintInternal(&p, evr, evreg, QPoint(), 1.0 );
    }
  }}

  QApplication::syncX();
}

void KGrGameCanvasWidget::processAnimations() {
  if(m_animated_items.empty() ) {
    priv->m_anim_timer.stop();
    return;
  }

  int tm = priv->m_anim_time.elapsed();
  for(int i=0;i<m_animated_items.size();i++) {
    KGrGameCanvasItem *el = m_animated_items[i];
    el->advance(tm);
  }

  if(m_animated_items.empty() )
    priv->m_anim_timer.stop();
}

void KGrGameCanvasWidget::setAnimationDelay(int d) {
  priv->m_anim_timer.setInterval(d);
}

int KGrGameCanvasWidget::mSecs() {
  return priv->m_anim_time.elapsed();
}

KGrGameCanvasWidget* KGrGameCanvasWidget::topLevelCanvas() {
  return this;
}

/*
    KGrGameCanvasItem
*/
KGrGameCanvasItem::KGrGameCanvasItem(KGrGameCanvasAbstract* KGrGameCanvas)
: m_visible(false)
, m_animated(false)
, m_opacity(255)
, m_pos(0,0)
, m_canvas(KGrGameCanvas)
, m_changed(false) {
  if(m_canvas) m_canvas->m_items.append(this);
}

KGrGameCanvasItem::~KGrGameCanvasItem() {
  if(m_canvas) {
    m_canvas->m_items.removeAll(this);
    if(m_animated)
      m_canvas->m_animated_items.removeAll(this);
    if(m_visible)
      m_canvas->invalidate(m_last_rect, false);
  }
}

void KGrGameCanvasItem::changed() {
  if(!m_changed) {
    m_changed = true;
    if(m_canvas)
      m_canvas->ensurePendingUpdate();
  }
}

void KGrGameCanvasItem::updateChanges() {
  if(!m_changed)
    return;
  if(m_canvas) {
    m_canvas->invalidate(m_last_rect, false);
    if(m_visible)
      m_canvas->invalidate(rect());
  }
  m_changed = false;
}

QPixmap *KGrGameCanvasItem::transparence_pixmap_cache = NULL;

QPixmap* KGrGameCanvasItem::getTransparenceCache(QSize s) {
  if(!transparence_pixmap_cache)
    transparence_pixmap_cache = new QPixmap();
  if(s.width()>transparence_pixmap_cache->width() ||
    s.height()>transparence_pixmap_cache->height()) {

    /* Strange that a pixmap with alpha should be created this way, i think a qt bug */
    *transparence_pixmap_cache = QPixmap::fromImage( QImage(
      s.expandedTo(transparence_pixmap_cache->size()), QImage::Format_ARGB32 ) );
  }

  return transparence_pixmap_cache;
}

void KGrGameCanvasItem::paintInternal(QPainter* pp, const QRect& /*prect*/,
                    const QRegion& /*preg*/, QPoint /*delta*/, double cumulative_opacity) {
  int opacity = int(cumulative_opacity*m_opacity + 0.5);

  if(opacity <= 0)
    return;

  if(opacity >= 255) {
    paint(pp);
    return;
  }

#if QT_VERSION >= 0x040200
  if(!layered()) {
    pp->setOpacity(opacity/255.0);
    paint(pp);
    pp->setOpacity(1.0);
    return;
  }
#endif

  QRect mr = rect();
  QPixmap* cache = getTransparenceCache(mr.size());

  {
    QPainter p(cache);

    /* clear */
    p.setBrush(QColor(255,255,255,0));
    p.setPen(Qt::NoPen);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawRect(QRect(QPoint(),mr.size()));

    /* paint on the item */
    p.translate(-mr.topLeft());
    paint(&p);
    p.translate(mr.topLeft());

    /* make the opacity */
    p.setBrush( QColor(255,255,255,255-opacity) );
    p.setPen( Qt::NoPen );
    p.setCompositionMode( QPainter::CompositionMode_DestinationOut );
    p.drawRect( QRect(QPoint(),mr.size()) );
  }

  pp->drawPixmap(mr.topLeft(), *cache, QRect(QPoint(),mr.size()) );
}

void KGrGameCanvasItem::putInCanvas(KGrGameCanvasAbstract *c) {
  if(m_canvas == c)
      return;

  if(m_canvas) {
    if(m_visible)
      m_canvas->invalidate(rect());
    m_canvas->m_items.removeAll(this);
    if(m_animated)
      m_canvas->m_animated_items.removeAll(this);
  }

  m_canvas = c;

  if(m_canvas) {
    m_canvas->m_items.append(this);
    if(m_animated) {
      m_canvas->m_animated_items.append(this);
      m_canvas->ensureAnimating();
    }
    if(m_visible)
      m_canvas->invalidate(rect());
  }
}

void KGrGameCanvasItem::setVisible(bool v) {
  if(m_visible == v)
      return;

  m_visible = v;
  if(m_canvas) {
    if(!v)
      m_canvas->invalidate(m_last_rect, false);
    else
      changed();
  }
  if(!v)
    m_last_rect = QRect();
}

void KGrGameCanvasItem::setAnimated(bool a) {
  if(m_animated == a)
    return;

  m_animated = a;
  if(m_canvas) {
    if(a) {
      m_canvas->m_animated_items.append(this);
      m_canvas->ensureAnimating();
    }
    else
      m_canvas->m_animated_items.removeAll(this);
  }
}

void KGrGameCanvasItem::setOpacity(int o) {
  m_opacity = o;

  if(m_canvas && m_visible)
    changed();
}

bool KGrGameCanvasItem::layered() const { return true; }

void KGrGameCanvasItem::advance(int /*msecs*/) { }

void KGrGameCanvasItem::updateAfterRestack(int from, int to)
{
    int inc = from>to ? -1 : 1;

    QRegion upd;
    for(int i=from; i!=to;i+=inc)
    {
        KGrGameCanvasItem *el = m_canvas->m_items[i];
        if(!el->m_visible)
            continue;

        QRect r = el->rect() & rect();
        if(!r.isEmpty())
            upd |= r;
    }

    if(!upd.isEmpty())
        m_canvas->invalidate(upd);
}

void KGrGameCanvasItem::raise()
{
    if(!m_canvas || m_canvas->m_items.last() == this)
        return;

    int old_pos = m_canvas->m_items.indexOf(this);
    m_canvas->m_items.removeAt(old_pos);
    m_canvas->m_items.append(this);
    if(m_visible)
        updateAfterRestack(old_pos, m_canvas->m_items.size()-1);
}

void KGrGameCanvasItem::lower()
{
    if(!m_canvas || m_canvas->m_items.first() == this)
        return;

    int old_pos = m_canvas->m_items.indexOf(this);
    m_canvas->m_items.removeAt(old_pos);
    m_canvas->m_items.prepend(this);

    if(m_visible)
        updateAfterRestack(old_pos, 0);
}

void KGrGameCanvasItem::stackOver(KGrGameCanvasItem* ref)
{
    if(!m_canvas)
        return;

    if(ref->m_canvas != m_canvas)
    {
        qCritical("KGrGameCanvasItem::stackOver: Argument must be a sibling item!\n");
        return;
    }

    int i = m_canvas->m_items.indexOf( ref );
    if(i < m_canvas->m_items.size()-2  &&  m_canvas->m_items[i+1] == this)
        return;

    int old_pos = m_canvas->m_items.indexOf(this);
    m_canvas->m_items.removeAt(old_pos);
    i = m_canvas->m_items.indexOf( ref, i-1 );
    m_canvas->m_items.insert(i+1,this);

    if(m_visible)
        updateAfterRestack(old_pos, i+1);
}

void KGrGameCanvasItem::stackUnder(KGrGameCanvasItem* ref)
{
    if(!m_canvas)
        return;


    if(ref->m_canvas != m_canvas)
    {
        qCritical("KGrGameCanvasItem::stackUnder: Argument must be a sibling item!\n");
        return;
    }

    int i = m_canvas->m_items.indexOf( ref );
    if(i >= 1  &&  m_canvas->m_items[i-1] == this)
        return;

    int old_pos = m_canvas->m_items.indexOf(this);
    m_canvas->m_items.removeAt(old_pos);
    i = m_canvas->m_items.indexOf( ref, i-1 );
    m_canvas->m_items.insert(i,this);

    if(m_visible)
        updateAfterRestack(old_pos, i);
}

void KGrGameCanvasItem::moveTo(QPoint newpos)
{
  if(m_pos == newpos)
    return;
  m_pos = newpos;
  if(m_visible && m_canvas)
    changed();
}

/*
    KGrGameCanvasGroup
*/
KGrGameCanvasGroup::KGrGameCanvasGroup(KGrGameCanvasAbstract* KGrGameCanvas)
: KGrGameCanvasItem(KGrGameCanvas)
, KGrGameCanvasAbstract()
, m_child_rect_changed(true) {

}

KGrGameCanvasGroup::~KGrGameCanvasGroup() {

}

void KGrGameCanvasGroup::ensureAnimating() {
  setAnimated(true);
}

void KGrGameCanvasGroup::ensurePendingUpdate() {
   if(!m_changed || !m_child_rect_changed) {
     m_child_rect_changed = true;
     KGrGameCanvasItem::changed();
   }
}

void KGrGameCanvasGroup::updateChanges() {
  if(!m_changed)
    return;
  for(int i=0;i<m_items.size();i++) {
    KGrGameCanvasItem *el = m_items[i];

    if(el->m_changed)
      el->updateChanges();
  }
  m_changed = false;
}

void KGrGameCanvasGroup::changed() {
  if(!m_changed) {
    KGrGameCanvasItem::changed();

    for(int i=0;i<m_items.size();i++)
      m_items[i]->changed();
  }
}

void KGrGameCanvasGroup::invalidate(const QRect& r, bool translate) {
  if(m_canvas)
    m_canvas->invalidate(translate ? r.translated(m_pos) : r);
  if(!m_changed)
    ensurePendingUpdate();
}

void KGrGameCanvasGroup::invalidate(const QRegion& r, bool translate) {
  if(m_canvas)
    m_canvas->invalidate(translate ? r.translated(m_pos) : r);
  if(!m_changed)
    ensurePendingUpdate();
}

void KGrGameCanvasGroup::advance(int msecs) {
  for(int i=0;i<m_animated_items.size();i++)
  {
      KGrGameCanvasItem *el = m_animated_items[i];
      el->advance(msecs);
  }

  if(m_animated_items.empty())
      setAnimated(false);
}

void KGrGameCanvasGroup::paintInternal(QPainter* p, const QRect& prect,
          const QRegion& preg, QPoint delta, double cumulative_opacity) {
  cumulative_opacity *= (m_opacity/255.0);

  delta += m_pos;
  p->translate(m_pos);

  for(int i=0;i<m_items.size();i++) {
    KGrGameCanvasItem *el = m_items[i];
    QRect r = el->rect().translated(delta);

    if( el->m_visible && prect.intersects( r ) && preg.contains( r ) ) {
      el->m_last_rect = r;
      el->paintInternal(p,prect,preg,delta,cumulative_opacity);
    }
  }

  p->translate(-m_pos);
}

void KGrGameCanvasGroup::paint(QPainter* /*p*/) {
  Q_ASSERT(!"This function should never be called");
}

QRect KGrGameCanvasGroup::rect() const
{
  if(!m_child_rect_changed)
    return m_last_child_rect.translated(m_pos);

  m_child_rect_changed = false;
  m_last_child_rect = QRect();
  for(int i=0;i<m_items.size();i++)
  {
    KGrGameCanvasItem *el = m_items[i];
    if(el->m_visible)
      m_last_child_rect |= el->rect();
  }

  return m_last_child_rect.translated(m_pos);
}

KGrGameCanvasWidget* KGrGameCanvasGroup::topLevelCanvas()
{
    return m_canvas ? m_canvas->topLevelCanvas() : NULL;
}

/*
    KGrGameCanvasDummy
*/
KGrGameCanvasDummy::KGrGameCanvasDummy(KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
{

}

KGrGameCanvasDummy::~KGrGameCanvasDummy()
{

}

void KGrGameCanvasDummy::paintInternal(QPainter* /*pp*/, const QRect& /*prect*/,
                    const QRegion& /*preg*/, QPoint /*delta*/, double /*cumulative_opacity*/) {
}

void KGrGameCanvasDummy::paint(QPainter* /*p*/) {
}

QRect KGrGameCanvasDummy::rect() const
{
    return QRect();
}


/*
    KGrGameCanvasPixmap
*/
KGrGameCanvasPixmap::KGrGameCanvasPixmap(const QPixmap& p, KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas), m_pixmap(p) {

}

KGrGameCanvasPixmap::KGrGameCanvasPixmap(KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas) {

}

KGrGameCanvasPixmap::~KGrGameCanvasPixmap() {

}

void KGrGameCanvasPixmap::setPixmap(const QPixmap& p) {
  m_pixmap = p;
  if(visible() && canvas() )
    changed();
}

#if QT_VERSION >= 0x040200
void KGrGameCanvasPixmap::paintInternal(QPainter* p, const QRect& /*prect*/,
                  const QRegion& /*preg*/, QPoint /*delta*/, double cumulative_opacity) {
  int op = int(cumulative_opacity*opacity() + 0.5);

  if(op <= 0)
    return;

  if(op < 255)
    p->setOpacity( op/255.0 );
  p->drawPixmap(pos(), m_pixmap);
  if(op < 255)
    p->setOpacity(1.0);
}
#endif

void KGrGameCanvasPixmap::paint(QPainter* p) {
  p->drawPixmap(pos(), m_pixmap);
}

QRect KGrGameCanvasPixmap::rect() const {
    return QRect(pos(), m_pixmap.size());
}


/*
    KGrGameCanvasTiledPixmap
*/
KGrGameCanvasTiledPixmap::KGrGameCanvasTiledPixmap(const QPixmap& pixmap, QSize size, QPoint origin,
                        bool move_orig, KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
    , m_pixmap(pixmap)
    , m_size(size)
    , m_origin(origin)
    , m_move_orig(move_orig) {

}

KGrGameCanvasTiledPixmap::KGrGameCanvasTiledPixmap(KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
    , m_size(0,0)
    , m_origin(0,0)
    , m_move_orig(false) {

}

KGrGameCanvasTiledPixmap::~KGrGameCanvasTiledPixmap() {

}

void KGrGameCanvasTiledPixmap::setPixmap(const QPixmap& pixmap) {
    m_pixmap = pixmap;
    if(visible() && canvas() )
      changed();
}

void KGrGameCanvasTiledPixmap::setSize(QSize size) {
  m_size = size;
  if(visible() && canvas() )
    changed();
}

void KGrGameCanvasTiledPixmap::setOrigin(QPoint origin)
{
  m_origin = m_move_orig ? origin - pos() : origin;

  if(visible() && canvas() )
    changed();
}


void KGrGameCanvasTiledPixmap::setMoveOrigin(bool move_orig)
{
  if(move_orig && !m_move_orig)
      m_origin -= pos();
  if(move_orig && !m_move_orig)
      m_origin += pos();
  m_move_orig = move_orig;
}

#if QT_VERSION >= 0x040200
void KGrGameCanvasTiledPixmap::paintInternal(QPainter* p, const QRect& /*prect*/,
                  const QRegion& /*preg*/, QPoint /*delta*/, double cumulative_opacity) {
  int op = int(cumulative_opacity*opacity() + 0.5);

  if(op <= 0)
    return;

  if(op < 255)
    p->setOpacity( op/255.0 );
  if(m_move_orig)
    p->drawTiledPixmap( rect(), m_pixmap, m_origin);
  else
    p->drawTiledPixmap( rect(), m_pixmap, m_origin+pos() );
  if(op < 255)
    p->setOpacity( op/255.0 );
}
#endif

void KGrGameCanvasTiledPixmap::paint(QPainter* p)
{
    if(m_move_orig)
        p->drawTiledPixmap( rect(), m_pixmap, m_origin);
    else
        p->drawTiledPixmap( rect(), m_pixmap, m_origin+pos() );
}

QRect KGrGameCanvasTiledPixmap::rect() const
{
    return QRect(pos(), m_size);
}


/*
    KGrGameCanvasRectangle
*/
KGrGameCanvasRectangle::KGrGameCanvasRectangle(const QColor& color, QSize size, KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
    , m_color(color)
    , m_size(size)
{

}

KGrGameCanvasRectangle::KGrGameCanvasRectangle(KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
    , m_size(0,0)
{

}

KGrGameCanvasRectangle::~KGrGameCanvasRectangle()
{

}

void KGrGameCanvasRectangle::setColor(const QColor& color)
{
  m_color = color;
  if(visible() && canvas() )
    changed();
}

void KGrGameCanvasRectangle::setSize(QSize size)
{
  m_size = size;
  if(visible() && canvas() )
    changed();
}

void KGrGameCanvasRectangle::paintInternal(QPainter* p, const QRect& /*prect*/,
                  const QRegion& /*preg*/, QPoint /*delta*/, double cumulative_opacity) {
  QColor col = m_color;
  cumulative_opacity *= opacity()/255.0;

  col.setAlpha( int(col.alpha()*cumulative_opacity+0.5) );
  p->fillRect( rect(), col );
}

void KGrGameCanvasRectangle::paint(QPainter* p) {
  p->fillRect( rect(), m_color );
}

QRect KGrGameCanvasRectangle::rect() const {
    return QRect(pos(), m_size);
}


/*
    KGrGameCanvasText
*/
KGrGameCanvasText::KGrGameCanvasText(const QString& text, const QColor& color,
                        const QFont& font, HPos hp, VPos vp,
                        KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
    , m_text(text)
    , m_color(color)
    , m_font(font)
    , m_hpos(hp)
    , m_vpos(vp) {
    calcBoundingRect();
}

KGrGameCanvasText::KGrGameCanvasText(KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
    , m_text("")
    , m_color(Qt::black)
    , m_font(QApplication::font())
    , m_hpos(HStart)
    , m_vpos(VBaseline) {

}

KGrGameCanvasText::~KGrGameCanvasText() {

}

void KGrGameCanvasText::calcBoundingRect() {
    m_bounding_rect = QFontMetrics(m_font).boundingRect(m_text);
    /*printf("b rect is %d %d %d %d\n",
        m_bounding_rect.x(),
        m_bounding_rect.y(),
        m_bounding_rect.width(),
        m_bounding_rect.height() );*/
}

void KGrGameCanvasText::setText(const QString& text) {
  if(m_text == text)
    return;
  m_text = text;
  calcBoundingRect();

  if(visible() && canvas() )
    changed();
}

void KGrGameCanvasText::setColor(const QColor& color) {
  m_color = color;
}

void KGrGameCanvasText::setFont(const QFont& font) {
  m_font = font;
  calcBoundingRect();

  if(visible() && canvas() )
    changed();
}

void KGrGameCanvasText::setPositioning(HPos hp, VPos vp) {
  pos() += offsetToDrawPos();
  m_hpos = hp;
  m_vpos = vp;
  pos() -= offsetToDrawPos();
}

QPoint KGrGameCanvasText::offsetToDrawPos() const {
    QPoint retv;

    switch(m_hpos) {
        case HStart:
            retv.setX(0);
            break;
        case HLeft:
            retv.setX(-m_bounding_rect.left());
            break;
        case HRight:
            retv.setX(-m_bounding_rect.right());
            break;
        case HCenter:
            retv.setX(-(m_bounding_rect.left()+m_bounding_rect.right())/2);
            break;
    }

    switch(m_vpos) {
        case VBaseline:
            retv.setY(0);
            break;
        case VTop:
            retv.setY(-m_bounding_rect.top());
            break;
        case VBottom:
            retv.setY(-m_bounding_rect.bottom());
            break;
        case VCenter:
            retv.setY(-(m_bounding_rect.top()+m_bounding_rect.bottom())/2);
            break;
    }

    return retv;
}

void KGrGameCanvasText::paintInternal(QPainter* p, const QRect& /*prect*/,
                  const QRegion& /*preg*/, QPoint /*delta*/, double cumulative_opacity) {
  QColor col = m_color;
  cumulative_opacity *= opacity()/255.0;

  col.setAlpha( int(col.alpha()*cumulative_opacity+0.5) );
  p->setPen(col);
  p->setFont(m_font);
  p->drawText( pos() + offsetToDrawPos(), m_text);
}

void KGrGameCanvasText::paint(QPainter* p) {
  p->setPen(m_color);
  p->setFont(m_font);
  p->drawText( pos() + offsetToDrawPos(), m_text);
}

QRect KGrGameCanvasText::rect() const {
    return m_bounding_rect.translated( pos() + offsetToDrawPos() );
}


/*
    KGrGameCanvasPicture
*/
KGrGameCanvasPicture::KGrGameCanvasPicture(const QPicture& p, KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas), m_picture(p)
{

}

KGrGameCanvasPicture::KGrGameCanvasPicture(KGrGameCanvasAbstract* KGrGameCanvas)
    : KGrGameCanvasItem(KGrGameCanvas)
{

}

KGrGameCanvasPicture::~KGrGameCanvasPicture()
{

}

void KGrGameCanvasPicture::setPicture(const QPicture& p)
{
  m_picture = p;

  if(visible() && canvas() )
    changed();
}

void KGrGameCanvasPicture::paint(QPainter* p)
{
    p->drawPicture(pos(), m_picture);
}

QRect KGrGameCanvasPicture::rect() const
{
    return m_picture.boundingRect().translated( pos());
}


#include "kgrgamecanvas.moc"
