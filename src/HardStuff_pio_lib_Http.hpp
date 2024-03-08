#pragma once

// libs
#include <Arduino.h>
#include <TimeLib.h>
#include <ArduinoHttpClient.h>

static const int MAX_HEADERS = 10;

#pragma region HTTP_STRUCTS
struct KeyValuePair
{
    String key = "";
    String value = "";
};

struct HttpRequest
{
    KeyValuePair headers[MAX_HEADERS]; // Request headers
    KeyValuePair params[MAX_HEADERS];  // Request headers
    int header_count = 0;              // Number of headers in request
    int param_count = 0;               // Number of params in request
    String content = "";               // Request content

    /**
     * @brief Add a header to the request in a key: value fashion
     */
    void addHeader(const String &key, const String &value)
    {
        if (header_count < MAX_HEADERS)
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
        if (param_count < MAX_HEADERS)
        {
            params[param_count].key = key;
            params[param_count].value = value;
            param_count++;
        }
        else
            Serial.println("MAX HEADERS REACHED!");
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
            // Reset headers and header count
            for (int i = 0; i < header_count; ++i)
            {
                headers[i].key = "";
                headers[i].value = "";
            }
            header_count = 0;
        }

        // Reset headers and header count
        for (int i = 0; i < param_count; ++i)
        {
            params[i].key = "";
            params[i].value = "";
        }
        param_count = 0;
    }

    /**
     * @brief Print the HttpRequest to serial (useful for debugging or analysing)
     *
     * @param output_stream Override the stream, e.g. if dumping to an FS file
     */
    void print(Stream *output_stream = &Serial)
    {
        output_stream->println("Headers:");
        for (int i = 0; i < header_count; i++)
        {
            output_stream->println(headers[i].key + " : " + headers[i].value);
        }
        output_stream->println("Content:");
        output_stream->println(content);
    }
};

struct HttpResponse
{
    int status_code = 0;               // HTTP status code
    KeyValuePair headers[MAX_HEADERS]; // Response headers
    int header_count = 0;              // Number of headers in response
    String body = "";                  // Response body
    int content_length = 0;            // Content length
    bool is_chunked = false;           // Flag for chunked response
    String error_message = "";         // Error message, if any

    // Function to add a header (can be used while parsing the response)
    void addHeader(const String &key, const String &value)
    {
        if (header_count < MAX_HEADERS)
        {
            headers[header_count].key = key;
            headers[header_count].value = value;
            header_count = min(header_count + 1, MAX_HEADERS);
        }
        else
            Serial.println("MAX HEADERS REACHED!");
    }

    // Function to check if there was an error
    bool hasError() const { return !error_message.isEmpty(); }
    bool success() const { return status_code >= 200 && status_code < 300; }

    /**
     * @brief Print the HttpResponse to serial (useful for debugging or analysing)
     *
     * @param output_stream Override the stream, e.g. if dumping to an FS file
     */
    void print(Stream *output_stream = &Serial) const
    {
        Serial.println("Response status code: " + String(status_code));
        Serial.println(F("Response Headers:"));
        for (int i = 0; i <= header_count; i++)
            Serial.println("    " + headers[i].key + " : " + headers[i].value);
        Serial.println("Content length: " + String(content_length));
        Serial.println(F("Response:"));
        Serial.println(body);
        Serial.println("Response is " + String(is_chunked ? "" : "not ") + "chunked.");
        Serial.println("Body length is: " + String(body.length()));
    }

    /**
     * @brief Clear the contents of the HTTP response (should you wish to recycle the response element)
     */
    void clear()
    {
        status_code = 0;      // HTTP status code
        headers[MAX_HEADERS]; // Response headers
        header_count = 0;     // Number of headers in response
        body = "";            // Response body
        content_length = 0;   // Content length
        is_chunked = false;   // Flag for chunked response
        error_message = "";   // Error message, if any

        // Reset headers and header count
        for (int i = 0; i < header_count; ++i)
        {
            headers[i].key = "";
            headers[i].value = "";
        }
        header_count = 0;
    }
};
#pragma endregion
class HardStuffHttpClient : public HttpClient
{
public:
    // HttpClient http(HTTP_UNDERLYING_CLIENT, HTTP_SERVER, HTTP_PORT);
    // Implementation of the constructor for HardStuffHttpClient
    HardStuffHttpClient(Client &underlying_client, const char *server_name, uint16_t server_port) : HttpClient(underlying_client, server_name, server_port){};

    /**
     * @brief Any dedicated init processes
     *
     * @returns true if successful, otherwise false
     */
    bool init()
    {
        bool _s = true;
        // HTTP_UNDERLYING_CLIENT.setTimeout(5000);
        // _s = _s && SIMCOM::client.setCertificate(AWS_CERT_CA);
        // this->connectionKeepAlive(); // Currently, this is needed for HTTPS
        // client.setHandshakeTimeout(5000);
        return _s;
    }

    /**
     * @brief Post some content string to a given endpoint of the attached server
     *
     * @param endpoint Given endpoint, e.g. "/device_1/shadow"
     * @param request A request object that contains contents and headers
     * @return HttpResponse, the response as an HttpResponse object
     */
    HttpResponse postToHTTPServer(String endpoint, HttpRequest *request)
    {
        HttpResponse response;
        // Post the HTTP request

        for (int i = 0; i < request->param_count; i++)
        {
            endpoint += i == 0 ? "?" : "&";
            if (request->params[i].key)
                endpoint += request->params[i].key + "=" + request->params[i].value;
        }

        this->beginRequest();
        this->post(endpoint);
        for (int i = 0; i < request->header_count; i++)
        {
            this->sendHeader(request->headers[i].key, request->headers[i].value);
        }
        Serial.println("B");
        this->sendHeader("Content-Length", request->content.length());
        this->beginBody();
        // this->println(request->content);
        this->println(request->content);
        this->endRequest();

        if (this->getWriteError() != 0)
        {
            response.status_code = HTTP_ERROR_TIMED_OUT;
            this->stop();
            return response;
        }

        Serial.print(F("Performing HTTP POST request... "));

        response.status_code = this->responseStatusCode();
        for (int i = 0; i < MAX_HEADERS; i++)
        {
            if (this->headerAvailable())
            {
                response.addHeader(this->readHeaderName(), this->readHeaderValue());
                response.header_count = i;
            }
            else
            {
                break;
            }
        }
        this->skipResponseHeaders();
        response.body = this->responseBody();
        this->stop();
        return response;
    }

    /**
     * @brief Get the whatever contents from a given endpoint of the attached server
     *
     * @param endpoint Given endpoint, e.g. "/version"
     * @param request
     * @param skip_body Rarely used, only in circumstances where you want to stream the body response somewhere else (other than a string)
     * @return HttpResponse
     */
    HttpResponse getFromHTTPServer(String endpoint, HttpRequest *request = nullptr, bool skip_body = false)
    {
        HttpResponse response;

        if (request != nullptr)
        {
            for (int i = 0; i < request->param_count; i++)
            {
                endpoint += i == 0 ? "?" : "&";
                if (request->params[i].key)
                {
                    endpoint += request->params[i].key + "=" + request->params[i].value;
                }
            }
        }
        // Get
        this->beginRequest();
        this->get(endpoint);

        if (this->getWriteError() != 0)
        {
            response.status_code = HTTP_ERROR_TIMED_OUT;
            this->stop();
            return response;
        }
        if (request != nullptr)
        {
            for (int i = 0; i < request->header_count; i++)
            {
                this->sendHeader(request->headers[i].key, request->headers[i].value);
            }
            if (request->content.length())
            {
                this->sendHeader("Content-Length", request->content.length());
                this->beginBody();
                this->println(request->content);
            }
        }
        this->endRequest();
        // Serial.print(F("Performing HTTP GET request... "));

        response.status_code = this->responseStatusCode();
        for (int i = 0; i < MAX_HEADERS; i++)
        {
            if (this->headerAvailable())
            {
                response.addHeader(this->readHeaderName(), this->readHeaderValue());
                response.header_count = i;
            }
            else
            {
                break;
            }
        }
        response.content_length = this->contentLength();
        response.is_chunked = this->isResponseChunked();
        if (!skip_body)
        {
            response.body = this->responseBody();
            this->stop();
        }
        return response;
    }

public:
    // helper functions
    String formatTimeISO8601(time_t t)
    {
        char buffer[25];

        // Break down time_t into its components
        int Year = year(t);
        int Month = month(t);
        int Day = day(t);
        int Hour = hour(t);
        int Minute = minute(t);
        int Second = second(t);

        // Format the string in ISO 8601 format
        // Note: This assumes UTC time. Adjust accordingly if using local time.
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
                 Year, Month, Day, Hour, Minute, Second);

        return String(buffer);
    }

    time_t formatTimeFromISO8601(String timestamp)
    {
        int Year, Month, Day, Hour, Minute, Second;
        sscanf(timestamp.c_str(), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
               &Year, &Month, &Day, &Hour, &Minute, &Second);
        tmElements_t tm;
        tm.Year = Year - 1970; // Adjust year
        tm.Month = Month;      // Adjust month
        tm.Day = Day;
        tm.Hour = Hour;
        tm.Minute = Minute;
        tm.Second = Second;
        return makeTime(tm); // Convert to time_t
    }
};