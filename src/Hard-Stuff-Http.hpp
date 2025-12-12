#pragma once

// libs
#include <TimeLib.h>
#include <ArduinoHttpClient.h>

#ifndef HTTP_MAX_HEADERS
#define HTTP_MAX_HEADERS 10
#endif

#pragma region HTTP_STRUCTS
struct KeyValuePair
{
  String key = "";
  String value = "";
};

struct HardStuffHttpRequest
{
  KeyValuePair headers[HTTP_MAX_HEADERS]; // Request headers
  KeyValuePair params[HTTP_MAX_HEADERS];  // Request headers
  int header_count = 0;                   // Number of headers in request
  int param_count = 0;                    // Number of params in request
  String content = "";                    // Request content

  /**
   * @brief Add a header to the request in a key: value fashion
   */
  void addHeader(const String &key, const String &value)
  {
    if (header_count < HTTP_MAX_HEADERS)
    {
      headers[header_count].key = key;
      headers[header_count].value = value;
      header_count++;
    }
    else
      Serial.println("MAX HEADERS REACHED!");
  }

  /**
   * @brief Add a header to the request in a key: value fashion
   */
  void addParam(const String &key, const String &value)
  {
    if (param_count < HTTP_MAX_HEADERS)
    {
      params[param_count].key = key;
      params[param_count].value = value;
      param_count++;
    }
    else
      Serial.println("MAX PARAMS REACHED!");
  }

  /**
   * @brief Clear the contents of the HTTP request
   *
   * @param clear_all
   */
  void clear(bool ignore_headers = false)
  {
    content = ""; // Clear the content
    if (!ignore_headers)
    {
      for (int i = 0; i < header_count; ++i)
      {
        headers[i].key = "";
        headers[i].value = "";
      }
      header_count = 0;
    }
    for (int i = 0; i < param_count; ++i)
    {
      params[i].key = "";
      params[i].value = "";
    }
    param_count = 0;
  }

  /**
   * @brief Print the HardStuffHttpRequest to serial (useful for debugging or analysing)
   *
   * @param output_stream Override the stream, e.g. if dumping to an FS file
   */
  void print(Stream *output_stream = &Serial)
  {
    output_stream->println("Headers:");
    for (int i = 0; i < header_count; i++)
      output_stream->println(headers[i].key + " : " + headers[i].value);
    output_stream->println("Content:");
    output_stream->println(content);
  }
};

struct HardStuffHttpResponse
{
  int status_code = 0;                    // HTTP status code
  KeyValuePair headers[HTTP_MAX_HEADERS]; // Response headers
  int header_count = 0;                   // Number of headers in response
  String body = "";                       // Response body
  int content_length = 0;                 // Content length
  bool is_chunked = false;                // Flag for chunked response

  // Quick-check if the status code was between 200 and 300
  bool success() const { return status_code >= 200 && status_code < 300; }
  /**
   * @brief Print the HardStuffHttpResponse to serial (useful for debugging or analysing)
   *
   * @param output_stream Override the stream, e.g. if dumping to an FS file
   */
  void print(Stream *output_stream = &Serial) const
  {
    output_stream->println("Response status code: " + String(status_code));
    output_stream->println(F("Response Headers:"));
    for (int i = 0; i <= header_count; i++)
      output_stream->println("    " + headers[i].key + " : " + headers[i].value);
    output_stream->println("Content length: " + String(content_length));
    output_stream->println(F("Response:"));
    if (body.length() < 1000)
      output_stream->println(body);
    else
    {
      output_stream->println(F("[Body too large to print]"));
      output_stream->println(body.substring(0, 1000) + "...");
    }
  }

  /**
   * @brief Clear the contents of the HTTP response (should you wish to recycle the response element)
   */
  void clear()
  {
    status_code = 0;
    header_count = 0;
    body = "";
    content_length = 0;
    is_chunked = false;
    for (int i = 0; i < HTTP_MAX_HEADERS; ++i)
    {
      headers[i].key = "";
      headers[i].value = "";
    }
  }
};
#pragma endregion

class HardStuffHttpClient : public HttpClient
{
private:
  // Keep reference to underlying client for cross-domain reconnections
  Client *_internal_client;
  String _current_host;
  uint16_t _current_port;

public:
  // HttpClient http(HTTP_UNDERLYING_CLIENT, HTTP_SERVER, HTTP_PORT);
  // Implementation of the constructor for HardStuffHttpClient
  HardStuffHttpClient(Client &underlying_client, const char *server_name, uint16_t server_port)
      : HttpClient(underlying_client, server_name, server_port)
  {
    _internal_client = &underlying_client;
    _current_host = String(server_name);
    _current_port = server_port;
  };

private:
  /**
   * @brief Helper to parse the new location from a 3xx response
   */
  bool _handleRedirect(String &location, String &endpoint, String &serverName, uint16_t &serverPort)
  {
    if (location.length() == 0)
      return false;

    // Case A: Full URL (e.g., https://api.github.com/new/path)
    if (location.startsWith("http"))
    {
      int protoEnd = location.indexOf("://");
      String protocol = "http";
      if (protoEnd > 0)
      {
        protocol = location.substring(0, protoEnd);
        location.remove(0, protoEnd + 3);
      }

      int pathStart = location.indexOf("/");
      if (pathStart == -1)
      {
        serverName = location;
        endpoint = "/";
      }
      else
      {
        serverName = location.substring(0, pathStart);
        endpoint = location.substring(pathStart);
        if (!endpoint.startsWith("/"))
          endpoint = "/" + endpoint;
      }
      serverPort = (protocol.equalsIgnoreCase("https")) ? 443 : 80;
    }
    // Case B: Relative path (e.g., /new/path)
    else
    {
      serverName = _current_host; // Same server
      serverPort = _current_port;
      endpoint = location.startsWith("/") ? location : "/" + location;
    }
    return true;
  }

public:
  /**
   * @brief Post some content string to a given endpoint of the attached server
   *
   * @param endpoint Given endpoint, e.g. "/device_1/shadow"
   * @param request A request object that contains contents and headers
   * @param skip_body Don't write the response's body to a .body (good for long bodies in small memory situations)
   * @return HardStuffHttpResponse, the response as an HardStuffHttpResponse object
   */
  HardStuffHttpResponse postToHTTPServer(String endpoint, HardStuffHttpRequest *request, bool skip_body = false)
  {
    HardStuffHttpResponse response;
    // Post the HTTP request

    for (int i = 0; i < request->param_count; i++)
    {
      endpoint += i == 0 ? "?" : "&";
      if (request->params[i].key)
        endpoint += request->params[i].key + "=" + request->params[i].value;
    }

    this->beginRequest();
    int result = this->post(endpoint);
    if (result < 0)
    {
      response.status_code = result;
      return response;
    }

    for (int i = 0; i < request->header_count; i++)
      this->sendHeader(request->headers[i].key, request->headers[i].value);

    this->sendHeader("Content-Length", request->content.length());
    this->beginBody();
    this->println(request->content);
    this->endRequest();

    if (this->getWriteError() != 0)
    {
      response.status_code = HTTP_ERROR_TIMED_OUT;
      this->stop();
      return response;
    }

    response.status_code = this->responseStatusCode();
    if (response.status_code < 0)
      return response;

    for (int i = 0; i < HTTP_MAX_HEADERS; i++)
    {
      if (this->headerAvailable())
      {
        response.headers[response.header_count].key = this->readHeaderName();
        response.headers[response.header_count].value = this->readHeaderValue();
        response.header_count = min(response.header_count + 1, HTTP_MAX_HEADERS);
      }
      else
        break;
    }
    this->skipResponseHeaders();
    response.content_length = this->contentLength();
    response.is_chunked = this->isResponseChunked();
    if (!skip_body)
    {
      response.body = this->responseBody();
      this->stop();
    }
    return response;
  }

  /**
   * @brief Get the whatever contents from a given endpoint of the attached server
   *
   * @param endpoint Given endpoint, e.g. "/version"
   * @param request
   * @param skip_body Don't write the response's body to a .body (good for long bodies in small memory situations)
   * * @param redirects_remaining Recursion depth limit (default 3)
   * @return HardStuffHttpResponse
   */
  HardStuffHttpResponse getFromHTTPServer(String endpoint, HardStuffHttpRequest *request = nullptr, bool skip_body = false, int redirects_remaining = 2)
  {
    HardStuffHttpResponse response;

    // 1. Prepare Params (Only if this is the original request, not a redirect with full params)
    // Note: If endpoint doesn't contain '?', append params.
    if (request != nullptr && endpoint.indexOf('?') == -1)
    {
      for (int i = 0; i < request->param_count; i++)
      {
        endpoint += i == 0 ? "?" : "&";
        if (request->params[i].key)
          endpoint += request->params[i].key + "=" + request->params[i].value;
      }
    }

    Serial.print("GET " + _current_host + endpoint + "... ");

    // 2. Execute Request
    this->beginRequest();
    int result = this->get(endpoint);

    if (result < 0)
    {
      response.status_code = result;
      return response;
    }
    if (this->getWriteError() != 0)
    {
      response.status_code = HTTP_ERROR_TIMED_OUT;
      this->stop();
      return response;
    }

    // Send Headers (only if request provided)
    if (request != nullptr)
    {
      for (int i = 0; i < request->header_count; i++)
        this->sendHeader(request->headers[i].key, request->headers[i].value);
      if (request->content.length())
      {
        this->sendHeader("Content-Length", request->content.length());
        this->beginBody();
        this->println(request->content);
      }

      Serial.println("Request: ");
      request->print();
    }
    this->endRequest();

    // 3. Process Status
    response.status_code = this->responseStatusCode();
    Serial.println(response.status_code);

    if (response.status_code < 0)
      return response;

    // 4. Read Headers & Check Location
    String locationHeader = "";

    while (this->headerAvailable())
    {
      String name = this->readHeaderName();
      String value = this->readHeaderValue();

      // Always capture the Location header, even if it's the 50th one
      if (name.equalsIgnoreCase("Location"))
      {
        locationHeader = value;
      }

      // Only store the first HTTP_MAX_HEADERS for the response object to save RAM
      if (response.header_count < HTTP_MAX_HEADERS)
      {
        response.headers[response.header_count].key = name;
        response.headers[response.header_count].value = value;
        response.header_count++;
      }
      else
      {
        // Optional: Debug if we are actually hitting the limit
        // Serial.println("Skipping storage of header: " + name);
      }
    }

    // 5. Handle Redirects (301, 302, 307, 308)
    if (redirects_remaining > 0 &&
        (response.status_code == 301 || response.status_code == 302 ||
         response.status_code == 307 || response.status_code == 308))
    {
      String newHost = "";
      String newPath = "";
      uint16_t newPort = 0;

      if (_handleRedirect(locationHeader, newPath, newHost, newPort))
      {
        Serial.println("Redirecting to: " + newHost + newPath + " on port: " + String(newPort));
        // Stop the current HTTP session
        this->stop();

        // Check if Cross-Domain (Switching from GitHub API to S3)
        if (!newHost.equalsIgnoreCase(_current_host))
        {
          Serial.println("Switching Server...");
          _internal_client->flush();
          _internal_client->stop();
          delay(20);

          HardStuffHttpClient tempClient(*_internal_client, newHost.c_str(), newPort);
          return tempClient.getFromHTTPServer(newPath, request, skip_body, redirects_remaining - 1);
        }
        else
        {
          // Same Domain: We can safely reuse 'this'
          return this->getFromHTTPServer(newPath, request, skip_body, redirects_remaining - 1);
        }
      }
    }

    // 6. Finalize Response (No redirect)
    response.content_length = this->contentLength();
    response.is_chunked = this->isResponseChunked();

    // Usually skipResponseHeaders() is redundant here as we read them above but good for safety if we hit MAX_HEADERS limit loop break.

    if (!skip_body)
    {
      response.body = this->responseBody();
      this->stop();
    }
    return response;
  }
};