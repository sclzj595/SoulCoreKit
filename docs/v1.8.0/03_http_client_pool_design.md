# HttpClient 连接池设计(v1.8.0 P1-B)

**文档状态**: Accepted
**优先级**: P1(必做)
**来源**: project_memory 项目约束("HttpClient 必须实现连接池复用以减少开销")

---

## 1. 现状分析

### 1.1 当前实现
- `HttpClient` 内部持有一个 `QNetworkAccessManager`
- `QNetworkAccessManager` 本身已内置连接复用(HTTP keep-alive)
- 缺失:无显式连接池配置接口,无法调整最大连接数、keepalive 超时、HTTP/2 流并发数

### 1.2 与 HTTP/2 的关系
HTTP/2 多路复用下,单连接可并发多个请求(默认最多 100 个流)。连接池配置与 HTTP/2 配置天然配套。

---

## 2. 接口设计

### 2.1 连接池配置结构

```cpp
struct ConnectionPoolConfig {
    int maxConnectionsPerHost = 6;       ///< 每主机最大连接数(HTTP/1.1 默认 6)
    int maxConcurrentStreams = 100;      ///< HTTP/2 单连接最大并发流(HTTP/2 规范推荐 100)
    int keepAliveTimeoutSec = 30;        ///< keep-alive 超时秒数
    bool enableHttp2 = true;             ///< 是否启用 HTTP/2(与 setHttp2Enabled 同步)
};
```

### 2.2 HttpClient 新增接口

```cpp
class SC_NETWORK_EXPORT HttpClient : public QObject {
public:
    void setConnectionPoolConfig(const ConnectionPoolConfig& config);
    ConnectionPoolConfig connectionPoolConfig() const;
};
```

---

## 3. 实现方案

### 3.1 复用 QNetworkAccessManager 内置连接池

**不重新造轮子**,复用 `QNetworkAccessManager` 内置的连接管理。

### 3.2 HTTP/2 流并发配置

通过 `QNetworkRequest::setHttp2Configuration()` 设置:
```cpp
QHttp2Configuration http2Config;
http2Config.setMaxConcurrentStreams(config.maxConcurrentStreams);
qrequest.setHttp2Configuration(http2Config);
```

### 3.3 连接数与 keepalive

`QNetworkAccessManager` 的连接数由 Qt 内部管理,通过 `QNetworkRequest` 的 `Connection` 头部控制 keep-alive 行为。

---

## 4. 验收标准

- 连接池配置接口可用
- HTTP/2 多路复用下,单连接并发多个请求(通过日志或响应头验证)
- 配置变更不影响默认行为(默认值与 Qt 推荐一致)

---

## 5. 设计原则

- **复用优先**:复用 `QNetworkAccessManager` 内置能力,不重新实现连接池
- **配置暴露**:暴露关键参数让用户可调
- **默认安全**:默认值与 Qt/HTTP 规范推荐一致
