# requests

## 简介

学习UNIX网络编程时写的C++客户端请求库，使用的是原生的Socket API。

库的名字来源于Python著名的requests库，该库的API设计也与该库相似。

## 依赖

* [zlib](https://zlib.net/)
* [OpenSSL](https://github.com/openssl/openssl)
* [Tencent/RapidJSON](https://github.com/Tencent/rapidjson)

## 示例

### 入门

最简单的一个GET请求

```C++
requests::HttpResponsePtr r = requests::get("https://api.github.com/events");
```

我们得到的是一个Response的智能指针，我们可以得到这个Response的正文、报头以及响应码

正文为std::string类型，响应码为int类型

```C++
std::cout << r->statusCode << std::endl;
std::cout << r->text << std::endl;
```

### 传递URL参数

当我们用GET发送数据时，数据会以urlencoded的方式发送

例如，http://httpbin.org/get?key1=val1&key2=val2，我们可以创建一个RequestOption对象传递paramsPtr参数，该库使用的JSON库为RapidJSON

```C++
requests::RequestOption requestOption;
rapidjson::Document d;
d.Parse(R"({"test":"test","hello":"world"})");
requestOption.paramsPtr = &d;
requests::HttpResponsePtr r = requests::get("http://httpbin.org/get", requestOption)
```

### 定制请求头

要定制请求头，我们需要先创建一个Headers对象，Headers对象为std::map<std::string, std::string>类型

```C++
requests::RequestOption requestOption;
requests::Headers headers;
headers["User-Agent"] =
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like "
    "Gecko) "
    "Chrome/74.0.3729.157 Safari/537.36";
requestOption.headersPtr = &headers;
requests::HttpResponsePtr r = requests::get("http://httpbin.org/get", requestOption)
```

要打印headers，可以像操作std::map一样打印

```C++
for (auto &p : r->headers) {
    std::cout << p.first << ": " << p.second << std::endl;
}
```

### POST数据

POST发送数据时，有两种方式，分别为x-www-form-urlencoded和json方式

前者为将json转化为urlencoded形式的data发送，而后者则是将json序列化后直接发送数据

我们首先创建RequestOption和待发送的JSON对象

```C++
requests::RequestOption requestOption;
rapidjson::Document d;
d.Parse(R"({"test":"test","hello":"world"})");
```

以x-www-form-urlencoded方式发送:

```C++
requestOption.dataPtr = &d;
requests::HttpResponsePtr r = post("http://httpbin.org/post", requestOption);
```

以json方式发送：

```C++
requestOption.jsonPtr = &d;
requests::HttpResponsePtr r = post("http://httpbin.org/post", requestOption);
```


## 已完成

* GET
* HTTPS支持
* POST

## TODO

* 编码转换
* Keep-Alive的Connection
* 错误处理