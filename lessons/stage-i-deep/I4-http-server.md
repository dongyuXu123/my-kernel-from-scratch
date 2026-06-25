# I4: HTTP 服务器 (Day 49)

## 对照源码
- RFC 7230 (HTTP/1.1), Linux 内核无内置 HTTP

## 学习目标
1. HTTP/1.1 服务器: 在 TCP 之上实现请求解析 + 响应构建
2. GET 路由: / → 首页, /about → 关于页, 其他 → 404
3. 响应格式: 状态行 + 头 + 空行 + Body (HTML)
4. 简化 sprintf: %d (整数) + %s (字符串) 格式化

## 代码导读
- `net/http.c`: http_server_start, http_handle_request, serial_sprintf
- 路由表: 路径 → 状态码 + Body

## 验证
```bash
cd course/day49 && make
qemu-system-x86_64 -kernel mykernel.elf -display none -serial stdio -no-reboot -no-shutdown
```
