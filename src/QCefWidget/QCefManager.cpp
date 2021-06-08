#include <include/cef_sandbox_win.h>
#include "QCefManager.h"
#include "QCefGlobalSetting.h"
#include <QWidget>
#include <QDebug>
#include <QApplication>
#include "QCefDevToolsWnd.h"

QCefManager::QCefManager() :
    initialized_(false) {}

QCefManager::~QCefManager() {}

QCefManager& QCefManager::getInstance() {
  static QCefManager s_instance;
  return s_instance;
}

void QCefManager::initializeCef() {
    qDebug() << __FUNCTION__ << this;
    if (initialized_)
    return;

  CefEnableHighDPISupport();
  QCefGlobalSetting::initializeInstance();

  Q_ASSERT((!QCefGlobalSetting::persist_session_cookies) ||
           (QCefGlobalSetting::persist_session_cookies &&
            QCefGlobalSetting::cache_path.length() > 0));

  CefString(&cefSettings_.browser_subprocess_path) = QCefGlobalSetting::browser_sub_process_path;
  CefString(&cefSettings_.resources_dir_path) = QCefGlobalSetting::resource_directory_path;
  CefString(&cefSettings_.locales_dir_path) = QCefGlobalSetting::locales_directory_path;
  CefString(&cefSettings_.user_agent) = QCefGlobalSetting::user_agent;
  CefString(&cefSettings_.cache_path) = QCefGlobalSetting::cache_path;
  CefString(&cefSettings_.user_data_path) = QCefGlobalSetting::user_data_path;
  CefString(&cefSettings_.locale) = QCefGlobalSetting::locale;
  CefString(&cefSettings_.accept_language_list) = QCefGlobalSetting::accept_language_list;
  CefString(&cefSettings_.log_file) = QCefGlobalSetting::debug_log_path;

  cefSettings_.persist_session_cookies = QCefGlobalSetting::persist_session_cookies ? 1 : 0;
  cefSettings_.persist_user_preferences = QCefGlobalSetting::persist_user_preferences ? 1 : 0;
  cefSettings_.remote_debugging_port = QCefGlobalSetting::remote_debugging_port;
  cefSettings_.no_sandbox = 1;
  cefSettings_.pack_loading_disabled = 0;
  cefSettings_.multi_threaded_message_loop = 1;
  cefSettings_.windowless_rendering_enabled = QCefGlobalSetting::osr_enabled ? 1 : 0;
  cefSettings_.ignore_certificate_errors = 1;

#ifndef NDEBUG
  cefSettings_.log_severity = LOGSEVERITY_INFO;
  cefSettings_.remote_debugging_port = 7777;
#else
  cefSettings_.log_severity = LOGSEVERITY_WARNING;
#endif

  app_ = new QCefBrowserApp();

#if (defined Q_OS_WIN32 || defined Q_OS_WIN64)
  HINSTANCE hInstance = ::GetModuleHandle(nullptr);
  CefMainArgs main_args(hInstance);
#else
#error "CefMainArgs no implement"
#endif

  void* sandboxInfo = nullptr;
  if (!CefInitialize(main_args, cefSettings_, app_, sandboxInfo))
    assert(0);
  initialized_ = true;
  connect(qApp, &QApplication::aboutToQuit, this, &QCefManager::uninitializeCef);
}

void QCefManager::uninitializeCef() {
    qDebug() << __FUNCTION__ << this;
    if (!initialized_)
    return;
  static bool hasCalled = false;
  Q_ASSERT(!hasCalled);
  hasCalled = true;

  app_ = nullptr;

  CefShutdown();

  initialized_ = false;
}

#include <QThread>

void QCefManager::waitCefQuit()
{
    int icout = 100;
    while (icout > 0)
    {
        QThread::msleep(50);
        --icout;
    }
}

QWidget* QCefManager::addBrowser(QWidget* pCefWidget,
                                 QCefWidgetImpl* cefWidgetImpl,
                                 CefRefPtr<CefBrowser> browser,
                                 bool osrMode) {
  std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  Q_ASSERT(pCefWidget && browser);
  if (!pCefWidget || !browser)
    return nullptr;

  QWidget* pTopWidget = (pCefWidget);
  Q_ASSERT(pTopWidget);

  if (!pTopWidget)
    return nullptr;

  qDebug() << __FUNCTION__ << pTopWidget << QString::number(pTopWidget->winId(), 16) << QString::number(pTopWidget->window()->winId(), 16) << browser->GetHost()->GetWindowHandle();
  CefInfo cefInfo;
  cefInfo.browser = browser;
  cefInfo.cefWidget = pCefWidget;
  cefInfo.cefWidgetImpl = cefWidgetImpl;
  cefInfo.cefWidgetTopWidget = pTopWidget;
  cefInfo.cefWidgetTopWidgetHwnd = (HWND)pTopWidget->window()->winId();
  cefInfo.osrMode = osrMode;
  cefInfo.browserStatus = BS_CREATED;

  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
    if (it->cefWidgetTopWidgetHwnd == cefInfo.cefWidgetTopWidgetHwnd) {
      cefInfo.cefWidgetTopWidgetPrevWndProc = it->cefWidgetTopWidgetPrevWndProc;
      break;
    }
  }

  if (!cefInfo.cefWidgetTopWidgetPrevWndProc) {
    //cefInfo.cefWidgetTopWidgetPrevWndProc = hookWidget(cefInfo.cefWidgetTopWidgetHwnd);
    cefInfo.cefWidgetTopWidget->installEventFilter(this);
  }
  //Q_ASSERT(cefInfo.cefWidgetTopWidgetPrevWndProc);

  cefs_.push_back(cefInfo);

  return pTopWidget;
}

void QCefManager::removeAllCefWidgets(QWidget* pTopWidget) {
    qDebug() << __FUNCTION__ << this;
    Q_ASSERT(pTopWidget);
  if (!pTopWidget)
    return;

  std::lock_guard<std::recursive_mutex> lg(cefsMutex_);

  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end();) {
    if (it->cefWidgetTopWidget == pTopWidget) {
      it = cefs_.erase(it);
    }
    else {
      it++;
    }
  }
}

void QCefManager::unhookTopWidget(QWidget* pTopWidget) {
    qDebug() << __FUNCTION__ << this;
  Q_ASSERT(pTopWidget);
  if (!pTopWidget)
    return;

  std::lock_guard<std::recursive_mutex> lg(cefsMutex_);

  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
    if (it->cefWidgetTopWidget == pTopWidget) {
      if (it->cefWidgetTopWidgetPrevWndProc && it->cefWidgetTopWidgetHwnd)
        it->cefWidgetTopWidgetPrevWndProc =
            (WNDPROC)::SetWindowLongPtr(it->cefWidgetTopWidgetHwnd, GWLP_WNDPROC, (LONG_PTR)it->cefWidgetTopWidgetPrevWndProc);
      return;
    }
  }
}

void QCefManager::tryCloseAllBrowsers(HWND hTopWidget) {
    qDebug() << __FUNCTION__ << this << "      hTopWidget=" << hTopWidget;
    HWND cefhwnd = NULL;
    do 
    {
        std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
        Q_ASSERT(hTopWidget);
        if (!hTopWidget)
            return;

        for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
            if (it->browserStatus == BS_CREATED) {
                if (it->cefWidgetTopWidgetHwnd == hTopWidget && it->browser &&
                    it->browser->GetHost()) {
                    it->browser->GetHost()->CloseBrowser(false);
                }
            }
            else if (it->browserStatus == BS_CLOSING && !it->osrMode) {
                if (it->browser && it->browser->GetHost())
                    //it->browser->GetHost()->CloseBrowser(true);
                    cefhwnd = it->browser->GetHost()->GetWindowHandle();
            }
        }
    } while (false);
    if (cefhwnd)
    {
        //it->browser->GetHost()->CloseBrowser(false);
        qDebug() << __FUNCTION__ << "   browser->GetHost()->GetWindowHandle() = " << cefhwnd;
        SendMessage(cefhwnd, WM_CLOSE, 0, 0);
    }
}

void QCefManager::tryCloseAllBrowsers(QWidget* pTopLevelWidget) {
    qDebug() << __FUNCTION__ << this << "      pTopLevelWidget=" << pTopLevelWidget;

    HWND cefhwnd = NULL;
    do
    {
        std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
        Q_ASSERT(pTopLevelWidget);
        if (!pTopLevelWidget)
            return;

        for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
            if (it->browserStatus == BS_CREATED) {
                if (it->cefWidgetTopWidget == pTopLevelWidget && it->browser &&
                    it->browser->GetHost()) {
                    it->browser->GetHost()->CloseBrowser(false);
                }
            }
            else if (it->browserStatus == BS_CLOSING && !it->osrMode) {
                HWND cefhwnd = NULL;
                if (it->browser && it->browser->GetHost())
                    //it->browser->GetHost()->CloseBrowser(true);
                    cefhwnd = it->browser->GetHost()->GetWindowHandle();
            }
        }
    } while (false);
    if (cefhwnd)
    {
        //it->browser->GetHost()->CloseBrowser(false);
        qDebug() << __FUNCTION__ << "   browser->GetHost()->GetWindowHandle() = " << cefhwnd;
        SendMessage(cefhwnd, WM_CLOSE, 0, 0);
    }
}

int QCefManager::aliveBrowserCount(HWND hTopWidget) {
    qDebug() << __FUNCTION__ << this;
    Q_ASSERT(hTopWidget);
  if (!hTopWidget)
    return 0;

  std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  int count = 0;
  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
    if (it->cefWidgetTopWidgetHwnd == hTopWidget) {
      if (it->browserStatus == BS_CREATED || it->browserStatus == BS_CLOSING)
        count++;
    }
  }

  return count;
}

int QCefManager::aliveBrowserCount(QWidget* pTopWidget) {
    qDebug() << __FUNCTION__ << this;
    Q_ASSERT(pTopWidget);
  if (!pTopWidget)
    return 0;

  std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  int count = 0;
  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
    if (it->cefWidgetTopWidget == pTopWidget) {
      if (it->browserStatus == BS_CREATED || it->browserStatus == BS_CLOSING)
        count++;
    }
  }

  return count;
}

void QCefManager::setBrowserClosing(QWidget* pCefWidget) {
    qDebug() << __FUNCTION__ << this;
    std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  Q_ASSERT(pCefWidget);
  if (!pCefWidget)
    return;

  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
    if (it->cefWidget == pCefWidget) {
      it->browserStatus = BS_CLOSING;
    }
  }
}

void QCefManager::setBrowserClosed(QWidget* pCefWidget) {
    qDebug() << __FUNCTION__ << this;
    std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  Q_ASSERT(pCefWidget);
  if (!pCefWidget)
    return;

  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
    if (it->cefWidget == pCefWidget) {
      it->browserStatus = BS_CLOSED;
      it->browser = nullptr;
    }
  }
}

void QCefManager::showDevTools(QWidget* pCefWidget) {
    qDebug() << __FUNCTION__ << this;
    std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  Q_ASSERT(pCefWidget);
  if (!pCefWidget)
    return;

  QMetaObject::invokeMethod(pCefWidget, [this, pCefWidget]() {
    for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
      if (it->cefWidget == pCefWidget) {
        if (it->devToolsWnd) {
          if (it->devToolsWnd->isMinimized())
            it->devToolsWnd->showNormal();
          else
            it->devToolsWnd->show();
          it->devToolsWnd->activateWindow();
          return;
        }

        it->devToolsWnd = new QCefDevToolsWnd(it->browser, nullptr);
        break;
      }
    }
  });
}

void QCefManager::closeDevTools(QWidget* pCefWidget) {
    qDebug() << __FUNCTION__ << this;
    std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  Q_ASSERT(pCefWidget);
  if (!pCefWidget)
    return;

  QMetaObject::invokeMethod(pCefWidget, [this, pCefWidget]() {
    for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
      if (it->cefWidget == pCefWidget) {
        if (it->devToolsWnd) {
          it->devToolsWnd->close();
          it->devToolsWnd = nullptr;
        }
        break;
      }
    }
  });
}

void QCefManager::devToolsClosedNotify(QCefDevToolsWnd* pWnd) {
    qDebug() << __FUNCTION__ << this;
    std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
  for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
    if (it->devToolsWnd == pWnd) {
      it->devToolsWnd = nullptr;
      break;
    }
  }
}

QWidget* QCefManager::getTopWidget(QWidget* pWidget) {
  Q_ASSERT(pWidget);
  if (!pWidget)
    return nullptr;

  QWidget* topWidget = pWidget;
  while (topWidget->parent()) {
    topWidget = (QWidget*)topWidget->parent();
  }

  Q_ASSERT(topWidget);
  return topWidget;
}

WNDPROC QCefManager::hookWidget(HWND hTopWidget) {
  Q_ASSERT(hTopWidget);
  if (!hTopWidget)
    return nullptr;
  ::SetWindowLongPtr(hTopWidget, GWLP_USERDATA, reinterpret_cast<LPARAM>(this));
  return (WNDPROC)SetWindowLongPtr(hTopWidget, GWLP_WNDPROC, (LONG_PTR)&newWndProc);
}
#include <QThread>
#if (defined Q_OS_WIN32 || defined Q_OS_WIN64)
LRESULT CALLBACK QCefManager::newWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#if (defined _DEBUG) || (defined DEBUG)
  if (uMsg == WM_CLOSE)
    qDebug().noquote() << "QCefManager::newWndProc WM_CLOSE, HWND:" << hWnd;
#endif

  QCefManager* pThis = reinterpret_cast<QCefManager*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
  if (!pThis)
    return 0;

  WNDPROC preWndProc = nullptr;
  QCefWidgetImpl* pCefWidgetImpl = nullptr;
  do {
    std::lock_guard<std::recursive_mutex> lg(pThis->cefsMutex_);
    for (std::list<CefInfo>::iterator it = pThis->cefs_.begin(); it != pThis->cefs_.end(); it++) {
      if (it->cefWidgetTopWidgetHwnd == hWnd) {
        preWndProc = it->cefWidgetTopWidgetPrevWndProc;
        pCefWidgetImpl = it->cefWidgetImpl;
        break;
      }
    }
  } while (false);

  if (!preWndProc) {
    qDebug().noquote() << "Not found cefWidgetTopWidgetHwnd" << "hWnd=" << hWnd;
    return 0;
  }

  if (uMsg == WM_CLOSE) {
    Q_ASSERT(pCefWidgetImpl);
    if (pCefWidgetImpl) {
      if (!pCefWidgetImpl->browserSetting().autoDestroyCefWhenCloseEvent) {
        qDebug().noquote() << "Not Destroy CEF" << "hWnd=" << hWnd;
        return ::CallWindowProc(preWndProc, hWnd, uMsg, wParam, lParam);
      }
    }
    pThis->tryCloseAllBrowsers(hWnd);

    if (pThis->aliveBrowserCount(hWnd) == 0) {
      ::SetWindowLongPtr(hWnd, GWLP_USERDATA, 0L);
      ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)preWndProc);
      // allow close
      qDebug().noquote() << "Accept WM_CLOSE" << "hWnd=" << hWnd;
      return ::CallWindowProc(preWndProc, hWnd, uMsg, wParam, lParam);
    }
    else {
      qDebug().noquote() << "Ignore WM_CLOSE" << "hWnd=" << hWnd;
      //QThread::msleep((long long)20 * 1000);
      // deny close
      return 0;
    }
  }
  else if (uMsg == WM_DPICHANGED) {
    if (pCefWidgetImpl)
      pCefWidgetImpl->dpiChangedNotify();
  }

  return ::CallWindowProc(preWndProc, hWnd, uMsg, wParam, lParam);
}


bool QCefManager::eventFilter(QObject* obj, QEvent* event) {
  if (event->type() == QEvent::Close) {
    qDebug().noquote() << "QCefManager::eventFilter QEvent::Close, obj:" << obj;
    std::lock_guard<std::recursive_mutex> lg(cefsMutex_);
    for (std::list<CefInfo>::iterator it = cefs_.begin(); it != cefs_.end(); it++) {
      if (it->cefWidgetTopWidget != obj)
        continue;
      QCefWidgetImpl* pImp = it->cefWidgetImpl;
      Q_ASSERT(pImp);
//       if (pImp && !pImp->browserSetting().autoDestroyCefWhenCloseEvent) {
//         qDebug().noquote() << "Not Destroy CEF";
// 
//         // Not return false
//         // pass control to the caller, caller's closeEvent(...) will be called.
//         return QObject::eventFilter(obj, event);
//       }
      this->tryCloseAllBrowsers(it->cefWidgetTopWidget);

      Q_ASSERT(this->aliveBrowserCount(it->cefWidgetTopWidget) > 0);

      it->cefWidgetTopWidget->removeEventFilter(this);
      

      event->ignore();
      qDebug().noquote() << "Ignore close event";
      return true;
    }
  }
  else if (QEvent::Destroy == event->type())
  {
      qDebug() << "QCefManager::eventFilter QEvent::Destroy, obj:" << obj;
  }

  return QObject::eventFilter(obj, event);
}

#endif
