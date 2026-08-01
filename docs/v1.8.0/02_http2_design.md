# HTTP/2 多路复用设计(v1.8.0 P1-A)

**文档状态**: Accepted
**优先级**: P1(必做)
**来源**: v1.7.0 P2 延期项

---

## 1. 现状分析

### 1.1 HttpClient
- 基于 `QNetworkAccessManager`
- 接口:`send`/`sendAsync`/`setTimeout`/`setRetryPolicy`/`addInterceptor`
- 缺失:未显式启用 HTTP/2,未暴露配置接口

### 1.2 HttpTransport
- 基于 `QNetworkAccessManager`
- 接口:`sendRequest`/`start`/`stop`/`setReadTimeout`
- 缺失:未透传 HTTP/2 配置

### 1.3 Qt 6.5+ HTTP/2 支持
- `QNetworkRequest::setHttp2Enabled(bool)` 自 Qt 5.15 可用
- `QNetworkRequest::http2Configuration()` / `setHttp2Configuration()`
- `QNetworkReply::ProtocolFailure` 处理 HTTP/2 特有错误
- 服务器不支持时 Qt 自动降级到 HTTP/1.1

---

## 2. 接口设计

### 2.1 HttpClient 新增接口

```cpp
class SC_NETWORK_EXPORT HttpClient : public QObject {
public:
    // HTTP/2 配置
    void setHttp2Enabled(bool enabled);
    bool isHttp2Enabled() const;
};
```

### 2.2 HttpTransport 新增接口

```cpp
class HttpTransport : public QObject, public IRpcTransport {
public:
    void setHttp2Enabled(bool enabled);
    bool isHttp2Enabled() const;
};
```

### 2.3 默认行为

- **默认启用 HTTP/2**(向后兼容:服务器不支持时 Qt 自动降级)
- 暴露 `setHttp2Enabled(false)` 允许显式关闭(调试场景)

---

## 3. 实现方案

### 3.1 HttpClient

在 `send` 和 `sendAsync` 中,构造 `QNetworkRequest` 后调用:
```cpp
if (m_http2Enabled) {
    qrequest.setHttp2Enabled(true);
}
```

### 3.2 HttpTransport

在 `sendRequest` 中:
```cpp
if (m_http2Enabled) {
    qrequest.setHttp2Enabled(true);
}
```

### 3.3 错误处理

`QNetworkReply::ProtocolFailure` 时检查 `http2Configuration()`,记录日志。

---

## 4. 验收标准

- HttpClient/HttpTransport 默认启用 HTTP/2
- 对不支持 HTTP/2 的服务器自动降级(向后兼容)
- 新增单元测试:
  - HTTP/2 启用/禁用接口测试
  - 协议版本检测(通过响应头)
  - 降级场景测试(本地 HTTP/1.1 服务器)

---

## 5. 向后兼容性

- 默认启用 HTTP/2,但 Qt 自动降级保证与 HTTP/1.1 服务器兼容
- 现有代码无需修改即可享受 HTTP/2 性能提升
- `setHttp2Enabled(false)` 提供回退路径
