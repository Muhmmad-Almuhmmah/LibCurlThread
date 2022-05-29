#ifndef CURLTHREAD_H
#define CURLTHREAD_H

#include <QObject>
#include <QThread>
#include <QApplication>
#include <stdio.h>
#include "curl/curl.h"
#include <string>
#include <QString>
#include <QDebug>
#include <wininet.h>
#include <Windows.h>
#include <Sensapi.h>
#include <QDeadlineTimer>

class JsKeys{
public:
    JsKeys(QString sKey,QString sValue){
        Key=sKey;
        Value=sValue;
        //        MSG("JS_KEY"<<Key<<Value);
    }
    QString Key;
    QString Value;
};

typedef QList<JsKeys> ListKeys;

//http Request

#define POST_Key    "POST"
#define GET_Key     "GET"
#define OPTIONS_Key "OPTIONS"
#define VIEW_Key    "VIEW"
#define PUT_Key     "PUT"
#define DELETE_Key  "DELETE"


#if LIBCURL_VERSION_NUM >= 0x073d00

#define TIME_IN_US 1 /* microseconds */
#define TIMETYPE curl_off_t
#define TIMEOPT CURLINFO_TOTAL_TIME_T
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3000000
#else
#define TIMETYPE double
#define TIMEOPT CURLINFO_TOTAL_TIME
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3
#endif

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000

struct myprogress {
    TIMETYPE lastruntime; /* type depends on version, see above */
    CURL *curl;
};

class curlThread : public QThread
{
    Q_OBJECT
    enum{
        CURL_DOWNLOADER=1,
        CURL_GET,
        CURL_POST,
        CURL_DELETE,
        CURL_PUT,
        CURL_VIEW,
        CURL_UPLOADER
    };
    QString url,output,Error,result;
    int method=0;
    ListKeys Options,Headers;
public:
    explicit curlThread(){}
    void setOptions(const ListKeys &value);
    void setHeaders(const ListKeys &value);
    bool DoHttpRequest(QString Url, const QString &RequestType);
    bool UploadFile(QString Url, const QString &file);
    bool DownloadFile(const QString &Url,const QString &Output);
    bool WaitforFinish();
    void run();
    bool Downloader(const QString &Url,const QString &Output);
    QString SetCurlError(CURLcode &code);
    bool UploadRequest(const QString &RequestType);
    bool UploadRequestWithHeader(const QString &RequestType);
    bool UploadRequestMulti(const QString &RequestType);
    bool Uploader(const QString &file);
    static QString timeConversion(int msecs);
    bool IsCancel();
    QString getError() const;
    QString getResult() const;
    bool PureHttpRequest(const QString &Url, const QString &RequestType, int MaxRetry);
    static bool QuickHttpRequest(const QString &Url,const QString &RequestType,QString &responce,int MaxRetry);

signals:
    void Update(const int &value);
#ifdef CUSTOM_METHOD
    void UpdateWait(const QString &Message,const int &value);
#endif
public slots:
    void emitCancel();
};

#endif // CURLTHREAD_H
