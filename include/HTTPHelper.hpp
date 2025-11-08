#ifndef __HTTPClient_H__
#define __HTTPClient_H__

#include <bits/stdc++.h>
#include <iomanip>
#include <curl/curl.h>
#include "json.hpp"

class HTTPClient
{
private:
  // Callback function to write response data
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response)
  {
    size_t totalSize = size * nmemb;
    response->append((char *)contents, totalSize);
    return totalSize;
  }

  CURL *curl;
  std::string response;

public:
  HTTPClient()
  {
    curl = curl_easy_init();
    if (curl)
    {
      curl_global_init(CURL_GLOBAL_DEFAULT);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; MyApp/1.0)");

      // For HTTPS
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disable SSL verification for simplicity
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
  }

  /* Retrieve URL raw response */
  std::string getAPIResult() const { return response; }

  /* Encode url if needed */
  std::string urlEncode(const std::string &str)
  {
    char *encoded = curl_easy_escape(nullptr, str.c_str(), str.length());
    if (encoded)
    {
      std::string result(encoded);
      curl_free(encoded);
      return result;
    }
    return str;
  }

  /* Make http calls */
  bool getAPI(const std::string &api_url)
  {
    response.clear();
    CURLcode res;

    if (!curl)
    {
      response = "Error: Failed to initialize CURL";
      return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
      response = "Error: " + std::string(curl_easy_strerror(res));
      return false;
    }

    return true;
  }

  /* Retrieve key and value pair of json API response */
  template <typename T>
  std::vector<std::pair<std::string, T>> parseJsonResponse(const std::pair<std::string, std::vector<std::string>> &key) const
  {
    using json = nlohmann::json;

    if (key.second.empty() || !json::accept(response))
      return {};

    std::vector<std::pair<std::string, T>> jsonResult;

    try
    {
      json data = json::parse(response);

      auto &result = data;
      if (data.is_array()) // Keeping it simple, pick the first element.
        result = data[0];

      const auto &[id, keys] = key; // separate parent id and key [if needed]

      if (!id.empty()) // pick the children of the parent
        result = result[id];

      if (!result.empty())
      {
        for (const auto &k : keys)
        {
          auto &r = result[k];
          T val;
          if (r.is_array())
            val = r[0];
          else
            val = r;
          jsonResult.push_back({k, val});
        }
      }
    }
    catch (const std::exception &e)
    {
      std::cout << "JSON parse error: " << e.what() << std::endl;
    }
    return jsonResult;
  }

  ~HTTPClient()
  {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    curl = nullptr;
  }
};

#endif //__HTTPClient_H__
