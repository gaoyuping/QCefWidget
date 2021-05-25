#include "Include/QCefQuery.h"
#include <include/cef_app.h>
#include <include/wrapper/cef_message_router.h>

int QCefQuery::TYPEID = qRegisterMetaType<QCefQuery>("QCefQuery");

QCefQuery::QCefQuery(QString req, int64_t query)
  : reqeust_(req)
  , id_(query)
  , restult_(false)
  , error_(0) {}

QCefQuery::QCefQuery()
  : id_(-1)
  , restult_(false)
  , error_(0) {}

QCefQuery::QCefQuery(const QCefQuery& other) {
  reqeust_ = other.reqeust_;
  id_ = other.id_;
  restult_ = other.restult_;
  response_ = other.response_;
  error_ = other.error_;
}

QCefQuery& QCefQuery::operator=(const QCefQuery& other) {
  reqeust_ = other.reqeust_;
  id_ = other.id_;
  restult_ = other.restult_;
  response_ = other.response_;
  error_ = other.error_;
  return *this;
}

QCefQuery::~QCefQuery() {}

const QString QCefQuery::reqeust() const {
  return reqeust_;
}

const int64_t QCefQuery::id() const {
  return id_;
}

const QString QCefQuery::response() const {
  return response_;
}

const bool QCefQuery::result() const {
  return restult_;
}

const int QCefQuery::error() const {
  return error_;
}

void QCefQuery::setResponseResult(bool success,
                                  const QString& response,
                                  int error /*= 0*/) const {
  restult_ = success;
  response_ = response;
  error_ = error;
}