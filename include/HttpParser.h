#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include "HttpRequest.h"
#include<string>
#include<iostream>

class HttpParser {
	public:
		HttpParser() = default;

		//驱动状态机，解析readBuffer中的原始数据
		//返回true表示一个完整的HttpRequest已解析完成
		//返回false表示数据不完整，需要继续read
		bool parse(std::string& readBuffer, HttpRequest& request) {
			switch (request.parseState()) {

				case HttpRequest::PARSE_REQUEST_LINE:
					if (!parseRequestLine(readBuffer, request)) {
						return false;
					}

					request.setParseState(HttpRequest::PARSE_HEADERS);
					[[fallthrough]];

				case HttpRequest::PARSE_HEADERS:
					if (!parseHeaders(readBuffer, request)) {
						return false;
					}

					if (request.getHeader("Content-Length").empty()) {
						request.setParseState(HttpRequest::PARSE_COMPLETE);
						return true;
					}

					request.setParseState(HttpRequest::PARSE_BODY);
					[[fallthrough]];

				case HttpRequest::PARSE_BODY:
					if (!parseBody(readBuffer, request)) {
						return false;
					}

					request.setParseState(HttpRequest::PARSE_COMPLETE);
					return true;

				case HttpRequest::PARSE_COMPLETE:
					return true;

				default:
					return false;
			}
		}                


	private:
		//解析请求行：GET /index.html HTTP/1.1\r\n
		//解析成功后从readBuffer中删除已解析的部分
		bool parseRequestLine(std::string& readBuffer,HttpRequest& request) {
			//1.查找\r\n,确定请求行结束位置
			size_t pos = readBuffer.find("\r\n");
			if(pos == std::string::npos) {
				return false; //还没收到完整的一行
			}

			//2.提取请求行(去掉\r\n)
			std::string line = readBuffer.substr(0,pos);

			//3.解析：GET /index.html HTTP/1.1
			size_t firstSpace = line.find(' ');
			if(firstSpace == std::string::npos) {
				return false; //格式错误
			}

			size_t secondSpace = line.find(' ',firstSpace+1);
			if(secondSpace == std::string::npos) {
				return false; //格式错误
			}

			//4.拆分并储存
			request.setMethod(line.substr(0,firstSpace));
			request.setPath(line.substr(firstSpace+1,secondSpace - firstSpace - 1));
			request.setVersion(line.substr(secondSpace+1));

			//5.从缓冲区删除已解析的部分（请求行+\r\n）
			readBuffer.erase(0,pos+2);

			return true;
		}

		//解析请求头
		//从readBuffer 中逐行读取，直到遇到空行
		//每解析完一行，就从readBuffer中删除该行
		//返回值：true表示请求头解析完成，false表示数据不完整，需要继续read
		bool parseHeaders(std::string& readBuffer,HttpRequest& request) {
			while(true) {
				//1.查找一行的结束位置（\r\n）
				size_t pos = readBuffer.find("\r\n");
				if(pos == std::string::npos) {
					return false; //还没收到完整的一行，等待下次read
				}

				//2.如果开头就是\r\n，说明是空行
				if(pos == 0)
				{
					//空行表示请求头结束
					readBuffer.erase(0,2); //从缓冲区删除这个空行
					return true; //请求头解析完成
				}

				//3.提取当前行（去掉结尾的\r\n）
				std::string line = readBuffer.substr(0,pos);

				//4.查找冒号，拆分key和value
				size_t colon = line.find(':');
				if(colon != std::string::npos) {
					std::string key = line.substr(0,colon); //冒号前是key
					std::string value = line.substr(colon+1); //冒号后是value
										  //5.去除value开头的空格
					while(!value.empty() && value.front() == ' ') {
						value.erase(0,1);
					}

					//6.存入HttpRequest的headers字典
					request.addHeader(key,value);
				}

				//7.从缓冲区删除已解析的这一行(包括\r\n)
				readBuffer.erase(0,pos+2);
			}
		}

		bool parseBody(std::string& readBuffer, HttpRequest& request) {
			// 从请求头中取出 Content-Length 值
			std::string contentLengthStr = request.getHeader("Content-Length");

			if (contentLengthStr.empty()) {
				return true;   // 没有请求体
			}

			size_t contentLength = std::stoul(contentLengthStr);

			if (readBuffer.size() < contentLength) {
				return false;
			}

			request.setBody(readBuffer.substr(0, contentLength));
			readBuffer.erase(0, contentLength);

			return true;
		}

};


#endif
