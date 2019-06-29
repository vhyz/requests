# requests

## 简介

学习UNIX网络编程时写的C++客户端请求库，使用的是原生的Socket API。

库的名字来源于Python著名的requests库，该库的API设计也鱼该库相似。

## 依赖

* [zlib](https://zlib.net/)
* [OpenSSL](https://github.com/openssl/openssl)
* [Tencent/RapidJSON](https://github.com/Tencent/rapidjson)

## 已完成

* GET
* HTTPS支持

## TODO

* POST
* HEAD
* 编码转换
* Keep-Alive的Connection
* 错误处理