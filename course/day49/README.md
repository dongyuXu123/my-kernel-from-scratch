# Day 49: HTTP 服务器

## 学习目标
- 在 TCP 之上实现 HTTP/1.1 服务器
- 解析 GET 请求，返回 HTML 响应
- 支持静态页面和简单路由

## 基础知识

### HTTP 请求/响应
```
请求: GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n
响应: HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html>...</html>
```

## 代码文件
- `net/http.c` — HTTP 服务器
- `net/tcp.c` — TCP 传输层

## QEMU 验证
```bash
cd course/day49
make
# 可配合 QEMU 端口转发测试 HTTP 请求
```
