#include "Include/QCefWidget.h"
#include "Include/QCefVersion.h"
#include "QCefWidgetImpl.h"
#include <QAbstractEventDispatcher>
#include <QDebug>
#include <QResizeEvent>
#include <QVariant>
#include <QPainter>
#include <include/base/cef_logging.h>
#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_frame.h>
#include <include/cef_sandbox_win.h>
#include "QCefManager.h"

QString g_strobj;;

QCefWidget::QCefWidget(const QString& url, QWidget* parent) :
    QWidget(parent) {
  setAttribute(Qt::WA_NativeWindow, true);
  setAttribute(Qt::WA_InputMethodEnabled, true);
  setAttribute(Qt::WA_StyledBackground, true);

  pImpl_ = std::make_unique<QCefWidgetImpl>(WidgetType::WT_Widget, this);
  //connect(pImpl_.get(), SIGNAL(destroyed(QObject *)), this, SIGNAL(signal_destroyed(QObject *)));
  connect(pImpl_.get(), SIGNAL(signal_close()), this, SIGNAL(signal_close()), Qt::QueuedConnection);
  static int i = 1;
  g_strobj = QString("strobj_%1_").arg(i++);
  pImpl_->setObjectName(QString("CefWidgetImpl_%1").arg(g_strobj));
  if (!url.isEmpty()) {
    pImpl_->navigateToUrl(url);
  }
}

QCefWidget::~QCefWidget() {
    emit signal_close();
  qDebug().noquote() << "QCefWidget::~QCefWidget" << this;
}

void QCefWidget::navigateToUrl(const QString& url) {
  Q_ASSERT(pImpl_);
  pImpl_->navigateToUrl(url);
}

bool QCefWidget::canGoBack() {
  Q_ASSERT(pImpl_);
  return pImpl_->canGoBack();
}

bool QCefWidget::canGoForward() {
  Q_ASSERT(pImpl_);
  return pImpl_->canGoForward();
}

void QCefWidget::goBack() {
  Q_ASSERT(pImpl_);
  pImpl_->goBack();
}

void QCefWidget::goForward() {
  Q_ASSERT(pImpl_);
  pImpl_->goForward();
}

bool QCefWidget::isLoadingBrowser() {
  Q_ASSERT(pImpl_);
  return pImpl_->isLoadingBrowser();
}

void QCefWidget::reloadBrowser(bool bIgnoreCache) {
  Q_ASSERT(pImpl_);
  pImpl_->reloadBrowser(bIgnoreCache);
}

void QCefWidget::stopLoadBrowser() {
  Q_ASSERT(pImpl_);
  pImpl_->stopLoadBrowser();
}

bool QCefWidget::triggerEvent(const QString& name, const QCefEvent& event) {
  Q_ASSERT(pImpl_);
  return pImpl_->triggerEvent(name, event);
}

bool QCefWidget::responseCefQuery(const QCefQuery& query) {
  Q_ASSERT(pImpl_);
  return pImpl_->responseCefQuery(query);
}

void QCefWidget::executeJavascript(const QString& javascript) {
  Q_ASSERT(pImpl_);
  pImpl_->executeJavascript(javascript);
}

bool QCefWidget::setOsrEnabled(bool b) {
  Q_ASSERT(pImpl_);
  return pImpl_->setOsrEnabled(b);
}

bool QCefWidget::osrEnabled() const {
  Q_ASSERT(pImpl_);
  return pImpl_->browserSetting().osrEnabled;
}

void QCefWidget::setContextMenuEnabled(bool b) {
  Q_ASSERT(pImpl_);
  pImpl_->setContextMenuEnabled(b);
}

bool QCefWidget::contextMenuEnabled() const {
  Q_ASSERT(pImpl_);
  return pImpl_->browserSetting().contextMenuEnabled;
}

void QCefWidget::setAutoShowDevToolsContextMenu(bool b) {
  Q_ASSERT(pImpl_);
  pImpl_->setAutoShowDevToolsContextMenu(b);
}

bool QCefWidget::autoShowDevToolsContextMenu() const {
  Q_ASSERT(pImpl_);
  return pImpl_->browserSetting().autoShowDevToolsContextMenu;
}

void QCefWidget::setAllowExecuteUnknownProtocolViaOS(bool b) {
  Q_ASSERT(pImpl_);
  pImpl_->setAllowExecuteUnknownProtocolViaOS(b);
}

bool QCefWidget::allowExecuteUnknownProtocolViaOS() const {
  Q_ASSERT(pImpl_);
  return pImpl_->browserSetting().executeUnknownProtocolViaOS;
}

void QCefWidget::setAutoDestoryCefWhenCloseEvent(bool b) {
  Q_ASSERT(pImpl_);
  pImpl_->setAutoDestoryCefWhenCloseEvent(b);
}

bool QCefWidget::autoDestoryCefWhenCloseEvent() {
  Q_ASSERT(pImpl_);
  return pImpl_->browserSetting().autoDestroyCefWhenCloseEvent;
}

void QCefWidget::setFPS(int fps) {
  Q_ASSERT(pImpl_);
  pImpl_->setFPS(fps);
}

int QCefWidget::fps() const {
  Q_ASSERT(pImpl_);
  return pImpl_->browserSetting().fps;
}

void QCefWidget::setBrowserBackgroundColor(const QColor& color) {
  Q_ASSERT(pImpl_);
  pImpl_->setBrowserBackgroundColor(color);
}

QColor QCefWidget::browserBackgroundColor() const {
  Q_ASSERT(pImpl_);
  return pImpl_->browserSetting().backgroundColor;
}

void QCefWidget::showDevTools() {
  QCefManager::getInstance().showDevTools(this);
}

void QCefWidget::closeDevTools() {
  QCefManager::getInstance().closeDevTools(this);
}

bool QCefWidget::addResourceProvider(QCefResourceProvider* provider,
                                     const QString& identifier) {
  Q_ASSERT(pImpl_);
  return pImpl_->addResourceProvider(provider, identifier);
}

bool QCefWidget::removeResourceProvider(const QString& identifier) {
  Q_ASSERT(pImpl_);
  return pImpl_->removeResourceProvider(identifier);
}

bool QCefWidget::removeAllResourceProvider() {
  Q_ASSERT(pImpl_);
  return pImpl_->removeAllResourceProvider();
}

QString QCefWidget::QCefWidgetVersion() {
  return QString("%1.%2.%3")
      .arg(QCEF_VERSION_MAJOR)
      .arg(QCEF_VERSION_MINOR)
      .arg(QCEF_VERSION_PATCH);
}

QString QCefWidget::CefVersion() {
  return QString("%1.%2.%3")
      .arg(CEF_VERSION_MAJOR)
      .arg(CEF_VERSION_MINOR)
      .arg(CEF_VERSION_PATCH);
}

bool QCefWidget::nativeEvent(const QByteArray& eventType,
                             void* message,
                             long* result) {
  // pImpl_ may be empty, if we call winId in QCefWidgetImpl::QCefWidgetImpl().
  if (!pImpl_)
    return QWidget::nativeEvent(eventType, message, result);
  if (pImpl_->nativeEvent(eventType, message, result))
    return true;
  return QWidget::nativeEvent(eventType, message, result);
}

void QCefWidget::showEvent(QShowEvent* event) {
  if (pImpl_) {
    pImpl_->visibleChangedNotify(true);
  }
  QWidget::showEvent(event);
}

void QCefWidget::hideEvent(QHideEvent* event) {
  if (pImpl_) {
    pImpl_->visibleChangedNotify(false);
  }
  QWidget::hideEvent(event);
}

bool QCefWidget::event(QEvent* event) {
  // pImpl_ may be empty, if we call winId in QCefWidgetImpl::QCefWidgetImpl().
    if (QEvent::Paint != event->type() && QEvent::ScreenChangeInternal != event->type() &&
        QEvent::PaletteChange != event->type() && QEvent::PaletteChange != event->type() &&
        QEvent::HideToParent != event->type() && QEvent::PlatformSurface != event->type() &&
        QEvent::Hide != event->type() && QEvent::UpdateLater != event->type() &&
        QEvent::MetaCall != event->type() && QEvent::DynamicPropertyChange != event->type() &&
        QEvent::ShowToParent != event->type() && QEvent::Polish != event->type() &&
        QEvent::Resize != event->type() && QEvent::Move != event->type() &&
        QEvent::WindowIconChange != event->type() && QEvent::StyleChange != event->type() &&
        QEvent::ParentAboutToChange != event->type() && QEvent::PolishRequest != event->type() &&
        QEvent::ParentChange != event->type() && QEvent::StyleChange != event->type() &&
        QEvent::WinIdChange != event->type() && QEvent::InputMethodQuery != event->type())
    {
        qDebug() << __FUNCTION__ << this << event->type();
    }
  if (!pImpl_)
    return QWidget::event(event);

  if (pImpl_->event(event)) {
    return true;
  }
  return QWidget::event(event);
}

void QCefWidget::paintEvent(QPaintEvent* event) {
  Q_ASSERT(pImpl_);
  if (!pImpl_ || !pImpl_->paintEventHandle(event)) {
    QWidget::paintEvent(event);
  }
}