#include "curlThread.h"

bool inDownloading=false,isCancel=false;

int64_t totalSize=0,totaldownloaded=0;

QString curlThread::getError() const
{
    return Error;
}

bool curlThread::IsCancel()
{
    return isCancel;
}

QString curlThread::getResult() const
{
    return result;
}

void curlThread::setOptions(const ListKeys &value)
{
    //    qDebug() <<value.at(0).Key<<value.at(0).Value;
    Options = value;
}

void curlThread::emitCancel()
{
    isCancel=true;
}

void curlThread::setHeaders(const ListKeys &value)
{
    //    qDebug() <<value.at(0).Key<<value.at(0).Value;
    Headers = value;
}

bool curlThread::DoHttpRequest(QString Url, const QString &RequestType)
{
    url=Url;
    if(RequestType==POST_Key)
        method=CURL_POST;
    else if(RequestType==GET_Key)
        method=CURL_GET;
    else if(RequestType==DELETE_Key)
        method=CURL_DELETE;
    else if(RequestType==PUT_Key)
        method=CURL_PUT;
    else if(RequestType==VIEW_Key)
        method=CURL_VIEW;
    return WaitforFinish();
}

bool curlThread::UploadFile(QString Url, const QString &file)
{
    method=CURL_UPLOADER;
    url=Url;
    output=file;
    return WaitforFinish();
}

bool curlThread::DownloadFile(const QString &Url, const QString &Output)
{
    method=CURL_DOWNLOADER;
    url=Url;
    output=Output;
    return WaitforFinish();
}

bool curlThread::WaitforFinish()
{
    start();
    while(isFinished()==false){
        QApplication::processEvents();
        if(inDownloading){
            emit Update((double(totaldownloaded)/totalSize)*100);
        }
    }
    return Error.isEmpty();
}

void curlThread::run()
{
    //QD("Run "<<currentThreadId());
    Error.clear();
    if(method==0)
        return;
    else if(method==CURL_DOWNLOADER){
        Downloader(url,output);
        return;
    }else if(method==CURL_UPLOADER){
        Uploader(output);
        return;
    }
    //    custom request with custom header and without
    {
        QString type;
        if(method==CURL_GET)
            type=GET_Key;
        else if(method==CURL_DELETE)
            type=DELETE_Key;
        else if(method==CURL_POST)
            type=POST_Key;
        else if(method==CURL_PUT)
            type=PUT_Key;
        else if(method==CURL_VIEW)
            type=VIEW_Key;
        if(Headers.count() and Options.length())
            UploadRequestMulti(type);
        else if(Headers.count())
            UploadRequestWithHeader(type);
        else
            UploadRequest(type);
    }
    inDownloading=false;
}

static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    struct myprogress *myp = (struct myprogress *)p;
    CURL *curl = myp->curl;
    TIMETYPE curtime = 0;

    curl_easy_getinfo(curl, TIMEOPT, &curtime);
    totalSize=dltotal;
    totaldownloaded=dlnow;
    if(isCancel)
        return 1;
    return 0;
}

//#if LIBCURL_VERSION_NUM < 0x072000
/* for libcurl older than 7.32.0 (CURLOPT_PROGRESSFUNCTION) */
static int older_progress(void *p,
                          double dltotal, double dlnow,
                          double ultotal, double ulnow)
{
    return xferinfo(p,
                    (curl_off_t)dltotal,
                    (curl_off_t)dlnow,
                    (curl_off_t)ultotal,
                    (curl_off_t)ulnow);
}
//#endif

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

bool curlThread::Downloader(const QString &Url, const QString &Output)
{
    inDownloading=true;
    CURL *curl;
    CURLcode res = CURLE_OK;
    std::string readBuffer;
    struct myprogress prog;

    curl = curl_easy_init();
    if(curl) {
        prog.lastruntime = 0;
        prog.curl = curl;

        curl_easy_setopt(curl, CURLOPT_URL, Url.toStdString().c_str());//"https://mirror2.internetdownloadmanager.com/idman638build22.exe?b=1&filename=idman638build22.exe");
        FILE *fp = fopen(Output.toStdString().c_str(),"wb");
#if LIBCURL_VERSION_NUM >= 0x072000
        /* xferinfo was introduced in 7.32.0, no earlier libcurl versions will
       compile as they won't have the symbols around.

       If built with a newer libcurl, but running with an older libcurl:
       curl_easy_setopt() will fail in run-time trying to set the new
       callback, making the older callback get used.

       New libcurls will prefer the new callback and instead use that one even
       if both callbacks are set. */

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        /* pass the struct pointer into the xferinfo function, note that this is
       an alias to CURLOPT_PROGRESSDATA */
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
#else
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, older_progress);
        /* pass the struct pointer into the progress function */
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
#endif
        //verify=False. SSL checking disabled
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        struct curl_slist *headers = NULL;
        QString arg;
        for(int i=0;i<Headers.count();i++){
            arg=QString("%1: %2").arg(Headers.at(i).Key).arg(Headers.at(i).Value);
            qInfo() <<arg;;
            headers = curl_slist_append(headers, arg.toStdString().c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        /* cap the download speed to 31415 bytes/sec */
        qDebug() <<"CURLOPT_MAX_RECV_SPEED_LARGE"<<curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)4294967295);
        /* cap the download speed to 31415 bytes/sec */
        qDebug() <<"CURLOPT_MAX_SEND_SPEED_LARGE"<<curl_easy_setopt(curl, CURLOPT_MAX_SEND_SPEED_LARGE, (curl_off_t)4294967295);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
        {
            SetCurlError(res);
        }
        if(CURLE_OK == res) {
            curl_off_t val;

            /* check for bytes downloaded */
            res = curl_easy_getinfo(curl,   CURLINFO_SIZE_DOWNLOAD_T, &val);
            if((CURLE_OK == res) && (val>0))
                printf("Data downloaded: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", val);

            /* check for total download time */
            res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &val);
            if((CURLE_OK == res) && (val>0))
                printf("Total download time: %" CURL_FORMAT_CURL_OFF_T ".%06ld sec.\n",
                       (val / 1000000), (long)(val % 1000000));

            /* check for average download speed */
            res = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &val);
            if((CURLE_OK == res) && (val>0))
                printf("Average download speed: %" CURL_FORMAT_CURL_OFF_T
                       " kbyte/sec.\n", val / 1024);

            {
                /* check for name resolution time */
                res = curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME_T, &val);
                if((CURLE_OK == res) && (val>0))
                    printf("Name lookup time: %" CURL_FORMAT_CURL_OFF_T ".%06ld sec.\n",
                           (val / 1000000), (long)(val % 1000000));

                /* check for connect time */
                res = curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME_T, &val);
                if((CURLE_OK == res) && (val>0))
                    printf("Connect time: %" CURL_FORMAT_CURL_OFF_T ".%06ld sec.\n",
                           (val / 1000000), (long)(val % 1000000));
            }
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    qInfo() <<QString::fromStdString(readBuffer);;
    if(res==CURLE_OK)
        result=QString::fromStdString(readBuffer);
    else{
        result=SetCurlError(res);
    }
    return res==CURLE_OK;
}

QString curlThread::SetCurlError(CURLcode &code)
{
    Error=QString(curl_easy_strerror(code));
    qDebug() <<"Error"<<Error;
    return Error;
}

bool curlThread::UploadRequest(const QString &RequestType)
{
    CURL *curl;
    CURLcode res=CURL_LAST;
    curl = curl_easy_init();
    std::string readBuffer;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, RequestType.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        //verify=False. SSL checking disabled
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        //        struct curl_slist *headers = NULL;
        //        headers = curl_slist_append(headers, ": ");
        //        headers = curl_slist_append(headers, "Authorization: Hello world flask**");
        //        headers = curl_slist_append(headers, "Password: 1234");
        //        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_mime *mime;
        curl_mimepart *part;
        mime = curl_mime_init(curl);
        for(int i=0;i<Options.count();i++){
            JsKeys ckey=Options.at(i);
            part = curl_mime_addpart(mime);
            curl_mime_name(part, ckey.Key.toStdString().c_str());
            curl_mime_data(part, ckey.Value.toStdString().c_str(), CURL_ZERO_TERMINATED);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        res = curl_easy_perform(curl);
        curl_mime_free(mime);
    }
    curl_easy_cleanup(curl);
    if(res==CURLE_OK)
        result=QString::fromStdString(readBuffer);
    else
        result=SetCurlError(res);
    return res==CURLE_OK;
}

bool curlThread::UploadRequestWithHeader(const QString &RequestType)
{
    std::string readBuffer;
    CURL *curl;
    CURLcode res=CURL_LAST;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, RequestType.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        //verify=False. SSL checking disabled
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        if(Headers.count()){
            struct curl_slist *headers = NULL;
            QString arg;
            for( int i=0;i<Headers.count();i++){
                arg=QString("%1: %2").arg(Headers.at(i).Key).arg(Headers.at(i).Value);
                headers = curl_slist_append(headers, arg.toStdString().c_str());
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
    if(res==CURLE_OK)
        result=QString::fromStdString(readBuffer);
    else
        result=SetCurlError(res);
    return res==CURLE_OK;
}

bool curlThread::UploadRequestMulti(const QString &RequestType)
{
    CURL *curl;
    CURLcode res=CURL_LAST;
    curl = curl_easy_init();
    std::string readBuffer;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, RequestType.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        //verify=False. SSL checking disabled
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        if(Headers.count()){
            struct curl_slist *headers = NULL;
            QString arg;
            for( int i=0;i<Headers.count();i++){
                arg=QString("%1: %2").arg(Headers.at(i).Key).arg(Headers.at(i).Value);
                headers = curl_slist_append(headers, arg.toStdString().c_str());
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        curl_mime *mime;
        if(Options.count()){
            curl_mimepart *part;
            mime = curl_mime_init(curl);
            for(int i=0;i<Options.count();i++){
                JsKeys ckey=Options.at(i);
                part = curl_mime_addpart(mime);
                curl_mime_name(part, ckey.Key.toStdString().c_str());
                curl_mime_data(part, ckey.Value.toStdString().c_str(), CURL_ZERO_TERMINATED);
            }
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if(Options.count())
            curl_mime_free(mime);
    }
    curl_easy_cleanup(curl);
    if(res==CURLE_OK)
        result=QString::fromStdString(readBuffer);
    else
        result=SetCurlError(res);
    return res==CURLE_OK;
}

bool curlThread::Uploader(const QString &file)
{
    //    inDownloading=true;
    std::string readBuffer;
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        if(!Headers.isEmpty())
        {
            qInfo() <<"Setup headers";
            QString arg;
            for( int i=0;i<Headers.count();i++){
                arg=QString("%1: %2").arg(Headers.at(i).Key).arg(Headers.at(i).Value);
                qInfo() <<arg;
                headers = curl_slist_append(headers, arg.toStdString().c_str());
            }
            headers = curl_slist_append(headers, "arg.toStdString().c_str()");
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        //verify=False. SSL checking disabled
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_mime *mime;
        curl_mimepart *part;
        mime = curl_mime_init(curl);
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "file");
        curl_mime_filedata(part, file.toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        res = curl_easy_perform(curl);
        curl_mime_free(mime);
    }
    curl_easy_cleanup(curl);
    //    CURL *curl;
    //    CURLcode res;
    //    curl = curl_easy_init();
    //    if(curl) {
    //      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    //      curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:5000/");
    //      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    //      curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
    //      struct curl_slist *headers = NULL;
    //      headers = curl_slist_append(headers, "Email: 3423452");
    //      headers = curl_slist_append(headers, "Password: dhfadsfasfda");
    //      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    //      curl_mime *mime;
    //      curl_mimepart *part;
    //      mime = curl_mime_init(curl);
    //      part = curl_mime_addpart(mime);
    //      curl_mime_name(part, "file");
    //      curl_mime_filedata(part, "t.txt");
    //      curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    //      res = curl_easy_perform(curl);
    //      curl_mime_free(mime);
    //    }
    //    curl_easy_cleanup(curl);

    qInfo() <<QString::fromStdString(readBuffer);;
    if(res==CURLE_OK)
        result=QString::fromStdString(readBuffer);
    else{
        result=SetCurlError(res);
    }
    return res==CURLE_OK;
}

QString curlThread::timeConversion(int msecs)
{
    QString formattedTime;

    int hours = msecs/(1000*60*60);
    int minutes = (msecs-(hours*1000*60*60))/(1000*60);
    int seconds = (msecs-(minutes*1000*60)-(hours*1000*60*60))/1000;
    int milliseconds = msecs-(seconds*1000)-(minutes*1000*60)-(hours*1000*60*60);

    formattedTime.append(QString("%1").arg(hours, 2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(minutes, 2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(seconds, 2, 10, QLatin1Char('0')) + ":" +
                         QString( "%1" ).arg(milliseconds, 3, 10, QLatin1Char('0')));

    return formattedTime;
}
