/*
 * This file is part of PowerDNS or dnsdist.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef WEBSERVER_HH
#define WEBSERVER_HH
#include <map>
#include <string>
#include <list>
#include <boost/utility.hpp>
#include <yahttp/yahttp.hpp>
#include "json11.hpp"
#include "namespaces.hh"
#include "sstuff.hh"

class WebServer;

class HttpRequest : public YaHTTP::Request {
public:
  HttpRequest() : YaHTTP::Request(), accept_json(false), accept_html(false), complete(false) { };

  bool accept_json;
  bool accept_html;
  bool complete;
  json11::Json json();

  // checks password _only_.
  bool compareAuthorization(const string &expected_password);
  bool compareHeader(const string &header_name, const string &expected_value);
};

class HttpResponse: public YaHTTP::Response {
public:
  HttpResponse() : YaHTTP::Response() { };
  HttpResponse(const YaHTTP::Response &resp) : YaHTTP::Response(resp) { };

  void setBody(const json11::Json& document);
  void setErrorResult(const std::string& message, const int status);
  void setSuccessResult(const std::string& message, const int status = 200);
};


class HttpException
{
public:
  HttpException(int status) : d_response()
  {
    d_response.status = status;
  };

  HttpException(int status, const string& msg) : d_response()
  {
    d_response.setErrorResult(msg, status);
  };

  HttpResponse response()
  {
    return d_response;
  }

protected:
  HttpResponse d_response;
};

class HttpBadRequestException : public HttpException {
public:
  HttpBadRequestException() : HttpException(400) { };
  HttpBadRequestException(const string& msg) : HttpException(400, msg) { };
};

class HttpUnauthorizedException : public HttpException {
public:
  HttpUnauthorizedException(string const &scheme) : HttpException(401)
  {
    d_response.headers["WWW-Authenticate"] = scheme + " realm=\"PowerDNS\"";
  }
};

class HttpForbiddenException : public HttpException {
public:
  HttpForbiddenException() : HttpException(403) { };
  HttpForbiddenException(const string& msg) : HttpException(403, msg) { };
};

class HttpNotFoundException : public HttpException {
public:
  HttpNotFoundException() : HttpException(404) { };
  HttpNotFoundException(const string& msg) : HttpException(404, msg) { };
};

class HttpMethodNotAllowedException : public HttpException {
public:
  HttpMethodNotAllowedException() : HttpException(405) { };
  HttpMethodNotAllowedException(const string& msg) : HttpException(405, msg) { };
};

class HttpConflictException : public HttpException {
public:
  HttpConflictException() : HttpException(409) { };
  HttpConflictException(const string& msg) : HttpException(409, msg) { };
};

class HttpInternalServerErrorException : public HttpException {
public:
  HttpInternalServerErrorException() : HttpException(500) { };
  HttpInternalServerErrorException(const string& msg) : HttpException(500, msg) { };
};

class ApiException : public runtime_error
{
public:
  ApiException(const string& what) : runtime_error(what) {
  }
};

class Server
{
public:
  Server(const string &localaddress, int port) : d_local(localaddress.empty() ? "0.0.0.0" : localaddress, port), d_server_socket(d_local.sin4.sin_family, SOCK_STREAM, 0) {
    d_server_socket.setReuseAddr();
    d_server_socket.bind(d_local);
    d_server_socket.listen();
  }
  virtual ~Server() { };

  ComboAddress d_local;

  std::shared_ptr<Socket> accept() {
    return std::shared_ptr<Socket>(d_server_socket.accept());
  }

protected:
  Socket d_server_socket;
};

class WebServer : public boost::noncopyable
{
public:
  WebServer(const string &listenaddress, int port);
  virtual ~WebServer() { };

  void setApiKey(const string &apikey) {
    if (d_registerApiHandlerCalled) {
      throw PDNSException("registerApiHandler has been called, can not change apikey");
    }
    d_apikey = apikey;
  }

  void setPassword(const string &password) {
    if (d_registerWebHandlerCalled) {
      throw PDNSException("registerWebHandler has been called, can not change password");
    }
    d_webserverPassword = password;
  }

  void setACL(const NetmaskGroup &nmg) {
    d_acl = nmg;
  }

  void bind();
  void go();

  void serveConnection(std::shared_ptr<Socket> client) const;
  void handleRequest(HttpRequest& request, HttpResponse& resp) const;

  typedef boost::function<void(HttpRequest* req, HttpResponse* resp)> HandlerFunction;
  void registerApiHandler(const string& url, HandlerFunction handler);
  void registerWebHandler(const string& url, HandlerFunction handler);

protected:
  void registerBareHandler(const string& url, HandlerFunction handler);

  virtual std::shared_ptr<Server> createServer() {
    return std::make_shared<Server>(d_listenaddress, d_port);
  }

  string d_listenaddress;
  int d_port;
  string d_password;
  std::shared_ptr<Server> d_server;

  std::string d_apikey;
  bool d_registerApiHandlerCalled{false};

  std::string d_webserverPassword;
  bool d_registerWebHandlerCalled{false};

  NetmaskGroup d_acl;
};

#endif /* WEBSERVER_HH */
