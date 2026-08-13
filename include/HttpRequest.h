#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include<string>
#include<unordered_map>

class HttpRequest {
public:
    //主状态机：当前正在解析的阶段
    enum ParseState {
     PARSE_REQUEST_LINE, //正在解析请求行
     PARSE_HEADERS,      //正在解析请求头
     PARSE_BODY,         //正在解析请求体
     PARSE_COMPLETE      //解析完成
    };

    HttpRequest() {clear();}

    void clear() {
       m_method.clear();
       m_path.clear();
       m_version.clear();
       m_headers.clear();
       m_body.clear();
       m_parseState = PARSE_REQUEST_LINE;
       m_curHeaderKey.clear();
       m_curHeaderValue.clear();
    }

    //Getter
    const std::string& method() const {return m_method;}
    const std::string& path() const {return m_path;}
    const std::string& version() const {return m_version;}
    const std::string& body() const {return m_body;}
    const std::unordered_map<std::string,std::string>& headers() const {return m_headers;}

    std::string getHeader(const std::string& key) const {
        auto it = m_headers.find(key);
	return it != m_headers.end() ? it->second : std::string();
    }

    //Setter
    void setMethod(const std::string& method) {m_method = method;}
    void setPath(const std::string& path) {m_path = path;}
    void setVersion(const std::string& version) {m_version = version;}
    void setBody(const std::string& body) {m_body = body;}
    void addHeader(const std::string& key,const std::string& value) {m_headers[key] = value;}

    //状态管理
    ParseState parseState() const {return m_parseState;}
    void setParseState(ParseState s) {m_parseState = s;}

    //解析中间状态：当前正在处理的请求头键值对
    const std::string& curHeaderKey() const {return m_curHeaderKey;}
    void setCurHeaderKey(const std::string& key) {m_curHeaderKey = key;}
    void setCurHeaderValue(const std::string& value) {m_curHeaderValue = value;}
    void commitCurHeader() {
         addHeader(m_curHeaderKey,m_curHeaderValue);
	 m_curHeaderKey.clear();
	 m_curHeaderValue.clear();
    }

private:
    std::string m_method;
    std::string m_path;
    std::string m_version;
    std::unordered_map<std::string,std::string> m_headers; 
    std::string m_body;

    //当前解析阶段
    ParseState m_parseState;

    //解析中间状态：暂存当前请求头的key/value
    std::string m_curHeaderKey;
    std::string m_curHeaderValue;
};

#endif
